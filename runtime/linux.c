#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>

#define CPU_FLAGS (1 << 0)
#define GPU_FLAGS (1 << 1)

void *runtime_mem_cpu;
size_t offset;
size_t size_cpu = 1024 * 1024; //1MB for now

static void init_memory_cpu()__attribute__((always_inline));
static inline void init_memory_cpu()
{
    //setup a pseudo-mem for runtime CPU
    runtime_mem_cpu = mmap(NULL, size_cpu, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (runtime_mem_cpu == MAP_FAILED)
        return ;
    offset = 0;
    printf("memory allocated at: %p\n", runtime_mem_cpu);
}

static void *allocate_cpu(size_t size, size_t align)__attribute__((always_inline));
static inline void *allocate_cpu(size_t size, size_t align)
{
    //malloc equivalent for my runtime cpu mem
    offset = (offset + align - 1) & ~(align - 1);
    void *ptr = (char *)runtime_mem_cpu + offset;
    offset += size;
    return (ptr);
}

static void init(uint8_t flags)__attribute__((always_inline));
static inline void init(uint8_t flags)
{
    if (flags & CPU_FLAGS)
    {
        init_memory_cpu();
    }
    if (flags & GPU_FLAGS)
        {};//TODO setup GPU memory
    if (flags & GPU_FLAGS && flags & CPU_FLAGS)
        {}; // setup a third-shared memory
    //printf("Success\n");
}

void *cmalloc(size_t size, size_t align)
{
    if (cpu)
        return (allocate_cpu(size, align));
    else if (gpu)
        return (allocate_gpu(size, align));
}

int main(int ac, char **av)
{
    uint8_t flags = 0;

    if (ac > 3 || ac < 2)
        return (-1);
    if (ac == 2 && av[1][0] == 'G')
        flags |= GPU_FLAGS;
    if (ac == 3 && av[2][0] == 'C')
        flags |= CPU_FLAGS;
    if (ac == 2 && av[1][0] == 'C')
        flags |= GPU_FLAGS;
    if (ac == 3 && av[2][0] == 'G')
        flags |= CPU_FLAGS; 
    init(flags); //TODO setup a unified lvl of abstraction for my API
    //link();
    //run();
    return (0);
}
