# What is MLIR ?

**MLIR** or *Multi-Level Intermediate Representation* is built on **LLVM**, and adds a level of abstraction to the LLVM with something called dialects, but to understand what is **MLIR** we first need to understand what is **LLVM**.

## LLVM IR

**LLVM IR** is a toolchain built for compilers, it's a language that allow us to support both stactic and dynamic compilations, it's using **SSA** (*Static Single Assignement*), and adds various layers of optimizations when compiled down to machine code. it's also hold various backend allowing us to aim for various architecture, here is a (non-exhausting) list:
```
Registered Targets:
    aarch64     - AArch64 (little endian)
    aarch64_32  - AArch64 (little endian ILP32)
    aarch64_be  - AArch64 (big endian)
    amdgcn      - AMD GCN GPUs
    arm         - ARM
    arm64       - ARM64 (little endian)
    arm64_32    - ARM64 (little endian ILP32)
    armeb       - ARM (big endian)
    avr         - Atmel AVR Microcontroller
    bpf         - BPF (host endian)
    bpfeb       - BPF (big endian)
    bpfel       - BPF (little endian)
    hexagon     - Hexagon
    lanai       - Lanai
    loongarch32 - 32-bit LoongArch
    loongarch64 - 64-bit LoongArch
    mips        - MIPS (32-bit big endian)
    mips64      - MIPS (64-bit big endian)
    mips64el    - MIPS (64-bit little endian)
    mipsel      - MIPS (32-bit little endian)
    msp430      - MSP430 [experimental]
    nvptx       - NVIDIA PTX 32-bit
    nvptx64     - NVIDIA PTX 64-bit
    ppc32       - PowerPC 32
    ppc32le     - PowerPC 32 LE
    ppc64       - PowerPC 64
    ppc64le     - PowerPC 64 LE
    r600        - AMD GPUs HD2XXX-HD6XXX
    riscv32     - 32-bit RISC-V
    riscv64     - 64-bit RISC-V
    sparc       - Sparc
    sparcel     - Sparc LE
    sparcv9     - Sparc V9
    systemz     - SystemZ
    thumb       - Thumb
    thumbeb     - Thumb (big endian)
    ve          - VE
    wasm32      - WebAssembly 32-bit
    wasm64      - WebAssembly 64-bit
    x86         - 32-bit X86: Pentium-Pro and above
    x86-64      - 64-bit X86: EM64T and AMD64
    xcore       - Xcore
```

Here in the list we can see various *processing unit* such as `x86` which is Intel 32-bits assembly (Intel CPU) or `nvptx` which is NVIDIA's PTX assembly for 32-bits (NVIDIA GPU).

The core problem is that traditional **LLVM IR** forces high-level, complex operations (like matrix multiplications) to be instantly flattened into low-level pointers and loops, destroying the rich semantic meaning needed to perform powerful, hardware-specific optimizations.

## Why MLIR

MLIR compared to LLVM adds another layer of abstraction towards machine code, it's built on LLVM so that the block diagram look like:
```
+------------------+
|   MLIR Source    |
|  DSLs & Languages|
+------------------+
        |
        V
+------------------+
|  MLIR Operations |
|  Custom Dialects |
+------------------+
        |
        V
+------------------+
|    LLVM IR       |
|  Intermediate    |
|  Representation  |
+------------------+
        |
        V
+-------------------+
| LLVM Optimizations|
|    and Code Gen   |
+-------------------+
        |
        V
+------------------+
|    Target Code   |
|    Generation    |
+------------------+
```

This is key because it's allow us kind of like `C++` to define our custom operations within dialects, kind of like abstract class in `C++` we can define abstract operations capturing high-level semantics without specifying implementation details.

## How MLIR solves that problem?

**MLIR** still uses **LLVM IR** eventually, so why should we still use it ?

Well, **MLIR** bring a sense of *progressive lowering*, allowing us to keep and optimize our rich semantics and splitting our steps in a more gentle lowering allow the compiler at each step to look and see how to lower properly optimizing in the way.

Let's see how it optimize through a concrete example:

Step 1: The high-level representation:
 
 --> firstly **MLIR** will just take your code and represent it as a high-level graph, without caring about memory allocations or hardware details. This high-level representation could be seen as:
 ```
/* high-level MLIR representation */
%tensor2 = custom.multiply %input_tensor, %tensor1   : tensor<1024xf32>
%tensor3 = custom.add      %tensor2, %tensor1   : tensor<1024xf32>

[%name] [=] [dialect].[operation] [inputs, inputs] [:] [data_type<shape>]
```

Let's now define a few things:

**Dialect**
A dialect could be seen as a self-contained module that extends the compiler with it's own unique vocabulary it can group a specific set of:

- operations
- data types
- attributes

But there is already a few core dialect inside MLIR, let's look at them:

```
- builtin -> the foundational one that defines basic type like int or float

- arith -> the one that handles standard arithmetic operations (add, multiply, bit shifts)
for integers and floats

- tensor / memref -> those are group together because they both handle multi-dimensional array
but one does abstract graph [tensor], and the other concrete physical memory buffer [memref]

- affine -> the one used to optimize structured loops and memory access patterns

- llvm -> this is the lowest one which is a 1:1 replica of traditional LLVM IR
```

**Data Types**

There is two groups of data types, the primitive / built-in and the dialect specific one

**Primitive Data Types**

There only two primitive data types, integer and float they are represented as:
```
integers are written as i, followed by the bit-width like i1 for booleans,i8 for short,
i32 and i64
floating point are written as f, followed by the bit-width like f16, f32, f64
```
**Built-in Data Types**

Those consists of a collection of primitive data types and crucial for high-level optimizations
```
tensor are written as tensor<4x4xf32> which here represents a 4 x 4 grid of 32-bit float
memref are written as memref<4x4xf32> which here represents a 4 x 4 memory ref to physical RAM
vector are written as vector<4xf32> which represents a 1D vector of 4 floats
```
**Dialect-Specific Types or DST**

Any dialect can introduce there own custom types. Those are written as:
```
![dialect name].[variable name]<primitive data type>
the '!' here symbolize a DST to avoid conflicts
```
