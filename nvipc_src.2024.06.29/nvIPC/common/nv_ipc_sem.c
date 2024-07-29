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
#include <semaphore.h>

#include "nv_ipc_sem.h"
#include "nv_ipc_utils.h"

#define TAG (NVLOG_TAG_BASE_NVIPC + 15) //"NVIPC.SEM"

#define DBG_SEM_INIT 1

#define NV_NAME_SUFFIX_MAX_LEN 16
#define NAME_SUFFIX_SEM_M2S "_sem_m2s"
#define NAME_SUFFIX_SEM_S2M "_sem_s2m"

typedef struct
{
    int primary;

    // Each cell has 1 TX semaphore and 1 RX semaphore
    sem_t* sem_tx;
    sem_t* sem_rx;
} priv_data_t;

static inline priv_data_t* get_private_data(nv_ipc_sem_t* ipc_sem)
{
    return (priv_data_t*)((int8_t*)ipc_sem + sizeof(nv_ipc_sem_t));
}

static sem_t* semaphore_create(const char* name, int primary)
{
    // RX semaphore
    NVLOGD(TAG, "%s: name=%s, primary=%d", __func__, name, primary);
    sem_t* sem = sem_open(name, O_CREAT, 0600, 0);
    if(sem == SEM_FAILED)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "sem_open failed: name = %s, err = %d:%s", name, errno, strerror(errno));
        return NULL;
    }

    if(primary && DBG_SEM_INIT)
    {
        NVLOGI(TAG, "Create semaphore %s and set value to 0", name);
        sem_init(sem, 1, 0); // Initiate the semaphore to be shared and set value to 0
    }
    else
    {
        NVLOGI(TAG, "Lookup semaphore %s", name);
    }
    return sem;
}

static int ipc_sem_open(nv_ipc_sem_t* ipc_sem, const char* prefix)
{
    priv_data_t* priv_data = get_private_data(ipc_sem);

    char name[NV_SEM_NAME_MAX_LEN + NV_NAME_SUFFIX_MAX_LEN];

    // TX semaphore
    nvlog_safe_strncpy(name, prefix, NV_SEM_NAME_MAX_LEN);
    if(priv_data->primary)
    {
        strncat(name, NAME_SUFFIX_SEM_M2S, NV_NAME_SUFFIX_MAX_LEN);
    }
    else
    {
        strncat(name, NAME_SUFFIX_SEM_S2M, NV_NAME_SUFFIX_MAX_LEN);
    }
    if((priv_data->sem_tx = semaphore_create(name, priv_data->primary)) == NULL)
    {
        return -1;
    }

    // RX semaphore
    nvlog_safe_strncpy(name, prefix, NV_SEM_NAME_MAX_LEN);
    if(priv_data->primary)
    {
        strncat(name, NAME_SUFFIX_SEM_S2M, NV_NAME_SUFFIX_MAX_LEN);
    }
    else
    {
        strncat(name, NAME_SUFFIX_SEM_M2S, NV_NAME_SUFFIX_MAX_LEN);
    }
    if((priv_data->sem_rx = semaphore_create(name, priv_data->primary)) == NULL)
    {
        return -1;
    }

    return 0;
}

static int ipc_sem_trywait(nv_ipc_sem_t* ipc_sem)
{
    if(ipc_sem == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ipc_sem);
    return sem_trywait(priv_data->sem_rx);
}

static int ipc_sem_timedwait(nv_ipc_sem_t* ipc_sem, struct timespec* ts)
{
    if(ipc_sem == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ipc_sem);
    return sem_timedwait(priv_data->sem_rx, ts);
}

static int ipc_sem_close(nv_ipc_sem_t* ipc_sem)
{
    if(ipc_sem == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }

    // Destroy the nv_ipc_sem_t instance
    priv_data_t* priv_data = get_private_data(ipc_sem);
    int          ret       = 0;

    if(priv_data->sem_tx != NULL)
    {
        if(sem_close(priv_data->sem_tx) < 0)
        {
            NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: sem_close sem_tx failed", __func__);
            ret = -1;
        }
    }

    if(priv_data->sem_rx != NULL)
    {
        if(sem_close(priv_data->sem_rx) < 0)
        {
            NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: sem_close sem_rx failed", __func__);
            ret = -1;
        }
    }
    free(ipc_sem);
    return ret;
}

static int ipc_sem_post(nv_ipc_sem_t* ipc_sem)
{
    if(ipc_sem == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ipc_sem);
    return sem_post(priv_data->sem_tx);
}

static int ipc_sem_wait(nv_ipc_sem_t* ipc_sem)
{
    if(ipc_sem == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ipc_sem);
    return sem_wait(priv_data->sem_rx);
}

static int ipc_sem_get_value(nv_ipc_sem_t* ipc_sem, int* value)
{
    if(ipc_sem == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(ipc_sem);
    return sem_getvalue(priv_data->sem_rx, value);
}

nv_ipc_sem_t* nv_ipc_sem_open(int primary, const char* prefix)
{
    if(prefix == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: configuration is NULL", __func__);
        return NULL;
    }

    int           size    = sizeof(nv_ipc_sem_t) + sizeof(priv_data_t);
    nv_ipc_sem_t* ipc_sem = malloc(size);
    if(ipc_sem == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: memory malloc failed", __func__);
        return NULL;
    }

    memset(ipc_sem, 0, size);

    priv_data_t* priv_data = get_private_data(ipc_sem);
    priv_data->primary     = primary;

    ipc_sem->sem_post  = ipc_sem_post;
    ipc_sem->sem_wait  = ipc_sem_wait;
    ipc_sem->get_value = ipc_sem_get_value;
    ipc_sem->close     = ipc_sem_close;

    if(ipc_sem_open(ipc_sem, prefix) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: Failed", __func__);
        ipc_sem_close(ipc_sem);
        return NULL;
    }
    else
    {
        NVLOGI(TAG, "%s: OK", __func__);
        return ipc_sem;
    }
}
