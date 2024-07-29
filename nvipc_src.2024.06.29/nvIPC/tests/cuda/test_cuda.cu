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
#include <cuda_runtime_api.h>

#include "test_cuda.h"
#include "nv_ipc_utils.h"

#define TAG "NVIPC.TESTCUDA"

#define N_BLOCK 64
#define N_THREAD 32;

static const char SUB_VAL = (char) ('A' - 'a');

inline cudaError __checkLastCudaError(const char* file, int line)
{
    cudaError lastErr = cudaGetLastError();
    if(lastErr != cudaSuccess)
    {
        NVLOGE_FMT(TAG, AERIAL_NVIPC_API_EVENT, "Error at {} line {}: {}", file, line, cudaGetErrorString(lastErr));
    }
    return lastErr;
}

#define checkLastCudaError() __checkLastCudaError(__FILE__, __LINE__)

#define HANDLE_ERROR(x)                                                                 \
    do                                                                                  \
    {                                                                                   \
        if((x) != cudaSuccess) { printf("Error %s line%d\n", __FUNCTION__, __LINE__); } \
    } while(0)
#define HANDLE_NULL(x)

static __global__ void gpu_to_lower_case(char* str, int length)
{
    int index  = threadIdx.x + blockIdx.x * blockDim.x;
    int stride = blockDim.x * gridDim.x;

    for(int i = index; i < length; i += stride)
    {
        if(str[i] >= 'A' && str[i] <= 'Z')
        {
            str[i] -= SUB_VAL;
        }
    }
}

void test_cuda_to_lower_case(int deviceId, char* str, int length, int gpu)
{
    if(deviceId < 0)
    {
        NVLOGD_FMT(TAG, "{}: deviceId={}, fall back to CPU IPC test", __func__, deviceId);
        gpu = 0;
    }

    NVLOGI_FMT(TAG, "{}: gpu={}", __func__, gpu);
    if(gpu)
    {
        int nblock  = N_BLOCK;
        int nthread = N_THREAD;
        HANDLE_ERROR(cudaSetDevice(deviceId));
        gpu_to_lower_case<<<nblock, nthread>>>(str, length);
        checkLastCudaError();
    }
    else
    {
        cpu_to_lower_case(str, length);
    }
}

int get_cuda_device_id(void)
{
    int num;
    cudaError_t err = cudaGetDeviceCount (&num);
    NVLOGC_FMT(TAG, "{}: err={} num={}", __func__, err, num);

    if (err == cudaSuccess && num > 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

void cuda_to_lower_case(char* str, int length, int deviceId)
{
    if(deviceId < 0)
    {
        NVLOGC_FMT(TAG, "{}: invalid CUDA deviceId: {}", __func__, deviceId);
        return;
    }

    int nblock  = N_BLOCK;
    int nthread = N_THREAD;
    HANDLE_ERROR(cudaSetDevice(deviceId));
    gpu_to_lower_case<<<nblock, nthread>>>(str, length);
    checkLastCudaError();
}
