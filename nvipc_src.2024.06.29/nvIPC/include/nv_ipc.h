/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NV_IPC_H_INCLUDED_
#define NV_IPC_H_INCLUDED_

#include <stdint.h>
#include <stddef.h>

#define NV_NAME_MAX_LEN 32

#define NV_IPV4_STRING_LEN 16

/*********** Library Version **************/
#define LIBRARY_VERSION "2.0"

// The IPC module types. PRIMARY and SECONDARY are for generic IPC use cases.
typedef enum
{
    NV_IPC_MODULE_MAC       = 0,
    NV_IPC_MODULE_PHY       = 1,
    NV_IPC_MODULE_SECONDARY = 2,
    NV_IPC_MODULE_PRIMARY   = 3,
    NV_IPC_MODULE_IPC_DUMP  = 4,
    NV_IPC_MODULE_MAX       = 5
} nv_ipc_module_t;

// IPC transport: UDP, Shared Memory.
typedef enum
{
    NV_IPC_TRANSPORT_UDP  = 0,
    NV_IPC_TRANSPORT_SHM  = 1,
    NV_IPC_TRANSPORT_DPDK = 2,
    NV_IPC_TRANSPORT_DOCA = 3,
    NV_IPC_TRANSPORT_MAX  = 4
} nv_ipc_transport_t;

// Define memory pools
typedef enum
{
    NV_IPC_MEMPOOL_CPU_MSG   = 0,
    NV_IPC_MEMPOOL_CPU_DATA  = 1,
    NV_IPC_MEMPOOL_CUDA_DATA = 2,
    NV_IPC_MEMPOOL_GPU_DATA  = 3,
    NV_IPC_MEMPOOL_NUM       = 4
} nv_ipc_mempool_id_t;

typedef struct
{
    int32_t msg_id;    // IPC message ID
    int32_t cell_id;   // Cell ID
    int32_t msg_len;   // MSG part length
    int32_t data_len;  // DATA part length
    int32_t data_pool; // DATA memory pool ID
    void*   msg_buf;   // MSG buffer pointer
    void*   data_buf;  // MSG buffer pointer
} nv_ipc_msg_t;

typedef struct
{
    int  local_port;
    int  remote_port;
    char local_addr[NV_IPV4_STRING_LEN];  // Only support IPv4 yet.
    char remote_addr[NV_IPV4_STRING_LEN]; // Only support IPv4 yet.

    int32_t msg_buf_size;
    int32_t data_buf_size;
} nv_ipc_config_udp_t;

/******* SHM IPC configuration *********************/
typedef struct
{
    int32_t buf_size;
    int32_t pool_len;
} nv_ipc_mempool_size_t;

typedef struct
{
    // PRIMARY is responsible to initiate the shared memory pools and semaphores
    int primary;

    // CUDA device ID for CUDA/GPU DATA memory pool
    int cuda_device_id;

    // The ring queue length for TX and RX
    int32_t ring_len;

    // Size of all memory pools: MSG, CPU_DATA, CUDA_DATA.
    nv_ipc_mempool_size_t mempool_size[NV_IPC_MEMPOOL_NUM];

    // The name prefix of a nv_ipc_t instance. Set unique name prefix for each nv_ipc_t instance.
    char prefix[NV_NAME_MAX_LEN];

} nv_ipc_config_shm_t;

typedef struct
{
    // PRIMARY is responsible to initiate the shared memory pools and semaphores
    int primary;

    // CUDA device ID for CUDA memory pool
    int cuda_device_id;

    uint16_t need_eal_init;
    uint16_t lcore_id;
    uint16_t nic_mtu;

    // Size of all memory pools: MSG, CPU_DATA, CUDA_DATA.
    nv_ipc_mempool_size_t mempool_size[NV_IPC_MEMPOOL_NUM];

    char prefix[NV_NAME_MAX_LEN];
    char local_nic_pci[NV_NAME_MAX_LEN];
    char peer_nic_mac[NV_NAME_MAX_LEN];

} nv_ipc_config_dpdk_t;

typedef struct
{
    // PRIMARY is responsible to initiate the shared memory pools and semaphores
    int primary;

    // CUDA device ID for CUDA memory pool
    int cuda_device_id;

    uint16_t cpu_core;
    uint16_t nic_mtu;

    // Size of all memory pools: MSG, CPU_DATA, CUDA_DATA.
    nv_ipc_mempool_size_t mempool_size[NV_IPC_MEMPOOL_NUM];

    char prefix[NV_NAME_MAX_LEN];
    char host_pci[NV_NAME_MAX_LEN];
    char dpu_pci[NV_NAME_MAX_LEN];

} nv_ipc_config_doca_t;

typedef struct
{
    nv_ipc_module_t    module_type;
    nv_ipc_transport_t ipc_transport;

    union
    {
        nv_ipc_config_udp_t  udp;
        nv_ipc_config_shm_t  shm;
        nv_ipc_config_dpdk_t dpdk;
        nv_ipc_config_doca_t doca;
    } transport_config;
} nv_ipc_config_t;

/* This API set support both UDP and SHM transport. */
typedef struct nv_ipc_t nv_ipc_t;
struct nv_ipc_t
{
    // De-initiate and destroy the nv_ipc_t instance
    int (*ipc_destroy)(nv_ipc_t* ipc);

    // Memory allocate/release for TX side
    int (*tx_allocate)(nv_ipc_t* ipc, nv_ipc_msg_t* msg, uint32_t options);
    int (*tx_release)(nv_ipc_t* ipc, nv_ipc_msg_t* msg);

    // Memory allocate/release for RX side
    int (*rx_allocate)(nv_ipc_t* ipc, nv_ipc_msg_t* msg, uint32_t options);
    int (*rx_release)(nv_ipc_t* ipc, nv_ipc_msg_t* msg);

    // Send a ipc_msg_t message. Return -1 if failed
    int (*tx_send_msg)(nv_ipc_t* ipc, nv_ipc_msg_t* msg);

    // Call tx_tti_sem_post() at the end of a TTI
    int (*tx_tti_sem_post)(nv_ipc_t* ipc);

    // Call rx_tti_sem_wait() and then receive all messages in a TTI
    int (*rx_tti_sem_wait)(nv_ipc_t* ipc);

    // Get a ipc_msg_t message. Return -1 if no available.
    int (*rx_recv_msg)(nv_ipc_t* ipc, nv_ipc_msg_t* msg);

    // Get SHM event FD or UDP socket FD for epoll
    int (*get_fd)(nv_ipc_t* ipc);

    // Write tx_fd to notify the event, Only need for SHM
    int (*notify)(nv_ipc_t* ipc, int value);

    // Read rx_fd to clear the event. Only need for SHM
    int (*get_value)(nv_ipc_t* ipc);

    // CUDA memory copy function
    int (*cuda_memcpy_to_host)(nv_ipc_t* ipc, void* host, const void* device, size_t size);
    int (*cuda_memcpy_to_device)(nv_ipc_t* ipc, void* device, const void* host, size_t size);

	// GDR copy function
	int (*gdr_memcpy_to_host)(nv_ipc_t* ipc, void* host, const void* device, size_t size);
    int (*gdr_memcpy_to_device)(nv_ipc_t* ipc, void* device, const void* host, size_t size);
};

#if defined(__cplusplus)
extern "C" {
#endif

int set_nv_ipc_default_config(nv_ipc_config_t* cfg, nv_ipc_module_t module_type);
int load_nv_ipc_yaml_config(nv_ipc_config_t* cfg, const char* yaml_path, nv_ipc_module_t module_type);

nv_ipc_t* create_nv_ipc_interface(const nv_ipc_config_t* cfg);

nv_ipc_t* nv_ipc_get_instance(const char* prefix);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* NV_IPC_H_INCLUDED_ */
