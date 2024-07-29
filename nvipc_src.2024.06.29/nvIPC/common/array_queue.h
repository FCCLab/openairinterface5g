/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _INTEGER_RING_H_
#define _INTEGER_RING_H_

#include <stdint.h>
#include <stdatomic.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define DEBUG_TEST

#define NV_NAME_MAX_LEN 32
#define NV_NAME_SUFFIX_MAX_LEN 16

typedef struct
{
    atomic_ulong head;      // Point to the earliest enqueued node.
    atomic_ulong tail;      // Point to the latest enqueued node.
    atomic_ulong enq_count; // Total enqueue counter, for debug info
    atomic_ulong deq_count; // Total dequeue counter, for debug info
    atomic_ulong queue[];   // Queue array data
} array_queue_header_t;

typedef struct array_queue_t array_queue_t;
struct array_queue_t
{
    int32_t (*get_length)(array_queue_t* queue);

    int (*enqueue)(array_queue_t* queue, int32_t value);

    int32_t (*dequeue)(array_queue_t* queue);

    int (*close)(array_queue_t* queue);

    // Debug functions, be careful to use
    char* (*get_name)(array_queue_t* queue);
    int32_t (*get_max_length)(array_queue_t* queue);
    int32_t (*get_count)(array_queue_t* queue);
    int32_t (*get_next)(array_queue_t* queue, int32_t base);
};

static inline int32_t align_8(int32_t num)
{
    return (num + 7) & 0xFFFFFFF8;
}

// The memory size preallocated for the queue structure
#define ARRAY_QUEUE_HEADER_SIZE(queue_len) (align_8(sizeof(array_queue_header_t) + sizeof(atomic_ulong) * (queue_len)))

// header size should be calculated by ARRAY_QUEUE_HEADER_SIZE(length)
array_queue_t* array_queue_open(int primary, const char* name, void* header, int32_t length);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* _INTEGER_RING_H_ */
