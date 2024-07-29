/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _NV_IPC_CUDA_UTILS_H_
#define _NV_IPC_CUDA_UTILS_H_

#if defined(__cplusplus)
extern "C" {
#endif

// Check whether CUDA driver and CUDA device exist. Return 0 if exist, else return -1
int cuda_version_check();

// Set page lock/unlock for host memory
int cuda_page_lock(void* phost, size_t size);
int cuda_page_unlock(void* phost);

// CUDA memory copy function wrapper
int nv_ipc_memcpy_to_host(void* host, const void* device, size_t size);
int nv_ipc_memcpy_to_device(void* device, const void* host, size_t size);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* _NV_IPC_CUDA_UTILS_H_ */
