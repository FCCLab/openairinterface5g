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
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>

#include "array_queue.h"
#include "nv_ipc_ring.h"
#include "nv_ipc_shm.h"
#include "nv_ipc_mempool.h"
#include "nv_ipc_utils.h"

#define TAG (NVLOG_TAG_BASE_NVIPC + 14) //"NVIPC.RING"

#define NV_NAME_MAX_LEN 32
#define NV_NAME_SUFFIX_MAX_LEN 16

#define NAME_SUFFIX_RING_FIFO "_fifo"
#define NAME_SUFFIX_RING_OBJS "_objs"

typedef struct
{
    int ring_type;

    int32_t buf_size;
    int32_t ring_len;

    nv_ipc_shm_t* shmpool; // SHM block for ring_header_t

    array_queue_t* ring_queue; // The ring queue

    array_queue_t* free_queue; // The free object buffers queue

    int8_t* header;
    int8_t* body;

    char* name;
} priv_data_t;

static inline priv_data_t* get_private_data(nv_ipc_ring_t* ring)
{
    return (priv_data_t*)((int8_t*)ring + sizeof(nv_ipc_ring_t));
}


static int32_t ipc_ring_alloc(nv_ipc_ring_t* ring)
{
    priv_data_t* priv_data = get_private_data(ring);
    return priv_data->free_queue->dequeue(priv_data->free_queue);
}

static int ipc_ring_free(nv_ipc_ring_t* ring, int32_t index)
{
    priv_data_t* priv_data = get_private_data(ring);
    if(index < 0 || index >= priv_data->ring_len)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: invalid index %d", __func__, index);
        return -1;
    }
    else
    {
        return priv_data->free_queue->enqueue(priv_data->free_queue, index);
    }
}

static int ipc_ring_get_index(nv_ipc_ring_t* ring, void* buf)
{
    if(ring == NULL || buf == NULL)
    {
        return -1;
    }
    else
    {
        priv_data_t* priv_data = get_private_data(ring);
        return ((int8_t*)buf - priv_data->body) / priv_data->buf_size;
    }
}

static void* ipc_ring_get_addr(nv_ipc_ring_t* ring, int32_t index)
{
    if(ring == NULL || index < 0)
    {
        return NULL;
    }
    else
    {
        priv_data_t* priv_data = get_private_data(ring);
        return priv_data->body + (size_t)priv_data->buf_size * index;
    }
}

static int ipc_ring_get_buf_size(nv_ipc_ring_t* ring)
{
    return ring == NULL ? -1 : get_private_data(ring)->buf_size;
}

static int ipc_ring_get_ring_len(nv_ipc_ring_t* ring)
{
    return ring == NULL ? -1 : get_private_data(ring)->ring_len;
}

static int ipc_ring_enqueue_by_index(nv_ipc_ring_t* ring, int32_t index)
{
    if(ring == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: invalid parameters", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ring);

    if(index < 0 || index >= priv_data->ring_len)
    {
        NVLOGW(TAG, "%s: invalid buffer index: %d", __func__, index);
        return -1;
    }

    // Enqueue
    if(priv_data->ring_queue->enqueue(priv_data->ring_queue, index) < 0)
    {
        // Normally should not run to here
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: enqueue failed", __func__);
        return -1;
    }

    NVLOGD(TAG, "enqueued=%d", index);
    return 0;
}

static int32_t ipc_ring_dequeue_by_index(nv_ipc_ring_t* ring)
{
    if(ring == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: invalid parameters", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ring);

    // Dequeue a ring object buffer
    int32_t index = priv_data->ring_queue->dequeue(priv_data->ring_queue);
    if(index < 0 || index >= priv_data->ring_len)
    {
        NVLOGV(TAG, "%s: ring is empty: %d", __func__, index);
        return -1;
    }
    NVLOGD(TAG, "dequeued=%d", index);

    return index;
}

//===============================

static int ipc_ring_enqueue(nv_ipc_ring_t* ring, void* obj)
{
    if(ring == NULL || obj == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: invalid parameters", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ring);

    // Allocate a ring object buffer
    int32_t index = priv_data->free_queue->dequeue(priv_data->free_queue);
    if(index < 0 || index >= priv_data->ring_len)
    {
        NVLOGW(TAG, "%s: ring is full: %d", __func__, index);
        return -1;
    }

    // Copy the data
    memcpy(priv_data->body + priv_data->buf_size * index, obj, priv_data->buf_size);

    // Enqueue
    if(priv_data->ring_queue->enqueue(priv_data->ring_queue, index) < 0)
    {
        // Normally should not run to here
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: enqueue failed", __func__);

        // If dequeue failed
        if(priv_data->free_queue->enqueue(priv_data->free_queue, index) < 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: recycle object buffer failed", __func__);
        }
        return -1;
    }

    NVLOGD(TAG, "enqueued=%d", index);
    return 0;
}

static int ipc_ring_dequeue(nv_ipc_ring_t* ring, void* obj)
{
    if(ring == NULL || obj == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: invalid parameters", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ring);

    // Dequeue a ring object buffer
    int32_t index = priv_data->ring_queue->dequeue(priv_data->ring_queue);
    if(index < 0 || index >= priv_data->ring_len)
    {
        NVLOGV(TAG, "%s: ring is empty: %d", __func__, index);
        return -1;
    }

    // Copy the data
    memcpy(obj, priv_data->body + priv_data->buf_size * index, priv_data->buf_size);

    // Recycle the object buffer
    if(priv_data->free_queue->enqueue(priv_data->free_queue, index) < 0)
    {
        // Normally should not run to here
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: recycle object buffer failed", __func__);
    }

    NVLOGD(TAG, "dequeued=%d", index);

    return 0;
}

int ipc_ring_get_count(nv_ipc_ring_t *ring) {
    priv_data_t *priv_data = get_private_data(ring);
    return ring == NULL ? -1 : priv_data->ring_queue->get_count(priv_data->ring_queue);
}

int ipc_ring_get_free_count(nv_ipc_ring_t *ring) {
    priv_data_t *priv_data = get_private_data(ring);
    return ring == NULL ? -1 : priv_data->free_queue->get_count(priv_data->free_queue);
}

static int ipc_ring_close(nv_ipc_ring_t* ring)
{
    if(ring == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ring);
    int          ret       = 0;

    if(priv_data->free_queue != NULL)
    {
        if(priv_data->free_queue->close(priv_data->free_queue) < 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: close free_queue failed", __func__);
            ret = -1;
        }
    }

    if(priv_data->ring_queue != NULL)
    {
        if(priv_data->ring_queue->close(priv_data->ring_queue) < 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: close ring_queue failed", __func__);
            ret = -1;
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
    else if (priv_data->header != NULL)
    {
        free(priv_data->header);
    }

    if(priv_data->name != NULL)
    {
        free(priv_data->name);
    }

    free(ring);

    if(ret == 0)
    {
        NVLOGI(TAG, "%s: OK", __func__);
    }
    return ret;
}

static int ipc_ring_open(priv_data_t* priv_data, const char* name)
{
    // Create a shared memory pool for header
    size_t header_size = ARRAY_QUEUE_HEADER_SIZE(priv_data->ring_len);
    size_t body_size   = (size_t)priv_data->buf_size * priv_data->ring_len;
    size_t shm_size    = header_size * 2 + body_size;

    // Create a memory pool for store the queues and objects
    int primary;
    switch (priv_data->ring_type) {
    case RING_TYPE_SHM_SECONDARY:
    case RING_TYPE_SHM_PRIMARY:
        // Allocate shared memory
        primary = priv_data->ring_type;
        if ((priv_data->shmpool = nv_ipc_shm_open(primary, name, shm_size)) == NULL) {
            return -1;
        }
        priv_data->header = priv_data->shmpool->get_mapped_addr(priv_data->shmpool);
        break;
    case RING_TYPE_APP_INTERNAL:
        // Allocate normal memory for application internal use
        primary = 1;
        priv_data->shmpool = NULL;
        if ((priv_data->header = malloc(shm_size)) == NULL) {
            return -1;
        }
        break;
    default:
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: error ring_type: %d", __func__,
                priv_data->ring_type);
        return -1;
    }

    // Create an array queue as the FIFO ring queue
    char queue_name[NV_NAME_MAX_LEN + NV_NAME_SUFFIX_MAX_LEN];
    nvlog_safe_strncpy(queue_name, name, NV_NAME_MAX_LEN + NV_NAME_SUFFIX_MAX_LEN);
    strncat(queue_name, NAME_SUFFIX_RING_FIFO, NV_NAME_SUFFIX_MAX_LEN);
    if((priv_data->ring_queue = array_queue_open(primary, queue_name, priv_data->header, priv_data->ring_len)) == NULL)
    {
        return -1;
    }

    // Create an array queue as the object buffer manager queue
    nvlog_safe_strncpy(queue_name, name, NV_NAME_MAX_LEN + NV_NAME_SUFFIX_MAX_LEN);
    strncat(queue_name, NAME_SUFFIX_RING_OBJS, NV_NAME_SUFFIX_MAX_LEN);
    if((priv_data->free_queue = array_queue_open(primary, queue_name, priv_data->header + header_size, priv_data->ring_len)) == NULL)
    {
        return -1;
    }

    // Enqueue the object buffer to the free_queue
    if(primary)
    {
        int i;
        for(i = 0; i < priv_data->ring_len; i++)
        {
            if(priv_data->free_queue->enqueue(priv_data->free_queue, i) < 0)
            {
                NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: enqueue initial value %d failed", __func__, i);
                return -1;
            }
        }
    }

    // Memory pool body
    priv_data->body = priv_data->header + header_size * 2;

    // For debug log
    priv_data->name = strdup(name);

    return 0;
}

nv_ipc_ring_t* nv_ipc_ring_open(ring_type_t type, const char* name, int32_t ring_len, int32_t buf_size)
{
    if(name == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: invalid parameter", __func__);
        return NULL;
    }

    int            size = sizeof(nv_ipc_ring_t) + sizeof(priv_data_t);
    nv_ipc_ring_t* ring = malloc(size);
    if(ring == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: memory malloc failed", __func__);
        return NULL;
    }
    memset(ring, 0, size);

    priv_data_t* priv_data = get_private_data(ring);
    priv_data->ring_type   = type;
    priv_data->buf_size    = buf_size;
    priv_data->ring_len    = ring_len;

    ring->alloc = ipc_ring_alloc;
    ring->free = ipc_ring_free;
    ring->get_index = ipc_ring_get_index;
    ring->get_addr = ipc_ring_get_addr;
    ring->enqueue_by_index = ipc_ring_enqueue_by_index;
    ring->dequeue_by_index = ipc_ring_dequeue_by_index;
    ring->get_buf_size = ipc_ring_get_buf_size;

    ring->enqueue = ipc_ring_enqueue;
    ring->dequeue = ipc_ring_dequeue;
    ring->close   = ipc_ring_close;

    ring->get_count = ipc_ring_get_count;
    ring->get_free_count = ipc_ring_get_free_count;

    if(ipc_ring_open(priv_data, name) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: name=%s Failed", __func__, name);
        ipc_ring_close(ring);
        return NULL;
    }
    else
    {
        NVLOGC(TAG, "%s: name=%s ring_len=%d buf_size=%d OK", __func__, name, ring_len, buf_size);
        return ring;
    }
}
