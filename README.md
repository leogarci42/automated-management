# CUCPP Language Reference Manual (v1.0)

## NAME
**CUCPP** — A minimalist, Static Single Assignment (SSA) friendly, purely computational programming language compiling to LLVM IR.

## SYNOPSIS
```console
./cucpp [--emit-llvm] [-o <exec_name>] <source_file.cucpp>
```

## DESCRIPTION
cucpp is built to enforce an SSA-friendly paradigm. Variable updates are lowered into SSA versions during code generation, and structured loops are emitted with explicit `phi` nodes. This keeps a 1-to-1 mapping with LLVM's Intermediate Representation (IR), eliminating the need for stack allocations (`alloca`) and memory store/loads entirely.

All integers in cucpp are treated as 32-bit signed integers (`i32` in LLVM).

---

## LANGUAGE SPECIFICATION

### 1. Program Structure
A cucpp program is a collection of computations. There are no global variables. The entry point of a cucpp compiled binary is the `main` computation.

### 2. Computation Definitions (`compute`)
Computations are the primary abstraction block. 
*   **Syntax:** `compute <name>(<arguments>) { <body> }`
*   Computations can take arguments and must return a value. 
*   **Example:**
    ```c
    compute z(a) {
        a1 = a - 1
        return a1
    }
    ```

### 3. Variables and SSA Bindings
Variables are bound to expressions using the `=` operator.
*   **SSA-friendly:** Re-assigning a name creates a new SSA version in the generated IR. Shadowing with new names is still recommended for clarity.
*   **Syntax:** `<name> = <expression>`
*   **Example:**
    ```c
    x1 = 12
    x2 = x1 - 1
    ```

### 4. Control Flow (`if` / `ifelse`)
cucpp branches logic based on conditions.
*   **Syntax:** `if (<condition>) { <body> }` or `ifelse (<condition>) { <body> }`
*   No parentheses are strictly required for the body unless utilizing multiple statements, but braces `{ }` and indentation govern the scoped body.
*   **Example:**
    ```c
    if (x > 0)
        return loop(x - 1)
    ```

### 5. Arrays
Fixed-size arrays are supported and passed by reference.
*   **Declaration:** `int arr[5]` or `char letters[3]`
*   **Literal init:** `arr = [1, 2, 3]` or `char letters[3] = ['a', 'b', 'c']`
*   **Indexing:** `x = arr[i]` and `arr[i] = x`
*   **Bounds checks:** Every access is checked at runtime and aborts on out-of-bounds.

### 6. Iteration (`while` / `loop`)
`while` and `loop` blocks are supported and compiled into SSA-friendly `phi` nodes.
*   **Important:** The loop condition must depend on a variable that is updated inside the body (e.g., `x2--`), or the loop will not terminate.
*   **Best Practice:** Use tail-recursive computations for complex loops or when you want explicit SSA transitions.
*   **Example:**
    ```c
    compute main() {
        x1 = 12
        x2 = 16
        while (x2 > x1) {
            print(x2)
            x2--
        }
        print(x2)
    }
    ```

### 7. Expressions & Computetion Calls
Variables can be passed to computations. The tokenizer parses any string matching the format `name(args)` as a `compute_call` natively.
*   **Syntax:** `var_name = compute_name(args)`
*   **Example:** `x2 = loop(x1)`

### 8. Built-in Computations
cucpp comes with statically linked built-ins that interact with the C standard library.
*   `print(x)`: Outputs an integer followed by a newline (internally maps to `printf("%d\n", x)`).

### 9. Return Statements (`return`)
Returns a value from the current computation block.
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
