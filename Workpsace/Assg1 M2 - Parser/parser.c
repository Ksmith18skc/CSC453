// // CSC 453 — Assignment 1 Milestone 2 parser.c
// // Kory Smith

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "scanner.h"

static int lookahead;

static void advance(void);
static void expect(int tok, const char *msg);
static void parse_program(void);
static void parse_func_defn(void);
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
  advance();     
  parse_program();
  if (lookahead != TOK_EOF) {
    parse_error("trailing garbage after program");
  }
  return 0;
}

// program -> func_defn program | ε
static void parse_program(void) {
  while (lookahead != TOK_EOF) {
    if (lookahead == kwINT) {
      parse_func_defn();
      continue;
    }
    parse_error("unexpected token at top-level: '%s'", lexeme);
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
  expect(kwINT, "'int'");

  expect(ID, "function name");

  // '(' opt_formals ')'. opt_formals is ε in G0, so require immediate ')'
  expect(LPAREN, "'('");
  expect(RPAREN, "')'");

  expect(LBRACE, "'{'");
  while (lookahead == ID) {
    parse_stmt();
  }
  expect(RBRACE, "'}'");
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
  expect(ID, "function name");
  expect(LPAREN, "'('");
  // no arguments allowed in G0 (so require immediate ')' )
  expect(RPAREN, "')'");
}
