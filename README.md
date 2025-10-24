# 🧠 CSC 453 — Compiler Design Project  
_A handwritten compiler built in C, developed milestone-by-milestone._

---

## 📂 Repository Layout

Top-level directories follow the course’s assignment/milestone structure:



Assg1 M1 – Scanner/ # Lexical Analysis (tokenizer)
Assg1 M2 – Parser/ # Basic grammar parser
Assg2 M1 – G1 Parse/ # Grammar 1 refinement
Assg2 M2 – G2 Parse/ # Grammar 2 (left-factored / LL(1))
Assg2 M3 – AST/ # Abstract Syntax Tree construction
Assg3 M1 – Semantic/ # Symbol tables + type checking
Assg3 M2 – IR/ # Three-address Intermediate Representation
Assg4 M1 – CodeGen/ # x86-64 code generation
Assg4 M2 – Final/ # Integration + testing
docs/ # Lecture notes, diagrams, and design write-ups
tests/ # Unit + golden test cases per phase
Makefile # Automated build + test
README.md # This file


Each milestone directory is self-contained with:


src/ → implementation (.c/.h)
tests/ → phase-specific test inputs & outputs
notes.md → design notes & weekly reflection


---

## 🧭 Phase Roadmap

| Phase | Focus | Representative Folder | Output |
|:------|:-------|:----------------------|:--------|
| **Lexical Analysis** | Tokenize source → tokens | `Assg1 M1 – Scanner` | Token stream |
| **Syntax Analysis 1 & 2** | Recursive-descent parser, Grammar 1→Grammar 2 | `Assg1 M2`, `Assg2 M1`, `Assg2 M2` | Parse tree |
| **AST Construction** | Build abstract syntax tree | `Assg2 M3 – AST` | AST dump |
| **Semantic Checking** | Scope + type analysis | `Assg3 M1 – Semantic` | Annotated AST |
| **Intermediate Representation** | 3-address code (TAC) | `Assg3 M2 – IR` | TAC listing |
| **Code Generation** | Map TAC → x86-64 assembly | `Assg4 M1 – CodeGen` | `.s` file |
| **Integration & Testing** | End-to-end compiler | `Assg4 M2 – Final` | Executable program |

---

## ⚙️ Build & Run

### Build
```bash
make debug     # -std=c17 -Wall -Wextra -fsanitize=address,undefined
make release   # -O2 -DNDEBUG

Run
./build/compiler input.c -o output.s
as output.s -o output.o
ld output.o -o a.out
./a.out

Clean
make clean

🧪 Testing
Category	Description
Unit tests	Per-phase validation (scanner tokens, parser trees, etc.)
Golden tests	Compare compiler output vs. reference text files
Property tests	Lexer round-trip, IR consistency
Memory checks	valgrind + ASan clean runs
Regression	make test run at each milestone tag
## 🧱 Language Subset

A restricted C-like language supporting:

int, bool, return
if, else, while
+, -, *, /, <, >, ==, !=
{ } ; ( )


No structs, pointers, or heap allocation; grammar kept LL(1)-friendly.

---

## 🧩 Intermediate Representation

Example TAC:

t0 = 2
t1 = t0 * x
t2 = t1 + 1
return t2


Dump with:

./build/compiler --dump-ir tests/example.c

---

## 🧾 Code Generation

Target: x86-64 (System V ABI)

Register use: rdi, rsi, rdx, rcx, r8, r9 for args

Function frame:

pushq %rbp
movq  %rsp, %rbp
...
leave
ret

---

## 🧠 Tools & Debugging
Tool	Use
gdb	Step through compiler or emitted assembly
valgrind	Detect leaks and UB
objdump -d	Inspect ELF binary
diff	Compare golden outputs
make VERBOSE=1	Enable compiler tracing

---

## 📚 References

Saumya Debray, CSc 453: Compilers & Systems Software

Lecture PDFs: Background, Lexical Analysis, Syntax Analysis, Semantic Checking, IR, Code Generation

---

## 👤 Author

Name: Kory Smith
Course: CSc 453 – Fall 2025
Instructor: Prof. Saumya Debray
Language: C-- (G2)
Platform: Linux x86-64 (gcc/clang)

“A compiler is a translator from meaning to mechanism.”
— CSC 453 Lecture 0
