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
#include "cuda.h"
#include "gdrapi.h"
#include "nv_ipc_gpudatapool.h"
#include "nv_ipc_utils.h"

static char TAG[]      = "NVIPC.GPUDATAPOOL";



#define CONFIG_CREATE_CUDA_STREAM 1


typedef struct
{
	CUipcMemHandle memHandle;
    CUipcEventHandle eventHandle;
	int off;
} gpudata_ipc_info_t;


typedef struct
{
    int             primary;    
    int             gpu_device_id;
	size_t          reqested_size;
    size_t          rounded_size;
	size_t          allocated_size;
    CUdevice        dev;
    CUcontext       dev_ctx;
	CUdeviceptr     ptr; /* Aligned pointer to be used to copy data */	
    CUdeviceptr     unaligned_ptr;  /* To be used to do cuMemFree */
    CUevent 		event;
	gpudata_ipc_info_t* ipc_info;
	gdr_t           g;
	gdr_mh_t        mh;
    void            *map_d_ptr;
	void            *buf_ptr; /* mapped GPU memory to the user space */

} priv_data_t;

static inline priv_data_t* get_private_data(nv_ipc_gpudatapool_t* gpudatapool)
{
    return (priv_data_t*)((char*)gpudatapool + sizeof(nv_ipc_gpudatapool_t));
}

/* To add GDR pin and gdr map here */
static int gpudatapool_create(priv_data_t* priv_data)
{


	
/* IPC info */
    memset(priv_data->ipc_info, 0, sizeof(gpudata_ipc_info_t));

/*   This may slow down memory operations */
#if 0	
    unsigned int flag = 1;
	ret = cuPointerSetAttribute(&flag, CU_POINTER_ATTRIBUTE_SYNC_MEMOPS, ptr);

	if (ret != CUDA_SUCCESS) {
            cuMemFree(ptr);
			return ret;   
		}

#endif
	NVLOGI(TAG,"[%s] memHandle = 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",__func__,priv_data->ipc_info->memHandle.reserved,priv_data->ipc_info->memHandle.reserved[0],priv_data->ipc_info->memHandle.reserved[1],
                                                priv_data->ipc_info->memHandle.reserved[2],priv_data->ipc_info->memHandle.reserved[3],priv_data->ipc_info->memHandle.reserved[4],
                                                priv_data->ipc_info->memHandle.reserved[61],priv_data->ipc_info->memHandle.reserved[62],priv_data->ipc_info->memHandle.reserved[63]);
                                                
    NVLOGI(TAG,"[%s] Primary process did not create gpupool",__func__);    
    return 0;

	
}

static int gpudatapool_lookup(priv_data_t* priv_data)
{
	
    CUdeviceptr ptr,out_ptr;

	CUresult ret = CUDA_SUCCESS;

    //memset(priv_data->ipc_info, 0, sizeof(gpudata_ipc_info_t));

	ret = cuMemAlloc(&ptr, priv_data->allocated_size);

	if (ret != CUDA_SUCCESS)
    {
        NVLOGE_NO(TAG, AERIAL_L2ADAPTER_EVENT, "[%s]cuMemAlloc failed!! Ret %d",ret);
        return -1;
    }

    out_ptr = ROUND_UP_GDR(ptr, GPU_PAGE_SIZE);

    priv_data->ptr = out_ptr;
    priv_data->unaligned_ptr = ptr;

    ret = cuIpcGetMemHandle(&(priv_data->ipc_info->memHandle), priv_data->unaligned_ptr);    
    
    if(ret != CUDA_SUCCESS)
    {        
        NVLOGE_NO(TAG, AERIAL_CUDA_API_EVENT, "%s: failed to create memory handler", __func__);
        return -1;
    }    
	
    NVLOGI(TAG,"[%s] memHandle = 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",__func__,priv_data->ipc_info->memHandle.reserved,priv_data->ipc_info->memHandle.reserved[0],priv_data->ipc_info->memHandle.reserved[1],
                                                priv_data->ipc_info->memHandle.reserved[2],priv_data->ipc_info->memHandle.reserved[3],priv_data->ipc_info->memHandle.reserved[4],
                                                priv_data->ipc_info->memHandle.reserved[61],priv_data->ipc_info->memHandle.reserved[62],priv_data->ipc_info->memHandle.reserved[63]);

    ret = cuEventCreate(&priv_data->event, CU_EVENT_DISABLE_TIMING | CU_EVENT_INTERPROCESS);
    if(ret != CUDA_SUCCESS)
    {     
        NVLOGE_NO(TAG, AERIAL_CUDA_API_EVENT, "%s: failed to create event", __func__);
        return -1;
    }


    ret = cuIpcGetEventHandle(&(priv_data->ipc_info->eventHandle), priv_data->event);	
    if( ret != CUDA_SUCCESS)
    {     
        NVLOGE_NO(TAG, AERIAL_CUDA_API_EVENT, "%s: failed to get event handler", __func__);
        return -1;
    }

	
	/* GDR */
    priv_data->g = gdr_open();
    if(priv_data->g == (void*)0)
    {
        NVLOGE_NO(TAG, AERIAL_L2ADAPTER_EVENT, "gdr_open error: Is gdrdrv driver installed and loaded\? %d",__LINE__);
        return -1;
    }    

    if(gdr_pin_buffer(priv_data->g, priv_data->ptr, priv_data->rounded_size, 0, 0, &priv_data->mh) != 0)
    {
        NVLOGE_NO(TAG, AERIAL_L2ADAPTER_EVENT, "<<< gdr_pin_buffer failed >>>");
        return -1;
    }
    else if(priv_data->mh.h == 0)
    {
        NVLOGE_NO(TAG, AERIAL_L2ADAPTER_EVENT, "<<< gdr_mh_t is NULL >>>");
        return -1;
    }

    //NVLOGE_NO(TAG, AERIAL_CUDA_API_EVENT, "\n%s: open GDR MH %d ", __func__,priv_data->mh.h);

	if(gdr_map(priv_data->g, priv_data->mh, &priv_data->map_d_ptr, priv_data->rounded_size) != 0)
    {
        NVLOGE_NO(TAG, AERIAL_L2ADAPTER_EVENT, "<<< gdr_map failed >>>");
        return -1;
    }

	gdr_info_t      info;
	if(gdr_get_info(priv_data->g, priv_data->mh, &info)!= 0)
	{
		NVLOGE_NO(TAG, AERIAL_L2ADAPTER_EVENT, "<<< gdr_get_info failed >>>");
        return -1;
	}

	//NVLOGI(TAG,"\n\ninfo.va: 0x%x\ninfo.mapped_size: %u\ninfo.page_size: %u\ninfo.page_size: %u\ninfo.mapped: %u\ninfo.wc_mapping: %u\npriv_data->ptr: 0x%x",
			//info.va,info.mapped_size,info.page_size,info.page_size,info.mapped,info.wc_mapping,priv_data->ptr);

	priv_data->ipc_info->off = info.va - priv_data->ptr;

	NVLOGI(TAG,"\n[%s] page offset: %d",__func__,priv_data->ipc_info->off);

	priv_data->buf_ptr = (uint32_t *)((char *)priv_data->map_d_ptr + priv_data->ipc_info->off);
	
    NVLOGI(TAG,"\n[%s] user-space pointer: 0x%x",__func__,priv_data->buf_ptr);
	
    return 0;    
}

static int gpudatapool_close(priv_data_t* priv_data)
{
	CUresult res = CUDA_SUCCESS;

	res = cuIpcCloseMemHandle(priv_data->unaligned_ptr);
    if(res != CUDA_SUCCESS)
    {        
        NVLOGE_NO(TAG, AERIAL_CUDA_API_EVENT, "%s: failed to close memory handler", __func__);
        return -1;
    }
    else
    {
        NVLOGI(TAG, "%s: OK", __func__);
        return 0;
    }
}

static int gpudatapool_destroy(priv_data_t* priv_data)
{
	if(gdr_unmap(priv_data->g, priv_data->mh, priv_data->map_d_ptr, priv_data->rounded_size) != 0)
    {
        NVLOGE_NO(TAG, AERIAL_L2ADAPTER_EVENT, "<<< gdr_unmap failed >>>");
        return -1;
    }

	if(gdr_unpin_buffer(priv_data->g, priv_data->mh) != 0)
    {
        NVLOGE_NO(TAG, AERIAL_L2ADAPTER_EVENT, "<<< gdr_unpin_buffer failed >>>");
        return -1;
    }

	if(gdr_close(priv_data->g) != 0)
    {
        NVLOGE_NO(TAG, AERIAL_L2ADAPTER_EVENT, "<<< gdr_close failed >>>");
        return -1;
    }
    
    if(cuEventDestroy(priv_data->event)!= CUDA_SUCCESS)
    {
        NVLOGE_NO(TAG, AERIAL_L2ADAPTER_EVENT, "%s: cuEventDestroy failed",__func__);
        return -1;
    }

	if(cuMemFree(priv_data->unaligned_ptr) != CUDA_SUCCESS)
    {        
        NVLOGE_NO(TAG, AERIAL_CUDA_API_EVENT, "%s: failed to free memory", __func__);
        return -1;
    }
    else
    {
        memset(priv_data, 0, sizeof(priv_data_t));
        NVLOGI(TAG, "%s: OK", __func__);
        return 0;
    }
}

static void* ipc_get_gpudatapool_addr(nv_ipc_gpudatapool_t* gpudatapool)
{
    if(gpudatapool == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return NULL;
    }
    priv_data_t* priv_data = get_private_data(gpudatapool);
	NVLOGI(TAG, "[%s]: buf_ptr = %p", __func__,priv_data->buf_ptr);
	return priv_data->buf_ptr;
}


/* Bhaskar : To update this with gdrcopy *************  !!! */
static int ipc_memcpy_to_host(nv_ipc_gpudatapool_t* gpudatapool, void* host, const void* device, size_t size)
{
    int ret = 0;
    if(gpudatapool == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }
    priv_data_t* priv_data = get_private_data(gpudatapool);

	NVLOGI(TAG,"[%s] host=%p device=%p  size=%u",device,host,size);

	gdr_copy_from_mapping(priv_data->mh,host,device,size);

    return ret;
}

static int ipc_memcpy_to_device(nv_ipc_gpudatapool_t* gpudatapool, void* device, const void* host, size_t size)
{
    int ret = 0;

    if(gpudatapool == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(gpudatapool);

	//NVLOGI(TAG,"[%s] device=%p host=%p size=%u",__func__,device,host,size);
    //NVLOGI(TAG,"[%s] device=0x%x host=0x%x size=%u",__func__,device,host,size);

	int copyBytes = gdr_copy_to_mapping(priv_data->mh,device,host,size);
	
    return ret;
}

static int ipc_gpudatapool_close(nv_ipc_gpudatapool_t* gpudatapool)
{
    int ret = 0;
    if(gpudatapool == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }

    priv_data_t* priv_data = get_private_data(gpudatapool);
    

    if(priv_data->primary)
    {
        ret = gpudatapool_close(priv_data);
        if(ret == -1)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: gpudatapool_close failed", __func__);
            return -1;
        }
        else if(CUDA_SUCCESS != cuDevicePrimaryCtxRelease(priv_data->dev))
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: primary=%d cuDevicePrimaryCtxRelease failed", __func__,priv_data->primary);
            return -1;
        }
    }
    else
    {        
        ret = gpudatapool_destroy(priv_data);
        if( ret == -1)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: gpudatapool_destroy returned -1", __func__);
            return -1;
        }
        else if(CUDA_SUCCESS != cuDevicePrimaryCtxRelease(priv_data->dev))
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: primary=%d cuDevicePrimaryCtxRelease failed", __func__,priv_data->primary);
            return -1;
        }
    }

    free(gpudatapool);

    if(ret == 0)
    {
        NVLOGI(TAG, "%s: OK", __func__);
    }
    return ret;
}

static int ipc_gpudatapool_open(priv_data_t* priv_data)
{	
    priv_data->rounded_size = (priv_data->reqested_size + GPU_PAGE_SIZE -1) & GPU_PAGE_MASK;
    priv_data->allocated_size = (priv_data->rounded_size + GPU_PAGE_SIZE - 1); 
		
    if(priv_data->primary)
    {
        return gpudatapool_create(priv_data);        
    }
    else
    {
        if(CUDA_SUCCESS !=cuInit(0))
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: cuInit failed", __func__);
            return -1;
        }
    
        if(CUDA_SUCCESS != cuDeviceGet(&priv_data->dev, priv_data->gpu_device_id))
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: cuDeviceGet failed", __func__);
            return -1;
        }
        if(CUDA_SUCCESS != cuDevicePrimaryCtxRetain(&priv_data->dev_ctx, priv_data->dev))
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: cuDevicePrimaryCtxRetain failed", __func__);
            return -1;
        }
        if(CUDA_SUCCESS != cuCtxSetCurrent(priv_data->dev_ctx))
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: cuCtxSetCurrent failed", __func__);
            return -1;
        }
        return gpudatapool_lookup(priv_data);        
    }
}


int nv_ipc_gpudatapool_reInit(nv_ipc_gpudatapool_t* pGpuDataPool)
{
	CUresult res = CUDA_SUCCESS;
	priv_data_t* priv_data = get_private_data(pGpuDataPool);

    if(CUDA_SUCCESS !=cuInit(0))
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: cuInit failed", __func__);
        return -1;
    }
 
    if(CUDA_SUCCESS != cuDeviceGet(&priv_data->dev, priv_data->gpu_device_id))
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: cuDeviceGet failed", __func__);
        return -1;
    }
    if(CUDA_SUCCESS != cuDevicePrimaryCtxRetain(&priv_data->dev_ctx, priv_data->dev))
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: cuDevicePrimaryCtxRetain failed", __func__);
        return -1;
    }
    if(CUDA_SUCCESS != cuCtxSetCurrent(priv_data->dev_ctx))
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: cuCtxSetCurrent failed", __func__);
        return -1;
    }

    //Debid end
    NVLOGI(TAG,"[%s] memHandle = 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",__func__,priv_data->ipc_info->memHandle.reserved,priv_data->ipc_info->memHandle.reserved[0],priv_data->ipc_info->memHandle.reserved[1],
                                                priv_data->ipc_info->memHandle.reserved[2],priv_data->ipc_info->memHandle.reserved[3],priv_data->ipc_info->memHandle.reserved[4],
                                                priv_data->ipc_info->memHandle.reserved[61],priv_data->ipc_info->memHandle.reserved[62],priv_data->ipc_info->memHandle.reserved[63]);

	res = cuIpcOpenMemHandle(&priv_data->unaligned_ptr,priv_data->ipc_info->memHandle,CU_IPC_MEM_LAZY_ENABLE_PEER_ACCESS);
	if( res != CUDA_SUCCESS )
	{
		NVLOGE_NO(TAG, AERIAL_CUDA_API_EVENT, "%s: cuIpcOpenMemHandle failed!! Error: 0x%x", __func__,res);
		return -1;
	}
	res = cuIpcOpenEventHandle ( &priv_data->event, priv_data->ipc_info->eventHandle );

	if( res != CUDA_SUCCESS )
	{
		NVLOGE_NO(TAG, AERIAL_CUDA_API_EVENT, "%s: cuIpcOpenEventHandle failed!! Error: 0x%x", __func__,res);
		return -1;
	}    
    else
    {
    	priv_data->buf_ptr = (void *) (uintptr_t)(priv_data->unaligned_ptr);// + priv_data->ipc_info->off;    	
        /* Use dptr = (CUdeviceptr) (uintptr_t) p; to get back CUdeviceptr from (void*) */

	    NVLOGI(TAG,"[%s] primary = %d buf_ptr = %p gpudatapool = 0x%x",__func__,priv_data->primary,priv_data->buf_ptr,pGpuDataPool);
        return 0;
    }
}

nv_ipc_gpudatapool_t* nv_ipc_gpudatapool_open(int primary, void* shm, size_t size, int device_id)
{
    if(shm == NULL || size <= 0 || device_id < 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: invalid parameter", __func__);
        return NULL;
    }

    int                struct_size = sizeof(nv_ipc_gpudatapool_t) + sizeof(priv_data_t);
    nv_ipc_gpudatapool_t* gpudatapool    = (nv_ipc_gpudatapool_t*)malloc(struct_size);
    if(gpudatapool == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: memory malloc failed", __func__);
        return NULL;
    }
    memset(gpudatapool, 0, struct_size);

    priv_data_t* priv_data = get_private_data(gpudatapool);
    priv_data->primary     = primary;
    priv_data->gpu_device_id   = device_id;
    priv_data->reqested_size = size;
    priv_data->ipc_info    = (gpudata_ipc_info_t *)shm;

    gpudatapool->get_gpudatapool_addr = ipc_get_gpudatapool_addr;
    gpudatapool->memcpy_to_host    = ipc_memcpy_to_host;
    gpudatapool->memcpy_to_device  = ipc_memcpy_to_device;
    gpudatapool->close             = ipc_gpudatapool_close;

    if(ipc_gpudatapool_open(priv_data) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: Failed", __func__);
        ipc_gpudatapool_close(gpudatapool);
        return NULL;
    }
    else
    {   
        NVLOGI(TAG,"[%s] primary = %d buf_ptr = %p gpudatapool = 0x%x",__func__,priv_data->primary,priv_data->buf_ptr,gpudatapool);
    	NVLOGI(TAG, "%s: OK", __func__);
        return gpudatapool;
    }
}
