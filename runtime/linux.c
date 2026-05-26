#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <CL/cl.h>
#define CPU_FLAGS (1 << 0)
#define GPU_FLAGS (1 << 1)

//CPU
void *runtime_mem_cpu;
size_t offset_cpu;
size_t size_cpu = 1024 * 1024; //1MB for now

//GPU
void *runtime_mem_gpu;
size_t size_gpu = 1024 * 1024; //1MB for now 
size_t offset_gpu;

static void init_memory_cpu()__attribute__((always_inline));
static inline void init_memory_cpu()
{
    //setup a pseudo-mem for runtime CPU
    runtime_mem_cpu = mmap(NULL, size_cpu, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (runtime_mem_cpu == MAP_FAILED)
        return ;
    offset_cpu = 0;
    printf("CPU memory allocated at: %p\n", runtime_mem_cpu);
}

static void init_memory_gpu_openCL() __attribute__((always_inline));
static inline void init_memory_gpu_openCL()
{
    // setup a pseudo-mem for runtime GPU (openCL)
    cl_uint num_platforms = 0;
    clGetPlatformIDs(0, NULL, &num_platforms);
    if (num_platforms == 0)
        return ((void)printf("No OpenCL platforms found.\n"));
    cl_platform_id *platforms = malloc(sizeof(cl_platform_id) * num_platforms);
    clGetPlatformIDs(num_platforms, platforms, NULL);
    cl_device_id device = NULL;
    cl_int errcode_ret = 0;
    for (cl_uint i = 0; i < num_platforms; ++i)
    {
        errcode_ret = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 1, &device, NULL);
        if (errcode_ret == CL_SUCCESS)
            break;
    }
    free(platforms);
    if (!device) 
        return ((void)printf("No GPU device found.\n"));
    char dname[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(dname), dname, NULL);
    printf("Using GPU device: %s\n", dname);
    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &errcode_ret);
    //printf("clCreateContext errcode_ret: %d\n", errcode_ret);
    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, 0, &errcode_ret);
    //printf("clCreateCommandQueueWithProperties errcode_ret: %d\n", errcode_ret);
    cl_mem buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, size_gpu, NULL, &errcode_ret);
    //printf("clCreateBuffer errcode_ret: %d\n", errcode_ret);
    runtime_mem_gpu = clEnqueueMapBuffer(queue, buffer, CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, size_gpu, 0, NULL, NULL, &errcode_ret);
    if (errcode_ret != CL_SUCCESS)
        return ((void)printf("clEnqueueMapBuffer errcode_ret: %d\n", errcode_ret));
    offset_gpu = 0;
    printf("GPU memory allocated at: %p\n", runtime_mem_gpu);
}

static void init_memory_gpu()__attribute__((always_inline));
static inline void init_memory_gpu()
{
    //TODO select which GPU setup for now fallback openCL
    init_memory_gpu_openCL();
}


static void *allocate_cpu(size_t size, size_t align)__attribute__((always_inline));
static inline void *allocate_cpu(size_t size, size_t align)
{
    //malloc equivalent for my runtime cpu mem
    offset_cpu = (offset_cpu + align - 1) & ~(align - 1);
    void *ptr = (char *)runtime_mem_cpu + offset_cpu;
    offset_cpu += size;
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
    {
        init_memory_gpu();
    }
    if (flags & GPU_FLAGS && flags & CPU_FLAGS)
        {}; //TODO setup a third-shared memory
    //printf("Success\n");
}

static void *allocate_gpu_openCL(size_t size, size_t align)__attribute__((always_inline));
static inline void *allocate_gpu_openCL(size_t size, size_t align)
{
    //malloc equivalent for my runtime gpu (using openCL) mem
    offset_gpu = (offset_gpu + align - 1) & ~(align - 1);
    void *ptr = (char *)runtime_mem_gpu + offset_gpu;
    offset_gpu += size;
    return (ptr);
}

static void *allocate_gpu(size_t size, size_t align)__attribute__((always_inline));
static inline void *allocate_gpu(size_t size, size_t align)
{
    return (allocate_gpu_openCL(size, align));
}

void *cmalloc(size_t size, size_t align)
{
    uint8_t cpu_flags = 1;
    uint8_t gpu_flags = 1;

    if (cpu_flags)
        return (allocate_cpu(size, align));
    /*else*/ if (gpu_flags)
        return (allocate_gpu(size, align));
    return (NULL);
}

int main(int ac, char **av)
{
    uint8_t flags = 0;

    if (ac > 3 || ac < 2)
        return (-1);
    if (ac >= 2 && av[1][0] == 'G')
        flags |= GPU_FLAGS;
    if (ac == 3 && av[2][0] == 'C')
        flags |= CPU_FLAGS;
    if (ac >= 2 && av[1][0] == 'C')
        flags |= CPU_FLAGS;
    if (ac == 3 && av[2][0] == 'G')
        flags |= GPU_FLAGS; 
    init(flags); // setup a unified lvl of abstraction for my API
    //link();
    //run();
    return (0);
}
