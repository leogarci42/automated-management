#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <CL/cl.h>
#include <stdbool.h>
#include <time.h>

#define MAX_DEVICES 8
#define MAX_ALLOCATIONS 1024

#define DEV_TYPE_CPU (1 << 0)
#define DEV_TYPE_GPU (1 << 1)

// branch prediction for the backend
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

// packed structure ordered largest to smallest to prevent padding
typedef struct __attribute__((packed, aligned(8))) {
    void     *mem_base;
    size_t   size;
    size_t   offset;
    uint32_t id;
    uint8_t  type;
} dmm_node_t;

typedef struct __attribute__((packed, aligned(8))) {
    size_t   offsets[MAX_DEVICES];
    size_t   alloc_size;
    uint8_t  dirty_mask;
} dmm_entry_t;

// global state grouped and aligned to 64 bytes (l1 Cache line) to avoid false sharing
typedef struct __attribute__((aligned(64))) {
    dmm_node_t  nodes[MAX_DEVICES];
    dmm_entry_t entries[MAX_ALLOCATIONS];
    size_t      node_count;
    size_t      entry_count;
    size_t      offset_shared;
    size_t      size_shared;
    bool        is_shared;
    bool        w_shared;
} dmm_state_t;

static dmm_state_t g_state;

// hadware handler
cl_context       g_cl_ctxs[MAX_DEVICES];
cl_command_queue g_cl_queues[MAX_DEVICES];
cl_mem           g_cl_buffers[MAX_DEVICES];

__attribute__((always_inline, cold))
static inline void register_cpu_node()
{
    if (UNLIKELY(g_state.node_count >= MAX_DEVICES))
        return ;        
    dmm_node_t node;
    node.type = DEV_TYPE_CPU;
    node.id = (uint32_t)g_state.node_count;
    node.size = 1024 * 1024; // 1MB
    node.offset = 0;
    size_t alloc_sz = g_state.is_shared ? (node.size + g_state.size_shared) : node.size;
    node.mem_base = mmap(NULL, alloc_sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (UNLIKELY(node.mem_base == MAP_FAILED))
        return ;
    g_state.nodes[g_state.node_count] = node;
    printf("Registered CPU Node [%d] memory allocated at: %p\n", node.id, node.mem_base);
    g_state.node_count++;
}

__attribute__((always_inline, cold))
static inline cl_device_id fetch_opencl_device()
{
    cl_uint num_platforms = 0;
    clGetPlatformIDs(0, NULL, &num_platforms);
    if (num_platforms == 0)
        return (NULL);
    cl_platform_id *platforms = malloc(sizeof(cl_platform_id) * num_platforms);
    clGetPlatformIDs(num_platforms, platforms, NULL);
    cl_device_id device = NULL;
    for (cl_uint i = 0; i < num_platforms; ++i)
        if (clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 1, &device, NULL) == CL_SUCCESS)
            break;
    free(platforms);
    return (device);
}

__attribute__((always_inline, cold))
static inline void register_gpu_node_openCL()
{
    if (UNLIKELY(g_state.node_count >= MAX_DEVICES))
        return ;
    cl_device_id device = fetch_opencl_device();
    if (!device) 
        return ((void)printf("No GPU device found.\n"));
    dmm_node_t node;
    node.type = DEV_TYPE_GPU;
    node.id = (uint32_t)g_state.node_count;
    node.size = 1024 * 1024; // 1MB
    node.offset = 0;
    cl_int err = 0;
    g_cl_ctxs[node.id] = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    g_cl_queues[node.id] = clCreateCommandQueueWithProperties(g_cl_ctxs[node.id], device, 0, &err);
    size_t total_gpu_size = g_state.is_shared ? (node.size + g_state.size_shared) : node.size;
    g_cl_buffers[node.id] = clCreateBuffer(g_cl_ctxs[node.id], CL_MEM_READ_WRITE, total_gpu_size, NULL, &err);
    node.mem_base = clEnqueueMapBuffer(g_cl_queues[node.id], g_cl_buffers[node.id], CL_TRUE, CL_MAP_READ | CL_MAP_WRITE, 0, total_gpu_size, 0, NULL, NULL, &err);
    if (UNLIKELY(err != CL_SUCCESS))
        return ((void)printf("clEnqueueMapBuffer errcode_ret failed\n"));
    g_state.nodes[g_state.node_count] = node;
    printf("Registered GPU Node [%d] memory allocated at: %p\n", node.id, node.mem_base);
    g_state.node_count++;
}

__attribute__((always_inline, cold))
static inline void init_topology(uint8_t target_mode)
{
    g_state.node_count = 0;
    g_state.entry_count = 0;
    g_state.offset_shared = 0;
    g_state.size_shared = 1024 * 1024;
    g_state.w_shared = false;
    if (target_mode == 0)
    {
        g_state.is_shared = true;
        register_cpu_node();
        register_cpu_node();
    }
    else if (target_mode == 1)
    {
        g_state.is_shared = true;
        register_cpu_node();
        register_gpu_node_openCL();
    }
}

__attribute__((always_inline, hot))
static inline void memory_cross_target(size_t entry_idx, size_t src_node_id, size_t dst_node_id)
{
    dmm_entry_t *entry = &g_state.entries[entry_idx];
    void *dst = (char *)g_state.nodes[dst_node_id].mem_base + entry->offsets[dst_node_id];
    void *src = (char *)g_state.nodes[src_node_id].mem_base + entry->offsets[src_node_id];
    
    // lowers to CPU vector/SIMD instructions instead of a while loop
    __builtin_memcpy(dst, src, entry->alloc_size); 
}

__attribute__((always_inline, hot))
static inline void *allocate_shared(size_t size, size_t align)
{
    if (UNLIKELY(g_state.entry_count >= MAX_ALLOCATIONS))
        return (NULL);
    g_state.offset_shared = (g_state.offset_shared + align - 1) & ~(align - 1);
    dmm_entry_t *entry = &g_state.entries[g_state.entry_count];
    entry->alloc_size = size;
    entry->dirty_mask = 0; 
    size_t node_idx = 0;
    while (node_idx < g_state.node_count)
    {
        entry->offsets[node_idx] = g_state.offset_shared;
        node_idx++;
    }
    void *ptr = (char *)g_state.nodes[0].mem_base + g_state.offset_shared;
    g_state.offset_shared += size;
    if (UNLIKELY(g_state.offset_shared > g_state.size_shared))
        return (NULL);
    g_state.entry_count++;
    g_state.w_shared = true;
    return (ptr);
}

__attribute__((always_inline, hot))
void *cmalloc(size_t size, size_t align)
{
    if (LIKELY(g_state.is_shared == true))
        return (allocate_shared(size, align));
    return (NULL);
}

__attribute__((always_inline, hot))
void link(uint32_t executing_node_id)
{
    size_t idx = 0;
    while (idx < g_state.entry_count)
    {
        dmm_entry_t *entry = &g_state.entries[idx];
        //fast evaluation for state checks
        if (UNLIKELY(entry->dirty_mask != 0 && !(entry->dirty_mask & (1 << executing_node_id))))
        {
            size_t src = 0;
            while (src < g_state.node_count)
            {
                if (entry->dirty_mask & (1 << src))
                {
                    memory_cross_target(idx, src, executing_node_id);
                    break;
                }
                src++;
            }
        }
        entry->dirty_mask = (1 << executing_node_id);
        idx++;
    }
}

__attribute__((always_inline, cold))
static inline void teardown_topology()
{
    size_t i = 0;
    while (i < g_state.node_count)
    {
        dmm_node_t *node = &g_state.nodes[i];
        if (node->type == DEV_TYPE_GPU && LIKELY(node->mem_base))
        {
            cl_event unmap_event;
            clEnqueueUnmapMemObject(g_cl_queues[node->id], g_cl_buffers[node->id], node->mem_base, 0, NULL, &unmap_event);
            clWaitForEvents(1, &unmap_event);
            clReleaseMemObject(g_cl_buffers[node->id]);
            clReleaseCommandQueue(g_cl_queues[node->id]);
            clReleaseContext(g_cl_ctxs[node->id]);
        }
        else if (node->type == DEV_TYPE_CPU && LIKELY(node->mem_base))
            munmap(node->mem_base, g_state.is_shared ? (node->size + g_state.size_shared) : node->size);
        i++;
    }
}

__attribute__((always_inline, cold))
static void assert_integrity(int node_id, size_t alloc_idx, int expected_val)
{
    dmm_entry_t *entry = &g_state.entries[alloc_idx];
    int *data = (int*)((char*)g_state.nodes[node_id].mem_base + entry->offsets[node_id]);
    if (data[0] != expected_val)
    {
        printf("CRITICAL ERROR: Integrity failure on Node %d at alloc %zu. Expected %d, got %d\n", 
                node_id, alloc_idx, expected_val, data[0]);
        exit(1);
    }
}

__attribute__((always_inline, cold))
static void stress_test_harness()
{
    printf("--- Starting distributed stress test ---\n");
    srand(42);
    size_t num_allocs = 10;
    for (size_t i = 0; i < num_allocs; i++)
    {
        cmalloc(sizeof(int), sizeof(int));
        int target_node = rand() % g_state.node_count;
        link(target_node);
        int *active_ptr = (int*)((char*)g_state.nodes[target_node].mem_base + g_state.entries[i].offsets[target_node]);
        *active_ptr = (int)i * 100;
        int verify_node = (target_node + 1) % g_state.node_count;
        link(verify_node);
        assert_integrity(verify_node, i, (int)i * 100);
        printf("Allocation %zu: successfully synchronized to Node %d\n", i, verify_node);
    }
    printf("--- Stress test passed: coherency maintained across %zu allocs ---\n", num_allocs);
}

int main(int ac, char **av)
{
    uint8_t target_mode = 0; 

    if (ac < 2)
        return (-1);
        
    if (ac > 1 && av[1][0] == 'T') //Test the code
    {
        init_topology(1);
        stress_test_harness();
        teardown_topology();
        return 0;
    }
    if (av[1][0] == '2')
        target_mode = 0;
    else if (av[1][0] == 'H')
        target_mode = 1;
    init_topology(target_mode);
    if (g_state.w_shared == true) 
    {
        int *shared_array = (int*)cmalloc(10 * sizeof(int), sizeof(int));
        link(0); 
        shared_array[0] = 8888;
        link(1); 
        int *verif_array = (int*)((char*)g_state.nodes[1].mem_base + g_state.entries[0].offsets[1]);
        printf("Node [1] verified modular cluster mirror data: %d\n", verif_array[0]);
    }
    teardown_topology();
    return (0);
}
