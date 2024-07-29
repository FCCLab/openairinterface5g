/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NV_IPC_FORWARD_H_INCLUDED_
#define NV_IPC_FORWARD_H_INCLUDED_

#include <time.h>
#include "nv_ipc.h"

#ifdef __cplusplus
extern "C" {
#endif

// count: 0 - infinite; other - stop forwarding after the count number of messages forwarded
int nvipc_fw_start(nv_ipc_t* ipc, uint32_t count);
int nvipc_fw_stop(nv_ipc_t* ipc);

// Use clock_gettime(CLOCK_REALTIME, ts_abs) to get current time stamp and add a timeout value
int nvipc_fw_sem_timedwait(nv_ipc_t* ipc, const struct timespec* ts_abs);

int nvipc_fw_sem_wait(nv_ipc_t* ipc);
int nvipc_fw_dequeue(nv_ipc_t* ipc, nv_ipc_msg_t* msg);
int nvipc_fw_free(nv_ipc_t* ipc, nv_ipc_msg_t* msg);

// Get lost message count which caused by queue full during forwarding
uint32_t nvipc_fw_get_lost(nv_ipc_t* ipc);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* NV_IPC_FORWARD_H_INCLUDED_ */
