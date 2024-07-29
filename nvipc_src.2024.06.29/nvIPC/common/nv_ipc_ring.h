/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _NV_IPC_RING_H_
#define _NV_IPC_RING_H_

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
    RING_TYPE_SHM_SECONDARY = 0, // Shared memory primary process
    RING_TYPE_SHM_PRIMARY = 1, // Shared memory secondary process
    RING_TYPE_APP_INTERNAL = 2, // Application internal memory, no share
    RING_TYPE_INVALID = 3,
} ring_type_t;

typedef struct nv_ipc_ring_t nv_ipc_ring_t;
struct nv_ipc_ring_t
{
    int32_t (*alloc)(nv_ipc_ring_t* ring);

    int (*free)(nv_ipc_ring_t* ring, int32_t index);

    int (*get_index)(nv_ipc_ring_t* ring, void* buf);

    void* (*get_addr)(nv_ipc_ring_t* ring, int32_t index);

    int (*enqueue_by_index)(nv_ipc_ring_t* ring, int32_t index);

    int32_t (*dequeue_by_index)(nv_ipc_ring_t* ring);

    int (*get_buf_size)(nv_ipc_ring_t* ring);

    // Action set: alloc, copy, enqueue
    int (*enqueue)(nv_ipc_ring_t* ring, void* obj);

    // Action copy, dequeue, free
    int (*dequeue)(nv_ipc_ring_t* ring, void* obj);

    int (*close)(nv_ipc_ring_t* ring);

    // Enqueued count. NOTE: count may be changing all the time in lock-free ring.
    int (*get_count)(nv_ipc_ring_t* ring);

    // Free buffer count. NOTE: free count may be changing all the time in lock-free ring.
    int (*get_free_count)(nv_ipc_ring_t* ring);
};

nv_ipc_ring_t* nv_ipc_ring_open(ring_type_t type, const char* name, int32_t rin_len, int32_t buf_size);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* _NV_IPC_RING_H_ */
