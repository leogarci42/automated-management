# Compiler Architecture: From AST to Native Binary

This document explains the overarching process of how your custom language (`.cucpp`) is translated into a runnable native executable, using the LLVM infrastructure.

## 1. The Frontend: Lexing & Parsing (AST Construction)
Before we can generate any code, the compiler needs to understand the structure of the program. Your `tokenizer.c` acts as the frontend. It reads the source file text (e.g., `test.cucpp`) and turns it into an **Abstract Syntax Tree (AST)**. 

Instead of reading a flat list of characters, your code organizes it hierarchically by function (`func z(a)`, `func main()`), blocks, loops (`while`), and statements. 
Our AST output looks something like this:
```text
[FUNC] main
  \-> [STATEMENT] x = 12
  \-> [LOOP] x > 0
        \-> [DECREMENT] x--
```

## 2. The Middle-end: LLVM Intermediate Representation (IR) 
Native machine code (x86, ARM, Apple Silicon, ptx) is highly complex and platform-dependent. LLVM provides an "Intermediate Representation" (IR) which looks like a cross between Assembly and C. This is what our `codegen.c` writes out to `output.ll`.

### The Core Concepts of LLVM IR:
1. **Infinite "Virtual" Registers (`%0`, `%1`, `%a`)**:
   LLVM uses **Static Single Assignment (SSA)**. This means a register (like `%1`) can only be assigned a value **exactly once**. You can never do `%1 = %1 + 1`.
2. **Allocating Variables (`alloca`)**: 
   Because registers are strictly write-once in SSA, to model "mutable" variables like `x` in your language, we allocate stack memory for them:
   ```llvm
   %x = alloca i32, align 4     ; Allocate 4 bytes on the stack for 'x'
   store i32 12, i32* %x        ; Write 12 into 'x'
   ```
3. **Using Variables (`load`)**:
   Every time you want to read `x` to do an operation, you pull it from memory into a fresh virtual register:
   ```llvm
   %0 = load i32, i32* %x       ; Read value of x into %0
   %1 = sub nsw i32 %0, 1       ; Compute %0 - 1, store in fresh register %1
   store i32 %1, i32* %x        ; Write the new value back to 'x' memory
   ```
4. **Control Flow (`br` and labels)**:
   Loops and If-Statements are converted into "Basic Blocks" ending in Branch (`br`) instructions. 
   To evaluate `while(x > 0)`, we compare the condition (`icmp sgt`) and branch (`br`) conditionally to either the loop body label or the loop end label.

Our `generate_llvm_ir_cpu` function steps node-by-node through your AST and manually prints out the specific textual LLVM instructions.

## 3. The Backend: Assembly and Linking (`clang`)
Once our compiler writes the `output.ll` file, that file is technically valid LLVM language, but it's not a runnable binary. It's just text.

We run `clang output.ll -o program_bin` inside the compiler (`system()` call) to cross the finish line. Under the hood, `clang` does this:
1. **Optimization**: It runs LLVM optimizer passes to simplify the IR.
2. **Code Emission (`llc`)**: It turns the LLVM IR into a machine-code Object File (`.o`) containing CPU-specific instructions (e.g. x86_64 assembly).
3. **Linking (`ld`)**: Finally, it links your Object File with the system's C Standard Library (`libc`) and the system startup routines (`_start`). Without this step, your Operating System wouldn't know how to launch your `main` function as a real process.

---

## What's Next? (Moving beyond a "toy")
Currently, only the CPU version have been dev, so:

#### GPU-based optimization:
* GPU-friendly generation, because we could use my already generated llvm for cpu and run it on cpu, but if i don't generate clear llvm for the target what is the point of it?

#### implement advanced optimzations option for CPU:
* implement various new features in code / at llvm generation like native pthreads, mutexes and atomic, SIMD

#### Algorithms to auto-determine what should run CPU / GPU and benchmarks

* by far the hardest, but try to determine and split the runtime through CPU / GPU, and make it efficient on benchmarks!