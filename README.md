# Cucpp Language Reference Manual (v1.0)

## NAME
**Cucpp** — A minimalist, Static Single Assignment (SSA) friendly, purely computetional programming language compiling to LLVM IR.

## SYNOPSIS
```console
./automated-management <source_file.cucpp> [--emit-llvm]
```

## DESCRIPTION
Cucpp is built to enforce an SSA-friendly paradigm. It removes mutable state (re-assignment) and standard loop constructs in favor of variable shadowing and tail-recursion. This ensures a 1-to-1 mapping with LLVM's Intermediate Representation (IR), eliminating the need for stack allocations (`alloca`) and memory store/loads entirely.

All integers in Cucpp are treated as 32-bit signed integers (`i32` in LLVM).

---

## LANGUAGE SPECIFICATION

### 1. Program Structure
A Cucpp program is a collection of computetions. There are no global variables. The entry point of a Cucpp compiled binary is the `main` computetion.

### 2. Computetion Definitions (`compute`)
Computetions are the primary abstraction block. 
*   **Syntax:** `compute <name>(<arguments>) { <body> }`
*   Computetions can take arguments and must return a value. 
*   **Example:**
    ```c
    compute z(a) {
        a1 = a - 1
        return a1
    }
    ```

### 3. Variables and Immutable Bindings
Variables are bound to expressions using the `=` operator.
*   **Mutation is avoided:** To abide by strict SSA form, a variable should not be re-assigned. Instead, compute a new variable (Shadowing).
*   **Syntax:** `<name> = <expression>`
*   **Example:**
    ```c
    x1 = 12
    x2 = x1 - 1
    ```

### 4. Control Flow (`if` / `ifelse`)
Cucpp branches logic based on conditions.
*   **Syntax:** `if (<condition>) { <body> }` or `ifelse (<condition>) { <body> }`
*   No parentheses are strictly strictly required for the body unless utilizing multiple statements, but braces `{ }` and indentation govern the scoped body.
*   **Example:**
    ```c
    if (x > 0)
        return loop(x - 1)
    ```

### 5. Iteration (Recursion)
`while`, `for`, and `loop` iteration blocks are parsed but actively discouraged in pure-SSA generated representations. 
*   **Best Practice:** Use tail-recursive computetions to represent iterative loops. LLVM natively optimizes tail-recursion back into pure SSA loop (`phi` nodes) without memory overhead.
*   **Example:**
    ```c
    compute loop(x) {
        if (x > 0)
            return loop(x - 1)
        return x
    }
    ```

### 6. Expressions & Computetion Calls
Variables can be passed to computetions. The tokenizer parses any string matching the format `name(args)` as a `compute_call` natively.
*   **Syntax:** `var_name = compute_name(args)`
*   **Example:** `x2 = loop(x1)`

### 7. Built-in Computetions
Cucpp comes with statically linked built-ins that interact with the C standard library.
*   `print(x)`: Outputs an integer followed by a newline (internally maps to `printf("%d\n", x)`).

### 8. Return Statements (`return`)
Returns a value from the current computetion block.
*   **Syntax:** `return <expression>`
*   **Example:** `return x3`

---

## AST AND COMPILATION PIPELINE
1. **Tokenizer:** Parses `.cucpp` files into token nodes (`compute`, `statement`, `compute_call`, `ifelse`, `ret_statement`).
2. **Type Checking:** Maps basic symbol tracking to ensure variable types aren't abruptly reassigned in validation.
3. **Codegen:** Generates SSA optimized `output.ll` using `printf` linkages.
4. **Clang backend:** Invokes `clang` on the generated IR to produce `a.out`.

## EXAMPLES

### Standard Recursive Decrement Program
```c
compute loop(x) {
    if (x > 0)
        return loop(x - 1)
    return x
}

compute main() {
    x1 = 12
    x2 = loop(x1)
    print(x2)
    return x2
}
```
