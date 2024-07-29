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

#include "nv_ipc_cudapool.h"
#include "nv_ipc_utils.h"

#define TAG "NVIPC.CUDAPOOL"

#define CONFIG_CREATE_CUDA_STREAM 1

inline cudaError __checkLastCudaError(const char* file, int line)
{
    cudaError lastErr = cudaGetLastError();
    if(lastErr != cudaSuccess)
    {
        NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "Error at {} line {}: {}", file, line, cudaGetErrorString(lastErr));
    }
    return lastErr;
}
#define checkLastCudaError() __checkLastCudaError(__FILE__, __LINE__)

typedef struct
{
    cudaIpcMemHandle_t   memHandle;
    cudaIpcEventHandle_t eventHandle;
} cuda_ipc_info_t;

typedef struct
{
    int primary;

    // CUDA device ID for CUDA memory case
    int device_id;

    size_t size;

    cudaEvent_t event;

    cudaStream_t stream;

    // For store the CUDA info communicated between different processes, should be in CPU SHM
    cuda_ipc_info_t* ipc_info;
    void*            cuda_addr;

} priv_data_t;

static inline priv_data_t* get_private_data(nv_ipc_cudapool_t* cudapool)
{
    return (priv_data_t*)((char*)cudapool + sizeof(nv_ipc_cudapool_t));
}

static int cudapool_create(priv_data_t* priv_data)
{
    if(cudaMalloc(&priv_data->cuda_addr, priv_data->size) != cudaSuccess)
    {
        checkLastCudaError();
        NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: failed to allocate device memory", __func__);
        return -1;
    }

    memset(priv_data->ipc_info, 0, sizeof(cuda_ipc_info_t));

    if(cudaIpcGetMemHandle(&priv_data->ipc_info->memHandle, priv_data->cuda_addr) != cudaSuccess)
    {
        checkLastCudaError();
        NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: failed to create memory handler", __func__);
        return -1;
    }

    if(cudaEventCreateWithFlags(&priv_data->event, cudaEventDisableTiming | cudaEventInterprocess) != cudaSuccess)
    {
        checkLastCudaError();
        NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: failed to create event", __func__);
        return -1;
    }

    if(cudaIpcGetEventHandle(&priv_data->ipc_info->eventHandle, priv_data->event) != cudaSuccess)
    {
        checkLastCudaError();
        NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: failed to get event handler", __func__);
        return -1;
    }
    else
    {
        NVLOGI_FMT(TAG, "{}: sizeof(cuda_ipc_info_t)={} device_id={} cuda_addr={} OK", __func__, sizeof(cuda_ipc_info_t), priv_data->device_id, priv_data->cuda_addr);
        return 0;
    }
}

static int cudapool_lookup(priv_data_t* priv_data)
{
    if(cudaIpcOpenMemHandle(&priv_data->cuda_addr, priv_data->ipc_info->memHandle, cudaIpcMemLazyEnablePeerAccess) != cudaSuccess)
    {
        checkLastCudaError();
        NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: failed to lookup memory handler", __func__);
        return -1;
    }

    if(cudaIpcOpenEventHandle(&priv_data->event, priv_data->ipc_info->eventHandle) != cudaSuccess)
    {
        checkLastCudaError();
        NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: failed to lookup event handler", __func__);
        return -1;
    }
    else
    {
        //NVLOGI_FMT(TAG, "{}: sizeof(cuda_ipc_info_t)={} device_id={} cuda_addr={} OK", __func__, sizeof(cuda_ipc_info_t), priv_data->device_id, (void *)priv_data->cuda_addr);
        return 0;
    }
}

static int cudapool_close(priv_data_t* priv_data)
{
    if(cudaIpcCloseMemHandle(priv_data->cuda_addr) != cudaSuccess)
    {
        checkLastCudaError();
        NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: failed to close memory handler", __func__);
        return -1;
    }
    else
    {
        NVLOGI_FMT(TAG, "{}: OK", __func__);
        return 0;
    }
}

static int cudapool_destroy(priv_data_t* priv_data)
{
    if(cudaFree(priv_data->cuda_addr) != cudaSuccess)
    {
        checkLastCudaError();
        NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: failed to free memory", __func__);
        return -1;
    }
    else
    {
        NVLOGI_FMT(TAG, "{}: OK", __func__);
        return 0;
    }
}

static void* ipc_get_cudapool_addr(nv_ipc_cudapool_t* cudapool)
{
    if(cudapool == NULL)
    {
        NVLOGE_FMT(TAG, AERIAL_NVIPC_API_EVENT, "{}: instance not exist", __func__);
        return NULL;
    }
    priv_data_t* priv_data = get_private_data(cudapool);
    return priv_data->cuda_addr;
}

static int ipc_memcpy_to_host(nv_ipc_cudapool_t* cudapool, void* host, const void* device, size_t size)
{
    NVLOGV_FMT(TAG, "{}: dst_host={} src_gpu={} size={}", __func__, host, (void *)device, size);

    if(cudapool == NULL)
    {
        NVLOGE_FMT(TAG, AERIAL_NVIPC_API_EVENT, "{}: instance not exist", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(cudapool);

    if(cudaSetDevice(priv_data->device_id) != cudaSuccess)
    {
        checkLastCudaError();
        NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: cudaSetDevice to {} failed", __func__, priv_data->device_id);
        return -1;
    }

    int ret = 0;

    if(CONFIG_CREATE_CUDA_STREAM)
    {
        if(cudaMemcpyAsync(host, device, size, cudaMemcpyDeviceToHost, priv_data->stream) != cudaSuccess)
        {
            checkLastCudaError();
            NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: cudaMemcpyAsync failed", __func__);
            ret = -1;
        }
        else if(cudaStreamSynchronize(priv_data->stream) != cudaSuccess)
        {
            checkLastCudaError();
            NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: cudaStreamSynchronize failed", __func__);
            ret = -1;
        }
    }
    else
    {
        if(cudaMemcpy(host, device, size, cudaMemcpyDeviceToHost) != cudaSuccess)
        {
            checkLastCudaError();
            NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: cudaMemcpy failed", __func__);
            ret = -1;
        }
    }

    return ret;
}

static int ipc_memcpy_to_device(nv_ipc_cudapool_t* cudapool, void* device, const void* host, size_t size)
{
    NVLOGV_FMT(TAG, "{}: dst_gpu={} src_host={} size={}", __func__, device, (void *)host, size);

    if(cudapool == NULL)
    {
        NVLOGE_FMT(TAG, AERIAL_NVIPC_API_EVENT, "{}: instance not exist", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(cudapool);

    if(cudaSetDevice(priv_data->device_id) != cudaSuccess)
    {
        checkLastCudaError();
        NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: cudaSetDevice to {} failed", __func__, priv_data->device_id);
        return -1;
    }

    int ret = 0;

    if(CONFIG_CREATE_CUDA_STREAM)
    {
        if(cudaMemcpyAsync(device, host, size, cudaMemcpyHostToDevice, priv_data->stream) != cudaSuccess)
        {
            checkLastCudaError();
            NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: cudaMemcpyAsync failed", __func__);
            ret = -1;
        }
        else if(cudaStreamSynchronize(priv_data->stream) != cudaSuccess)
        {
            checkLastCudaError();
            NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: cudaStreamSynchronize failed", __func__);
            ret = -1;
        }
    }
    else
    {
        if(cudaMemcpy(device, host, size, cudaMemcpyHostToDevice) != cudaSuccess)
        {
            checkLastCudaError();
            NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: cudaMemcpy failed", __func__);
            ret = -1;
        }
    }

    return ret;
}

static int ipc_cudapool_close(nv_ipc_cudapool_t* cudapool)
{
    if(cudapool == NULL)
    {
        NVLOGE_FMT(TAG, AERIAL_NVIPC_API_EVENT, "{}: instance not exist", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(cudapool);

    int ret = 0;
    if(cudaSetDevice(priv_data->device_id) != cudaSuccess)
    {
        checkLastCudaError();
        NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: cudaSetDevice to {} failed", __func__, priv_data->device_id);
        ret = -1;
    }

    if(CONFIG_CREATE_CUDA_STREAM)
    {
        if(cudaStreamDestroy(priv_data->stream) != cudaSuccess)
        {
            NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: cudaStreamDestroy failed", __func__);
            ret = -1;
        }
    }

    if(priv_data->primary)
    {
        ret = cudapool_destroy(priv_data);
    }
    else
    {
        ret = cudapool_close(priv_data);
    }

    free(cudapool);

    if(ret == 0)
    {
        NVLOGI_FMT(TAG, "{}: OK", __func__);
    }
    return ret;
}

static int ipc_cudapool_open(priv_data_t* priv_data)
{
    if(cudaSetDevice(priv_data->device_id) != cudaSuccess)
    {
        checkLastCudaError();
        NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: cudaSetDevice to {} failed", __func__, priv_data->device_id);
        return -1;
    }

    if(CONFIG_CREATE_CUDA_STREAM)
    {
        if(cudaStreamCreate(&priv_data->stream) != cudaSuccess)
        {
            NVLOGE_FMT(TAG, AERIAL_CUDA_API_EVENT, "{}: cudaStreamCreate failed", __func__);
            return -1;
        }
    }

    if(priv_data->primary)
    {
        return cudapool_create(priv_data);
    }
    else
    {
        return cudapool_lookup(priv_data);
    }
}

nv_ipc_cudapool_t* nv_ipc_cudapool_open(int primary, void* shm, size_t size, int device_id)
{
    if(shm == NULL || size <= 0 || device_id < 0)
    {
        NVLOGE_FMT(TAG, AERIAL_NVIPC_API_EVENT, "{}: invalid parameter", __func__);
        return NULL;
    }

    int                struct_size = sizeof(nv_ipc_cudapool_t) + sizeof(priv_data_t);
    nv_ipc_cudapool_t* cudapool    = (nv_ipc_cudapool_t*)malloc(struct_size);
    if(cudapool == NULL)
    {
        NVLOGE_FMT(TAG, AERIAL_SYSTEM_API_EVENT, "{}: memory malloc failed", __func__);
        return NULL;
    }
    memset(cudapool, 0, struct_size);

    priv_data_t* priv_data = get_private_data(cudapool);
    priv_data->primary     = primary;
    priv_data->device_id   = device_id;
    priv_data->size        = size;
    priv_data->ipc_info    = (cuda_ipc_info_t*)shm;

    cudapool->get_cudapool_addr = ipc_get_cudapool_addr;
    cudapool->memcpy_to_host    = ipc_memcpy_to_host;
    cudapool->memcpy_to_device  = ipc_memcpy_to_device;
    cudapool->close             = ipc_cudapool_close;

    if(ipc_cudapool_open(priv_data) < 0)
    {
        NVLOGE_FMT(TAG, AERIAL_NVIPC_API_EVENT, "{}: Failed", __func__);
        ipc_cudapool_close(cudapool);
        return NULL;
    }
    else
    {
        NVLOGI_FMT(TAG, "{}: OK", __func__);
        return cudapool;
    }
}
