/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _NV_IPC_SEM_H_
#define _NV_IPC_SEM_H_

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define NV_SEM_NAME_MAX_LEN 32

typedef struct nv_ipc_sem_t nv_ipc_sem_t;
struct nv_ipc_sem_t
{
    int (*sem_post)(nv_ipc_sem_t* ipc_sem);

    int (*sem_wait)(nv_ipc_sem_t* ipc_sem);

    int (*get_value)(nv_ipc_sem_t* ipc_sem, int* value);

    int (*close)(nv_ipc_sem_t* ipc_sem);
};

nv_ipc_sem_t* nv_ipc_sem_open(int primary, const char* prefix);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* _NV_IPC_SEM_H_ */
