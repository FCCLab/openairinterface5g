/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _NV_IPC_MEMPOOL_H_
#define _NV_IPC_MEMPOOL_H_

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

/* A nv_ipc_mempool_t contains a CPU SHM block which is divided into two parts: header and body.
 * For CUDA pool, the CPU SHM body is used for store CUDA IPC info, and the body for buffer
 * allocation is replaced with CUDA SHM block.
 */
typedef struct nv_ipc_mempool_t nv_ipc_mempool_t;
struct nv_ipc_mempool_t
{
    int32_t (*alloc)(nv_ipc_mempool_t* mempool);

    int (*free)(nv_ipc_mempool_t* mempool, int32_t index);

    int (*get_index)(nv_ipc_mempool_t* mempool, void* buf);

    void* (*get_addr)(nv_ipc_mempool_t* mempool, int32_t index);

    int (*get_buf_size)(nv_ipc_mempool_t* mempool);

    int (*get_pool_len)(nv_ipc_mempool_t* mempool);

    int (*close)(nv_ipc_mempool_t* mempool);

    // Debug functions, be careful to use
    void* (*get_free_queue)(nv_ipc_mempool_t* mempool);
    int (*get_free_count)(nv_ipc_mempool_t* mempool);
    int (*memcpy_to_host)(nv_ipc_mempool_t* mempool, void* host, const void* device, size_t size);
    int (*memcpy_to_device)(nv_ipc_mempool_t* mempool, void* device, const void* host, size_t size);

#ifdef NVIPC_GDRCPY_ENABLE
    // ReInit for GPU_DATA_POOL
    int (*poolReInit)(nv_ipc_mempool_t* mempool);
#endif
};

#define NV_IPC_MEMPOOL_NO_CUDA_DEV (-1)
#define NV_IPC_MEMPOOL_USE_EXT_DOCA_BUFS (-2)

nv_ipc_mempool_t* nv_ipc_mempool_open(int primary, const char* name, int buf_size, int pool_len, int cuda_device_id);

int nv_ipc_mempool_set_ext_bufs(nv_ipc_mempool_t* mempool, void* gpu_addr, void* cpu_addr);

int nv_ipc_page_lock(void* phost, size_t size);
int nv_ipc_page_unlock(void* phost);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* _NV_IPC_MEMPOOL_H_ */
