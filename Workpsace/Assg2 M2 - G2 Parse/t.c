// /* ok_basic.cmm — EXPECT: exit 0 */
// int f(int a,int b){ int x; x=a; if (x<b) x=x; while (x<b) x=x; return x; }
// int main(){ int y; y=0; f(1,2); return 0; }


/* err_arity.cmm
 * EXPECT: exit 1
 * MESSAGE CONTAINS: function 'f' called with 2 args but declared with 1
 */
// int f(int a){ return a; }
// int main(){ f(1,2); return 0; }


/* err_undeclared_assign.cmm
 * EXPECT: exit 1
 * MESSAGE CONTAINS: assignment to undeclared variable 'x'
 */
// int main(){ x=1; return 0; }


/* err_undeclared_in_expr.cmm
 * EXPECT: exit 1
 * MESSAGE CONTAINS: use of undeclared variable 'y'
 */
// int main(){ int a; if (a<y) a=a; return 0; }


/* err_falloff_return.cmm
 * EXPECT: exit 1
 * MESSAGE CONTAINS: function 'main' may fall off the end without a return
 */
// int main(){ int a; a=1; }


/* err_call_before_decl.cmm
 * EXPECT: exit 1
 * MESSAGE CONTAINS: use of undeclared function 'g'
 */
// int main(){ g(); return 0; }
// int g(){ return 0; }

/* err_double_decl_same_scope.cmm
 * EXPECT: exit 1
 * MESSAGE CONTAINS: variable 'x' is doubly declared
 */
// int main(){ int x, x; return 0; }


/* err_not_a_variable.cmm
 * EXPECT: exit 1
 * MESSAGE CONTAINS: 'f' is not a variable
 */
int f(){ return 0; }
int main(){ int y; y = f; return 0; }
