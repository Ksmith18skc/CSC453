// CSC 453 — Assignment 1 Milestone 2 parser.c
// Kory Smith (modified to build ASTs when print_ast_flag is set)

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "scanner.h"
#include "symtab.h"
#include <string.h>
#include "ast.h"          /* new: AST factories & printers */

/* Code generation function declarations */
extern void generate_tac(void *ast);
extern void generate_mips(void *ast);
extern void register_global_var(const char *name);

static int lookahead;
extern int chk_decl_flag;
extern int print_ast_flag; /* driver.c controls this */
extern int gen_code_flag;  /* driver.c controls this */

/* forward declarations - many signatures changed to return AST nodes (void*) or
   to return additional info (e.g., expression lists return counts). */

static void advance(void);
static void expect(int tok, const char *msg);
static void parse_program(void);
static void parse_top_level(const char *first_id);

/* formals: now return names via out param and count as return value */
// static int parse_opt_formals(char ***out_names);
static int parse_formals(char ***out_names);

static void parse_var_decl(void);
static void parse_id_list_after_first_id(void);
static void parse_type(void);

/* stmt-list returns AST (STMT_LIST or NULL) */
static void *parse_opt_var_decls(void); /* still void*, kept for future use */
static void *parse_opt_stmt_list(void);

/* statements return AST nodes */
static void *parse_stmt(void);
// static void *parse_fn_call(void);

/* expressions return AST nodes */
static void *parse_bool_exp(void);
static NodeType parse_relop(void);
static void *parse_arith_exp(void);

/* statement kinds */
static void *parse_if_stmt(void);
static void *parse_while_stmt(void);
static void *parse_return_stmt(void);
static void *parse_block_stmt(void);   /* returns STMT_LIST or NULL */
static void *parse_assg_or_call_stmt(void); /* starts with ID */

/* calls with args: build expr-list AST and also return count via out param */
static void * parse_opt_expr_list(int *out_count); /* returns EXPR_LIST AST or NULL */
static void * parse_expr_list(int *out_count);     /* returns EXPR_LIST AST */

static void parse_func_defn(const char *fname); /* builds and prints AST if requested */

static int have_return_stmt = 0; /* reset per function */

/* error printing, line num. */
static void parse_error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int line = get_lineno();
  fprintf(stderr, "ERROR LINE %d: ", line);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(1);
}

static void advance(void) {
  lookahead = get_token();
}

static void expect(int tok, const char *msg) {
  if (lookahead != tok) {
    parse_error("expected %s, got '%s'", msg, lexeme);
  }
  advance();
}

int parse(void) {
  st_init();  // initialize symbol table
  advance();
  parse_program();
  if (lookahead != TOK_EOF) {
    parse_error("trailing garbage after program");
  }
  return 0;
}

static char *dup_lexeme(void) {
  size_t n = strlen(lexeme) + 1;
  char *s = malloc(n);
  if (!s) { perror("malloc"); exit(1); }
  memcpy(s, lexeme, n);
  return s;
}

/* program -> func_defn program | var_decl program | ε */
static void parse_program(void) {
  while (lookahead != TOK_EOF) {
    if (lookahead == kwINT) {
      advance();
      char *first_id = dup_lexeme();
      expect(ID, "identifier");
      parse_top_level(first_id);
      free(first_id);
      continue;
    }
    parse_error("unexpected token at top-level: '%s'", lexeme);
  }
}

/* parse top level after type and id have been consumed
   top_level -> func_defn | var_decl */
static void parse_top_level(const char *first_id) {
  if (lookahead == LPAREN) { /* func. def */
    if (chk_decl_flag){
      if (st_insert(first_id, SYM_FUNC, get_lineno()) != NULL) {
        parse_error("function '%s' is doubly declared", first_id);
      }
    }
    parse_func_defn(first_id);
  } else if (lookahead == COMMA || lookahead == SEMI) { /* var decl */
    if (chk_decl_flag){
      if (st_insert(first_id, SYM_VAR, get_lineno()) != NULL) {
        parse_error("variable '%s' is doubly declared", first_id);
      }
    }
    /* Register as global variable for code generation */
    if (gen_code_flag) {
      register_global_var(first_id);
    }
    parse_id_list_after_first_id();
    expect(SEMI, "';'");
  } else {
    parse_error("expected '(' or ',' or ';' after type and identifier, got '%s'", lexeme);
  }
}

/* func_defn -> type ID LPAREN opt_formals RPAREN LBRACE opt_var_decls opt_stmt_list RBRACE
   Build AST for function and print it if print_ast_flag is set. */
static void parse_func_defn(const char *fname) {
  expect(LPAREN, "'('");
  if (chk_decl_flag) { st_enter_scope(); }

  int arity = 0;
  char **param_names = NULL;

  if (lookahead != RPAREN) {
    arity = parse_formals(&param_names);
  }
  expect(RPAREN, "')'");

  if (chk_decl_flag) st_set_func_arity(fname, arity);

  expect(LBRACE, "'{'");
  have_return_stmt = 0;
  parse_opt_var_decls(); /* currently just does semantic checks */

  /* parse body as STMT_LIST AST when print_ast_flag or gen_code_flag is set. */
  void *body_ast = parse_opt_stmt_list();

  expect(RBRACE, "'}'");

  if (print_ast_flag || gen_code_flag) {
    void *fn = ast_new_func_def((char*)fname, arity, param_names, body_ast);
    
    if (print_ast_flag) {
      print_ast(fn);
    }
    
    if (gen_code_flag) {
      /* Only generate MIPS, not TAC */
      generate_mips(fn);
    }
    
    /* ast_new_func_def duplicates names; free local copies */
    if (param_names) {
      for (int i = 0; i < arity; ++i) free(param_names[i]);
      free(param_names);
    }
    /* Note: AST is leaked here (no frees) — acceptable for this assignment. */
  } else {
    /* free param_names if any (we didn't hand them to AST) */
    if (param_names) {
      for (int i = 0; i < arity; ++i) free(param_names[i]);
      free(param_names);
    }
  }

  if (chk_decl_flag) {
    st_leave_scope();
  }
}

/* opt_formals -> formals | ε */
// static int parse_opt_formals(char ***out_names) {
//   if (lookahead == kwINT) {
//     return parse_formals(out_names);
//   }
//   *out_names = NULL;
//   return 0; /* ε case */
// }

/* formals -> type ID COMMA formals | type ID
   returns count and allocates an array of names (caller frees) */
static int parse_formals(char ***out_names) {
  char **names = NULL;
  int n = 0;
  do {
    parse_type();
    char *p = dup_lexeme(); expect(ID, "parameter name");
    if (chk_decl_flag && st_insert(p, SYM_VAR, get_lineno()))
      parse_error("parameter '%s' declared more than once in this function", p);
    /* store p into names array */
    char **tmp = realloc(names, sizeof(char*) * (n + 1));
    if (!tmp) { perror("realloc"); exit(1); }
    names = tmp;
    names[n++] = p; /* note: we keep the duplicated string to pass to AST or free later */
    if (lookahead != COMMA) break;
    advance();
  } while (1);
  *out_names = names;
  return n;
}

/* var_decl -> type id_list SEMI */
static void parse_var_decl(void) {
  parse_type();
  char *id_name = dup_lexeme();
  expect(ID, "identifier");
  if (chk_decl_flag && st_insert(id_name, SYM_VAR, get_lineno())) {
    parse_error("variable '%s' is doubly declared", id_name);
  }
  free(id_name);

  while(lookahead == COMMA) {
    advance();
    char *id_name2 = dup_lexeme();
    expect(ID, "identifier");
    if (chk_decl_flag && st_insert(id_name2, SYM_VAR, get_lineno())) {
      parse_error("variable '%s' is doubly declared", id_name2);
    }
    free(id_name2);
  }

  expect(SEMI, "';'");
}

static void parse_id_list_after_first_id(void) {
  if (lookahead == COMMA) {
    advance();
    char *id_name = dup_lexeme();
    expect(ID, "identifier");
    if (chk_decl_flag){
      if (st_insert(id_name, SYM_VAR, get_lineno()) != NULL) {
        parse_error("variable '%s' is doubly declared", id_name);
      }
    }
    /* Register as global variable for code generation */
    if (gen_code_flag) {
      register_global_var(id_name);
    }
    free(id_name);
    parse_id_list_after_first_id();
  } else {
    return; /* ε case */
  }
}

/* type -> kwINT */
static void parse_type(void) {
  expect(kwINT, "'int'");
}

/* opt_var_decls -> var_decl opt_var_decls | ε
   (no AST for var decls needed; keep semantics) */
static void *parse_opt_var_decls(void) {
  while (lookahead == kwINT) {
    parse_var_decl();
  }
  return NULL;
}

/* opt_stmt_list -> stmt opt_stmt_list | ε
   returns STMT_LIST AST (or NULL) */
static int is_stmt_start(int t){
  return t==ID || t==kwIF || t==kwWHILE || t==kwRETURN || t==LBRACE || t==SEMI;
}
static void *parse_opt_stmt_list(void) {
  if (!is_stmt_start(lookahead)) return NULL;
  void *head = parse_stmt();
  void *rest = parse_opt_stmt_list();
  return ast_new_stmt_list(head, rest);
}

/* stmt -> ... returns AST node for the statement */
static void *parse_stmt(void) {
  switch (lookahead) {
    case ID:      return parse_assg_or_call_stmt();
    case kwIF:    return parse_if_stmt();
    case kwWHILE: return parse_while_stmt();
    case kwRETURN:return parse_return_stmt();
    case LBRACE:  return parse_block_stmt();
    case SEMI:    advance(); /* empty statement */ return NULL;
    default:
      parse_error("expected a statement, got '%s'", lexeme);
      return NULL;
  }
}

static void *parse_block_stmt(void) {
  expect(LBRACE, "'{'");
  void *body = parse_opt_stmt_list();
  expect(RBRACE, "'}'");
  return body; /* body is a STMT_LIST or NULL */
}

static void *parse_if_stmt(void) {
  expect(kwIF, "'if'");
  expect(LPAREN, "'('");
  void *cond = parse_bool_exp();
  expect(RPAREN, "')'");
  void *thenp = parse_stmt();
  void *elsep = NULL;
  if (lookahead == kwELSE) {
    advance();
    elsep = parse_stmt();
  }
  return ast_new_if(cond, thenp, elsep);
}

static void *parse_while_stmt(void) {
  expect(kwWHILE, "'while'");
  expect(LPAREN, "'('");
  void *cond = parse_bool_exp();
  expect(RPAREN, "')'");
  void *body = parse_stmt();
  return ast_new_while(cond, body);
}

static void *parse_return_stmt(void) {
  expect(kwRETURN, "'return'");
  void *expr_ast = NULL;
  if (lookahead == SEMI) {
    advance(); /* return without expr */
  } else {
    expr_ast = parse_arith_exp();
    expect(SEMI, "';'");
  }
  have_return_stmt = 1;
  return ast_new_return(expr_ast);
}

// /* parse assignment or call statement; return AST for statement (ASSG or FUNC_CALL) */
// static void *parse_assg_or_callstmt_build_call(const char *name, int argc, void *args_ast) {
//   /* build AST for function call statement */
//   return ast_new_func_call((char*)name, args_ast);
// }

static void *parse_assg_or_call_stmt(void) {
  char *name = dup_lexeme();
  expect(ID, "identifier");
  if (lookahead == opASSG) {
    /* assignment */
    if (chk_decl_flag) {
      // printf("debugging: parsing assignment to '%s' at line %d\n", name, get_lineno());

      Sym *s = st_lookup(name);
      // printf("debugging: st_lookup returned %p\n", (void*)s);
      if (!s) parse_error("assignment to undeclared variable '%s'", name);
      // printf("debugging: found symbol '%s' of kind %d\n", s->name, s->kind);
      if (s->kind != SYM_VAR) parse_error("'%s' is not a variable", name);
    }
    advance(); /* '=' */
    void *rhs = parse_arith_exp();
    expect(SEMI, "';'");
    void *assg = ast_new_assg(name, rhs);
    // fprintf(stderr, "debugging created ASSG for '%s' at line %d\n", name, get_lineno());
    free(name);
    return assg;
  } else if (lookahead == LPAREN) {
    /* function call */
    if (chk_decl_flag) {
      if (st_lookup_current(name))
        parse_error("'%s' is a local variable; cannot call it as a function", name);
      Sym *g = st_lookup_global(name);
      if (!g || g->kind != SYM_FUNC)
        parse_error("use of undeclared function '%s'", name);
      int use_line = get_lineno();
      if (g->decl_line > use_line)
        parse_error("call to '%s' occurs before its declaration", name);
    }
    advance(); /* '(' */
    int argc = 0;
    void *args_ast = parse_opt_expr_list(&argc);
    expect(RPAREN, "')'");

    /* capture line of the call (the semicolon token is on the same line) */
    int call_line = get_lineno();

    expect(SEMI, "';'");
    if (chk_decl_flag) {
      Sym *g = st_lookup_global(name);
      if (g && g->arity != argc) {
        /* report using saved line so the error points to the call line */
        fprintf(stderr, "ERROR LINE %d: function '%s' called with %d args but declared with %d\n",
                call_line, name, argc, g->arity);
        exit(1);
      }
    }
    void *call_ast = NULL;
    if (print_ast_flag) {
      call_ast = ast_new_func_call(name, args_ast);
    } else {
      /* still create minimal AST to return so caller code is uniform */
      call_ast = ast_new_func_call(name, args_ast);
    }
    free(name);
    return call_ast;
  } else {
    parse_error("expected '=' or '(' after identifier '%s'", name);
    free(name);
    return NULL;
  }
}

static void parse_relop_error(void) {
  parse_error("expected relational operator, got '%s'", lexeme);
}

/* parse_relop now returns NodeType for the operator and consumes it */
static NodeType parse_relop(void) {
  switch (lookahead) {
    case opLT: advance(); return LT;
    case opLE: advance(); return LE;
    case opGT: advance(); return GT;
    case opGE: advance(); return GE;
    case opEQ: advance(); return EQ;
    case opNE: advance(); return NE;
    default: parse_relop_error(); return DUMMY;
  }
}

/* parse boolean expression: arith_expr relop arith_expr -> build binary AST */
static void *parse_bool_exp(void) {
  void *left = parse_arith_exp();
  NodeType op = parse_relop();
  void *right = parse_arith_exp();
  return ast_new_binary(op, left, right);
}

/* arithmetic expressions in G0: ID | INTCON | function-call-as-expression
   returns an expression AST node */
static void *parse_arith_exp(void) {
  if (lookahead == ID) {
    char *name = dup_lexeme();
    advance();
    if (lookahead == LPAREN) {
      /* function-call expression */
      if (chk_decl_flag) {
        if (st_lookup_current(name)) {
          parse_error("'%s' is declared as a local in this function; cannot call it as a function", name);
        }
        Sym *g = st_lookup_global(name);
        if (!g || g->kind != SYM_FUNC) {
          parse_error("use of undeclared function '%s'", name);
        }
        int use_line = get_lineno();
        if (g->decl_line > use_line) {
          parse_error("call to '%s' occurs before its declaration", name);
        }
      }
      advance(); /* '(' */
      int argc = 0;
      void *args_ast = parse_opt_expr_list(&argc);
      expect(RPAREN, "')'");
      if (chk_decl_flag) {
        Sym *g = st_lookup_global(name);
        if (g && g->arity != argc) {
          parse_error("function '%s' called with %d args but declared with %d",
                      name, argc, g->arity);
        }
      }
      void *call = ast_new_func_call(name, args_ast);
      free(name);
      return call;
    } else {
      /* plain identifier */
      if (chk_decl_flag) {
        Sym *s = st_lookup(name);
        if (!s) parse_error("use of undeclared variable '%s'", name);
        if (s->kind != SYM_VAR) parse_error("'%s' is not a variable", name);
      }
      void *id = ast_new_identifier(name);
      free(name);
      return id;
    }
  } else if (lookahead == INTCON) {
    int val = atoi(lexeme);
    advance();
    return ast_new_intconst(val);
  } else {
    parse_error("expected identifier or integer constant, got '%s'", lexeme);
    return NULL;
  }
}

/* parse_opt_expr_list / parse_expr_list build EXPR_LIST AST and return count via out param */
static void *parse_opt_expr_list(int *out_count) {
  if (lookahead == RPAREN) {
    *out_count = 0;
    return NULL;
  } else {
    return parse_expr_list(out_count);
  }
}

static void *parse_expr_list(int *out_count) {
  int count_here = 0;
  void *head = parse_arith_exp();
  count_here = 1;
  if (lookahead == COMMA) {
    advance();
    int rest_count = 0;
    void *rest = parse_expr_list(&rest_count);
    *out_count = count_here + rest_count;
    return ast_new_expr_list(head, rest);
  } else {
    *out_count = count_here;
    return ast_new_expr_list(head, NULL);
  }
}

// static void *parse_fn_call(void) {
//   char *func_name = dup_lexeme();
//   expect(ID, "function name");

//   if (chk_decl_flag) {
//     int use_line = get_lineno();

//     if (st_lookup_current(func_name)) {
//       parse_error("'%s' is declared as a local in this function; cannot call it as a function", func_name);
//     }

//     Sym *g = st_lookup_global(func_name);
//     if (!g || g->kind != SYM_FUNC) {
//       parse_error("use of undeclared function '%s'", func_name);
//     }
//     if (g->decl_line > use_line) {
//       parse_error("call to '%s' occurs before its declaration", func_name);
//     }
//   }
//   expect(LPAREN, "'('");
//   int argc = 0;
//   void *args_ast = parse_opt_expr_list(&argc);
//   expect(RPAREN, "')'");
//   void *call_ast = ast_new_func_call(func_name, args_ast);
//   free(func_name);
//   return call_ast;
// }
