// CSC 453 — Milestone 1 scanner
// Kory Smith

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "scanner.h"

#ifndef LEXEME_MAX
#define LEXEME_MAX 1024
#endif

//lexeme buffer
static char lexeme_buf[LEXEME_MAX];
char *lexeme = lexeme_buf;

static inline int nextc(void) {
  return getchar();
}
static inline void unnextc(int c) {
  if (c != EOF) ungetc(c, stdin);
}
static inline void clear_lexeme(void) {
  lexeme_buf[0] = '\0';
}
static inline void push_lexeme(size_t *n, int c) {
  if (*n + 1 < LEXEME_MAX) {
    lexeme_buf[*n] = (char)c;
    (*n)++;
    lexeme_buf[*n] = '\0';
  }
}
static inline void set_lexeme1(int c) {
  lexeme_buf[0] = (char)c;
  lexeme_buf[1] = '\0';
}

static bool is_ident_start(int c) {
  return isalpha(c) || c == '_';
}
static bool is_ident_cont(int c) {
  return isalnum(c) || c == '_';
}

// ---- comment skipping ----


// Trashes /*...*/ block comments; returns true if successful.
static bool consume_block_comment(int first_slash_consumed) {
  if (!first_slash_consumed) return false;
  int d = nextc();
  if (d != '*') { unnextc(d); return false; }
  // eat until closing */
  int prev = 0, c;
  while ((c = nextc()) != EOF) {
    if (prev == '*' && c == '/') break;
    prev = c;
  }
  return true; 
}

// Skip any amount of whitespace and comments.
static void skip_whitespace_and_comments(void) {
  for (;;) {
    // skip whitespace
    int c = nextc();
    while (c != EOF && isspace(c)) c = nextc();
    if (c == EOF) { unnextc(c); return; }

    if (c == '/') {
      // try comments; else, unget
      if (consume_block_comment(/*first_slash_consumed=*/1)) continue;
      // not a comment
      unnextc(c);
      return;
    }

    // nothing to skip
    unnextc(c);
    return;
  }
}

// Map identifier lexeme to KW or ID.
static Token maybe_keyword(const char *s) {
  if (strcmp(s, "int") == 0) 
    return kwINT;
  if (strcmp(s, "if") == 0) 
    return kwIF;
  if (strcmp(s, "else") == 0) 
    return kwELSE;
  if (strcmp(s, "while") == 0) 
    return kwWHILE;
  if (strcmp(s, "return") == 0) 
    return kwRETURN;
  return ID;
}

int get_token(void) {
  clear_lexeme();
  
  skip_whitespace_and_comments();

  int c = nextc();
  if (c == EOF) return EOF;

  // Identifiers / keywords: ('Letter followed by seq. of letters, digits, underscores')
  if (is_ident_start(c)) {
    size_t n = 0;
    push_lexeme(&n, c);
    while (true) {
      int d = nextc();
      if (!is_ident_cont(d)) {
        unnextc(d);
        break;
      }
      push_lexeme(&n, d);
    }
    return maybe_keyword(lexeme_buf);
  }

  // Integer constants: 'If (pattern match for INTCONST), save int const. val and return INTCONST token to driver)'
  if (isdigit(c)) {
    size_t n = 0;
    push_lexeme(&n, c);
    while (true) {
      int d = nextc();
      if (!isdigit(d)) {
        unnextc(d);
        break;
      }
      push_lexeme(&n, d);
    }
    return INTCON;
  }

  // Two Char. op(s) (Check for extra char.)
  {
    int d = nextc();
    if (c == '=' && d == '=') { 
      strcpy(lexeme_buf, "=="); return opEQ; }

    if (c == '!' && d == '=') { 
      strcpy(lexeme_buf, "!="); return opNE; }

    if (c == '>' && d == '=') { 
      strcpy(lexeme_buf, ">="); return opGE; }

    if (c == '<' && d == '=') { 
      strcpy(lexeme_buf, "<="); return opLE; }

    if (c == '&' && d == '&') { 
      strcpy(lexeme_buf, "&&"); return opAND; }

    if (c == '|' && d == '|') { 
      strcpy(lexeme_buf, "||"); return opOR; }
    // not a 2-char op.
    unnextc(d);
  }

  // Single-character tokens and operators
  set_lexeme1(c);
  switch (c) {
    case '(': return LPAREN;
    case ')': return RPAREN;
    case '{': return LBRACE;
    case '}': return RBRACE;
    case ',': return COMMA;
    case ';': return SEMI;
    case '=': return opASSG;
    case '+': return opADD;
    case '-': return opSUB;
    case '*': return opMUL;
    case '/': return opDIV;
    case '>': return opGT;
    case '<': return opLT;
    case '!': return opNOT;
    default:  return UNDEF; // unknown char.
  }
}
