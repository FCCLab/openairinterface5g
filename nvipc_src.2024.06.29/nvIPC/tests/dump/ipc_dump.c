/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <stdatomic.h>
#include <sys/queue.h>
#include <sys/epoll.h>

#include "nv_ipc_debug.h"
#include "nv_ipc_utils.h"

// Log level: NVLOG_ERROR, NVLOG_CONSOLE, NVLOG_WARN, NVLOG_INFO, NVLOG_DEBUG, NVLOG_VERBOSE
#define DEFAULT_TEST_LOG_LEVEL NVLOG_DEBUG
#define DEFAULT_TEST_LOG_LEVEL_CONSOLE NVLOG_CONSOLE

#define TEST_MSG_COUNT 3
#define MAX_EVENTS 10
#define TEST_DATA_BUF_LEN 256

#define UDP_PACKET_MAX_SIZE 65000
#define SHM_MSG_BUF_SIZE (512)
#define SHM_DATA_BUF_SIZE (UDP_PACKET_MAX_SIZE - SHM_MSG_BUF_SIZE) // PDU buffer size

#define IPC_DATA_SIZE (100 * 1024)

#define NIC_PCI_ADDR "b5:00.1"
#define ETH_MAC_PRIMARY "b8:ce:f6:33:fe:23"
#define ETH_MAC_SECONDARY "00:00:00:00:00:00" // No need to configure secondary MAC

#define TAG (NVLOG_TAG_BASE_NVIPC + 25) // "NVIPC.DUMP"

nv_ipc_transport_t ipc_transport = NV_IPC_TRANSPORT_SHM;
nv_ipc_module_t    module_type   = NV_IPC_MODULE_SECONDARY;
nv_ipc_t*          ipc           = NULL;

// The CUDA device ID. Can set to -1 to fall back to CPU memory IPC
int test_cuda_device_id = -1;

static char logger_name[NVLOG_NAME_MAX_LEN] = "phy";
static char nvipc_prefix[NVLOG_NAME_MAX_LEN] = "nvipc";


int load_hard_code_config(nv_ipc_config_t* config, int primary, nv_ipc_transport_t _transport)
{
    // Create configuration
    config->ipc_transport = _transport;
    if(set_nv_ipc_default_config(config, module_type) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: set configuration failed", __func__);
        return -1;
    }

    // Override default CUDA device ID for SHM
    if(_transport == NV_IPC_TRANSPORT_SHM)
    {
        config->transport_config.shm.cuda_device_id = -1;
        if(test_cuda_device_id >= 0)
        {
            config->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_CUDA_DATA].pool_len = 128;
        }
    }
    else if(_transport == NV_IPC_TRANSPORT_UDP)
    {
        config->transport_config.udp.msg_buf_size  = SHM_MSG_BUF_SIZE;
        config->transport_config.udp.data_buf_size = SHM_DATA_BUF_SIZE;
    }
    else if(_transport == NV_IPC_TRANSPORT_DPDK)
    {
        config->transport_config.dpdk.mempool_size[NV_IPC_MEMPOOL_CPU_MSG].pool_len  = 4096;
        config->transport_config.dpdk.mempool_size[NV_IPC_MEMPOOL_CPU_MSG].buf_size  = 8192;
        config->transport_config.dpdk.mempool_size[NV_IPC_MEMPOOL_CPU_DATA].pool_len = 1024;
        config->transport_config.dpdk.mempool_size[NV_IPC_MEMPOOL_CPU_DATA].buf_size = 576000;

        config->transport_config.dpdk.cuda_device_id = -1;
        config->transport_config.dpdk.need_eal_init  = 1;
        config->transport_config.dpdk.lcore_id       = 11;
        nvlog_safe_strncpy(config->transport_config.dpdk.prefix, "nvipc", NV_NAME_MAX_LEN);
        nvlog_safe_strncpy(config->transport_config.dpdk.local_nic_pci, NIC_PCI_ADDR, NV_NAME_MAX_LEN);
        if(primary)
        {
            nvlog_safe_strncpy(config->transport_config.dpdk.peer_nic_mac, ETH_MAC_SECONDARY, NV_NAME_MAX_LEN);
        }
        else
        {
            nvlog_safe_strncpy(config->transport_config.dpdk.peer_nic_mac, ETH_MAC_PRIMARY, NV_NAME_MAX_LEN);
        }
    }

    return 0;
}

int main(int argc, char** argv)
{
    int primary = 0;

    int use_yaml_config = 0;

    // Get nvipc configuration
    nv_ipc_config_t config;
    if(argc < 3)
    {
        load_hard_code_config(&config, primary, ipc_transport);
    }
    else
    {
        load_nv_ipc_yaml_config(&config, argv[2], module_type);
        ipc_transport = config.ipc_transport;
    }

    if(argc >= 2)
    {
        if(strnlen(argv[1], NVLOG_NAME_MAX_LEN) > 0)
        {
            snprintf(nvipc_prefix, NVLOG_NAME_MAX_LEN, "%s", argv[1]);
        }

        if(config.ipc_transport == NV_IPC_TRANSPORT_SHM)
        {
            nvlog_safe_strncpy(config.transport_config.shm.prefix, nvipc_prefix, NV_NAME_MAX_LEN);
        }
        else if(config.ipc_transport == NV_IPC_TRANSPORT_DPDK)
        {
            nvlog_safe_strncpy(config.transport_config.dpdk.prefix, nvipc_prefix, NV_NAME_MAX_LEN);
        }
        else if(config.ipc_transport == NV_IPC_TRANSPORT_DOCA)
        {
            nvlog_safe_strncpy(config.transport_config.doca.prefix, nvipc_prefix, NV_NAME_MAX_LEN);
        }
        else
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: unsupported transport type: %d", __func__, config.ipc_transport);
        }

    }

//    if(ipc_transport != NV_IPC_TRANSPORT_SHM)
//    {
//        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: unsupported transport type: %d", __func__, ipc_transport);
//        return -1;
//    }

    nvlog_c_init("/var/log/aerial/ipc_dump.log");
    NVLOGC(TAG, "%s: nvlog [%s] opened ...", __func__, logger_name);
    NVLOGC(TAG, "========================================");

    config.module_type = NV_IPC_MODULE_IPC_DUMP;

    // Create nv_ipc_t instance
    if((ipc = create_nv_ipc_interface(&config)) == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: create IPC interface failed", __func__);
        return -1;
    }

    NVLOGD(TAG, "%s: Dump IPC start ...", __func__);
    NVLOGD(TAG, "========================================");
    nv_ipc_dump(ipc);
    NVLOGD(TAG, "========================================");
    NVLOGD(TAG, "%s: Dump IPC finished", __func__);
    nvlog_c_close();
    return 0;
}
