/*
 * Copyright (c) 2019-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _NV_IPC_DEBUG_H_
#define _NV_IPC_DEBUG_H_

#include <stdint.h>
#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

#include <time.h>
#include <stdatomic.h>

#include "nv_ipc.h"
#include "nv_ipc_shm.h"
#include "stat_log.h"
#include "shm_logger.h"

#define FAPI_DATA_SIZE_LIMIT (8096)

#define NV_IPC_MSG_ID_MAX 128

#define DEBUG_HIGH_RESOLUTION_TIME
#ifdef DEBUG_HIGH_RESOLUTION_TIME
typedef struct timespec timestamp_t;
#else
typedef struct timeval timestamp_t;
#endif

typedef struct
{
    timestamp_t ts_alloc; // After allocate
    timestamp_t ts_send;  // Before send
    timestamp_t ts_recv;  // After receive
    timestamp_t ts_free;  // Before free

    timestamp_t ts_fw_deq;  // After receive
    timestamp_t ts_fw_free; // Before free
} msg_timing_t;

typedef struct
{
    timestamp_t ts_post; // Before post
    timestamp_t ts_wait; // After wait finishes
} sync_timing_t;

typedef struct
{
    atomic_ulong post_counter_m2s;
    atomic_ulong wait_counter_m2s;
    atomic_ulong post_counter_s2m;
    atomic_ulong wait_counter_s2m;
    atomic_ulong ipc_dumping;
} debug_shm_data_t;

typedef struct nv_ipc_debug_t nv_ipc_debug_t;
struct nv_ipc_debug_t
{
    int  primary;
    int  msg_pool_len;
    char prefix[NV_NAME_MAX_LEN];

    nv_ipc_transport_t transport;

    int enable_pcap;
    int enable_timing;

    nv_ipc_t* ipc;

    nv_ipc_shm_t*     shmpool;
    debug_shm_data_t* shm_data;
    shmlogger_t*      shmlogger;

    msg_timing_t*  msg_timing;
    sync_timing_t* sync_timing_m2s;
    sync_timing_t* sync_timing_s2m;

    stat_log_t* stat_msg_build;
    stat_log_t* stat_msg_transport;
    stat_log_t* stat_msg_handle;
    stat_log_t* stat_msg_total;

    stat_log_t* stat_wait_delay;
    stat_log_t* stat_post_interval;

    int (*alloc_hook)(nv_ipc_debug_t* ipc_debug, nv_ipc_msg_t* msg, int32_t buf_index);
    int (*free_hook)(nv_ipc_debug_t* ipc_debug, nv_ipc_msg_t* msg, int32_t buf_index);
    int (*send_hook)(nv_ipc_debug_t* ipc_debug, nv_ipc_msg_t* msg, int32_t buf_index);
    int (*recv_hook)(nv_ipc_debug_t* ipc_debug, nv_ipc_msg_t* msg, int32_t buf_index);

    int (*post_hook)(nv_ipc_debug_t* ipc_debug);
    int (*wait_hook)(nv_ipc_debug_t* ipc_debug);

    int (*fw_deq_hook)(nv_ipc_debug_t* ipc_debug, nv_ipc_msg_t* msg, int32_t buf_index);
    int (*fw_free_hook)(nv_ipc_debug_t* ipc_debug, nv_ipc_msg_t* msg, int32_t buf_index);
};

// Get integer system environment value
long get_env_long(const char* name, long def);

int nv_ipc_dump_msg(nv_ipc_debug_t* ipc_debug, nv_ipc_msg_t* msg, int32_t buf_index, const char* info);

int shm_ipc_dump(nv_ipc_t* ipc);
int doca_ipc_dump(nv_ipc_t* ipc);

int64_t nv_ipc_get_buffer_ts_send(nv_ipc_debug_t* ipc_debug, int32_t buf_index);

int nv_ipc_select(const char* prefix);
int nv_ipc_get_mempool_free_count(nv_ipc_mempool_id_t pool_id);

int64_t nv_ipc_convert_pcap(FILE* record_file, char* pcap_filepath, long shm_cache_size, long total_size, uint64_t break_offset);

int nv_ipc_log_collect(const char* prefix, const char* path);

// For debug
int  nv_ipc_app_config_shmpool_open(int primary, const char* prefix);
void load_debug_config();

nv_ipc_debug_t* nv_ipc_debug_open(nv_ipc_t* ipc, const nv_ipc_config_t* cfg);

int get_pcap_max_msg_size();
int get_pcap_max_data_size();

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* _NV_IPC_DEBUG_H_ */
