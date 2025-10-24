#include "symtab.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static char *xstrdup(const char *s){
    size_t n = strlen(s) + 1;
    char *p = (char*)malloc(n);
    if (!p) abort();
    memcpy(p, s, n);
    return p;
}

static Sym *global_head = NULL;
static Sym *local_head  = NULL;
static bool local_active = false;

static Sym *find_in(Sym *head, const char *name){
    for (Sym *p = head; p; p = p->next) if (strcmp(p->name, name) == 0) return p;
    return NULL;
}

void st_init(void){
    // free old lists if you want; for a short-lived compiler we can reset
    global_head = NULL;
    local_head = NULL;
    local_active = false;
    
    // Register built-in function println(int) with arity 1
    Sym *println_sym = malloc(sizeof(Sym));
    if (!println_sym) { perror("malloc"); exit(1); }
    println_sym->name = xstrdup("println");
    println_sym->kind = SYM_FUNC;
    println_sym->decl_line = 0;  // Built-in, declared at line 0
    println_sym->arity = 1;      // Takes one integer argument
    println_sym->next = global_head;
    global_head = println_sym;
}

void st_enter_scope(void){
    assert(!local_active && "nested local scopes not used in G1");
    local_active = true;
    local_head = NULL;
}

void st_leave_scope(void){
    if (!local_active) return;
    // free locals
    Sym *p = local_head;
    while (p){
        Sym *n = p->next;
        free((void*)p->name);
        free(p);
        p = n;
    }
    local_head = NULL;
    local_active = false;
}

Sym *st_insert(const char *name, SymKind kind, int decl_line){
    Sym **head = local_active ? &local_head : &global_head;
    Sym *existing = find_in(*head, name);
    if (existing) return existing;

    Sym *s = (Sym*)malloc(sizeof(Sym));
    if (!s) abort();
    s->name = xstrdup(name);
    s->kind = kind;
    s->arity = 0;
    s->decl_line = decl_line;
    s->next = *head;
    *head = s;
    return NULL; // success
}

Sym *st_lookup_current(const char *name){
    return local_active ? find_in(local_head, name) : NULL;
}

Sym *st_lookup_global(const char *name){
    return find_in(global_head, name);
}

Sym *st_lookup(const char *name){
    if (local_active){
        Sym *p = find_in(local_head, name);
        if (p) return p;
    }
    return find_in(global_head, name);
}

void st_set_func_arity(const char *name, int arity){
  Sym *s = st_lookup_global(name);
  if (s && s->kind == SYM_FUNC){
    s->arity = arity;
  }
}
