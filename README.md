# 🧠 CSC 453 — Compiler Design Project

> *A handwritten compiler built in C, developed milestone-by-milestone.*

[![Language](https://img.shields.io/badge/language-C17-blue.svg)](https://en.wikipedia.org/wiki/C17_(C_standard_revision))
[![Platform](https://img.shields.io/badge/platform-Linux%20x86--64-lightgrey.svg)](https://en.wikipedia.org/wiki/X86-64)
[![License](https://img.shields.io/badge/license-Academic-green.svg)](#)

---

## 📖 Overview

A complete compiler implementation targeting a C-like subset language (C--), translating source code through lexical analysis, parsing, semantic checking, intermediate representation, and x86-64 code generation. Built incrementally across structured milestones for CSC 453.

**Language Features:** `int`, `bool`, `if/else`, `while`, arithmetic/logical operators  
**Target Architecture:** x86-64 assembly (System V ABI)  
**Intermediate Form:** Three-address code (TAC)

---

## 📂 Repository Structure

```
.
├── Assg1 M1 – Scanner/      # Lexical analysis (tokenizer)
├── Assg1 M2 – Parser/       # Basic recursive-descent parser
├── Assg2 M1 – G1 Parse/     # Grammar 1 refinement
├── Assg2 M2 – G2 Parse/     # Grammar 2 (LL(1) compatible)
├── Assg2 M3 – AST/          # Abstract syntax tree construction
├── Assg3 M1 – Semantic/     # Symbol tables & type checking
├── Assg3 M2 – IR/           # Three-address intermediate code
├── Assg4 M1 – CodeGen/      # x86-64 assembly generation
├── Assg4 M2 – Final/        # Integration & end-to-end testing
├── docs/                    # Design documentation & lecture notes
├── tests/                   # Test suite (unit, golden, regression)
├── Makefile                 # Build automation
└── README.md
```

**Each milestone contains:**
- `src/` — Implementation files (`.c`/`.h`)
- `tests/` — Phase-specific test cases
- `notes.md` — Design decisions & reflections

---

## 🚀 Quick Start

### Build

```bash
# Debug build (with sanitizers)
make debug

# Optimized release build
make release
```

### Compile a Program

```bash
./build/compiler input.c -o output.s
as output.s -o output.o
ld output.o -o a.out
./a.out
```

### Run Tests

```bash
make test
```

### Clean Build Artifacts

```bash
make clean
```

---

## 🧪 Testing Strategy

| Category | Description |
|:---------|:------------|
| **Unit Tests** | Per-phase validation (tokens, parse trees, TAC) |
| **Golden Tests** | Output comparison against reference files |
| **Property Tests** | Lexer round-trip, IR consistency checks |
| **Memory Safety** | Valgrind + AddressSanitizer validation |
| **Regression** | Full suite executed at milestone tags |

---

## 📊 Compilation Pipeline

```
┌─────────────┐     ┌─────────┐     ┌──────────┐     ┌──────────┐
│ Source Code │ --> │ Scanner │ --> │  Parser  │ --> │   AST    │
│   (C--)     │     │ (Lexer) │     │  (CFG)   │     │  Builder │
└─────────────┘     └─────────┘     └──────────┘     └──────────┘
                                                            │
                         ┌──────────────────────────────────┘
                         ↓
                    ┌──────────┐     ┌─────────┐     ┌──────────┐
                    │ Semantic │ --> │   IR    │ --> │ Code Gen │
                    │ Analysis │     │  (TAC)  │     │ (x86-64) │
                    └──────────┘     └─────────┘     └──────────┘
```

### Phase Breakdown

| Phase | Milestone | Output | Key Concepts |
|:------|:----------|:-------|:-------------|
| **Lexical Analysis** | `Assg1 M1` | Token stream | DFA, regex patterns |
| **Syntax Analysis** | `Assg1 M2`, `Assg2 M1-M2` | Parse tree | LL(1) parsing, left-factoring |
| **AST Construction** | `Assg2 M3` | Abstract syntax tree | Tree traversal, node types |
| **Semantic Checking** | `Assg3 M1` | Annotated AST | Symbol tables, type inference |
| **IR Generation** | `Assg3 M2` | TAC listing | SSA form, basic blocks |
| **Code Generation** | `Assg4 M1` | Assembly (`.s`) | Register allocation, ABI |
| **Integration** | `Assg4 M2` | Executable | Linking, system calls |

---

## 🔧 Language Specification (C--)

### Supported Constructs

```c
// Variable declarations
int x;
bool flag;

// Control flow
if (x > 0) {
    return x;
} else {
    return -x;
}

while (x < 10) {
    x = x + 1;
}

// Expressions
int result = (2 * x + 3) / 5;
bool condition = (x == 0) || (x > 100);
```

### Grammar Features

- **Types:** `int`, `bool`
- **Operators:** `+`, `-`, `*`, `/`, `<`, `>`, `==`, `!=`, `||`, `&&`
- **Statements:** `if/else`, `while`, `return`, assignment
- **No support for:** Pointers, structs, arrays, heap allocation

**Grammar Properties:** LL(1) compatible, left-factored, unambiguous

---

## 🧩 Intermediate Representation

The compiler generates three-address code (TAC) for platform-independent optimization:

**Source:**
```c
return 2 * x + 1;
```

**TAC:**
```
t0 = 2
t1 = t0 * x
t2 = t1 + 1
return t2
```

**View IR:**
```bash
./build/compiler --dump-ir tests/example.c
```

---

## ⚙️ Code Generation

### Target Platform
- **ISA:** x86-64
- **ABI:** System V AMD64
- **Assembler:** GNU `as` (AT&T syntax)

### Register Usage
- **Arguments:** `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`
- **Temporaries:** `rax`, `r10`, `r11`
- **Callee-saved:** `rbx`, `r12-r15`

### Stack Frame
```asm
pushq   %rbp
movq    %rsp, %rbp
subq    $16, %rsp       # Allocate locals
...
leave
ret
```

---

## 🛠️ Development Tools

| Tool | Purpose |
|:-----|:--------|
| `gdb` | Debug compiler or generated assembly |
| `valgrind` | Memory leak detection |
| `objdump -d` | Disassemble ELF binaries |
| `diff` | Compare golden test outputs |
| `make VERBOSE=1` | Enable compiler trace logs |

### Debug a Generated Binary

```bash
./build/compiler test.c -o test.s
as test.s -o test.o
ld test.o -o test
gdb ./test
```

---

## 📚 References

- **Course:** CSc 453: Compilers & Systems Software (Fall 2025)
- **Instructor:** Prof. Saumya Debray, University of Arizona
- **Lectures:** Lexical Analysis, Syntax Analysis, Semantic Checking, IR, Code Generation
- **Textbooks:**
  - *Compilers: Principles, Techniques, and Tools* (Dragon Book)
  - *Engineering a Compiler* by Cooper & Torczon

---

## 📝 Project Information

- **Author:** Kory Smith
- **Course:** CSc 453 – Fall 2025
- **Language Standard:** C17
- **Compiler Toolchain:** GCC/Clang
- **Build System:** GNU Make

---

## 📜 License

This project is submitted for academic coursework. All rights reserved under university academic integrity policies.

---

> *"A compiler is a translator from meaning to mechanism."*  
> — CSC 453 Lecture 0
