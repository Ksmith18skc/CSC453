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

static void parse_func_defn(void);
static void parse_opt_formals(void);
static void parse_formals(void);

static void parse_var_decl(void);
static void parse_id_list_after_first_id(void);
static void parse_type(void);

static void parse_opt_var_decls(void);
static void parse_opt_stmt_list(void);


static void parse_stmt(void);
static void parse_fn_call(void);

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
    parse_func_defn();
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

static void parse_func_defn(void) {
  // expect(kwINT, "'int'");

  // expect(ID, "function name");

  expect(LPAREN, "'('");
  if (chk_decl_flag)
  {
    st_enter_scope(); // enter f(n) local scope
  }
  parse_opt_formals();
  expect(RPAREN, "')'");

  expect(LBRACE, "'{'");
  parse_opt_var_decls();
  parse_opt_stmt_list();
  expect(RBRACE, "'}'");
  if (chk_decl_flag)
  {
    st_leave_scope(); // exit f(n) local scope
  }
}

// opt_formals -> formals | ε
static void parse_opt_formals(void) {
  if (lookahead == kwINT) {
    parse_formals();
  }else {
    return; // ε case, do nothing
  }
}

// formals -> type ID COMMA formals | type ID
static void parse_formals(void){
  // first param
  parse_type();
  char *p = dup_lexeme(); expect(ID, "parameter name");
  if (chk_decl_flag && st_insert(p, SYM_VAR, get_lineno()))
    parse_error("parameter '%s' declared more than once in this function", p);
  free(p);

  while (lookahead == COMMA) {
    advance();
    parse_type();
    p = dup_lexeme(); expect(ID, "parameter name");
    if (chk_decl_flag && st_insert(p, SYM_VAR, get_lineno()))
      parse_error("parameter '%s' declared more than once in this function", p);
    free(p);
  }
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
static void parse_opt_stmt_list(void) {
  while (lookahead == ID) {
    parse_stmt();
  }
  return; // ε case, do nothing
}

// stmt -> fn_call SEMI
// fn_call -> ID LPAREN opt_expr_list RPAREN
// opt_expr_list -> ε  (so require immediate ')' )
static void parse_stmt(void) {
  parse_fn_call();
  expect(SEMI, "';'");
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
