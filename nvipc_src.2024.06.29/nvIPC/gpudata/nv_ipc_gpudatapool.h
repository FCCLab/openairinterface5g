/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _NV_IPC_GPUDATAPOOL_H_
#define _NV_IPC_GPUDATAPOOL_H_

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define GPUDATA_INFO_SIZE 512
#define ROUND_UP_GDR(x, n)     (((x) + ((n) - 1)) & ~((n) - 1))

typedef struct nv_ipc_gpudatapool_t nv_ipc_gpudatapool_t;
struct nv_ipc_gpudatapool_t
{
    void* (*get_gpudatapool_addr)(nv_ipc_gpudatapool_t* gpudatapool);

    int (*memcpy_to_host)(nv_ipc_gpudatapool_t* gpudatapool, void* host, const void* device, size_t size);

    int (*memcpy_to_device)(nv_ipc_gpudatapool_t* gpudatapool, void* device, const void* host, size_t size);

    int (*close)(nv_ipc_gpudatapool_t* gpudatapool);
};

nv_ipc_gpudatapool_t* nv_ipc_gpudatapool_open(int primary, void* shm, size_t size, int deviceId);
int nv_ipc_gpudatapool_reInit(nv_ipc_gpudatapool_t* pGpuDataPool);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* _NV_IPC_GPUDATAPOOL_H_ */
