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
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <unistd.h>

#include "nv_ipc.h"
#include "nv_ipc_debug.h"
#include "nv_ipc_efd.h"
#include "nv_ipc_sem.h"
#include "nv_ipc_epoll.h"
#include "nv_ipc_shm.h"
#include "nv_ipc_mempool.h"
#include "nv_ipc_utils.h"
#include "array_queue.h"
#include "nv_ipc_cuda_utils.h"
#include "nv_ipc_config.h"
#include "nv_ipc_forward.h"
#include "nv_ipc_cuda_utils.h"

#define TAG (NVLOG_TAG_BASE_NVIPC + 6) // "NVIPC.SHM"

#define DBG_SEM_INIT 1

// Configuration: use semaphore or event FD for synchronization: 0 - semaphore; 1 - event FD
#define CONFIG_USE_EVENT_FD 1

// Configuration: create epoll wrapper to provide blocking wait APIs
#define CONFIG_CREATE_EPOLL 1
#define CONFIG_EPOLL_EVENT_MAX 1

#define CONFIG_ENABLE_HOST_PAGE_LOCK 1

// Configuration: name suffix for SHM pools and semaphores
#define NV_NAME_SUFFIX_MAX_LEN 16

static const char shm_suffix[]                                               = "_shm";
static const char forward_suffix[]                                           = "_fw";
static const char ring_suffix[2][NV_NAME_SUFFIX_MAX_LEN]                     = {"_ring_s2m", "_ring_m2s"};
static const char mempool_suffix[NV_IPC_MEMPOOL_NUM][NV_NAME_SUFFIX_MAX_LEN] = {"_cpu_msg", "_cpu_data", "_cuda_data", "_gpu_data"};
#ifdef NVIPC_GDRCPY_ENABLE
static unsigned char firstMempoolsAlloc = 1;
#endif

typedef struct
{
    atomic_uint forward_started; // Flag indicates forwarding status
    atomic_uint msg_buf_count;   // MSG buffer count in fw_ring
    atomic_uint data_buf_count;  // DATA buffer count in fw_ring
    atomic_uint ipc_total;       // Total IPC message count to forward
    atomic_uint ipc_forwarded;   // Forwarded IPC message count since forwarding start
    atomic_uint ipc_lost;        // Lost IPC message count since forwarding start
    int32_t     queue_header[];
} forwarder_data_t;

typedef struct
{
    int32_t msg_id;
    int32_t cell_id;
    int32_t msg_len;
    int32_t data_len;
    int32_t data_index;
    int32_t data_pool;
} paket_info_t;

typedef struct
{
    int primary;

    // CUDA device ID for CUDA memory pool
    int cuda_device_id;

    int32_t ring_len;

    // The TX and RX rings
    nv_ipc_shm_t* shmpool;

    array_queue_t* tx_ring;
    array_queue_t* rx_ring;

    /* forward_enable: configured from yaml file
     * 0: disabled;
     * 1: enabled but doesn't start forwarding at initial;
     * -1: start forwarding at initial with count = 0;
     * Other positive number: start forwarding at initial with count = forward_enable.
     */
    int32_t forward_enable;

    int32_t           fw_max_msg_buf_count;
    int32_t           fw_max_data_buf_count;
    nv_ipc_shm_t*     fw_shmpool;
    forwarder_data_t* fw_data;
    array_queue_t*    fw_ring;
    sem_t*            fw_sem;

    paket_info_t* packet_infos;

    // Lock-less memory pool for MSG, CPU DATA and CUDA DATA
    nv_ipc_mempool_t* mempools[NV_IPC_MEMPOOL_NUM];

    // For synchronization between the two processes
    nv_ipc_efd_t*   ipc_efd;
    nv_ipc_sem_t*   ipc_sem;
    nv_ipc_epoll_t* ipc_epoll;

    // For debug
    nv_ipc_debug_t* ipc_debug;
    int is_ipc_dump;
} priv_data_t;

#define IPC_DUMPING_CHECK(priv_data)                                      \
    if (atomic_load(&priv_data->ipc_debug->shm_data->ipc_dumping))        \
    {                                                                     \
        NVLOGI(TAG, "%s line %d: ipc dumping, skip", __func__, __LINE__); \
        return -1;                                                        \
    }

#define IPC_DUMPING_CHECK_BLOCKING(priv_data)                             \
    if (atomic_load(&priv_data->ipc_debug->shm_data->ipc_dumping))        \
    {                                                                     \
        NVLOGI(TAG, "%s line %d: ipc dumping, wait", __func__, __LINE__); \
        sleep(100000);                                                    \
        return 0;                                                        \
    }

static inline priv_data_t* get_private_data(nv_ipc_t* ipc)
{
    return (priv_data_t*)((int8_t*)ipc + sizeof(nv_ipc_t));
}

// 1 - started; 0 - stopped
static int get_forward_started(priv_data_t* priv_data)
{
    if(priv_data->forward_enable)
    {
        return atomic_load(&priv_data->fw_data->forward_started);
    }
    else
    {
        return 0;
    }
}

static int shm_ipc_open(nv_ipc_t* ipc, const nv_ipc_config_shm_t* cfg)
{
    priv_data_t* priv_data    = get_private_data(ipc);
    priv_data->primary        = cfg->primary;
    priv_data->ring_len       = cfg->mempool_size[NV_IPC_MEMPOOL_CPU_MSG].pool_len;
    priv_data->cuda_device_id = cfg->cuda_device_id;

    // Check prefix string length
    size_t prefix_len = strnlen(cfg->prefix, NV_NAME_MAX_LEN);
    if(prefix_len <= 0 || prefix_len >= NV_NAME_MAX_LEN)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s error prefix string length %lu", __func__, prefix_len);
        return -1;
    }

    char name[NV_NAME_MAX_LEN + NV_NAME_SUFFIX_MAX_LEN];

    // Create a shared memory pool for the TX and RX queue
    nvlog_safe_strncpy(name, cfg->prefix, NV_NAME_MAX_LEN);
    strncat(name, shm_suffix, NV_NAME_SUFFIX_MAX_LEN);
    size_t ring_queue_size = ARRAY_QUEUE_HEADER_SIZE(priv_data->ring_len);
    size_t ring_objs_size  = sizeof(paket_info_t) * priv_data->ring_len;
    size_t shm_size        = ring_queue_size * 2 + ring_objs_size;
    if((priv_data->shmpool = nv_ipc_shm_open(priv_data->primary, name, shm_size)) == NULL)
    {
        return -1;
    }
    int8_t* shm_addr        = priv_data->shmpool->get_mapped_addr(priv_data->shmpool);
    priv_data->packet_infos = (paket_info_t*)(shm_addr + ring_queue_size * 2);

    // TX ring
    int alternative = priv_data->primary ? 1 : 0;
    nvlog_safe_strncpy(name, cfg->prefix, NV_NAME_MAX_LEN);
    strncat(name, ring_suffix[alternative], NV_NAME_SUFFIX_MAX_LEN);
    if((priv_data->tx_ring = array_queue_open(priv_data->primary, name, shm_addr + alternative * ring_queue_size, priv_data->ring_len)) == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: create tx_ring failed", __func__);
        return -1;
    }

    // RX ring
    nvlog_safe_strncpy(name, cfg->prefix, NV_NAME_MAX_LEN);
    strncat(name, ring_suffix[1 - alternative], NV_NAME_SUFFIX_MAX_LEN);
    if((priv_data->rx_ring = array_queue_open(priv_data->primary, name, shm_addr + (1 - alternative) * ring_queue_size, priv_data->ring_len)) == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: create rx_ring failed", __func__);
        return -1;
    }

    int pool_id;
    for(pool_id = 0; pool_id < NV_IPC_MEMPOOL_NUM; pool_id++)
    {
        NVLOGD(TAG, "%s: Primary=%d pool_id=%d buff_size=%d pool_size=%d ", __func__,cfg->primary,pool_id,cfg->mempool_size[pool_id].buf_size,cfg->mempool_size[pool_id].pool_len);
        if(cfg->mempool_size[pool_id].buf_size > 0 && cfg->mempool_size[pool_id].pool_len > 0)
        {
            nvlog_safe_strncpy(name, cfg->prefix, NV_NAME_MAX_LEN);
            strncat(name, mempool_suffix[pool_id], NV_NAME_SUFFIX_MAX_LEN);

            int cuda_device_id = -1;
            if((pool_id == NV_IPC_MEMPOOL_CUDA_DATA)||(pool_id == NV_IPC_MEMPOOL_GPU_DATA))
            {
                cuda_device_id = cfg->cuda_device_id;
            }

            if((priv_data->mempools[pool_id] = nv_ipc_mempool_open(cfg->primary, name, cfg->mempool_size[pool_id].buf_size, cfg->mempool_size[pool_id].pool_len, cuda_device_id)) == NULL)
            {
                NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: create memory pool %s failed", __func__, name);
                return -1;
            }

            if(CONFIG_ENABLE_HOST_PAGE_LOCK && cfg->cuda_device_id >= 0 && pool_id == NV_IPC_MEMPOOL_CPU_DATA)
            {
                size_t size = cfg->mempool_size[pool_id].buf_size * cfg->mempool_size[pool_id].pool_len;
                if(nv_ipc_page_lock(priv_data->mempools[pool_id]->get_addr(priv_data->mempools[pool_id], 0), size) < 0)
                {
                    return -1;
                }
            }
        }        
    }

    if(CONFIG_USE_EVENT_FD)
    {
        // Create instance of nv_ipc_efd_t
        // if((priv_data->ipc_efd = nv_ipc_efd_open(cfg->primary, cfg->prefix)) == NULL)
        // {
        //     return -1;
        // }

        // Do not create instance of nv_ipc_sem_t
        priv_data->ipc_sem = NULL;
    }
    else
    {
        // Do not create instance of nv_ipc_efd_t
        priv_data->ipc_efd = NULL;

        // Create instance of nv_ipc_sem_t
        if((priv_data->ipc_sem = nv_ipc_sem_open(cfg->primary, cfg->prefix)) == NULL)
        {
            return -1;
        }
    }

    priv_data->forward_enable = nv_ipc_app_config_get(NV_IPC_CFG_FORWARD_ENABLE);
    if(priv_data->forward_enable)
    {
        // Set max length for forwarder
        priv_data->fw_max_msg_buf_count = priv_data->ring_len / 3;
        if(cfg->mempool_size[NV_IPC_MEMPOOL_CPU_DATA].pool_len > 0)
        {
            priv_data->fw_max_data_buf_count = cfg->mempool_size[NV_IPC_MEMPOOL_CPU_DATA].pool_len / 3;
        }
        else
        {
            priv_data->fw_max_data_buf_count = cfg->mempool_size[NV_IPC_MEMPOOL_CUDA_DATA].pool_len / 3;
        }

        // Create a shared memory pool for the forwarder
        nvlog_safe_strncpy(name, cfg->prefix, NV_NAME_MAX_LEN);
        strncat(name, forward_suffix, NV_NAME_SUFFIX_MAX_LEN);
        shm_size = sizeof(forwarder_data_t) + ARRAY_QUEUE_HEADER_SIZE(priv_data->ring_len);
        if((priv_data->fw_shmpool = nv_ipc_shm_open(priv_data->primary, name, shm_size)) == NULL)
        {
            return -1;
        }
        priv_data->fw_data = priv_data->fw_shmpool->get_mapped_addr(priv_data->fw_shmpool);

        if(priv_data->primary)
        {
            atomic_store(&priv_data->fw_data->forward_started, 0);
            atomic_store(&priv_data->fw_data->msg_buf_count, 0);
            atomic_store(&priv_data->fw_data->data_buf_count, 0);
            atomic_store(&priv_data->fw_data->ipc_total, 0);
            atomic_store(&priv_data->fw_data->ipc_forwarded, 0);
            atomic_store(&priv_data->fw_data->ipc_lost, 0);

            if(priv_data->forward_enable > 1)
            {
                // Start with count = priv_data->forward_enable at initial
                nvipc_fw_start(ipc, priv_data->forward_enable);
            }
            else if(priv_data->forward_enable == -1)
            {
                // Start infinite forwarding at initial
                nvipc_fw_start(ipc, 0);
            }
        }

        // Forwarder ring
        nvlog_safe_strncpy(name, cfg->prefix, NV_NAME_MAX_LEN);
        strncat(name, forward_suffix, NV_NAME_SUFFIX_MAX_LEN);
        if((priv_data->fw_ring = array_queue_open(priv_data->primary, name, priv_data->fw_data->queue_header, priv_data->ring_len)) == NULL)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: create fw_ring failed", __func__);
            return -1;
        }

        sem_t* sem = sem_open(name, O_CREAT, 0600, 0);
        if(sem == SEM_FAILED)
        {
            NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "sem_open failed: name=%s", name);
            return -1;
        }

        if(priv_data->primary)
        {
            sem_init(sem, 1, 0); // Initiate the semaphore to be shared and set value to 0
            NVLOGI(TAG, "Semaphore create: %s", name);
        }
        else
        {
            NVLOGI(TAG, "Semaphore lookup: %s", name);
        }
        priv_data->fw_sem = sem;
    }

    if (priv_data->primary) {
        NVLOGC(TAG, "nvipc server initialized");
        if((priv_data->ipc_efd = nv_ipc_efd_open(cfg->primary, cfg->prefix)) == NULL)
        {
            return -1;
        }
    }

    if(CONFIG_USE_EVENT_FD && CONFIG_CREATE_EPOLL && !priv_data->is_ipc_dump)
    {
        // Create epoll wrapper for converting to blocking-wait API interface
        int fd = priv_data->ipc_efd->get_fd(priv_data->ipc_efd);
        if(fd < 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: invalid Event FD: %d", __func__, fd);
            return -1;
        }
        if((priv_data->ipc_epoll = ipc_epoll_create(CONFIG_EPOLL_EVENT_MAX, fd)) == NULL)
        {
            return -1;
        }
    }
    else
    {
        priv_data->ipc_epoll = NULL;
    }

    NVLOGC(TAG, "%s: forward_enable=%d fw_max_msg_buf_count=%d fw_max_data_buf_count=%d", __func__, priv_data->forward_enable, priv_data->fw_max_msg_buf_count, priv_data->fw_max_data_buf_count);

    return 0;
}

static int shm_ipc_close(nv_ipc_t* ipc)
{
    priv_data_t* priv_data = get_private_data(ipc);
    int          ret       = 0;

    IPC_DUMPING_CHECK(priv_data)

    if(priv_data->ipc_sem != NULL)
    {
        if(priv_data->ipc_sem->close(priv_data->ipc_sem) < 0)
        {
            ret = -1;
        }
    }

    if(priv_data->ipc_efd != NULL)
    {
        if(priv_data->ipc_efd->close(priv_data->ipc_efd) < 0)
        {
            ret = -1;
        }
    }

    // Close epoll FD if exist
    if(priv_data->ipc_epoll != NULL)
    {
        if(ipc_epoll_destroy(priv_data->ipc_epoll) < 0)
        {
            ret = -1;
        }
    }

    if(priv_data->tx_ring != NULL)
    {
        if(priv_data->tx_ring->close(priv_data->tx_ring) < 0)
        {
            ret = -1;
        }
    }

    if(priv_data->rx_ring != NULL)
    {
        if(priv_data->rx_ring->close(priv_data->rx_ring) < 0)
        {
            ret = -1;
        }
    }

    int i;
    for(i = 0; i < NV_IPC_MEMPOOL_NUM; i++)
    {
        if(priv_data->mempools[i] != NULL)
        {
            if(CONFIG_ENABLE_HOST_PAGE_LOCK && priv_data->cuda_device_id >= 0 && i == NV_IPC_MEMPOOL_CPU_DATA)
            {
                if(nv_ipc_page_unlock(priv_data->mempools[i]->get_addr(priv_data->mempools[i], 0)) < 0)
                {
                    ret = -1;
                }
            }
            if(priv_data->mempools[i]->close(priv_data->mempools[i]) < 0)
            {
                ret = -1;
            }
        }
    }

    if(priv_data->shmpool != NULL)
    {
        if(priv_data->shmpool->close(priv_data->shmpool) < 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: close shmpool failed", __func__);
            ret = -1;
        }
    }

    // Destroy the nv_ipc_t instance
    free(ipc);

    if(ret == 0)
    {
        NVLOGI(TAG, "%s: OK", __func__);
    }
    return ret;
}

// Send
static int ipc_sem_post(nv_ipc_t* ipc)
{
    priv_data_t* priv_data = get_private_data(ipc);
    IPC_DUMPING_CHECK(priv_data)

    priv_data->ipc_debug->post_hook(priv_data->ipc_debug);

    // Should have semaphore or event_fd created, otherwise returns fail.
    if(priv_data->ipc_sem != NULL)
    {
        return priv_data->ipc_sem->sem_post(priv_data->ipc_sem);
    }
    else if(priv_data->ipc_epoll != NULL)
    {
        return priv_data->ipc_efd->notify(priv_data->ipc_efd, 1);
    }
    else
    {
        return -1;
    }
}

// Receive: call sem_wait() and then get the SHM buffers from RX queue/list
static int ipc_sem_wait(nv_ipc_t* ipc)
{
    priv_data_t* priv_data = get_private_data(ipc);
    IPC_DUMPING_CHECK_BLOCKING(priv_data)

    int ret = -1;
    // Should have semaphore or event_fd created, otherwise return fail.
    if(priv_data->ipc_sem != NULL)
    {
        ret = priv_data->ipc_sem->sem_wait(priv_data->ipc_sem);
    }
    else if(priv_data->ipc_epoll != NULL)
    {
        ret = ipc_epoll_wait(priv_data->ipc_epoll);
        priv_data->ipc_efd->get_value(priv_data->ipc_efd);
    }

    priv_data->ipc_debug->wait_hook(priv_data->ipc_debug);

    return ret;
}

// Get SHM event FD or UDP socket FD for epoll
static int efd_get_fd(nv_ipc_t* ipc)
{
    priv_data_t* priv_data = get_private_data(ipc);
    IPC_DUMPING_CHECK(priv_data)

    if(priv_data->ipc_efd != NULL)
    {
        return priv_data->ipc_efd->get_fd(priv_data->ipc_efd);
    }
    else
    {
        // If no event_fd created, return fail.
        return -1;
    }
}

// Read and clear the receiving event
static int efd_get_value(nv_ipc_t* ipc)
{
    priv_data_t* priv_data = get_private_data(ipc);
    IPC_DUMPING_CHECK_BLOCKING(priv_data)

    priv_data->ipc_debug->wait_hook(priv_data->ipc_debug);

    if(priv_data->ipc_efd != NULL)
    {
        return priv_data->ipc_efd->get_value(priv_data->ipc_efd);
    }
    else
    {
        // If no event_fd created, return fail.
        return -1;
    }
}

// Notify sending event
static int efd_notify(nv_ipc_t* ipc, int value)
{
    priv_data_t* priv_data = get_private_data(ipc);
    IPC_DUMPING_CHECK(priv_data)

    priv_data->ipc_debug->post_hook(priv_data->ipc_debug);

    if(priv_data->ipc_efd != NULL)
    {
        return priv_data->ipc_efd->notify(priv_data->ipc_efd, value);
    }
    else
    {
        // If no event_fd created, return fail.
        return -1;
    }
}

static int dummy_allocate(nv_ipc_t* ipc, nv_ipc_msg_t* msg, uint32_t options)
{
    priv_data_t *priv_data = get_private_data(ipc);
    IPC_DUMPING_CHECK(priv_data)

    return 0;
}

static int dummy_release(nv_ipc_t* ipc, nv_ipc_msg_t* msg)
{
    priv_data_t *priv_data = get_private_data(ipc);
    IPC_DUMPING_CHECK(priv_data)

    return 0;
}

static int mempools_alloc(nv_ipc_t* ipc, nv_ipc_msg_t* msg, uint32_t options)
{
    if(ipc == NULL || msg == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: NULL pointer", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ipc);
    IPC_DUMPING_CHECK(priv_data)

    // Allocate MSG buffer
    nv_ipc_mempool_t* msgpool   = priv_data->mempools[NV_IPC_MEMPOOL_CPU_MSG];
    int32_t           msg_index = msgpool->alloc(msgpool);
    if(msg_index < 0)
    {
        NVLOGW(TAG, "%s: MSG pool is full", __func__);
        return -1;
    }
    msg->msg_buf = msgpool->get_addr(msgpool, msg_index);

    paket_info_t* info = priv_data->packet_infos + msg_index;

#ifdef NVIPC_GDRCPY_ENABLE
	if((firstMempoolsAlloc) && (priv_data->primary))
	{
		nv_ipc_mempool_t* gpudatapool = priv_data->mempools[NV_IPC_MEMPOOL_GPU_DATA];
		if(gpudatapool != NULL)
		{
            int ret = gpudatapool->poolReInit(gpudatapool);			
			firstMempoolsAlloc = 0; /* reset */
            if(ret == -1)
                return -1;
		}
	}
#endif
    // Allocate DATA buffer if has data
    if((msg->data_pool == NV_IPC_MEMPOOL_CPU_DATA) || 
       (msg->data_pool == NV_IPC_MEMPOOL_CUDA_DATA) || 
       (msg->data_pool == NV_IPC_MEMPOOL_GPU_DATA))
    {
        nv_ipc_mempool_t* datapool = priv_data->mempools[msg->data_pool];
        if(datapool == NULL)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: DATA pool %d is not configured", __func__, msg->data_pool);
            return -1;
        }

        int32_t data_index = datapool->alloc(datapool);
        if(data_index < 0)
        {
            // If MSG buffer allocation failed, free the MSG buffer and return error
            msgpool->free(msgpool, msg_index);
            NVLOGW(TAG, "%s: DATA pool %d is full", __func__, msg->data_pool);
            return -1;
        }
        msg->data_buf    = datapool->get_addr(datapool, data_index);        
        info->data_pool  = msg->data_pool;
        info->data_index = data_index;
    }
    else
    {
        // No DATA buffer
        msg->data_buf    = NULL;
        info->data_pool  = NV_IPC_MEMPOOL_CPU_MSG;
        info->data_index = -1;
    }

    priv_data->ipc_debug->alloc_hook(priv_data->ipc_debug, msg, msg_index);

    // NVLOGD(TAG, "%s: msg_buf=%p data_buf=%p", __func__, msg->msg_buf, msg->data_buf);
    return 0;
}

static int mempools_free(nv_ipc_t* ipc, nv_ipc_msg_t* msg)
{
    if(ipc == NULL || msg == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: NULL pointer", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ipc);
    IPC_DUMPING_CHECK(priv_data)

    int ret1 = 0, ret2 = 0;
    if(msg->msg_buf != NULL)
    {
        nv_ipc_mempool_t* msgpool   = priv_data->mempools[NV_IPC_MEMPOOL_CPU_MSG];
        int32_t           msg_index = msgpool->get_index(msgpool, msg->msg_buf);
        priv_data->ipc_debug->free_hook(priv_data->ipc_debug, msg, msg_index);

        if(get_forward_started(priv_data))
        {
            int fw_ret = -1;

            // Move to fw_ring if forwarded used MSG buffers and DATA buffers doesn't exceed the max allowed count
            if(atomic_load(&priv_data->fw_data->msg_buf_count) < priv_data->fw_max_msg_buf_count && (msg->data_buf == NULL || atomic_load(&priv_data->fw_data->data_buf_count) < priv_data->fw_max_data_buf_count))
            {
                uint32_t forwarded = atomic_fetch_add(&priv_data->fw_data->ipc_forwarded, 1);
                uint32_t total     = atomic_load(&priv_data->fw_data->ipc_total);
                if(total == 0 || total > forwarded)
                {
                    if((fw_ret = priv_data->fw_ring->enqueue(priv_data->fw_ring, msg_index)) < 0)
                    {
                        // The fw_ring size is large enough, normally should not run to here
                        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: fw_ring enqueue error", __func__);
                    }
                }
                else
                {
                    // Stop forwarding
                    atomic_fetch_sub(&priv_data->fw_data->ipc_forwarded, 1);
                    atomic_store(&priv_data->fw_data->forward_started, 0);
                }
            }
            else
            {
                // Forwarder buffers is full, skip, go to normal free
                atomic_fetch_add(&priv_data->fw_data->ipc_lost, 1);
            }

            if(fw_ret == 0)
            {
                atomic_fetch_add(&priv_data->fw_data->msg_buf_count, 1);
                if(msg->data_buf != NULL)
                {
                    atomic_fetch_add(&priv_data->fw_data->data_buf_count, 1);
                }

                NVLOGD(TAG, "Forwarder: enqueued msg_id=0x%02X", msg->msg_id);

                if(sem_post(priv_data->fw_sem) < 0)
                {
                    NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: sem_post error", __func__);
                }
                // Forwarded the message to fw_ring, do not free the buffers, skip
                return 0;
            }
        }

        ret1 = msgpool->free(msgpool, msg_index);
    }

    if(msg->data_buf != NULL)
    {
        if(msg->data_pool == NV_IPC_MEMPOOL_CPU_DATA || msg->data_pool == NV_IPC_MEMPOOL_CUDA_DATA || msg->data_pool == NV_IPC_MEMPOOL_GPU_DATA)
        {
            nv_ipc_mempool_t* datapool = priv_data->mempools[msg->data_pool];
            if(datapool == NULL)
            {
                NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: DATA pool %d is not configured", __func__, msg->data_pool);
                return -1;
            }

            int32_t data_index = datapool->get_index(datapool, msg->data_buf);
            ret2               = datapool->free(datapool, data_index);
        }
        else
        {
            ret2 = -1;
        }
    }

    return (ret1 == 0 && ret2 == 0) ? 0 : -1;
}

static int ring_tx_enqueue(nv_ipc_t* ipc, nv_ipc_msg_t* msg)
{
    if(ipc == NULL || msg == NULL)
    {
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ipc);
    IPC_DUMPING_CHECK(priv_data)

    nv_ipc_mempool_t* msgpool   = priv_data->mempools[NV_IPC_MEMPOOL_CPU_MSG];
    int32_t           msg_index = msgpool->get_index(msgpool, msg->msg_buf);
    if(msg_index < 0 || msg_index >= priv_data->ring_len)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: error index %d", __func__, msg_index);
        return -1;
    }

    priv_data->ipc_debug->send_hook(priv_data->ipc_debug, msg, msg_index);

    paket_info_t* info = priv_data->packet_infos + msg_index;
    info->msg_id       = msg->msg_id;
    info->cell_id      = msg->cell_id;
    info->msg_len      = msg->msg_len;
    info->data_len     = msg->data_len;

    NVLOGD(TAG, "send: cell_id=%d msg_id=0x%02X msg_len=%d data_len=%d data_pool=%d", msg->cell_id, msg->msg_id, msg->msg_len, msg->data_len, msg->data_pool);

    return priv_data->tx_ring->enqueue(priv_data->tx_ring, msg_index);
}

int nv_ipc_shm_rx_poll(nv_ipc_t* ipc)
{
    if(ipc == NULL)
    {
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ipc);
    IPC_DUMPING_CHECK(priv_data)

    return priv_data->rx_ring->get_count(priv_data->rx_ring);
}

static int ring_rx_enqueue(nv_ipc_t* ipc, nv_ipc_msg_t* msg)
{
    if(ipc == NULL || msg == NULL)
        return -1;

    priv_data_t* priv_data = get_private_data(ipc);
    IPC_DUMPING_CHECK(priv_data)

    nv_ipc_mempool_t* msgpool   = priv_data->mempools[NV_IPC_MEMPOOL_CPU_MSG];
    int32_t msg_index = msgpool->get_index(msgpool, msg->msg_buf);
    if(msg_index < 0 || msg_index >= priv_data->ring_len)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: error index %d", __func__, msg_index);
        return -1;
    }

    priv_data->ipc_debug->send_hook(priv_data->ipc_debug, msg, msg_index);

    paket_info_t* info = priv_data->packet_infos + msg_index;
    info->msg_id       = msg->msg_id;
    info->cell_id      = msg->cell_id;
    info->msg_len      = msg->msg_len;
    info->data_len     = msg->data_len;

    NVLOGD(TAG, "loopback send: cell_id=%d msg_id=0x%02X msg_len=%d data_len=%d data_pool=%d", msg->cell_id, msg->msg_id, msg->msg_len, msg->data_len, msg->data_pool);

    return priv_data->rx_ring->enqueue(priv_data->rx_ring, msg_index);
}

static int ring_rx_dequeue(nv_ipc_t* ipc, nv_ipc_msg_t* msg)
{
    if(ipc == NULL || msg == NULL)
    {
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ipc);
    IPC_DUMPING_CHECK(priv_data)

    int32_t msg_index = priv_data->rx_ring->dequeue(priv_data->rx_ring);
    if(msg_index < 0 || msg_index > priv_data->ring_len)
    {
        return -1;
    }
    else
    {
        nv_ipc_mempool_t* msgpool = priv_data->mempools[NV_IPC_MEMPOOL_CPU_MSG];
        msg->msg_buf              = msgpool->get_addr(msgpool, msg_index);

        paket_info_t* info = priv_data->packet_infos + msg_index;
        if(info->data_pool == NV_IPC_MEMPOOL_CPU_DATA || info->data_pool == NV_IPC_MEMPOOL_CUDA_DATA || info->data_pool == NV_IPC_MEMPOOL_GPU_DATA)
        {
            nv_ipc_mempool_t* datapool = priv_data->mempools[info->data_pool];
            msg->data_pool             = info->data_pool;
            msg->data_buf              = datapool->get_addr(datapool, info->data_index);            
        }
        else
        {
            msg->data_pool = NV_IPC_MEMPOOL_CPU_MSG;
            msg->data_buf  = NULL;
        }

        msg->msg_id   = info->msg_id;
        msg->cell_id  = info->cell_id;
        msg->msg_len  = info->msg_len;
        msg->data_len = info->data_len;

        priv_data->ipc_debug->recv_hook(priv_data->ipc_debug, msg, msg_index);

        NVLOGD(TAG, "recv: cell_id=%d msg_id=0x%02X msg_len=%d data_len=%d data_pool=%d", msg->cell_id, msg->msg_id, msg->msg_len, msg->data_len, msg->data_pool);
        //NVLOGD(TAG, "recv: cell_id=%d msg_id=0x%02X msg_len=%d data_len=%d data_pool=%d msg->data_buf=%p in_gpu=%d",
        //        msg->cell_id, msg->msg_id, msg->msg_len, msg->data_len, msg->data_pool, msg->data_buf, is_device_pointer(msg->data_buf));

        return 0;
    }
}

static int cuda_memcpy_to_host(nv_ipc_t* ipc, void* host, const void* device, size_t size)
{
    if(ipc == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }
    priv_data_t*      priv_data    = get_private_data(ipc);
    IPC_DUMPING_CHECK(priv_data)

    nv_ipc_mempool_t* cuda_mempool = priv_data->mempools[NV_IPC_MEMPOOL_CUDA_DATA];
    if(cuda_mempool == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: CUDA memory pool not exist", __func__);
        return -1;
    }
    else
    {
        return cuda_mempool->memcpy_to_host(cuda_mempool, host, device, size);
    }
}

static int cuda_memcpy_to_device(nv_ipc_t* ipc, void* device, const void* host, size_t size)
{
    if(ipc == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }
    priv_data_t*      priv_data    = get_private_data(ipc);
    IPC_DUMPING_CHECK(priv_data)

    nv_ipc_mempool_t* cuda_mempool = priv_data->mempools[NV_IPC_MEMPOOL_CUDA_DATA];
    if(cuda_mempool == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: CUDA memory pool not exist", __func__);
        return -1;
    }
    else
    {
        return cuda_mempool->memcpy_to_device(cuda_mempool, device, host, size);
    }
}

static int gpudata_memcpy_to_host(nv_ipc_t* ipc, void* host, const void* device, size_t size)
{
    if(ipc == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }
    priv_data_t*      priv_data    = get_private_data(ipc);
    IPC_DUMPING_CHECK(priv_data)

    nv_ipc_mempool_t* gpudata_mempool = priv_data->mempools[NV_IPC_MEMPOOL_GPU_DATA];
    if(gpudata_mempool == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: gpudata_mempool memory pool not exist", __func__);
        return -1;
    }
    else
    {
        return gpudata_mempool->memcpy_to_host(gpudata_mempool, host, device, size);
    }
}

static int gpudata_memcpy_to_device(nv_ipc_t* ipc, void* device, const void* host, size_t size)
{
    if(ipc == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }
    priv_data_t*      priv_data    = get_private_data(ipc);
    IPC_DUMPING_CHECK(priv_data)

    nv_ipc_mempool_t* gpudata_mempool = priv_data->mempools[NV_IPC_MEMPOOL_GPU_DATA];
    if(gpudata_mempool == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: gpudata_mempool memory pool not exist", __func__);
        return -1;
    }
    else
    {
        return gpudata_mempool->memcpy_to_device(gpudata_mempool, device, host, size);
    }
}



static int debug_get_msg(priv_data_t* priv_data, nv_ipc_msg_t* msg, int msg_index)
{
    nv_ipc_mempool_t* msgpool = priv_data->mempools[NV_IPC_MEMPOOL_CPU_MSG];
    if((msg->msg_buf = msgpool->get_addr(msgpool, msg_index)) == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: invalid msg_index=%d", __func__, msg_index);
        return -1;
    }

    paket_info_t* info = priv_data->packet_infos + msg_index;
    msg->msg_id        = info->msg_id;
    msg->cell_id       = info->cell_id;
    msg->msg_len       = info->msg_len;
    msg->data_len      = info->data_len;
    msg->data_pool     = info->data_pool;

    if(msg->data_pool > 0)
    {
        nv_ipc_mempool_t* datapool = priv_data->mempools[msg->data_pool];
        msg->data_buf              = datapool->get_addr(datapool, info->data_index);
    }
    return 0;
}

static int debug_dump_queue(priv_data_t* priv_data, array_queue_t* queue, int32_t* mempool_status, int32_t mempool_size, const char* info)
{
    char*   queue_name = queue->get_name(queue);
    int32_t count      = queue->get_count(queue);
    int32_t max_length = queue->get_max_length(queue);
    int32_t base = -1, counter = 0;
    NVLOGC(TAG, "%s: count=%d max_length=%d", info, count, max_length);

    int32_t msg_index;
    while((msg_index = queue->get_next(queue, base)) >= 0 && counter < count)
    {
        nv_ipc_msg_t msg;
        if(debug_get_msg(priv_data, &msg, msg_index) == 0)
        {
            nv_ipc_dump_msg(priv_data->ipc_debug, &msg, msg_index, info);
        }

        if(msg_index < mempool_size)
        {
            *(mempool_status + msg_index) |= 0x1;
        }
        else
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: msg_index=%d < mempool_size=%d - error", info, msg_index, mempool_size);
        }
        base = msg_index;
        counter++;
    }
    return 0;
}

static int debug_dump_mempools(priv_data_t* priv_data, int32_t* mempool_status, int32_t mempool_size, const char* info)
{
    nv_ipc_mempool_t* mempool = priv_data->mempools[NV_IPC_MEMPOOL_CPU_MSG];
    array_queue_t*    queue   = mempool->get_free_queue(mempool);

    char*   queue_name = queue->get_name(queue);
    int32_t count      = queue->get_count(queue);
    int32_t max_length = queue->get_max_length(queue);
    int32_t base = -1, counter = 0;

    NVLOGC(TAG, "%s: mempool_size=%d free_count=%d max_length=%d", info, mempool_size, count, max_length);

    int32_t msg_index;
    while((msg_index = queue->get_next(queue, base)) >= 0 && counter < count)
    {
        if(msg_index < mempool_size)
        {
            *(mempool_status + msg_index) |= 0x8;
        }
        else
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: msg_index=%d < mempool_size=%d - error", info, msg_index, mempool_size);
        }
        base = msg_index;
        counter++;
    }

    for(msg_index = 0; msg_index < mempool_size; msg_index++)
    {
        if(*(mempool_status + msg_index) == 0)
        {
            nv_ipc_msg_t msg;
            if(debug_get_msg(priv_data, &msg, msg_index) == 0)
            {
                nv_ipc_dump_msg(priv_data->ipc_debug, &msg, msg_index, info);
            }
        }
    }

    return 0;
}

int64_t nv_ipc_get_ts_send(nv_ipc_t* ipc, nv_ipc_msg_t* msg)
{
    if(ipc == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }
    priv_data_t* priv_data = get_private_data(ipc);

    nv_ipc_mempool_t* msgpool   = priv_data->mempools[NV_IPC_MEMPOOL_CPU_MSG];
    int32_t           msg_index = msgpool->get_index(msgpool, msg->msg_buf);
    if(msg_index < 0 || msg_index >= priv_data->ring_len)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: error index %d", __func__, msg_index);
        return -1;
    }

    return nv_ipc_get_buffer_ts_send(priv_data->ipc_debug, msg_index);
}

int shm_ipc_dump(nv_ipc_t* ipc)
{
    if(ipc == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }
    priv_data_t* priv_data = get_private_data(ipc);
    atomic_fetch_add(&priv_data->ipc_debug->shm_data->ipc_dumping, 1);

    nv_ipc_mempool_t* mempool      = priv_data->mempools[NV_IPC_MEMPOOL_CPU_MSG];
    int32_t           mempool_size = mempool->get_pool_len(mempool);

    // Status of each MSG buffer: 0 - free; 0x1 - in DL queue; 0x2 - in UL queue; 0x4 - not released
    int32_t* mempool_status = malloc(mempool_size * sizeof(int32_t));
    if (mempool_status == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: malloc failed", __func__);
        return -1;
    }

    memset(mempool_status, 0, mempool_size * sizeof(int32_t));

    array_queue_t* dl_ring = priv_data->primary ? priv_data->rx_ring : priv_data->tx_ring;
    array_queue_t* ul_ring = priv_data->primary ? priv_data->tx_ring : priv_data->rx_ring;

    NVLOGC(TAG, "========== Dump DL queue: DL not received ======================");
    debug_dump_queue(priv_data, dl_ring, mempool_status, mempool_size, "DL");
    NVLOGC(TAG, "========== Dump UL queue: UL not received ======================");
    debug_dump_queue(priv_data, ul_ring, mempool_status, mempool_size, "UL");

    if(priv_data->forward_enable)
    {
        NVLOGC(TAG, "========== Dump FW queue: FW not dequeued ======================");
        uint32_t started   = atomic_load(&priv_data->fw_data->forward_started);
        uint32_t forwarded = atomic_load(&priv_data->fw_data->ipc_forwarded);
        uint32_t lost      = atomic_load(&priv_data->fw_data->ipc_lost);
        uint32_t total     = atomic_load(&priv_data->fw_data->ipc_total);
        uint32_t msg_buf   = atomic_load(&priv_data->fw_data->msg_buf_count);
        uint32_t data_buf  = atomic_load(&priv_data->fw_data->data_buf_count);
        NVLOGC(TAG, "FW status: started=%u forwarded=%u total=%u lost=%u msg_buf=%u data_buf=%u", started, forwarded, total, lost, msg_buf, data_buf);

        debug_dump_queue(priv_data, priv_data->fw_ring, mempool_status, mempool_size, "FW");
    }

    NVLOGC(TAG, "========== Dump memory pool: buffers allocated but not send OR received but not released ==================");
    debug_dump_mempools(priv_data, mempool_status, mempool_size, "MEMPOOL");
    NVLOGC(TAG, "========== Dump finished =========================");

    free(mempool_status);
    return 0;
}

int nvipc_fw_start(nv_ipc_t* ipc, uint32_t count)
{
    if(ipc == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: NULL pointer", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ipc);
    if(priv_data->fw_data != NULL)
    {
        if(atomic_load(&priv_data->fw_data->forward_started) != 0)
        {
            NVLOGW(TAG, "%s: already started", __func__);
            return -1;
        }
        else
        {
            atomic_store(&priv_data->fw_data->ipc_total, count);
            atomic_store(&priv_data->fw_data->ipc_forwarded, 0);
            atomic_store(&priv_data->fw_data->ipc_lost, 0);
            atomic_store(&priv_data->fw_data->forward_started, 1);
            return 0;
        }
    }
    else
    {
        return -1;
    }
}

int nvipc_fw_stop(nv_ipc_t* ipc)
{
    if(ipc == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: NULL pointer", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ipc);
    if(priv_data->fw_data != NULL)
    {
        atomic_store(&priv_data->fw_data->forward_started, 0);
        return 0;
    }
    else
    {
        return -1;
    }
}

uint32_t nvipc_fw_get_lost(nv_ipc_t* ipc)
{
    if(ipc == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: NULL pointer", __func__);
        return 0;
    }

    priv_data_t* priv_data = get_private_data(ipc);
    if(priv_data->fw_data != NULL)
    {
        return atomic_load(&priv_data->fw_data->ipc_lost);
    }
    else
    {
        return 0;
    }
}

int nvipc_fw_sem_timedwait(nv_ipc_t* ipc, const struct timespec* ts_abs)
{
    if(ipc == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: NULL pointer", __func__);
        return 0;
    }

    priv_data_t* priv_data = get_private_data(ipc);
    if(priv_data->fw_sem == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: semaphore doesn't exist", __func__);
        return -1;
    }

    if(ts_abs == NULL)
    {
        return sem_wait(priv_data->fw_sem);
    }
    else
    {
        return sem_timedwait(priv_data->fw_sem, ts_abs);
    }
}

int nvipc_fw_sem_wait(nv_ipc_t* ipc)
{
    return nvipc_fw_sem_timedwait(ipc, NULL);
}

int nvipc_fw_dequeue(nv_ipc_t* ipc, nv_ipc_msg_t* msg)
{
    priv_data_t* priv_data = get_private_data(ipc);
    int          msg_index = priv_data->fw_ring->dequeue(priv_data->fw_ring);
    if(msg_index >= 0)
    {
        int ret = debug_get_msg(priv_data, msg, msg_index);
        priv_data->ipc_debug->fw_deq_hook(priv_data->ipc_debug, msg, msg_index);
        return ret;
    }
    else
    {
        // No more message in the fw_ring, skip
        return -1;
    }
}

int nvipc_fw_free(nv_ipc_t* ipc, nv_ipc_msg_t* msg)
{
    if(ipc == NULL || msg == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: NULL pointer", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ipc);

    int ret1 = 0, ret2 = 0;
    if(msg->msg_buf != NULL)
    {
        nv_ipc_mempool_t* msgpool   = priv_data->mempools[NV_IPC_MEMPOOL_CPU_MSG];
        int32_t           msg_index = msgpool->get_index(msgpool, msg->msg_buf);
        priv_data->ipc_debug->fw_free_hook(priv_data->ipc_debug, msg, msg_index);
        ret1 = msgpool->free(msgpool, msg_index);
        if(ret1 == 0)
        {
            atomic_fetch_sub(&priv_data->fw_data->msg_buf_count, 1);
        }
    }

    if(msg->data_buf != NULL)
    {
        if(msg->data_pool == NV_IPC_MEMPOOL_CPU_DATA || msg->data_pool == NV_IPC_MEMPOOL_CUDA_DATA)
        {
            nv_ipc_mempool_t* datapool = priv_data->mempools[msg->data_pool];
            if(datapool == NULL)
            {
                NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: DATA pool %d is not configured", __func__, msg->data_pool);
                return -1;
            }

            int32_t data_index = datapool->get_index(datapool, msg->data_buf);
            ret2               = datapool->free(datapool, data_index);
            if(ret2 == 0)
            {
                atomic_fetch_sub(&priv_data->fw_data->data_buf_count, 1);
            }
        }
        else
        {
            ret2 = -1;
        }
    }

    return (ret1 == 0 && ret2 == 0) ? 0 : -1;
}

int nv_ipc_shm_send_loopback(nv_ipc_t* ipc, nv_ipc_msg_t* msg)
{
    return ring_rx_enqueue(ipc, msg);
}

nv_ipc_t* create_shm_nv_ipc_interface(const nv_ipc_config_t* cfg)
{
    if(cfg == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: configuration is NULL", __func__);
        return NULL;
    }

    int       size = sizeof(nv_ipc_t) + sizeof(priv_data_t);
    nv_ipc_t* ipc  = malloc(size);
    if(ipc == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: memory malloc failed", __func__);
        return NULL;
    }

    memset(ipc, 0, size);

    priv_data_t* priv_data = get_private_data(ipc);

    ipc->ipc_destroy = shm_ipc_close;

    ipc->tx_allocate = mempools_alloc;
    ipc->rx_release  = mempools_free;

    ipc->tx_release  = mempools_free;
    ipc->rx_allocate = mempools_alloc;

    ipc->tx_send_msg = ring_tx_enqueue;
    ipc->rx_recv_msg = ring_rx_dequeue;

    // Semaphore synchronization
    ipc->tx_tti_sem_post = ipc_sem_post;
    ipc->rx_tti_sem_wait = ipc_sem_wait;

    // Event FD synchronization
    ipc->get_fd    = efd_get_fd;
    ipc->get_value = efd_get_value;
    ipc->notify    = efd_notify;

    ipc->cuda_memcpy_to_host   = cuda_memcpy_to_host;
    ipc->cuda_memcpy_to_device = cuda_memcpy_to_device;

	ipc->gdr_memcpy_to_host   = gpudata_memcpy_to_host;
	ipc->gdr_memcpy_to_device = gpudata_memcpy_to_device; 

    priv_data->is_ipc_dump = cfg->module_type == NV_IPC_MODULE_IPC_DUMP ? 1 : 0;

    // Create instance of nv_ipc_efd_t
    const nv_ipc_config_shm_t* shm_cfg = &cfg->transport_config.shm;
    if (shm_cfg->primary == 0) {
        if(!priv_data->is_ipc_dump && (priv_data->ipc_efd = nv_ipc_efd_open(shm_cfg->primary, shm_cfg->prefix)) == NULL)
        {
            free(ipc);
            return NULL;
        }

        NVLOGC(TAG, "Start initialize nvipc client");
        nv_ipc_app_config_shmpool_open(0, shm_cfg->prefix);
    }

    if((priv_data->ipc_debug = nv_ipc_debug_open(ipc, cfg)) == NULL)
    {
        free(ipc);
        return NULL;
    }

    int ret = 0;
    if(shm_ipc_open(ipc, shm_cfg) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: Failed", __func__);
        ret = -1;
    }

    if (ret < 0) {
        shm_ipc_close(ipc);
        return NULL;
    } else {
        NVLOGC(TAG, "%s: OK", __func__);
        return ipc;
    }
}
