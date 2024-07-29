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
#include <sys/mman.h>
#include "cuda.h"
#include "gdrapi.h"
#include "nv_ipc_gpudata_utils.h"
#include "nv_ipc_utils.h"

static char TAG[]      = "NVIPC.GPUDATAUTILS";

#if 0
#define CUDA_CHECK( fn ) do { \
  CUresult status = (fn); \
  if ( CUDA_SUCCESS != status ) { \
    const char* errstr; \
    cuGetErrorString(status, &errstr); \
    NVLOGE_NO(TAG, AERIAL_CUDA_API_EVENT, "CUDA Driver Failure (line %d of file %s):\n\t%s returned 0x%x (%s)", __LINE__, __FILE__, #fn, status, errstr);\
    exit(EXIT_FAILURE); \
  } \
} while (0)

#endif

// Check whether CUDA driver and CUDA device exist. Return 0 if exist, else return -1
int gpu_cuda_version_check()
{
    int driverVersion  = -1;

	CUresult res = CUDA_SUCCESS;

	res = cuDriverGetVersion ( &driverVersion );
    if(res != CUDA_SUCCESS)
    {
        // checkLastCudaError();
        NVLOGE(TAG, "%s: cuDriverGetVersion failed", __func__);
        return -1;
    }
    else
    {
         NVLOGI(TAG, "%s: cuDriverGetVersion = %d ", __func__,driverVersion);    
    }

    // NVLOGC(TAG, "%s: driverVersion=%d runtimeVersion=%d", __func__, driverVersion, runtimeVersion);

    if(driverVersion > 0 )
    {
        return 0;
    }
    else
    {
        return -1;
    }
}



int nv_ipc_gpu_data_page_lock(void* phost, size_t size)
{
    if(gpu_cuda_version_check() < 0)
    {
        NVLOGE(TAG, "%s: CUDA driver or device not exist, skip", __func__);
        return -1;
    }

    unsigned int flag = CU_MEMHOSTREGISTER_PORTABLE | CU_MEMHOSTREGISTER_DEVICEMAP;
    CUresult res = cuMemHostRegister (phost, size, flag);

    if( res != CUDA_SUCCESS)
    {
        NVLOGE_NO(TAG, AERIAL_CUDA_API_EVENT, "%s: cuMemHostRegister failed!! res=%d", __func__,res);
        //CUDA_CHECK(res);
        return -1;
    }
    else
    {
        NVLOGI(TAG, "%s: OK", __func__);
        return 0;
    }
}


int nv_ipc_gpu_data_page_unlock(void* phost)
{
    if(gpu_cuda_version_check() < 0)
    {
        NVLOGI(TAG, "%s: CUDA driver or device not exist, skip", __func__);
        return -1;
    }

    if(cuMemHostUnregister (phost) != CUDA_SUCCESS)
    {        
        NVLOGE_NO(TAG, AERIAL_CUDA_API_EVENT, "%s: cuMemHostUnregister failed", __func__);
        return -1;
    }
    else
    {
        NVLOGI(TAG, "%s: OK", __func__);
        return 0;
    }
}


int nv_ipc_gdrmemcpy_to_host(void* host, const void* device, size_t size)
{
#if 0
	int gdr_copy_from_mapping(gdr_mh_t handle, void *h_ptr, const void *map_d_ptr, size_t size)
#endif
    return 0;
}

int nv_ipc_gdrmemcpy_to_device(void* device, const void* host, size_t size)
{
#if 0
    gdr_copy_to_mapping(gdr_mh_t handle, void * map_d_ptr, const void * h_ptr, size_t size);
	gdr_copy_to_mapping_internal(void * map_d_ptr, const void * h_ptr, size_t size, int wc_mapping)
#endif
	return 0;
}
