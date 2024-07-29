/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _NV_IPC_SHM_H_
#define _NV_IPC_SHM_H_

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct nv_ipc_shm_t nv_ipc_shm_t;
struct nv_ipc_shm_t
{
    void* (*get_mapped_addr)(nv_ipc_shm_t* ipc_shm);

    size_t (*get_size)(nv_ipc_shm_t* ipc_shm);

    int (*close)(nv_ipc_shm_t* ipc_shm);
};

nv_ipc_shm_t* nv_ipc_shm_open(int primary, const char* name, size_t size);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* _NV_IPC_SHM_H_ */
