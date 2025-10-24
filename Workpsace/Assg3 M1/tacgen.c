/*
 * File: tacgen.c
 * Purpose: Three-address code generation from AST
 * CSC 453 - Assignment 3 Milestone 1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

/* Global counters for temporaries and labels */
static int temp_counter = 0;
static int label_counter = 0;

/* Helper to generate new temporary variable names */
static char *new_temp(void) {
  char *t = malloc(16);
  if (!t) { perror("malloc"); exit(1); }
  sprintf(t, "t%d", temp_counter++);
  return t;
}

/* Helper to generate new label names */
static char *new_label(void) {
  char *l = malloc(16);
  if (!l) { perror("malloc"); exit(1); }
  sprintf(l, "L%d", label_counter++);
  return l;
}

/* Forward declarations */
static char *gen_expr(void *expr);
static void gen_stmt(void *stmt);
static void gen_stmt_list(void *stmt_list);

/*
 * gen_expr: Generate TAC for an expression and return the temp holding result
 * Returns the name of the temporary variable holding the result
 */
static char *gen_expr(void *expr) {
  if (!expr) return NULL;
  
  NodeType type = ast_node_type(expr);
  
  switch (type) {
    case IDENTIFIER: {
      /* For identifiers, just return the name (no need for temp) */
      char *name = expr_id_name(expr);
      char *result = malloc(strlen(name) + 1);
      if (!result) { perror("malloc"); exit(1); }
      strcpy(result, name);
      return result;
    }
    
    case INTCONST: {
      /* For constants, create a temp and assign */
      char *t = new_temp();
      int val = expr_intconst_val(expr);
      printf("    %s = %d\n", t, val);
      return t;
    }
    
    case FUNC_CALL: {
      /* Function call: push args, call, get result */
      char *callee = func_call_callee(expr);
      void *args = func_call_args(expr);
      
      /* Generate code for each argument and push */
      void *arg_list = args;
      while (arg_list) {
        void *arg = expr_list_head(arg_list);
        char *arg_temp = gen_expr(arg);
        printf("    push %s\n", arg_temp);
        free(arg_temp);
        arg_list = expr_list_rest(arg_list);
      }
      
      /* Call function */
      char *result = new_temp();
      printf("    %s = call %s\n", result, callee);
      return result;
    }
    
    case ADD:
    case SUB:
    case MUL:
    case DIV: {
      /* Arithmetic binary operations */
      void *left = expr_operand_1(expr);
      void *right = expr_operand_2(expr);
      
      char *t1 = gen_expr(left);
      char *t2 = gen_expr(right);
      char *result = new_temp();
      
      const char *op;
      switch (type) {
        case ADD: op = "+"; break;
        case SUB: op = "-"; break;
        case MUL: op = "*"; break;
        case DIV: op = "/"; break;
        default: op = "?"; break;
      }
      
      printf("    %s = %s %s %s\n", result, t1, op, t2);
      free(t1);
      free(t2);
      return result;
    }
    
    case EQ:
    case NE:
    case LT:
    case LE:
    case GT:
    case GE: {
      /* Relational operations - return a temp with 0 or 1 */
      void *left = expr_operand_1(expr);
      void *right = expr_operand_2(expr);
      
      char *t1 = gen_expr(left);
      char *t2 = gen_expr(right);
      char *result = new_temp();
      
      const char *op;
      switch (type) {
        case EQ: op = "=="; break;
        case NE: op = "!="; break;
        case LT: op = "<"; break;
        case LE: op = "<="; break;
        case GT: op = ">"; break;
        case GE: op = ">="; break;
        default: op = "?"; break;
      }
      
      printf("    %s = %s %s %s\n", result, t1, op, t2);
      free(t1);
      free(t2);
      return result;
    }
    
    case UMINUS: {
      /* Unary minus */
      void *operand = expr_operand_1(expr);
      char *t1 = gen_expr(operand);
      char *result = new_temp();
      printf("    %s = - %s\n", result, t1);
      free(t1);
      return result;
    }
    
    default:
      fprintf(stderr, "gen_expr: unknown expression type %d\n", type);
      return NULL;
  }
}

/*
 * gen_stmt: Generate TAC for a single statement
 */
static void gen_stmt(void *stmt) {
  if (!stmt) return;
  
  NodeType type = ast_node_type(stmt);
  
  switch (type) {
    case ASSG: {
      /* Assignment: x = expr */
      char *lhs = stmt_assg_lhs(stmt);
      void *rhs = stmt_assg_rhs(stmt);
      char *rhs_temp = gen_expr(rhs);
      printf("    %s = %s\n", lhs, rhs_temp);
      free(rhs_temp);
      break;
    }
    
    case FUNC_CALL: {
      /* Function call as statement (result discarded) */
      char *callee = func_call_callee(stmt);
      void *args = func_call_args(stmt);
      
      /* Generate code for each argument and push */
      void *arg_list = args;
      while (arg_list) {
        void *arg = expr_list_head(arg_list);
        char *arg_temp = gen_expr(arg);
        printf("    push %s\n", arg_temp);
        free(arg_temp);
        arg_list = expr_list_rest(arg_list);
      }
      
      /* Call function (result unused) */
      printf("    call %s\n", callee);
      break;
    }
    
    case IF: {
      /* If statement: if (cond) then_stmt else else_stmt */
      void *cond = stmt_if_expr(stmt);
      void *then_part = stmt_if_then(stmt);
      void *else_part = stmt_if_else(stmt);
      
      char *cond_temp = gen_expr(cond);
      char *else_label = new_label();
      char *end_label = new_label();
      
      /* if cond == 0 goto else_label */
      printf("    ifz %s goto %s\n", cond_temp, else_label);
      free(cond_temp);
      
      /* Then part */
      if (then_part) {
        if (ast_node_type(then_part) == STMT_LIST) {
          gen_stmt_list(then_part);
        } else {
          gen_stmt(then_part);
        }
      }
      
      /* Jump to end after then */
      printf("    goto %s\n", end_label);
      
      /* Else label */
      printf("%s:\n", else_label);
      
      /* Else part */
      if (else_part) {
        if (ast_node_type(else_part) == STMT_LIST) {
          gen_stmt_list(else_part);
        } else {
          gen_stmt(else_part);
        }
      }
      
      /* End label */
      printf("%s:\n", end_label);
      
      free(else_label);
      free(end_label);
      break;
    }
    
    case WHILE: {
      /* While statement: while (cond) body */
      void *cond = stmt_while_expr(stmt);
      void *body = stmt_while_body(stmt);
      
      char *start_label = new_label();
      char *end_label = new_label();
      
      /* Start label */
      printf("%s:\n", start_label);
      
      /* Check condition */
      char *cond_temp = gen_expr(cond);
      printf("    ifz %s goto %s\n", cond_temp, end_label);
      free(cond_temp);
      
      /* Body */
      if (body) {
        if (ast_node_type(body) == STMT_LIST) {
          gen_stmt_list(body);
        } else {
          gen_stmt(body);
        }
      }
      
      /* Jump back to start */
      printf("    goto %s\n", start_label);
      
      /* End label */
      printf("%s:\n", end_label);
      
      free(start_label);
      free(end_label);
      break;
    }
    
    case RETURN: {
      /* Return statement */
      void *expr = stmt_return_expr(stmt);
      if (expr) {
        char *result = gen_expr(expr);
        printf("    return %s\n", result);
        free(result);
      } else {
        printf("    return\n");
      }
      break;
    }
    
    case STMT_LIST: {
      /* Statement list - recursively process */
      gen_stmt_list(stmt);
      break;
    }
    
    default:
      fprintf(stderr, "gen_stmt: unknown statement type %d\n", type);
      break;
  }
}

/*
 * gen_stmt_list: Generate TAC for a statement list
 */
static void gen_stmt_list(void *stmt_list) {
  void *current = stmt_list;
  while (current) {
    void *head = stmt_list_head(current);
    if (head) {
      gen_stmt(head);
    }
    current = stmt_list_rest(current);
  }
}

/*
 * gen_function: Generate TAC for a function definition
 */
void gen_function(void *func) {
  if (!func || ast_node_type(func) != FUNC_DEF) {
    return;
  }
  
  /* Reset counters for each function */
  temp_counter = 0;
  label_counter = 0;
  
  /* Function header */
  char *fname = func_def_name(func);
  printf("\n/* Function: %s */\n", fname);
  printf("begin_func %s\n", fname);
  
  /* Parameters */
  int nargs = func_def_nargs(func);
  for (int i = 1; i <= nargs; i++) {
    char *param = func_def_argname(func, i);
    printf("    param %s\n", param);
  }
  
  /* Function body */
  void *body = func_def_body(func);
  if (body) {
    gen_stmt_list(body);
  }
  
  /* End function */
  printf("end_func %s\n", fname);
}

/*
 * generate_tac: Main entry point for TAC generation
 * Called from the driver when gen_code_flag is set
 */
void generate_tac(void *ast) {
  if (!ast) return;
  
  printf("\n/* Three-Address Code */\n");
  gen_function(ast);
}
