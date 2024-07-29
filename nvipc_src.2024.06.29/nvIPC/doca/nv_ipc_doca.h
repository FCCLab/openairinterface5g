/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NV_IPC_DOCA_H
#define NV_IPC_DOCA_H

#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>

#include <doca_dev.h>
#include <doca_comm_channel.h>
#include <doca_buf_inventory.h>
#include <doca_mmap.h>
#include <doca_dma.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define PCI_ADDR_LEN 32

#define NVIPC_DOCA_CC_MAX_MSG_SIZE 4080
#define CC_MAX_QUEUE_SIZE 8192       /* Max number of messages on Comm Channel queue */
#define WORKQ_DEPTH 8192         /* Work queue depth */
#define SLEEP_IN_NANOS (10 * 1000) /* Sample the job every 10 microseconds  */

#define TEST_BUF_LEN 1024

#define MAX_MMAP_EXPORT_DESC_LEN 300

typedef struct {
    // DOCA global address
    int mode; // 0 - Host; 1 - DPU

    uint32_t nb_devs;
    struct doca_devinfo **dev_list;

    struct doca_devinfo *devinfo_dma;
    struct doca_devinfo *devinfo_cc;

    char name[NV_NAME_MAX_LEN * 2];
    char dev_pci_str[PCI_ADDR_LEN];         /* Comm Channel DOCA device PCI address */
    char rep_pci_str[PCI_ADDR_LEN];         /* Comm Channel DOCA device representor PCI address */

    // DOCA common channel
    struct doca_comm_channel_ep_t *ep;
    struct doca_comm_channel_addr_t *peer_addr;
    struct doca_dev *cc_dev;
    struct doca_dev_rep *cc_dev_rep;

    // DOCA DMA
    struct doca_dev *dma_dev;
//    struct doca_mmap *mmap_local;
//    struct doca_mmap *mmap_remote;
//    struct doca_buf_inventory *buf_inv;
    struct doca_ctx *ctx;
    struct doca_dma *dma_ctx;
//    struct doca_workq *workq;
//
//    struct doca_buf *remote_doca_buf;
//    struct doca_buf *local_doca_buf;

//    uint8_t *remote_buf;
//    uint8_t local_buffer[TEST_BUF_LEN];

    // struct dma_copy_cfg dma_cfg;
    // struct core_state core_state;

    char gpu_pcie_addr[PCI_ADDR_LEN];
    struct doca_gpu *gpu_dev;
} doca_info_t;

typedef struct {
    atomic_int status_mask;
    // struct doca_event event;    // TODO: upgrade for DOCA 2.5
    struct doca_buf *local_buf;
    struct doca_buf *remote_buf;
} dma_job_t;

typedef struct {
//    nv_ipc_mempool_t *host_mempool;
//    nv_ipc_mempool_t *dpu_mempool;
    struct doca_mmap *mmap_local;
    struct doca_mmap *mmap_remote;
    struct doca_buf_inventory *buf_inv;
    struct doca_workq *workq;
    dma_job_t* jobs;
} ipc_mmap_t;

typedef union {
    uint64_t u64;
    struct {
        int32_t pool_id;
        int16_t pkt_id;
        int16_t buf_id;
    } i32;
} buf_info_t;

typedef struct {
    uint64_t mmap_addr;
    uint64_t mmap_size;
    size_t export_desc_len;
    size_t test_msg_size;
    char export_desc[];
} host_mmap_info_t;

typedef struct {
    uint8_t* mmap_addr;
    uint64_t mmap_size;
    size_t desc_len;
    uint8_t desc[MAX_MMAP_EXPORT_DESC_LEN];
} mmap_info_t;

typedef struct {
    struct doca_buf_inventory *buf_inv;
    struct doca_mmap *mmap_local;
    struct doca_mmap *mmap_remote;
    struct doca_workq *workq;
    dma_job_t* jobs;
    int inflight;
} dma_info_t;

typedef struct {
    struct doca_dev *dma_dev;
    struct doca_ctx *ctx;
    struct doca_dma *dma_ctx;
    struct doca_devinfo *devinfo_dma;
} dma_comm_t;

enum dma_copy_mode {
    DMA_COPY_MODE_HOST, /* Run endpoint in Host */
    DMA_COPY_MODE_DPU /* Run endpoint in DPU */
};

enum {
    CC_MSG_IPC_PACKET_EVENT,
    CC_MSG_UL_IPC_MSG_READY,
    CC_MSG_DL_IPC_MSG_READY,
    CC_MSG_IPC_MSG_FREE,
    CC_MSG_NO_PAYLOAD,
};

typedef struct {
    struct timespec ts_send;
    int32_t type;
    int32_t len;
    uint8_t payload[];
} cc_msg_t;

typedef struct
{
    cc_msg_t cc_msg;

    int32_t msg_id;
    int32_t cell_id;
    int32_t msg_len;
    int32_t data_len;
    int32_t msg_index;
    int32_t data_index;
    int32_t data_pool;

    uint8_t payload[0];

} packet_info_t;

void print_doca_sdk_version();

int  nvipc_doca_sample_test(const nv_ipc_config_doca_t* cfg);
void doca_print_cpu_core();

int doca_cc_send(doca_info_t *di, cc_msg_t *cmsg);
int doca_cc_recv(doca_info_t *di, cc_msg_t *cmsg);

int doca_comm_recv(doca_info_t* doca_info, void* buf, size_t* p_size);
int doca_comm_send(doca_info_t* doca_info, void* buf, size_t size);

doca_error_t wait_for_successful_status_msg(struct doca_comm_channel_ep_t *ep,
        struct doca_comm_channel_addr_t **peer_addr);
doca_error_t send_status_msg(struct doca_comm_channel_ep_t *ep,
        struct doca_comm_channel_addr_t **peer_addr, bool status);

int doca_dma_test(doca_info_t *di);
int doca_comm_channel_init(doca_info_t *di);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* NV_IPC_DOCA_H */
