/*
 * Copyright (c) 2021-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _NV_IPC_CONFIG_H_
#define _NV_IPC_CONFIG_H_

#include "nv_ipc.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* NV_IPC_CFG_FAPI_TYPE
 * -- 0: Altran FAPI
 * -- 1: SCF FAPI
 */

/* NV_IPC_CFG_FAPI_TB_LOC:
 * -- 0: FAPI message and TB both in CPU_DATA buffer
 * -- 1: FAPI message in CPU_MSG buffer, TB in CPU_DATA buffer
 * -- 2: FAPI message in CPU_MSG buffer, TB in GPU_DATA buffer
 * -- 3: FAPI message in CPU_MSG buffer, TB in GPU_DATA buffer (with GDRCopy enabled)
*/

typedef enum
{
    NV_IPC_CFG_FAPI_TYPE      = 0,
    NV_IPC_CFG_FAPI_TB_LOC    = 1,
    NV_IPC_CFG_FORWARD_ENABLE = 2,
    NV_IPC_CFG_DEBUG_TIMING   = 3,
    NV_IPC_CFG_PCAP_ENABLE    = 4,
    NV_IPC_CFG_PCAP_CPU_CORE  = 5,
    NV_IPC_CFG_PCAP_CACHE_BIT = 6,
    NV_IPC_CFG_PCAP_FILE_BIT  = 7,
    NV_IPC_CFG_PCAP_MAX_MSG_SIZE  = 8,
    NV_IPC_CFG_PCAP_MAX_DATA_SIZE = 9,
    NV_IPC_CFG_ITEM_NUM       = 10,
} config_item_t;

// Deprecated, keep compatible. Use nv_ipc_app_config_set/get() instead
void    nv_ipc_set_config(nv_ipc_t* ipc, config_item_t item, int32_t value);
int32_t nv_ipc_get_config(nv_ipc_t* ipc, config_item_t item);

void    nv_ipc_app_config_set(config_item_t item, int32_t value);
int32_t nv_ipc_app_config_get(config_item_t item);

int nv_ipc_app_config_open(const nv_ipc_config_t* cfg);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* _NV_IPC_CONFIG_H_ */
