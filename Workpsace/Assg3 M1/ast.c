/*
 * Simple AST runtime: node struct, factories, getters and a printer
 * Reformats AST printout to match the example in the spec.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

typedef struct ASTNode {
  NodeType type;
  char *name;           /* identifier / function name / LHS name */
  int ival;             /* integer constant */
  void *op1;            /* operand / head / body / expr etc. */
  void *op2;            /* second operand / rest / then / body etc. */
  void *op3;            /* else-part or extra pointer */
  int nargs;
  char **argnames;      /* array of parameter names for func def */
} ASTNode;

/* helpers */
static ASTNode *mk(NodeType t) {
  ASTNode *n = malloc(sizeof(ASTNode));
  if (!n) { perror("malloc"); exit(1); }
  n->type = t;
  n->name = NULL; n->ival = 0; n->op1 = n->op2 = n->op3 = NULL;
  n->nargs = 0; n->argnames = NULL;
  return n;
}

static char *dupstr(const char *s) {
  if (!s) return NULL;
  size_t n = strlen(s)+1;
  char *r = malloc(n);
  if (!r) { perror("malloc"); exit(1); }
  memcpy(r, s, n);
  return r;
}

/* Factory implementations */
void *ast_new_func_def(char *name, int nargs, char **argnames, void *body) {
  ASTNode *n = mk(FUNC_DEF);
  n->name = dupstr(name);
  n->nargs = nargs;
  if (nargs > 0) {
    n->argnames = malloc(sizeof(char*) * nargs);
    for (int i = 0; i < nargs; ++i) n->argnames[i] = dupstr(argnames[i]);
  }
  n->op1 = body; /* body stored in op1 */
  return n;
}

void *ast_new_func_call(char *callee, void *args) {
  ASTNode *n = mk(FUNC_CALL);
  n->name = dupstr(callee);
  n->op1 = args; /* expr_list */
  return n;
}

void *ast_new_stmt_list(void *head, void *rest) {
  ASTNode *n = mk(STMT_LIST);
  n->op1 = head;
  n->op2 = rest;
  return n;
}

void *ast_new_expr_list(void *head, void *rest) {
  ASTNode *n = mk(EXPR_LIST);
  n->op1 = head;
  n->op2 = rest;
  return n;
}

void *ast_new_identifier(char *name) {
  ASTNode *n = mk(IDENTIFIER);
  n->name = dupstr(name);
  return n;
}

void *ast_new_intconst(int val) {
  ASTNode *n = mk(INTCONST);
  n->ival = val;
  return n;
}

void *ast_new_binary(NodeType type, void *op1, void *op2) {
  ASTNode *n = mk(type);
  n->op1 = op1;
  n->op2 = op2;
  return n;
}

void *ast_new_unary(NodeType type, void *op1) {
  ASTNode *n = mk(type);
  n->op1 = op1;
  return n;
}

void *ast_new_if(void *cond, void *thenp, void *elsep) {
  ASTNode *n = mk(IF);
  n->op1 = cond;
  n->op2 = thenp;
  n->op3 = elsep;
  return n;
}

void *ast_new_while(void *cond, void *body) {
  ASTNode *n = mk(WHILE);
  n->op1 = cond;
  n->op2 = body;
  return n;
}

void *ast_new_assg(char *lhs_name, void *rhs) {
  ASTNode *n = mk(ASSG);
  n->name = dupstr(lhs_name);
  n->op1 = rhs;
  return n;
}

void *ast_new_return(void *expr) {
  ASTNode *n = mk(RETURN);
  n->op1 = expr;
  return n;
}

/* Getter implementations */
NodeType ast_node_type(void *ptr) { return ((ASTNode*)ptr)->type; }
char * func_def_name(void *ptr) { return ((ASTNode*)ptr)->name; }
int func_def_nargs(void *ptr) { return ((ASTNode*)ptr)->nargs; }
char *func_def_argname(void *ptr, int n) { return ((ASTNode*)ptr)->argnames[n-1]; }
void * func_def_body(void *ptr) { return ((ASTNode*)ptr)->op1; }
char * func_call_callee(void *ptr) { return ((ASTNode*)ptr)->name; }
void * func_call_args(void *ptr) { return ((ASTNode*)ptr)->op1; }
void * stmt_list_head(void *ptr) { return ((ASTNode*)ptr)->op1; }
void * stmt_list_rest(void *ptr) { return ((ASTNode*)ptr)->op2; }
void * expr_list_head(void *ptr) { return ((ASTNode*)ptr)->op1; }
void * expr_list_rest(void *ptr) { return ((ASTNode*)ptr)->op2; }
char *expr_id_name(void *ptr) { return ((ASTNode*)ptr)->name; }
int expr_intconst_val(void *ptr) { return ((ASTNode*)ptr)->ival; }
void * expr_operand_1(void *ptr) { return ((ASTNode*)ptr)->op1; }
void * expr_operand_2(void *ptr) { return ((ASTNode*)ptr)->op2; }
void * stmt_if_expr(void *ptr) { return ((ASTNode*)ptr)->op1; }
void * stmt_if_then(void *ptr) { return ((ASTNode*)ptr)->op2; }
void * stmt_if_else(void *ptr) { return ((ASTNode*)ptr)->op3; }
char *stmt_assg_lhs(void *ptr) { return ((ASTNode*)ptr)->name; }
void *stmt_assg_rhs(void *ptr) { return ((ASTNode*)ptr)->op1; }
void *stmt_while_expr(void *ptr) { return ((ASTNode*)ptr)->op1; }
void *stmt_while_body(void *ptr) { return ((ASTNode*)ptr)->op2; }
void *stmt_return_expr(void *ptr) { return ((ASTNode*)ptr)->op1; }

/* Printing helpers -------------------------------------------------------- */

static void print_indent(int indent) {
  for (int i = 0; i < indent; ++i) putchar(' ');
}

/* Forward decls */
static void print_stmt_list(void *node, int indent, int enclose);
static void print_stmt(void *node, int indent);
static void print_expr(void *node);

/* print expression lists as comma-separated */
static void print_expr_list_inline(void *elist) {
  if (!elist) return;
  void *head = expr_list_head(elist);
  print_expr(head);
  void *rest = expr_list_rest(elist);
  if (rest) {
    printf(", ");
    print_expr_list_inline(rest);
  }
}

static void print_call_inline(void *node) {
  printf("%s", func_call_callee(node));
  void *args = func_call_args(node);
  printf("(");
  if (args) print_expr_list_inline(args);
  printf(")");
}

static void print_expr(void *node) {
  if (!node) {return;}
  switch (ast_node_type(node)) {
    case IDENTIFIER:
      printf("%s", expr_id_name(node));
      return;
    case INTCONST:
      printf("%d", expr_intconst_val(node));
      return;
    case FUNC_CALL:
      /* print a function call expression properly */
      print_call_inline(node);
      return;
    default:
      /* unary */
      if (ast_node_type(node) == UMINUS) {
        printf("-");
        print_expr(expr_operand_1(node));
        return;
      }
      /* binary */
      {
        void *l = expr_operand_1(node);
        void *r = expr_operand_2(node);
        if (l) print_expr(l);
        switch (ast_node_type(node)) {
          case ADD:  printf(" + "); break;
          case SUB:  printf(" - "); break;
          case MUL:  printf(" * "); break;
          case DIV:  printf(" / "); break;
          case EQ:   printf(" == "); break;
          case NE:   printf(" != "); break;
          case LT:   printf(" < "); break;
          case LE:   printf(" <= "); break;
          case GT:   printf(" > "); break;
          case GE:   printf(" >= "); break;
          case AND:  printf(" && "); break;
          case OR:   printf(" || "); break;
          default:   printf(" ? "); break;
        }
        if (r) print_expr(r);
        return;
      }
  }
}

/* Print a single statement (could be a STMT_LIST when block) */
static void print_stmt(void *node, int indent) {
  if (!node) return;
  NodeType t = ast_node_type(node);
  switch (t) {
    case STMT_LIST:
      print_stmt_list(node, indent, 1); /* when a STMT_LIST appears as a stmt, print braces */
      break;
    case FUNC_CALL:
      print_indent(indent);
      print_call_inline(node);
      printf("\n");
      break;
    case ASSG:
      print_indent(indent);
      printf("%s = ", stmt_assg_lhs(node));
      print_expr(stmt_assg_rhs(node));
      printf("\n");
      break;
    case IF: {
      /* print if at current indent */
      print_indent(indent);
      printf("if (");
      print_expr(stmt_if_expr(node));
      printf("):\n");

      /* then: at same indent as 'if' */
      print_indent(indent);
      printf("then:\n");

      /* then-block at indent+4 */
      void *thenp = stmt_if_then(node);
      if (thenp && ast_node_type(thenp) == STMT_LIST) {
        print_stmt_list(thenp, indent + 4, 1);
      } else {
        print_stmt(thenp, indent + 4);
      }

      /* else: at same indent as 'if' */
      print_indent(indent);
      printf("else:\n");
      void *elsep = stmt_if_else(node);
      if (elsep) {
        if (ast_node_type(elsep) == STMT_LIST) {
          print_stmt_list(elsep, indent + 4, 1);
        } else {
          print_stmt(elsep, indent + 4);
        }
      }

      /* end_if at same indent as 'if' */
      print_indent(indent);
      printf("end_if\n");
      break;
    }
    case WHILE: {
      print_indent(indent);
      printf("while (");
      print_expr(stmt_while_expr(node));
      printf("):\n");
      void *body = stmt_while_body(node);
      if (body && ast_node_type(body) == STMT_LIST) {
        print_stmt_list(body, indent + 4, 1);
      } else {
        print_stmt(body, indent + 4);
      }
      /* end_while at same indent as 'while' */
      print_indent(indent);
      printf("end_while\n");
      break;
    }
    case RETURN:
      print_indent(indent);
      printf("return: ");
      if (stmt_return_expr(node))
        print_expr(stmt_return_expr(node));
      printf("\n");
      break;
    default:
      print_indent(indent);
      print_expr(node);
      printf("\n");
      break;
  }
}

/* print a statement list; if enclose is true print { and } around contents */
static void print_stmt_list(void *node, int indent, int enclose) {
  if (!node) {
    if (enclose) {
      print_indent(indent);
      printf("{\n");
      print_indent(indent);
      printf("}\n");
    }
    return;
  }
  if (enclose) {
    print_indent(indent);
    printf("{\n");
  }

  /* iterate through head/rest */
  void *cur = node;
  while (cur) {
    void *head = stmt_list_head(cur);
    if (head) print_stmt(head, enclose ? indent + 4 : indent);
    cur = stmt_list_rest(cur);
  }

  if (enclose) {
    print_indent(indent);
    printf("}\n");
  }
}

/* Top-level AST printing tuned to the expected format */
void print_ast(void *tree) {
  if (!tree) return;
  if (ast_node_type(tree) != FUNC_DEF) {
    /* support printing arbitrary nodes by dispatching */
    switch (ast_node_type(tree)) {
      case STMT_LIST: print_stmt_list(tree, 0, 0); break;
      default: print_stmt(tree, 0); break;
    }
    return;
  }

  /* function header */
  char *fname = func_def_name(tree);
  printf("func_def: %s\n", fname ? fname : "<anon>");

  /* formals */
  int nargs = func_def_nargs(tree);
  printf("  formals: ");
  if (nargs == 0) {
    printf("\n");
  } else {
    for (int i = 1; i <= nargs; ++i) {
      printf("%s", func_def_argname(tree, i));
      if (i < nargs) printf(", ");
    }
    printf("\n");
  }

  /* body */
  printf("  body:\n");
  void *body = func_def_body(tree);
  if (!body) {
    /* empty body -> print nothing between body: and closing comment */
  } else {
    /* body should be a STMT_LIST -> print with braces and indent two spaces further */
    print_stmt_list(body, 4, 1);
  }

  printf("/* func_def: %s */\n\n", fname ? fname : "<anon>");
}

