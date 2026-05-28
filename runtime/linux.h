#pragma once

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
