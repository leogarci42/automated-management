# Cucpp Language Documentation

## Overview
Cucpp is a minimal, custom Static Single Assignment (SSA) friendly language that compiles directly down to LLVM IR, avoiding memory allocations and stack operations wherever possible. 

This document outlines the core features of the language and the recent improvements to the compiler pipeline.

## Language Syntax & Features

### 1. `compute` blocks (formerly `func`)
The primary abstraction unit in Cucpp is the `compute` block. 
These are equivalent to functions in C. They take arguments, execute a block of code, and return an evaluated result.

**Syntax:**
```c
compute loop_example(x) {
    // Body code here
    return x
}
```

### 2. Variables & Scoping
Variables are immutable. As Cucpp compiles down to pure LLVM IR, variable re-assignments are not structurally supported by standard SSA semantics. Shadowing variable names (e.g. `x1`, `x2`, `x3`) is the proper way to handle successive evaluation steps. The compiler tracks variable bindings and evaluates them mathematically directly into LLVM `%` registers.

**Syntax:**
```c
x1 = 12
x2 = x1 - 1
```

*(Note: Simple math operations like `+`, `-`, `*` are natively supported).*

### 3. Control Flow (`if` / `ifelse`)
Basic branching logic runs recursively or implicitly into successive evaluations.

**Syntax:**
```c
if (x > 0)
    return loop(x - 1)
```

### 4. Function Calls (`compute_call`)
Executing another compute block or passing standard definitions operates natively on variable assignments or inside `return` tokens.

**Syntax:**
```c
a = z(x3)             // Store into variable `a`
return loop(x - 1)    // Tail recurse or function return evaluation
print(x3)             // Call native I/O routines
```

## Advanced Codegen Pipeline
Historically, the `cucpp` codegen hardcoded specific string permutations (like exact token matches for `x2 = loop(x1)`). 
The compiler now implements a **generic parser-driven LLVM code generator**!

**Codegen Features:**
- **Dynamic Variable Binding**: Any variable `var` translates iteratively into LLVM instructions (ex: `%var`). Variables matching compute arguments translate reliably without shadowing clashing (e.g. `i32 %x_arg`).
- **Dynamic Expression Evaluation**: The LLVM string generation is now built off of standard C String Formats (`sscanf()`). Tokens like statement definitions are split automatically into `Left-hand = Right-hand1 Operator Right-hand2`.
- **Dynamic Branch Parsing**: Conditional blocks like `x > 0` are evaluated character-by-character into `icmp` evaluation flags seamlessly dynamically.
- **Embedded `printf` execution**: Support for external built-ins is completely preserved and scalable inside the modular IR design space.
