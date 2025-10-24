/*
 * File: mipsgen.c
 * Purpose: MIPS code generation from AST
 * CSC 453 - Assignment 3 Milestone 1
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "symtab.h"

/* Register allocation */
#define NUM_TEMP_REGS 10
static int temp_reg_counter = 0;

/* Stack frame management */
static int stack_offset = 0;
static int max_args = 0;  /* Track maximum args needed for calls */

/* Variable location tracking */
typedef struct VarLocation {
    char *name;
    int offset;      /* offset from $fp (negative for locals) */
    int is_global;   /* 1 if global, 0 if local */
    struct VarLocation *next;
} VarLocation;

static VarLocation *var_locations = NULL;
static int local_var_count = 0;
static int println_used = 0;  /* Track if println is called */

/* Global variable tracking */
typedef struct GlobalVar {
    char *name;
    struct GlobalVar *next;
} GlobalVar;

static GlobalVar *global_vars = NULL;

/* Add a global variable to the list */
void register_global_var(const char *name) {
  /* Check if already registered */
  for (GlobalVar *g = global_vars; g; g = g->next) {
    if (strcmp(g->name, name) == 0) return;
  }
  
  GlobalVar *gv = malloc(sizeof(GlobalVar));
  if (!gv) { perror("malloc"); exit(1); }
  gv->name = malloc(strlen(name) + 1);
  strcpy(gv->name, name);
  gv->next = global_vars;
  global_vars = gv;
}

/* Emit all global variables in .data section */
static void emit_global_vars(void) {
  if (!global_vars) return;
  
  for (GlobalVar *g = global_vars; g; g = g->next) {
    printf("_%s: .word 0\n", g->name);
  }
  printf("\n");
}

/* Helper to get next available temporary register */
static const char *get_temp_reg(void) {
  static const char *temp_regs[NUM_TEMP_REGS] = {
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "$t9"
  };
  int reg = temp_reg_counter % NUM_TEMP_REGS;
  temp_reg_counter++;
  return temp_regs[reg];
}

/* Label generation */
static int label_counter = 0;

static char *new_label(void) {
  char *l = malloc(16);
  if (!l) { perror("malloc"); exit(1); }
  sprintf(l, "L%d", label_counter++);
  return l;
}

/* Variable location management */
static void clear_var_locations(void) {
  VarLocation *curr = var_locations;
  while (curr) {
    VarLocation *next = curr->next;
    free(curr->name);
    free(curr);
    curr = next;
  }
  var_locations = NULL;
  local_var_count = 0;
}

static void add_var_location(const char *name, int offset, int is_global) {
  VarLocation *loc = malloc(sizeof(VarLocation));
  if (!loc) { perror("malloc"); exit(1); }
  loc->name = malloc(strlen(name) + 1);
  strcpy(loc->name, name);
  loc->offset = offset;
  loc->is_global = is_global;
  loc->next = var_locations;
  var_locations = loc;
}

static VarLocation *find_var_location(const char *name) {
  for (VarLocation *loc = var_locations; loc; loc = loc->next) {
    if (strcmp(loc->name, name) == 0) {
      return loc;
    }
  }
  return NULL;
}

/* Scan function body to count local variables */
static void scan_for_locals(void *body);
static void scan_expr_for_vars(void *expr);

static void scan_expr_for_vars(void *expr) {
  if (!expr) return;
  
  NodeType type = ast_node_type(expr);
  
  switch (type) {
    case IDENTIFIER: {
      /* Just checking if variable exists in our table */
      break;
    }
    case FUNC_CALL: {
      char *callee = func_call_callee(expr);
      /* Check if println is being called */
      if (strcmp(callee, "println") == 0) {
        println_used = 1;
      }
      
      void *args = func_call_args(expr);
      void *arg_list = args;
      while (arg_list) {
        void *arg = expr_list_head(arg_list);
        scan_expr_for_vars(arg);
        arg_list = expr_list_rest(arg_list);
      }
      break;
    }
    case ADD:
    case SUB:
    case MUL:
    case DIV:
    case EQ:
    case NE:
    case LT:
    case LE:
    case GT:
    case GE: {
      scan_expr_for_vars(expr_operand_1(expr));
      scan_expr_for_vars(expr_operand_2(expr));
      break;
    }
    case UMINUS: {
      scan_expr_for_vars(expr_operand_1(expr));
      break;
    }
    default:
      break;
  }
}

static void scan_stmt_for_locals(void *stmt) {
  if (!stmt) return;
  
  NodeType type = ast_node_type(stmt);
  
  switch (type) {
    case ASSG: {
      char *lhs = stmt_assg_lhs(stmt);
      void *rhs = stmt_assg_rhs(stmt);
      
      /* Check if LHS variable is already tracked */
      if (!find_var_location(lhs)) {
        /* Check if it's a global variable */
        int is_global = 0;
        for (GlobalVar *g = global_vars; g; g = g->next) {
          if (strcmp(g->name, lhs) == 0) {
            is_global = 1;
            break;
          }
        }
        
        if (!is_global) {
          /* This is a local variable, allocate stack space */
          int offset = -4 * (local_var_count + 1);
          add_var_location(lhs, offset, 0);
          local_var_count++;
        }
      }
      
      /* Scan RHS expression for any variable references */
      scan_expr_for_vars(rhs);
      break;
    }
    case FUNC_CALL: {
      char *callee = func_call_callee(stmt);
      /* Check if println is being called */
      if (strcmp(callee, "println") == 0) {
        println_used = 1;
      }
      
      void *args = func_call_args(stmt);
      void *arg_list = args;
      while (arg_list) {
        void *arg = expr_list_head(arg_list);
        scan_expr_for_vars(arg);
        arg_list = expr_list_rest(arg_list);
      }
      break;
    }
    case IF: {
      scan_expr_for_vars(stmt_if_expr(stmt));
      void *then_part = stmt_if_then(stmt);
      void *else_part = stmt_if_else(stmt);
      scan_stmt_for_locals(then_part);
      scan_stmt_for_locals(else_part);
      break;
    }
    case WHILE: {
      scan_expr_for_vars(stmt_while_expr(stmt));
      void *body = stmt_while_body(stmt);
      scan_stmt_for_locals(body);
      break;
    }
    case RETURN: {
      scan_expr_for_vars(stmt_return_expr(stmt));
      break;
    }
    case STMT_LIST: {
      scan_for_locals(stmt);
      break;
    }
    default:
      break;
  }
}

static void scan_for_locals(void *body) {
  if (!body) return;
  
  void *current = body;
  while (current) {
    void *head = stmt_list_head(current);
    if (head) {
      scan_stmt_for_locals(head);
    }
    current = stmt_list_rest(current);
  }
}

/* Forward declarations */
static const char *gen_expr_mips(void *expr);
static void gen_stmt_mips(void *stmt);
static void gen_stmt_list_mips(void *stmt_list);

/*
 * gen_expr_mips: Generate MIPS code for an expression
 * Returns the register name holding the result
 */
static const char *gen_expr_mips(void *expr) {
  if (!expr) return NULL;
  
  NodeType type = ast_node_type(expr);
  
  switch (type) {
    case IDENTIFIER: {
      /* Load variable from memory */
      char *name = expr_id_name(expr);
      const char *reg = get_temp_reg();
      VarLocation *loc = find_var_location(name);
      if (loc) {
        if (loc->is_global) {
          printf("    lw %s, _%s\n", reg, name);
        } else {
          printf("    lw %s, %d($fp)\n", reg, loc->offset);
        }
      } else {
        /* Assume it's a global if not found in local scope */
        printf("    lw %s, _%s\n", reg, name);
      }
      return reg;
    }
    
    case INTCONST: {
      /* Load immediate constant and store on stack */
      int val = expr_intconst_val(expr);
      const char *reg = get_temp_reg();
      
      /* Calculate temp offset: after all local variables */
      int temp_offset = -4 * (local_var_count + temp_reg_counter);
      
      printf("    li %s, %d\n", reg, val);
      printf("    sw %s, %d($fp)\n", reg, temp_offset);
      printf("    lw %s, %d($fp)\n", reg, temp_offset);
      return reg;
    }
    
    case FUNC_CALL: {
      /* Function call: push args, call, get result */
      char *callee = func_call_callee(expr);
      void *args = func_call_args(expr);
      
      /* Count arguments */
      int argc = 0;
      void *arg_count = args;
      while (arg_count) {
        argc++;
        arg_count = expr_list_rest(arg_count);
      }
      
      /* Evaluate arguments and push onto stack */
      void *arg_list = args;
      while (arg_list) {
        void *arg = expr_list_head(arg_list);
        const char *arg_reg = gen_expr_mips(arg);
        printf("    la $sp, -4($sp)\n");
        printf("    sw %s, 0($sp)\n", arg_reg);
        arg_list = expr_list_rest(arg_list);
      }
      
      /* Call function with underscore prefix */
      printf("    jal _%s\n", callee);
      
      /* Clean up stack (pop arguments) */
      if (argc > 0) {
        printf("    la $sp, %d($sp)\n", argc * 4);
      }
      
      /* Result is in $v0 */
      const char *result_reg = get_temp_reg();
      printf("    move %s, $v0\n", result_reg);
      return result_reg;
    }
    
    case ADD:
    case SUB:
    case MUL:
    case DIV: {
      /* Arithmetic binary operations */
      void *left = expr_operand_1(expr);
      void *right = expr_operand_2(expr);
      
      const char *r1 = gen_expr_mips(left);
      const char *r2 = gen_expr_mips(right);
      const char *result = get_temp_reg();
      
      switch (type) {
        case ADD:
          printf("    add %s, %s, %s\n", result, r1, r2);
          break;
        case SUB:
          printf("    sub %s, %s, %s\n", result, r1, r2);
          break;
        case MUL:
          printf("    mul %s, %s, %s\n", result, r1, r2);
          break;
        case DIV:
          printf("    div %s, %s\n", r1, r2);
          printf("    mflo %s\n", result);
          break;
        default:
          break;
      }
      
      return result;
    }
    
    case EQ:
    case NE:
    case LT:
    case LE:
    case GT:
    case GE: {
      /* Relational operations */
      void *left = expr_operand_1(expr);
      void *right = expr_operand_2(expr);
      
      const char *r1 = gen_expr_mips(left);
      const char *r2 = gen_expr_mips(right);
      const char *result = get_temp_reg();
      
      switch (type) {
        case EQ:
          printf("    seq %s, %s, %s\n", result, r1, r2);
          break;
        case NE:
          printf("    sne %s, %s, %s\n", result, r1, r2);
          break;
        case LT:
          printf("    slt %s, %s, %s\n", result, r1, r2);
          break;
        case LE:
          printf("    sle %s, %s, %s\n", result, r1, r2);
          break;
        case GT:
          printf("    sgt %s, %s, %s\n", result, r1, r2);
          break;
        case GE:
          printf("    sge %s, %s, %s\n", result, r1, r2);
          break;
        default:
          break;
      }
      
      return result;
    }
    
    case UMINUS: {
      /* Unary minus */
      void *operand = expr_operand_1(expr);
      const char *r1 = gen_expr_mips(operand);
      const char *result = get_temp_reg();
      printf("    neg %s, %s\n", result, r1);
      return result;
    }
    
    default:
      fprintf(stderr, "gen_expr_mips: unknown expression type %d\n", type);
      return NULL;
  }
}

/*
 * gen_stmt_mips: Generate MIPS code for a single statement
 */
static void gen_stmt_mips(void *stmt) {
  if (!stmt) return;
  
  NodeType type = ast_node_type(stmt);
  
  switch (type) {
    case ASSG: {
      /* Assignment: x = expr */
      char *lhs = stmt_assg_lhs(stmt);
      void *rhs = stmt_assg_rhs(stmt);
      const char *rhs_reg = gen_expr_mips(rhs);
      
      VarLocation *loc = find_var_location(lhs);
      if (loc) {
        if (loc->is_global) {
          printf("    sw %s, _%s\n", rhs_reg, lhs);
        } else {
          printf("    sw %s, %d($fp)\n", rhs_reg, loc->offset);
        }
      } else {
        /* Assume it's a global if not found */
        printf("    sw %s, _%s\n", rhs_reg, lhs);
      }
      break;
    }
    
    case FUNC_CALL: {
      /* Function call as statement (result discarded) */
      char *callee = func_call_callee(stmt);
      void *args = func_call_args(stmt);
      
      /* Count arguments */
      int argc = 0;
      void *arg_count = args;
      while (arg_count) {
        argc++;
        arg_count = expr_list_rest(arg_count);
      }
      
      /* Evaluate arguments and push onto stack */
      void *arg_list = args;
      while (arg_list) {
        void *arg = expr_list_head(arg_list);
        const char *arg_reg = gen_expr_mips(arg);
        printf("    la $sp, -4($sp)\n");
        printf("    sw %s, 0($sp)\n", arg_reg);
        arg_list = expr_list_rest(arg_list);
      }
      
      /* Call function with underscore prefix */
      printf("    jal _%s\n", callee);
      
      /* Clean up stack */
      if (argc > 0) {
        printf("    la $sp, %d($sp)\n", argc * 4);
      }
      break;
    }
    
    case IF: {
      /* If statement: if (cond) then_stmt else else_stmt */
      void *cond = stmt_if_expr(stmt);
      void *then_part = stmt_if_then(stmt);
      void *else_part = stmt_if_else(stmt);
      
      const char *cond_reg = gen_expr_mips(cond);
      char *else_label = new_label();
      char *end_label = new_label();
      
      /* Branch if condition is zero (false) */
      printf("    beqz %s, %s\n", cond_reg, else_label);
      
      /* Then part */
      if (then_part) {
        if (ast_node_type(then_part) == STMT_LIST) {
          gen_stmt_list_mips(then_part);
        } else {
          gen_stmt_mips(then_part);
        }
      }
      
      /* Jump to end after then */
      printf("    j %s\n", end_label);
      
      /* Else label */
      printf("%s:\n", else_label);
      
      /* Else part */
      if (else_part) {
        if (ast_node_type(else_part) == STMT_LIST) {
          gen_stmt_list_mips(else_part);
        } else {
          gen_stmt_mips(else_part);
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
      const char *cond_reg = gen_expr_mips(cond);
      printf("    beqz %s, %s\n", cond_reg, end_label);
      
      /* Body */
      if (body) {
        if (ast_node_type(body) == STMT_LIST) {
          gen_stmt_list_mips(body);
        } else {
          gen_stmt_mips(body);
        }
      }
      
      /* Jump back to start */
      printf("    j %s\n", start_label);
      
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
        const char *result = gen_expr_mips(expr);
        printf("    move $v0, %s\n", result);
      }
      
      /* Restore stack and return */
      printf("    la $sp, 0($fp)\n");
      printf("    lw $ra, 0($sp)\n");
      printf("    lw $fp, 4($sp)\n");
      printf("    la $sp, 8($sp)\n");
      printf("    jr $ra\n");
      break;
    }
    
    case STMT_LIST: {
      /* Statement list - recursively process */
      gen_stmt_list_mips(stmt);
      break;
    }
    
    default:
      fprintf(stderr, "gen_stmt_mips: unknown statement type %d\n", type);
      break;
  }
}

/*
 * gen_stmt_list_mips: Generate MIPS code for a statement list
 */
static void gen_stmt_list_mips(void *stmt_list) {
  void *current = stmt_list;
  while (current) {
    void *head = stmt_list_head(current);
    if (head) {
      gen_stmt_mips(head);
    }
    current = stmt_list_rest(current);
  }
}

/*
 * gen_function_mips: Generate MIPS code for a function definition
 */
void gen_function_mips(void *func) {
  if (!func || ast_node_type(func) != FUNC_DEF) {
    return;
  }
  
  /* Reset counters and clear variable locations for each function */
  temp_reg_counter = 0;
  label_counter = 0;
  stack_offset = 0;
  clear_var_locations();
  println_used = 0;  /* Reset println tracking */
  
  /* Function header with underscore prefix */
  char *fname = func_def_name(func);
  printf("_%s:\n", fname);
  
  /* Scan function body to count local variables and build location table */
  void *body = func_def_body(func);
  if (body) {
    scan_for_locals(body);
  }
  
  /* Calculate total stack frame size:
   * - local variables: local_var_count * 4
   * - temp variables: estimate 4-8 temps needed
   */
  int frame_size = local_var_count * 4;
  
  /* Function prologue - save $fp and $ra */
  printf("    la $sp, -8($sp)\n");
  printf("    sw $fp, 4($sp)\n");
  printf("    sw $ra, 0($sp)\n");
  printf("    la $fp, 0($sp)\n");
  
  /* Allocate space for local variables */
  if (frame_size > 0) {
    printf("    la $sp, -%d($sp)\n", frame_size);
  }
  
  /* Parameters - would be loaded from caller's stack if needed */
  int nargs = func_def_nargs(func);
  if (nargs > 0) {
    for (int i = 1; i <= nargs; i++) {
      char *param = func_def_argname(func, i);
      /* Parameters are accessed at positive offsets from $fp */
      add_var_location(param, 8 + (i - 1) * 4, 0);
    }
  }
  
  /* Function body */
  if (body) {
    gen_stmt_list_mips(body);
  }
  
  /* Function epilogue (if no explicit return) */
  printf("    la $sp, 0($fp)\n");
  printf("    lw $ra, 0($sp)\n");
  printf("    lw $fp, 4($sp)\n");
  printf("    la $sp, 8($sp)\n");
  printf("    jr $ra\n");
}

/*
 * generate_mips: Main entry point for MIPS code generation
 * Called from the driver when gen_code_flag is set
 */
void generate_mips(void *ast) {
  if (!ast) return;
  
  /* Reset println tracking */
  println_used = 0;
  
  /* First pass: scan to see if println is used */
  void *body = func_def_body(ast);
  if (body) {
    scan_for_locals(body);
  }

  
  /* Data section with newline string (only if println is used) and global variables */
  static int data_emitted = 0;
  if (!data_emitted && (println_used || global_vars)) {
    printf(".align 2\n");
    printf(".data\n");
    if (println_used) {
      printf("_nl: .asciiz \"\\n\"\n");
    }
    emit_global_vars();
    data_emitted = 1;
  }
  
  /* Text section */
  printf(".align 2\n");
  printf(".text\n\n");
  
  /* Generate main jump if this is the main function */
  char *fname = func_def_name(ast);
  if (strcmp(fname, "main") == 0) {
    printf(".globl main\n");
    printf("main: j _main\n\n");
  }
  
  /* Built-in println function (only if used) - emit once before first function */
  static int println_emitted = 0;
  if (println_used && !println_emitted) {
    printf("_println:\n");
    printf("    li $v0, 1\n");
    printf("    lw $a0, 0($sp)\n");
    printf("    syscall\n");
    printf("    li $v0, 4\n");
    printf("    la $a0, _nl\n");
    printf("    syscall\n");
    printf("    jr $ra\n\n");
    println_emitted = 1;
  }
  
  gen_function_mips(ast);
}
