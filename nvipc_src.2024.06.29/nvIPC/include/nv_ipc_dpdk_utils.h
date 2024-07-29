/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NV_IPC_DPDK_UTILS_H
#define NV_IPC_DPDK_UTILS_H

#include <stdint.h>
#include <stddef.h>

#include "nv_ipc.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if 0
int memcpy_from_nvipc(const nv_ipc_msg_t* src, uint8_t* dst, uint32_t size);
int memcpy_to_nvipc(nv_ipc_msg_t* dst, const uint8_t* src, uint32_t size);
#endif

int  create_dpdk_task(int (*func)(void* arg), void* arg, uint16_t lcore_id);
int  nv_ipc_dpdk_init(const char* argv0, const nv_ipc_config_dpdk_t* cfg);
void dpdk_print_lcore(const char* info);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* NV_IPC_DPDK_UTILS_H */
