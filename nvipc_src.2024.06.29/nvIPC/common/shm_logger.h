/*
 * Copyright (c) 2021-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _SHM_LOGGER_H_
#define _SHM_LOGGER_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
// #include <stdatomic.h>
#include <semaphore.h>

#include "nv_ipc.h"
#include "nv_ipc_shm.h"

#define SHMLOG_DEFAULT_NAME "shmlog"

#define SHMLOG_NAME_MAX_LEN 32

typedef enum
{
    SHMLOG_STATE_INIT   = 0,
    SHMLOG_STATE_OPENED = 1,
    SHMLOG_STATE_CLOSED = 2,
} shmlogger_state_t;

typedef struct
{
    uint64_t max_file_size;  // Max log length for one time log API call
    int64_t  shm_cache_size; // Shared memory size, see /dev/shm/. Value will be aligned to 2^n automatically
    int32_t  save_to_file;   // Whether to start a background thread for save SHM cache to disk file.
    int32_t  cpu_core_id;    // CPU core ID for the background if enabled. -1 means no core binding.
} shmlogger_config_t;

typedef struct
{
    struct timeval tv;
    int32_t        buf_size;
    int32_t        flags;
    char           buf[0];
} record_t;

typedef struct shmlogger_t shmlogger_t;

shmlogger_t* shmlogger_open(int primary, const char* name, shmlogger_config_t* cfg);
int          shmlogger_close(shmlogger_t* logger);

void shmlogger_save_ipc_msg(shmlogger_t* logger, nv_ipc_msg_t* msg, int32_t flags);
void shmlogger_save_buffer(shmlogger_t* logger, const char* buffer, int32_t size, int32_t flags);
int  shmlogger_collect(const char* prefix, const char* type, const char* path);

static inline int get_record_size(record_t* record)
{
    return sizeof(record_t) + record->buf_size + sizeof(record->buf_size);
}

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* _SHM_LOGGER_H_ */
