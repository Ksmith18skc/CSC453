// // CSC 453 — Assignment 1 Milestone 2 parser.c
// // Kory Smith

// #include <stdio.h>
// #include <stdlib.h>
// #include <stdarg.h>
// #include "scanner.h"

// static int lookahead;

// static void advance(void);
// static void parse_program(void);
// // static void parse_function(void);
// static void parse_compound_stmt(void);
// static void parse_decl(void);
// static void parse_stmt(void);
// static void parse_expr_stmt(void);
// static void parse_expr(void);
// static void parse_top_decl_or_function(void);
// static void parse_function_rest(void);

// // error printing, line num.
// static void parse_error(const char *fmt, ...) {
//   va_list ap;
//   va_start(ap, fmt);
//   int line = get_lineno();
//   fprintf(stderr, "ERROR LINE %d: ", line);
//   vfprintf(stderr, fmt, ap);
//   fprintf(stderr, "\n");
//   va_end(ap); 
//   exit(1);
// }

// static void advance(void) {
//   lookahead = get_token();
// }

// // error message details
// static void expect(int tok, const char *msg) {
//   if (lookahead != tok) {
//     parse_error("expected %s, got '%s'", msg, lexeme);
//   }
//   advance();
// }


// int parse(void) {
//   advance(); 
//   parse_program(); 
//   // if (lookahead == -1) { testing
//   //   return 0;
//   // }
//   if (lookahead != TOK_EOF && lookahead != 0) {
//     parse_error("trailing garbage after program");
//   }
//   return 0;
// }


// // G0 grammar rules

// // program -> { function }
// static void parse_program(void) {
//   while (lookahead != TOK_EOF) {
//     // if (lookahead == -1){ testing
//     //   break;
//     // }
//     if (lookahead == kwINT) {
//       parse_top_decl_or_function();
//     } else {
//       parse_error("unexpected token at top-level: '%s'", lexeme);
//     }
//   //   if (lookahead == TOK_EOF) break;
//   //   parse_function();
//   // }
//   }
// }

// static void parse_top_decl_or_function(void) {
//   /* we see 'int' at lookahead */
//   expect(kwINT, "'int' at top-level");
//   if (lookahead != ID) parse_error("expected identifier after 'int' at top-level");
//   advance(); // consume the identifier

//   if (lookahead == LPAREN) {
//     /* it's a function definition: parse the '(' ... ) compound_stmt */
//     parse_function_rest();
//     return;
//   }

//   /* otherwise it's a top-level variable declaration (possibly list) */
//   while (lookahead == COMMA) {
//     advance(); /* consume ',' */
//     if (lookahead != ID) parse_error("expected identifier after ',' in declaration");
//     advance(); /* consume identifier */
//   }
//   expect(SEMI, "';' after top-level declaration");
// }

// // --- New helper: parse the remainder of a function after 'int' ID was consumed ---
// // This mirrors your original parse_function but does NOT expect the leading 'int' ID.
// static void parse_function_rest(void) {
//   /* lookahead == LPAREN */
//   expect(LPAREN, "'('");
//   /* parameter list (optional) */
//   if (lookahead == kwINT) {
//     for (;;) {
//       advance(); /* consume 'int' */
//       if (lookahead != ID) parse_error("expected parameter name after 'int'");
//       advance(); /* consume param name */
//       if (lookahead == COMMA) { advance(); continue; }
//       break;
//     }
//   }
//   expect(RPAREN, "')'");
//   parse_compound_stmt();
// }

// // function -> 'int' ID '(' [ param-list ] ')' compound_stmt
// // static void parse_function(void) {
// //   if (lookahead != kwINT) parse_error("expected 'int' at start of function");
// //   advance();
// //   if (lookahead != ID) parse_error("expected function name after 'int'");
// //   advance();
// //   expect(LPAREN, "'('");
// //   if (lookahead == kwINT) {
// //     for (;;) {
// //       advance();
// //       if (lookahead != ID) parse_error("expected parameter name after 'int'");
// //       advance();
// //       if (lookahead == COMMA) { advance(); continue; }
// //       break;
// //     }
// //   }
// //   expect(RPAREN, "')'");
// //   parse_compound_stmt();
// // }

// // compound_stmt -> '{' { decl | stmt }* '}'
// //  A declaration is: 'int' ID ';'
// static void parse_compound_stmt(void) {
//   expect(LBRACE, "'{'");
//   while (lookahead != RBRACE) {
//     if (lookahead == kwINT) {
//       /* declaration */
//       parse_decl();
//     } else {
//       parse_stmt();
//     }
//     if (lookahead == TOK_EOF) parse_error("unexpected end of file inside compound statement");
//   }
//   expect(RBRACE, "'}'");
// }

// // decl -> 'int' ID ';'
// static void parse_decl(void) {
//   expect(kwINT, "'int' in declaration");
//   if (lookahead != ID) parse_error("expected identifier in declaration");
//   advance();
//   expect(SEMI, "';'");
// }

// // stmt -> compound_stmt | if_stmt | while_stmt | return_stmt | expr_stmt
// static void parse_stmt(void) {
//   if (lookahead == LBRACE) {
//     parse_compound_stmt();
//     return;
//   }
//   if (lookahead == kwIF) {
//     advance();
//     expect(LPAREN, "'(' after if");
//     parse_expr();
//     expect(RPAREN, "')'");
//     parse_stmt();
//     if (lookahead == kwELSE) {
//       advance();
//       parse_stmt();
//     }
//     return;
//   }
//   if (lookahead == kwWHILE) {
//     advance();
//     expect(LPAREN, "'(' after while");
//     parse_expr();
//     expect(RPAREN, "')'");
//     parse_stmt();
//     return;
//   }
//   if (lookahead == kwRETURN) {
//     advance();
//     parse_expr();
//     expect(SEMI, "';'");
//     return;
//   }
//   parse_expr_stmt();
// }

// // expr_stmt -> [ expr ] ';'   (we require expr before ';' for simplicity when lexeme is not ';')
// static void parse_expr_stmt(void) {
//   if (lookahead == SEMI) {
//     advance();
//     return;
//   }
//   parse_expr();
//   expect(SEMI, "';'");
// }


//     //  ||  (prec 10)
//     //  &&  (prec 20)
//     //  ==, != (prec 30)
//     //  <, <=, >, >= (prec 40)
//     //  +, - (prec 50)
//     //  *, / (prec 60)

// static int get_binop_prec(int tok) {
//   switch (tok) {
//     case opOR:  return 10;
//     case opAND: return 20;
//     case opEQ: case opNE: return 30;
//     case opLT: case opLE: case opGT: case opGE: return 40;
//     case opADD: case opSUB: return 50;
//     case opMUL: case opDIV: return 60;
//     default: return -1;
//   }
// }

// // parse a primary: INTCON | ID [ maybe assignment or function call? ] | '(' expr ')'
// static void parse_primary(void) {
//   if (lookahead == INTCON) {
//     advance();
//     return;
//   }
//   if (lookahead == ID) {
//     advance();
//     return;
//   }
//   if (lookahead == LPAREN) {
//     advance();
//     parse_expr();
//     expect(RPAREN, "')'");
//     return;
//   }
//   parse_error("expected expression, got '%s'", lexeme);
// }

// // parse_expr: parse unary then call parse_binary_rhs
// static void parse_expr(void) {
//   if (lookahead == opNOT || lookahead == opSUB) {
//     advance();
//     parse_expr();
//     return;
//   }
//   parse_primary();

//   // handle binary rhs with precedence climbing
//   int tok_prec = get_binop_prec(lookahead);
//   while (tok_prec >= 0) {
//     advance();
//     if (lookahead == opNOT || lookahead == opSUB) {
//       advance();
//       parse_expr();
//     } else {
//       parse_primary();
//     }

//     tok_prec = get_binop_prec(lookahead);
//   }
// }


// parser.c  -- G0-only parser for ASSG1 M2
// Accepts only the grammar described for G0
// Kory / CSC453 — trimmed to G0 rules