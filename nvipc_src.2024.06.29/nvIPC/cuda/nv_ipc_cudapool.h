/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _NV_IPC_CUDAPOOL_H_
#define _NV_IPC_CUDAPOOL_H_

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define CUDA_INFO_SIZE 512

typedef struct nv_ipc_cudapool_t nv_ipc_cudapool_t;
struct nv_ipc_cudapool_t
{
    void* (*get_cudapool_addr)(nv_ipc_cudapool_t* cudapool);

    int (*memcpy_to_host)(nv_ipc_cudapool_t* cudapool, void* host, const void* device, size_t size);

    int (*memcpy_to_device)(nv_ipc_cudapool_t* cudapool, void* device, const void* host, size_t size);

    int (*close)(nv_ipc_cudapool_t* cudapool);
};

nv_ipc_cudapool_t* nv_ipc_cudapool_open(int primary, void* shm, size_t size, int deviceId);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* _NV_IPC_CUDAPOOL_H_ */
