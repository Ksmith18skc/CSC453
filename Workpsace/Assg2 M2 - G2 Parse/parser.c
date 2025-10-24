// // CSC 453 — Assignment 1 Milestone 2 parser.c
// // Kory Smith

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "scanner.h"
// #include "symtab.c"
#include "symtab.h"
#include <string.h>


static int lookahead;
extern int chk_decl_flag;


static void advance(void);
static void expect(int tok, const char *msg);
static void parse_program(void);
// static void parse_top_level(void);
static void parse_top_level(const char *first_id);

// static void parse_func_defn(void);
static int parse_opt_formals(void);
static int parse_formals(void);

static void parse_var_decl(void);
static void parse_id_list_after_first_id(void);
static void parse_type(void);

static void parse_opt_var_decls(void);
static void parse_opt_stmt_list(void);


static void parse_stmt(void);
static void parse_fn_call(void);

// expressions
static void parse_bool_exp(void);
static void parse_relop(void);
static void parse_arith_exp(void);

// statements
static void parse_stmt(void);
static void parse_if_stmt(void);
static void parse_while_stmt(void);
static void parse_return_stmt(void);
static void parse_block_stmt(void);   // { opt_stmt_list }
static void parse_assg_or_call_stmt(void); // starts with ID

// calls with args
static int  parse_opt_expr_list(void); // returns count
static int  parse_expr_list(void);     // returns count

// function defs: pass the name so we can set arity
static void parse_func_defn(const char *fname);

static int have_return_stmt = 0; // reset per function


// // error printing, line num.
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

// // make a heap copy of lexeme for symbol table storage
// static char *duplicate_lexeme(void){
//   char *copy = malloc(strlen(lexeme) + 1);
//   if (copy == NULL) {
//     fprintf(stderr, "Out of memory\n");
//     exit(1);
//   }
//   strcpy(copy, lexeme);
//   return copy;
// }

static char *dup_lexeme(void) {
  size_t n = strlen(lexeme) + 1;
  char *s = malloc(n);
  if (!s) { perror("malloc"); exit(1); }
  memcpy(s, lexeme, n);
  return s;
}


// program -> func_defn program | var_decl program |ε
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

// parse top level after type and id have been consumed
// top_level -> func_defn | var_decl
static void parse_top_level(const char *first_id) {
  if (lookahead == LPAREN) { // func. def
    if (chk_decl_flag){
      if (st_insert(first_id, SYM_FUNC, get_lineno()) != NULL) {
        parse_error("function '%s' is doubly declared", first_id);
      }
    }
    parse_func_defn(first_id);
  } else if (lookahead == COMMA || lookahead == SEMI) { // var decl
    if (chk_decl_flag){
      if (st_insert(first_id, SYM_VAR, get_lineno()) != NULL) {
        parse_error("variable '%s' is doubly declared", first_id);
      }
    }
    parse_id_list_after_first_id();
    expect(SEMI, "';'");
  } else {
    parse_error("expected '(' or ',' or ';' after type and identifier, got '%s'", lexeme);
  }
}



// func_defn -> type ID LPAREN opt_formals RPAREN LBRACE opt_var_decls opt_stmt_list RBRACE
// In G0: 
//        type -> kwINT
//        opt_formals -> ε
//        opt_var_decls -> ε
//        opt_stmt_list -> stmt*
// = 'int' ID '(' ')' '{' { stmt } '}'.

static void parse_func_defn(const char *fname) {
  expect(LPAREN, "'('");
  if (chk_decl_flag) { st_enter_scope(); }

  // count parameters
  int arity = 0;
  if (lookahead != RPAREN) {
    arity = parse_opt_formals();   //parse_opt_formals() returns count
  }
  expect(RPAREN, "')'");

  // record the arity in the symbol table
  if (chk_decl_flag) {
    st_set_func_arity(fname, arity);
  }

  expect(LBRACE, "'{'");
  have_return_stmt = 0;
  parse_opt_var_decls();
  parse_opt_stmt_list();
  expect(RBRACE, "'}'");

  if (chk_decl_flag) {
    // if (!have_return_stmt) {
    //   parse_error("function '%s' may fall off the end without a return", fname);
    // }
    st_leave_scope();
  }
}



// opt_formals -> formals | ε
static int parse_opt_formals(void) {
  if (lookahead == kwINT) {
    return parse_formals();
  }
  return 0; // ε case, do nothing
}

// formals -> type ID COMMA formals | type ID
static int parse_formals(void) {
  int n = 0;
  do {
    parse_type();
    char *p = dup_lexeme(); expect(ID, "parameter name");
    if (chk_decl_flag && st_insert(p, SYM_VAR, get_lineno()))
      parse_error("parameter '%s' declared more than once in this function", p);
    free(p);
    n++;
    if (lookahead != COMMA) break;
    advance();
  } while (1);
  return n;
}


// var_decl -> type id_list SEMI
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
    char *id_name = dup_lexeme();
    expect(ID, "identifier");
    if (chk_decl_flag && st_insert(id_name, SYM_VAR, get_lineno())) {
      parse_error("variable '%s' is doubly declared", id_name);
    }
    free(id_name);
  }


  // parse_id_list_after_first_id();
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
    free(id_name);
    parse_id_list_after_first_id();
  } else {
    return; // ε case, do nothing
  }
}

// type -> kwINT
static void parse_type(void) {
  expect(kwINT, "'int'");
}

// opt_var_decls -> var_decl opt_var_decls | ε
static void parse_opt_var_decls(void) {
  while (lookahead == kwINT) {
    parse_var_decl();
  }
  return; // ε case, do nothing
}

// opt_stmt_list -> stmt opt_stmt_list | ε
static int is_stmt_start(int t){
  return t==ID || t==kwIF || t==kwWHILE || t==kwRETURN || t==LBRACE || t==SEMI;
}
static void parse_opt_stmt_list(void) {
  while (is_stmt_start(lookahead)) parse_stmt();
}


// stmt -> fn_call SEMI
// fn_call -> ID LPAREN opt_expr_list RPAREN
// opt_expr_list -> ε  (so require immediate ')' )
static void parse_stmt(void) {
  switch (lookahead) {
    case ID:      parse_assg_or_call_stmt(); break;
    case kwIF:    parse_if_stmt(); break;
    case kwWHILE: parse_while_stmt(); break;
    case kwRETURN:parse_return_stmt(); break;
    case LBRACE:  parse_block_stmt(); break;
    case SEMI:    advance(); /* empty statement */ break;
    default:
      parse_error("expected a statement, got '%s'", lexeme);
  }
}

static void parse_block_stmt(void) {
  expect(LBRACE, "'{'");
  parse_opt_stmt_list();
  expect(RBRACE, "'}'");
  // (G2 has no new decls inside blocks; we do not open a new scope.)
}

static void parse_if_stmt(void) {
  expect(kwIF, "'if'");
  expect(LPAREN, "'('");
  parse_bool_exp();
  expect(RPAREN, "')'");
  parse_stmt();
  if (lookahead == kwELSE) {
    advance();
    parse_stmt();
  }
}

static void parse_while_stmt(void) {
  expect(kwWHILE, "'while'");
  expect(LPAREN, "'('");
  parse_bool_exp();
  expect(RPAREN, "')'");
  parse_stmt();
}

static void parse_return_stmt(void) {
  expect(kwRETURN, "'return'");
  if (lookahead == SEMI) {
    advance(); // return without expr
  } else {
    parse_arith_exp();
    expect(SEMI, "';'");
  }
  have_return_stmt = 1;
}

static void parse_assg_or_call_stmt(void) {
  char *name = dup_lexeme();
  expect(ID, "identifier");
  if (lookahead == opASSG) {
    // assignment
    if (chk_decl_flag) {
      Sym *s = st_lookup(name);
      if (!s) parse_error("assignment to undeclared variable '%s'", name);
      if (s->kind != SYM_VAR) parse_error("'%s' is not a variable", name);
    }
    advance(); // '='
    parse_arith_exp();
    expect(SEMI, "';'");
  } else if (lookahead == LPAREN) {
    // function call (with args now)
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
    advance(); // '('
    int argc = parse_opt_expr_list();
    expect(RPAREN, "')'");
    expect(SEMI, "';'");
    if (chk_decl_flag) {
      Sym *g = st_lookup_global(name);
      if (g && g->arity != argc) {
        parse_error("function '%s' called with %d args but declared with %d",
                    name, argc, g->arity);
      }
    }
  } else {
    parse_error("expected '=' or '(' after identifier '%s'", name);
  }
  free(name);
}

static void parse_bool_exp(void) {
  parse_arith_exp();
  parse_relop();
  parse_arith_exp();
}

static void parse_relop(void) {
  switch (lookahead) {
    case opLT: case opLE: case opGT: case opGE: case opEQ: case opNE:
      advance();
      break;
    default:
      parse_error("expected relational operator, got '%s'", lexeme);
  }
}

static void parse_arith_exp(void) {
  // in G0, just ID or INTLIT
  if (lookahead == ID) {
    char *name = dup_lexeme();
    if (chk_decl_flag) {
      Sym *s = st_lookup(name);
      if (!s) parse_error("use of undeclared variable '%s'", name);
      if (s->kind != SYM_VAR) parse_error("'%s' is not a variable", name);
    }
    advance();
    free(name);
  } else if (lookahead == INTCON) {
    advance();
  } else {
    parse_error("expected identifier or integer constant, got '%s'", lexeme);
  }
}

static int parse_opt_expr_list(void) {
  if (lookahead == RPAREN) {
    return 0; // ε case, do nothing
  } else {
    return parse_expr_list();
  }
}

static int parse_expr_list(void) {
  int n = 1;
  parse_arith_exp();
  while (lookahead == COMMA) {
    advance();
    parse_arith_exp();
    n++;
  }
  return n;
}


// fn_call -> ID LPAREN opt_expr_list RPAREN
// opt_expr_list -> ε in G0, so accept only ID '(' ')' here.
static void parse_fn_call(void) {
  char *func_name = dup_lexeme();
  expect(ID, "function name");

    if (chk_decl_flag) {
    int use_line = get_lineno();

    // Rule (1): no local with this name in the *containing* function
    if (st_lookup_current(func_name)) {
      parse_error("'%s' is declared as a local in this function; cannot call it as a function", func_name);
    }

    // Rule (2): must have a PRIOR global function declaration
    Sym *g = st_lookup_global(func_name);
    if (!g || g->kind != SYM_FUNC) {
      parse_error("use of undeclared function '%s'", func_name);
    }
    if (g->decl_line > use_line) {
      parse_error("call to '%s' occurs before its declaration", func_name);
    }
  }
  expect(LPAREN, "'('");
  // no arguments allowed in G0 (so require immediate ')' )
  expect(RPAREN, "')'");
  free(func_name);
}
