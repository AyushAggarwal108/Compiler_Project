# Mini Compiler + Linker in C++

A fully functional compiler and linker for the **Toy (.toy)** programming language, implemented in modern C++17. Demonstrates all core phases of a real compiler pipeline — from raw source text through lexing, parsing, semantic analysis, IR generation, optimization, pseudo-assembly emission, and multi-file linking.

> **Designed to signal:** Compiler fundamentals · Linker internals · Recursive-descent parsing · Data structures · Systems-level C++ programming

---

## Architecture

```
Source File(s)
     │
     ▼
┌──────────┐
│  Lexer   │  Tokenizes source into a typed token stream
└────┬─────┘
     │  Token[]
     ▼
┌──────────┐
│  Parser  │  Recursive-descent parser → Abstract Syntax Tree
└────┬─────┘
     │  AST (ProgramNode)
     ▼
┌──────────────────┐
│ SemanticAnalyzer │  Type checking · Scoping · Symbol table build
└────┬─────────────┘
     │  Validated AST + SymbolTable
     ▼
┌───────────────┐
│ CodeGenerator │  Three-address IR · Constant folding · DCE · Pseudo-ASM
└────┬──────────┘
     │  ObjectFile { IR, ASM, exported/imported symbols }
     ▼
┌────────┐
│ Linker │  Symbol resolution · Duplicate/undefined-ref detection
└────┬───┘
     │
     ▼
  output/linked.ir   output/linked.asm   output/linked.sym
```

### Class Hierarchy

| Class | Responsibility |
|---|---|
| `Lexer` | Tokenises source, supports line comments (`//`) and block comments (`/* */`) |
| `Token` | Typed lexeme with source location (line, col) |
| `Parser` | Recursive-descent; produces full AST with panic-mode error recovery |
| `ASTNode` (hierarchy) | Visitor-pattern node tree (VarDecl, Assign, BinaryExpr, Print, …) |
| `SemanticAnalyzer` | ASTVisitor walk; type-checks, scopes, fills SymbolTable |
| `SymbolTable` | Scoped hash-map stack; O(1) lookup per scope level |
| `CodeGenerator` | ASTVisitor walk; emits TAC IR, applies constant folding + DCE, then lowers to pseudo-ASM |
| `Linker` | Merges ObjectFiles; resolves global symbols; detects duplicates and undefined references |
| `CompilerDriver` | Orchestrates full pipeline; handles CLI flags and file I/O |

---

## Language Grammar

```ebnf
program     ::= statement* EOF

statement   ::= varDecl
              | assignment ';'
              | printStmt

varDecl     ::= ('int' | 'float') IDENTIFIER ('=' expression)? ';'
assignment  ::= IDENTIFIER '=' expression ';'
printStmt   ::= 'print' '(' expression ')' ';'

expression  ::= comparison
comparison  ::= addSub ( ('==' | '!=' | '<' | '>' | '<=' | '>=') addSub )*
addSub      ::= mulDiv  ( ('+' | '-') mulDiv )*
mulDiv      ::= unary   ( ('*' | '/') unary )*
unary       ::= '-' primary | primary
primary     ::= INT_LITERAL | FLOAT_LITERAL | IDENTIFIER | '(' expression ')'

types       ::= 'int' | 'float'
keywords    ::= 'int' | 'float' | 'print' | 'if' | 'else' | 'while' | 'return'
```

---

## Compiler Pipeline — Stage by Stage

### Stage 1 — Lexer
Scans source character-by-character, producing a flat `std::vector<Token>`. Handles:
- Integer and floating-point literals (including scientific notation: `1.5e3`)
- Identifiers and keyword disambiguation via a static lookup table
- All arithmetic, comparison, and assignment operators
- Line comments (`//`) and block comments (`/* */`)
- Full source location tracking (line + column) on every token

### Stage 2 — Parser
A hand-written **recursive-descent parser** consuming the token stream and building a typed AST:
- Operator precedence encoded directly in the grammar hierarchy (`parseComparison → parseAddSub → parseMulDiv → parseUnary → parsePrimary`)
- **Panic-mode error recovery**: on a parse error, synchronises to the next statement boundary (`;` or keyword) and continues, collecting multiple errors per run
- Returns ownership via `std::unique_ptr` (no raw memory)

### Stage 3 — Semantic Analysis
Walks the AST via the **Visitor pattern**; maintains a `SymbolTable` (scoped hash-map stack):
- Variable redeclaration detection (within the same scope)
- Undeclared variable usage
- Type-mismatch checking with implicit `int → float` promotion
- Narrowing `float → int` warnings
- **Static division-by-zero** detection at compile time
- All errors carry exact source locations

### Stage 4 — Code Generation
Second AST walk; emits **Three-Address Code (TAC)**:
```
t0 = 5
a  = t0
t1 = a + 2
b  = t1
print b
```

**Optimization passes** (applied to the flat IR):

| Pass | Description |
|---|---|
| Constant Folding | Replaces compile-time-known binary ops with their result: `4 * 5 + 2` → `22` |
| Dead Code Elimination | Marks temporaries whose definitions are never read and removes them from output |

IR is then **lowered to pseudo-assembly**:
```asm
    MOV R0, 5
    STORE a, R0
    MOV R1, 7
    STORE b, R1
    PRINT 7
    HALT
```

### Stage 5 — Linker
Accepts multiple `ObjectFile` structs (one per compiled unit):
- Builds a **global symbol table** from all exported symbol lists
- Detects **duplicate symbol definitions** across units
- Detects **undefined references** (imported but never exported)
- Merges IR and ASM streams in link order
- Emits `linked.ir`, `linked.asm`, and a human-readable `linked.sym`

---

## Build Instructions

### Requirements
- g++ ≥ 9 or clang++ ≥ 10 (C++17 support required)
- GNU Make

### Build

```bash
git clone https://github.com/yourname/Mini-Compiler.git
cd Mini-Compiler
make          # release build → ./compiler
make debug    # debug build with ASAN + UBSan
make clean    # remove build artifacts
```

### Run Tests

```bash
make test
# or
bash tests/run_tests.sh
```

---

## CLI Usage

```bash
# Compile a single file (shows IR + ASM by default)
./compiler examples/hello.toy

# Show token stream
./compiler examples/hello.toy --tokens

# Pretty-print AST
./compiler examples/hello.toy --ast

# Show three-address IR
./compiler examples/hello.toy --ir

# Show pseudo-assembly
./compiler examples/hello.toy --asm

# Show symbol table
./compiler examples/hello.toy --symbols

# All flags combined
./compiler examples/hello.toy --tokens --ast --ir --asm --symbols

# Link multiple source files
./compiler examples/main.toy examples/math.toy examples/utils.toy --link

# Disable optimizations
./compiler examples/arithmetic.toy --ir --no-opt

# Help
./compiler --help
```

Output files are written to `output/`:
- `<stem>.ir`  — Three-address IR
- `<stem>.asm` — Pseudo-assembly
- `linked.ir` / `linked.asm` / `linked.sym` — Linker output

---

## Example Programs

### hello.toy
```c
int a = 5;
int b = a + 2;
float x = 3.14;

print(a);
print(b);
print(x);
```

**IR output (with constant folding):**
```
0  a = 5
1  ; [dead] (eliminated)    ← t0 folded away
2  b = 7                    ← a+2 = 7 at compile time
3  x = 3.14
4  print 5
5  print 7
6  print 3.14
```

### arithmetic.toy
```c
int a = 10;
int b = 3;
int c = a + b;
int folded = 4 * 5 + 2;     // constant-folded to 22
int also_folded = 100 - 25 * 2;  // folded to 50
```

### errors.toy (semantic error showcase)
```c
int a = 5;
int a = 10;          // ERROR: redeclaration of 'a'
int b = c + 1;       // ERROR: undeclared variable 'c'
float x = 2.5;
int bad = x + 1;     // WARNING: narrowing float → int
int zero_div = 10/0; // ERROR: division by zero (static)
```

---

## Sample Output

```
 ╔══════════════════════════════════════════════════════╗
 ║        Mini Compiler + Linker  [.toy language]       ║
 ║   Lexer → Parser → Semantic → CodeGen → Linker       ║
 ╚══════════════════════════════════════════════════════╝

  Source: examples/hello.toy

══════ Stage 1: Lexical Analysis ══════
  ✔ Lexing successful  (33 tokens)

══════ Stage 2: Parsing ══════
  ✔ Parsing successful

══════ Stage 3: Semantic Analysis ══════
  ✔ Semantic checks passed

┌─────────────────────────────────────────────────────────┐
│                     SYMBOL TABLE                        │
├──────────────┬───────┬───────┬──────┬─────────────────┤
│ Name         │ Type  │ Scope │ Slot │ Initialized     │
├──────────────┼───────┼───────┼──────┼─────────────────┤
│ a            │ int   │ 0     │ 0    │ yes             │
│ b            │ int   │ 0     │ 1    │ yes             │
│ x            │ float │ 0     │ 2    │ yes             │
└──────────────┴───────┴───────┴──────┴─────────────────┘

══════ Stage 4: Code Generation ══════
  ✔ IR generated  (7 instructions)
  ✔ Optimizations applied (constant folding + DCE)
  ✔ Pseudo-assembly generated

══════ Output Files ══════
    → Written to output/hello.ir
    → Written to output/hello.asm

  ✔ Compilation successful  [0.81 ms]
```

---

## Project Structure

```
Mini-Compiler/
├── src/
│   ├── main.cpp          CLI entry point, argument parsing
│   ├── Lexer.cpp         Tokenizer
│   ├── Parser.cpp        Recursive-descent parser
│   ├── AST.cpp           AST node pretty-printer
│   ├── Semantic.cpp      Type checker + symbol table
│   ├── CodeGen.cpp       IR generation + optimizations + pseudo-ASM
│   └── Linker.cpp        Multi-unit linker
├── include/
│   ├── Token.h           Token types and Token struct
│   ├── Lexer.h
│   ├── Parser.h
│   ├── AST.h             Full AST node hierarchy + Visitor interface
│   ├── Semantic.h        SemanticAnalyzer + SymbolTable
│   ├── CodeGen.h         CodeGenerator + IRInstruction + ObjectFile
│   ├── Linker.h          Linker + LinkedOutput
│   └── CompilerDriver.h  Pipeline orchestrator
├── examples/
│   ├── hello.toy         Basic declarations + print
│   ├── arithmetic.toy    Integer arithmetic + constant folding demo
│   ├── floats.toy        Float arithmetic
│   ├── main.toy          Multi-file link (entry point)
│   ├── math.toy          Multi-file link (math unit)
│   ├── utils.toy         Multi-file link (utils unit)
│   └── errors.toy        Intentional semantic errors
├── tests/
│   └── run_tests.sh      Automated test suite
├── output/               Generated IR, ASM, and linker output
├── build/                Object files (generated)
├── Makefile
└── README.md
```

---

## Design Decisions

**Why Visitor pattern?** Each compiler stage (semantic analysis, code generation) is a separate Visitor over the same AST. This cleanly separates concerns without modifying AST node classes — analogous to LLVM's pass infrastructure.

**Why recursive descent?** Hand-written parsers are idiomatic in production compilers (GCC, Clang, Rustc). They give precise control over error messages and recovery, unlike table-driven parsers.

**Why flat IR before assembly?** The three-address code IR is a well-defined intermediate; optimization passes (constant folding, DCE) operate on it independently of both the source language and the target ISA. This mirrors SSA-based IRs like LLVM IR.

**Why a separate Linker stage?** Real toolchains separate compilation from linking. Each `.toy` file compiles to an `ObjectFile` that records exported and imported symbols — the linker then resolves them, exactly mirroring `.o` + `ld` semantics.

---

## Future Improvements

- [ ] Control flow: `if/else`, `while` loops with label-based IR lowering
- [ ] Functions: call/return with a call stack and activation records
- [ ] SSA form: convert TAC to Static Single Assignment for richer optimizations
- [ ] Register allocation: linear scan or graph-coloring allocator
- [ ] Real target backend: emit x86-64 AT&T syntax for `as` + `ld`
- [ ] Arrays and pointers
- [ ] Scope-aware name mangling for function overloading
- [ ] REPL mode: interactive compilation of single statements

---

## Skills Demonstrated

| Area | Implementation |
|---|---|
| Compiler theory | Full pipeline: lex → parse → semantic → IR → asm → link |
| Parsing | Hand-written recursive descent with panic-mode recovery |
| Data structures | Scoped hash-map symbol table, unique_ptr AST, flat IR vector |
| OOP / design patterns | Visitor pattern, clean header/implementation separation |
| C++17 | `std::optional`, `std::variant`, `std::filesystem`, move semantics |
| Optimization | Constant folding, dead-code elimination |
| Systems programming | Linker symbol resolution, object file model, ANSI terminal output |
| Build tooling | Makefile with release/debug/test/install targets |
| Error handling | Multi-error reporting with line:col attribution, panic-mode sync |
