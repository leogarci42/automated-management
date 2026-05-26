# MIMD (Multiple Instruction Multiple Data)

## Brief definition of what is a MIMD

**MIMD** or *Multiple Instruction Multiple Data* is a key concept in computer architecture, and especially for **HPC** (*High Performance Computing*), where we aim to gain high speed in scientific computation by breaking the computation into pieces which are independent enough to be performed in parralel using several process running on separate hardware units but cooperative enough that they can be able to solve a single problem. In order to achieve such capacity, it exists two basic types of **MIMD**, one being the *Shared Memory Multiprocessors* or **SMM** and the other being *Distributed Memory Multiprocessors* or **DMM**.

## Shared Memory Multiprocessors (SMM)

### Brief definition of what is a SMM:

An **SMM** uses a *Shared Memory Switch*, which can be visualize as the tool that allow our device to access the memory (e.g. read, write), retrieve the data or modify it and send it back to us if necessary. Thus, to prevent data race, and ensure synchronization it's necessary to use a lock system to modify or retrieve it's value safely (e.g. Mutex or Semaphore).

simple visualization of it:
```
   +---------------+   +---------------+   +---------------+   +---------------+
   |  Processor 1  |   |  Processor 2  |   |  Processor 3  |   |  Processor 4  |
   |     (CPU)     |   |     (GPU)     |   |     (CPU)     |   |     (GPU)     |
   +-------+-------+   +-------+-------+   +-------+-------+   +-------+-------+
           |                   |                   |                   |
           |                   |                   |                   |
  =========v===================v===================v===================v========
  |                                                                            |
  |                             Shared Memory Switch                           |
  |                                                                            |
  =========+===================+===================+===================+========
           |                   |                   |                   |
           |                   |                   |                   |
   +-------v-------+   +-------v-------+   +-------v-------+   +-------v-------+
   | Memory Mod. 1 |   | Memory Mod. 2 |   | Memory Mod. 3 |   | Memory Mod. 4 |
   |     (M1)      |   |     (M2)      |   |     (M3)      |   |     (M4)      |
   +---------------+   +---------------+   +---------------+   +---------------+
```

## Distributed Memory Multiprocessors (DMM)

### Brief definition of what is a DMM:

An **DMM** to the contrary of an **SMM**, each processor carry their own memory, and the *Communications Switch* only allows to pass the memory from one to the other, the switch don't have to worry about the memory itself, as each processor does already for themselves.

simple visualization of it:
```
   +-------------------+                   +-------------------+
   |      NODE 1       |                   |      NODE 2       |
   | +---------------+ |                   | +---------------+ |
   | |  Processor 1  | |                   | |  Processor 2  | |
   | |     (CPU)     | |                   | |     (GPU)     | |
   | +-------+-------+ |                   | +-------+-------+ |
   |         |         |                   |         |         |
   | +-------v-------+ |                   | +-------v-------+ |
   | | Local Memory  | |                   | | Local Memory  | |
   | | (Private RAM) | |                   | | (Private VRAM)| |
   | +---------------+ |                   | +---------------+ |
   +---------+---------+                   +---------+---------+
             |                                       |
             |       [ EXPLICIT MESSAGE PASSING ]    |
             +--------> (Communications Switch) <----+
             |            (e.g., PCIe / MPI)         |
             |                                       |
   +---------+---------+                   +---------+---------+
   | +---------------+ |                   | +---------------+ |
   | | Local Memory  | |                   | | Local Memory  | |
   | | (Private RAM) | |                   | | (Private VRAM)| |
   | +-------^-------+ |                   | +-------^-------+ |
   |         |         |                   |         |         |
   | +-------+-------+ |                   | +-------+-------+ |
   | |  Processor 3  | |                   | |  Processor 4  | |
   | |     (CPU)     | |                   | |     (GPU)     | |
   | +---------------+ |                   | +---------------+ |
   |      NODE 3       |                   |      NODE 4       |
   +-------------------+                   +-------------------+
```

## SMM vs DMM

### What are the pros and cons of using an SMM:
- pros:
    - Easier to implement.
- cons:
    - Interfacing many processors may lead to long and variable memory latency.
    - Harder to scale and more vulnerable to failure since a fault can affect the whole system.

### What are the pros and cons of using an DMM:
- pros:
    - more scalable and fault tolerant.
- cons:
    - Harder to implement.

## DOCUMENTATION

[Shared Versus Distributed Memory Multiprocessors - Harry F. Jordan](https://www.ecmwf.int/sites/default/files/elibrary/1990/10302-shared-versus-distributed-memory-multiprocessors.pdf)

[Very High-Speed Computing Systems - Michael J. Flynn](https://safari.ethz.ch/architecture/fall2019/lib/exe/fetch.php?media=flynn_1966.pdf)

[Compiling programs for distributed-memory multiprocessors - David Callahan & Ken Kennedy](https://www.researchgate.net/publication/225208856_Compiling_programs_for_distributed-memory_multiprocessors)
