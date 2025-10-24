#include <stdbool.h>

typedef enum { SYM_VAR, SYM_FUNC } SymKind;

typedef struct Sym {
  const char *name;     // heap-owned copy
  SymKind kind;
  int arity;            // 0 in G1 (no args at call sites)
  int decl_line;        // line number of declaration (for “prior to call”)
  struct Sym *next;     // bucket chain
} Sym;

// scope management: one global scope; one local scope per function
void st_init(void);               // call once at start of parse()
void st_enter_scope(void);        // enter function-local scope
void st_leave_scope(void);        // leave function-local scope

// insert into current scope; returns existing symbol if duplicate in THIS scope
Sym *st_insert(const char *name, SymKind kind, int decl_line);

// lookups
Sym *st_lookup(const char *name);              // nearest (local→global)
Sym *st_lookup_current(const char *name);      // only current scope
Sym *st_lookup_global(const char *name);       // only global scope
