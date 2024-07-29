/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include <stdlib.h>

#include "nv_ipc.h"
#include "nv_ipc_utils.h"
#include "nv_ipc_config.h"
#include "nv_ipc_shm.h"
#include "nv_ipc_debug.h"

#define TAG (NVLOG_TAG_BASE_NVIPC + 8) // "NVIPC.IPC"

#define NVIPC_DEFAULT_PREFIX "nvipc"
#define NVIPC_CONFIG_SHMPOOL_NAME "_app_config"
#define NV_NAME_SUFFIX_MAX_LEN 16

nv_ipc_t* create_shm_nv_ipc_interface(const nv_ipc_config_t* cfg);
nv_ipc_t* create_udp_nv_ipc_interface(const nv_ipc_config_t* cfg);
#ifdef NVIPC_DPDK_ENABLE
nv_ipc_t* create_dpdk_nv_ipc_interface(const nv_ipc_config_t* cfg);
#endif
#ifdef NVIPC_DOCA_ENABLE
nv_ipc_t* create_doca_nv_ipc_interface(const nv_ipc_config_t* cfg);
#endif

typedef struct nv_ipc_link_node_t nv_ipc_link_node_t;
struct nv_ipc_link_node_t
{
    nv_ipc_t*           ipc;
    nv_ipc_link_node_t* next;
    char                prefix[NV_NAME_MAX_LEN];
    nv_ipc_config_t     config;
};

typedef struct
{
    int32_t config_items[NV_IPC_CFG_ITEM_NUM];
} config_items_t;

static nv_ipc_shm_t*       cfgpool       = NULL;
static config_items_t*     configs       = NULL;
static nv_ipc_link_node_t* ipc_list_head = NULL;

static void nv_ipc_add_instance(nv_ipc_t* ipc, const char* prefix, const nv_ipc_config_t* cfg)
{
    nv_ipc_link_node_t* pnode = malloc(sizeof(nv_ipc_link_node_t));
    if (pnode == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: malloc failed", __func__);
        return;
    }

    pnode->ipc                = ipc;
    pnode->next               = NULL;
    nvlog_safe_strncpy(pnode->prefix, prefix, NV_NAME_MAX_LEN);
    memcpy(&pnode->config, cfg, sizeof(nv_ipc_config_t));

    if(ipc_list_head == NULL)
    {
        ipc_list_head = pnode;
    }
    else
    {
        nv_ipc_link_node_t* curr = ipc_list_head;
        while(curr->next != NULL)
        {
            curr = curr->next;
        }
        curr->next = pnode;
    }
}

nv_ipc_config_t* nv_ipc_get_config_instance(const nv_ipc_t* ipc)
{
    nv_ipc_config_t* cfg = NULL;

    if(ipc == NULL)
    {
        return ipc_list_head == NULL ? NULL : &ipc_list_head->config;
    }
    else
    {
        nv_ipc_link_node_t* curr;
        for(curr = ipc_list_head; curr != NULL; curr = curr->next)
        {
            if(ipc == curr->ipc)
            {
                break;
            }
        }
        return curr == NULL ? NULL : &curr->config;
    }
}

nv_ipc_config_t* nv_ipc_get_config_by_name(const char* prefix)
{
    nv_ipc_config_t* cfg = NULL;

    if(prefix == NULL)
    {
        return ipc_list_head == NULL ? NULL : &ipc_list_head->config;
    }
    else
    {
        nv_ipc_link_node_t* curr;
        for(curr = ipc_list_head; curr != NULL; curr = curr->next)
        {
            if(strncmp(prefix, curr->prefix, NV_NAME_MAX_LEN - 1))
            {
                break;
            }
        }
        return curr == NULL ? NULL : &curr->config;
    }
}

nv_ipc_t* nv_ipc_get_instance(const char* prefix)
{
    if(prefix == NULL)
    {
        return ipc_list_head == NULL ? NULL : ipc_list_head->ipc;
    }
    else
    {
        nv_ipc_link_node_t* curr;
        for(curr = ipc_list_head; curr != NULL; curr = curr->next)
        {
            if(strncmp(prefix, curr->prefix, NV_NAME_MAX_LEN - 1))
            {
                break;
            }
        }
        return curr == NULL ? NULL : curr->ipc;
    }
}

void nv_ipc_app_config_set(config_item_t item, int32_t value)
{
    if(configs == NULL || item >= NV_IPC_CFG_ITEM_NUM)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: failed: configs=%p item=%d", __func__, configs, item);
        return;
    }

    configs->config_items[item] = value;
}

int32_t nv_ipc_app_config_get(config_item_t item)
{
    if(configs == NULL || item >= NV_IPC_CFG_ITEM_NUM)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: failed: configs=%p item=%d", __func__, configs, item);
        return 0;
    }
    return configs->config_items[item];
}

// Old function, keep compatible
void nv_ipc_set_config(nv_ipc_t* ipc, config_item_t item, int32_t value)
{
    nv_ipc_app_config_set(item, value);
}

// Old function, keep compatible
int32_t nv_ipc_get_config(nv_ipc_t* ipc, config_item_t item)
{
    return nv_ipc_app_config_get(item);
}

int nv_ipc_app_config_shmpool_open(int primary, const char* prefix)
{
    char name[NV_NAME_MAX_LEN + 16];
    nvlog_safe_strncpy(name, "nvipc_cfg", NV_NAME_MAX_LEN);
    strncat(name, NVIPC_CONFIG_SHMPOOL_NAME, NV_NAME_SUFFIX_MAX_LEN);

    if((cfgpool = nv_ipc_shm_open(primary, name, sizeof(config_items_t))) == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: primary=%d, prefix=%s", __func__, primary, prefix);
        return -1;
    }
    configs = cfgpool->get_mapped_addr(cfgpool);
    if(primary)
    {
        // Default configurations
        nv_ipc_app_config_set(NV_IPC_CFG_FAPI_TYPE, 1);
        nv_ipc_app_config_set(NV_IPC_CFG_FAPI_TB_LOC, 1);
        nv_ipc_app_config_set(NV_IPC_CFG_FORWARD_ENABLE, 0);
        nv_ipc_app_config_set(NV_IPC_CFG_DEBUG_TIMING, 0);
        nv_ipc_app_config_set(NV_IPC_CFG_PCAP_ENABLE, 0);
    }
    return 0;
}

int nv_ipc_app_config_open(const nv_ipc_config_t* cfg)
{
    nv_ipc_module_t    module_type   = cfg->module_type;
    nv_ipc_transport_t ipc_transport = cfg->ipc_transport;
    NVLOGI(TAG, "%s: module_type=%d, transport=%d cfgpool=%p", __func__, module_type, ipc_transport, cfgpool);

    if(cfgpool == NULL)
    {
        if(ipc_transport == NV_IPC_TRANSPORT_SHM)
        {
            if (cfg->transport_config.shm.primary)
            {
                // SHM secondary app should call nv_ipc_app_config_shmpool_open() after connected
                // nv_ipc_app_config_shmpool_open(1, cfg->transport_config.shm.prefix);
                nv_ipc_app_config_shmpool_open(1, "nvipc_cfg");
            }
        }
        else if(ipc_transport == NV_IPC_TRANSPORT_DPDK)
        {
            nv_ipc_app_config_shmpool_open(1, cfg->transport_config.dpdk.prefix);
        }
        else if(ipc_transport == NV_IPC_TRANSPORT_DOCA)
        {
            nv_ipc_app_config_shmpool_open(1, cfg->transport_config.doca.prefix);
        }
        else
        {
            nv_ipc_app_config_shmpool_open(1, NVIPC_DEFAULT_PREFIX);
        }
    }
    return 0;
}

int nv_ipc_dump(nv_ipc_t* ipc)
{
    nv_ipc_config_t* cfg = nv_ipc_get_config_instance(ipc);
    if (cfg == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s instance not exist: ipc=0x%p", __func__, ipc);
        return -1;
    }

    if(cfg->ipc_transport == NV_IPC_TRANSPORT_SHM)
    {
        return shm_ipc_dump(ipc);
    }
    else if(cfg->ipc_transport == NV_IPC_TRANSPORT_DPDK)
    {
        NVLOGI(TAG, "%s: TODO module_type=%d, transport=%d", __func__, cfg->module_type,
                cfg->ipc_transport);
        return -1;
    }
#ifdef NVIPC_DOCA_ENABLE
    else if(cfg->ipc_transport == NV_IPC_TRANSPORT_DOCA)
    {
        return doca_ipc_dump(ipc);
    }
#endif
    else
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s unsupported transport: %d", __func__, cfg->ipc_transport);
        return -1;
    }
}

nv_ipc_t* create_nv_ipc_interface(const nv_ipc_config_t* cfg)
{
    NVLOGI(TAG, "%s: module_type=%d, transport=%d", __func__, cfg->module_type, cfg->ipc_transport);

    if(cfg->module_type < 0 || cfg->module_type >= NV_IPC_MODULE_MAX)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s unsupported module_type: %d", __func__, cfg->module_type);
        return NULL;
    }

    // Open APP configuration SHM pool if not exist
    nv_ipc_app_config_open(cfg);

    nv_ipc_t* ipc = NULL;
    if(cfg->ipc_transport == NV_IPC_TRANSPORT_SHM)
    {
        ipc = create_shm_nv_ipc_interface(cfg);
        nv_ipc_add_instance(ipc, cfg->transport_config.shm.prefix, cfg);
        if((cfg->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_GPU_DATA].pool_len > 0) && (cfg->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_GPU_DATA].buf_size > 0))
        {
            /* If we are here, that means GPU_DATA mempool has been successfully created */
            NVLOGI(TAG, "%s: NV_IPC_CFG_FAPI_TB_LOC set to 3", __func__);
            nv_ipc_app_config_set(NV_IPC_CFG_FAPI_TB_LOC, 3);
        }
    }
    else if(cfg->ipc_transport == NV_IPC_TRANSPORT_UDP)
    {
        ipc = create_udp_nv_ipc_interface(cfg);
        nv_ipc_add_instance(ipc, NVIPC_DEFAULT_PREFIX, cfg);
    }
#ifdef NVIPC_DPDK_ENABLE
    else if(cfg->ipc_transport == NV_IPC_TRANSPORT_DPDK)
    {
        ipc = create_dpdk_nv_ipc_interface(cfg);
        nv_ipc_add_instance(ipc, cfg->transport_config.dpdk.prefix, cfg);
    }
#endif
#ifdef NVIPC_DOCA_ENABLE
    else if(cfg->ipc_transport == NV_IPC_TRANSPORT_DOCA)
    {
        ipc = create_doca_nv_ipc_interface(cfg);
        nv_ipc_add_instance(ipc, cfg->transport_config.doca.prefix, cfg);
    }
#endif
    else
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s unsupported transport: %d", __func__, cfg->ipc_transport);
        return NULL;
    }

    return ipc;
}
