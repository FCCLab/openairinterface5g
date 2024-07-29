/*
 * Copyright (c) 2021-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>

#include "shm_logger.h"
#include "nv_ipc_utils.h"
#include "nv_ipc_debug.h"

#define TAG (NVLOG_TAG_BASE_NVIPC + 16) // "NVIPC.SHMLOG"
#define TAG_LOG_COLLECT (NVLOG_TAG_BASE_NVIPC + 16) // "NVIPC.SHMLOG"
#define NV_PATH_MAX_LEN 512

shmlogger_t* shmlogger_default = NULL; // &default_logger;

static atomic_uint logger_counter = 0;

typedef struct
{
    atomic_ulong offset; // File buffer offset
    atomic_ulong counter;
    atomic_ulong total_saved;   // File buffer offset
    atomic_ulong max_file_size; // The maximum count of SHM cache block saving to file
    uint64_t     page_start_offset[1024*1024]; // The first integrative pcap record in current page (1 block = 2 page)
    char         shmbuf[];
} shmlogger_shm_t;

struct shmlogger_t
{
    shmlogger_config_t config;
    char               name[SHMLOG_NAME_MAX_LEN];

    int        primary;
    int        module_type;
    atomic_int log_state;

    // SHM cache handler
    nv_ipc_shm_t*    shmpool;
    shmlogger_shm_t* shmlog;

    // FILE save handler
    pthread_t tid;
    size_t    file_offset;
    FILE*     logfile;
    sem_t*    sem;
    uint32_t  file_blocks; // The maximum count of SHM cache block saving to file
};

static inline uint64_t get_file_page_index(shmlogger_t* logger, size_t offset)
{
    // Store the next block start offset
    uint64_t total_page_index = offset / (logger->config.shm_cache_size >> 1);
    uint64_t file_page_index  = total_page_index;

    if(total_page_index >= logger->file_blocks * 2)
    {
        // Reserve the first block (2 pages), overwriting starts from the second block.
        file_page_index = (total_page_index - 2) % (logger->file_blocks * 2) + 2;
    }
    return file_page_index;
}

static inline void shm_cache_check(shmlogger_t* logger, size_t before, size_t after)
{
    if(logger == NULL)
    {
        return;
    }

    // If higher bits than half of the SHM cache changed
    if((before ^ after) & (logger->config.shm_cache_size >> 1))
    {
        if(sem_post(logger->sem) < 0)
        {
            NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: sem_post error", __func__);
        }

        uint64_t file_page_index = get_file_page_index(logger, after);
        logger->shmlog->page_start_offset[file_page_index] = after;
    }
}

// When 'total_offset' exceeds 'max_file_size', reserve the first SHM cache block, overwrite from the second SHM cache block.
static uint64_t get_file_offset(shmlogger_t* logger, size_t total_offset)
{
    uint64_t total_index = total_offset / logger->config.shm_cache_size;
    uint64_t curr_index  = 0;

    if(total_index > 0)
    {
        // Reserve the first block, overwriting starts from the second block.
        curr_index = (total_index - 1) % (logger->file_blocks - 1) + 1;
    }

    if(curr_index > logger->file_blocks)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: error: curr_index=%lu", __func__, curr_index);
    }

    uint64_t curr_offset = curr_index * logger->config.shm_cache_size + (total_offset & (logger->config.shm_cache_size - 1));

    NVLOGV(TAG, "%s: blocks=%d curr_index=%lu total_offset=0x%lX curr_offset=0x%lX", __func__, logger->file_blocks, curr_index, total_offset, curr_offset);
    return curr_offset;
}

static int save_half_shm_cache_to_file(shmlogger_t* logger, size_t size)
{
    if(logger == NULL)
    {
        return 0;
    }
    char* start = logger->shmlog->shmbuf + (logger->file_offset & (logger->config.shm_cache_size - 1));

    // Overwrite if file_offset exceeds maximum log file size
    uint64_t log_file_offset = get_file_offset(logger, logger->file_offset);
    if(fseek(logger->logfile, log_file_offset, SEEK_SET) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: fseek error: log_file_offset=%llu err=%d - %s", __func__, log_file_offset, errno, strerror(errno));
        return -1;
    }

    NVLOGI(TAG, "%s: fwrite start: size=0x%lX file_offset=0x%lX write_offset=0x%lX", __func__, size, logger->file_offset, log_file_offset);

    size_t wrote = fwrite(start, 1, size, logger->logfile);
    fflush(logger->logfile);
    if(wrote > 0)
    {
        logger->file_offset += wrote;
    }
    if(wrote != size)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: fwrite error: size=%ld wrote=%ld err=%d - %s", __func__, size, wrote, errno, strerror(errno));
        return -1;
    }
    else
    {
        uint64_t total_saved = atomic_fetch_add(&logger->shmlog->total_saved, size) + size;
        int64_t remaining = atomic_load(&logger->shmlog->offset) - total_saved;
        if (remaining < 0 || remaining > (logger->config.shm_cache_size >> 1)) {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: fwrite saving error: remaining=%ld size=0x%lX wrote=0x%lX file_offset=0x%lX total_saved=0x%lX write_offset=0x%lX",
                    __func__, remaining, size, wrote, logger->file_offset, total_saved, log_file_offset);
        } else {
            NVLOGI(TAG, "%s: fwrite ok: remaining=%ld size=0x%lX wrote=0x%lX file_offset=0x%lX total_saved=0x%lX write_offset=0x%lX",
                    __func__, remaining, size, wrote, logger->file_offset, total_saved, log_file_offset);
        }
        return 0;
    }
}

static sem_t* semaphore_create(const char* name, int primary)
{
    NVLOGI(TAG, "%s: name=%s, primary=%d", __func__, name, primary);
    sem_t* sem = sem_open(name, O_CREAT, 0600, 0);
    if(sem == SEM_FAILED)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "sem_open failed: name = %s", name);
        return NULL;
    }

    if(primary)
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

static void* log_file_save_thread(void* shmlogger_arg)
{
    shmlogger_t* logger = (shmlogger_t*)shmlogger_arg;
    if(logger == NULL)
    {
        return 0;
    }

    if(logger->config.cpu_core_id >= 0)
    {
        cpu_set_t mask;
        CPU_ZERO(&mask);
        CPU_SET(logger->config.cpu_core_id, &mask);
        if(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) != 0)
        {
            NVLOGW(TAG, "%s: set thread core failed", __func__);
        }
        else
        {
            NVLOGI(TAG, "%s: set thread shmlogger_%s core to %d", __func__, logger->name, logger->config.cpu_core_id);
        }
    }

    long counter = 0;
    long total   = 0;
    while(sem_wait(logger->sem) >= 0)
    {
        if(atomic_load(&logger->log_state) != SHMLOG_STATE_CLOSED)
        {
            save_half_shm_cache_to_file(logger, (logger->config.shm_cache_size >> 1));
            counter++;
        }
        else
        {
            // size_t size = (atomic_load(&logger->shmlog->offset) - logger->file_offset) & (logger->config.shm_cache_size - 1);
            // save_half_shm_cache_to_file(logger, size);
            break;
        }
    }
    return NULL;
}

static int log_file_open(shmlogger_t* logger, const char* name)
{
    // Create or lookup the semaphore
    logger->sem = semaphore_create(name, logger->primary);

    if(logger->primary)
    {
        char path[SHMLOG_NAME_MAX_LEN + NV_PATH_MAX_LEN];

        // Create folder if not exist
        snprintf(path, SHMLOG_NAME_MAX_LEN + NV_PATH_MAX_LEN, "mkdir -p %s", LOG_TEMP_FILE_PATH);
        if(system(path) != 0)
        {
            NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: %s: err=%d - %s", __func__, path, errno, strerror(errno));
        }

        // Set the temporary log file path
        snprintf(path, SHMLOG_NAME_MAX_LEN + NV_PATH_MAX_LEN, "%s/%s", LOG_TEMP_FILE_PATH, name);

        // Open a temperate file to store the logs
        if((logger->logfile = fopen(path, "w")) == NULL)
        {
            NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: failed to open file %s", __func__, path);
            return -1;
        }

        if(pthread_create(&logger->tid, NULL, log_file_save_thread, logger) != 0)
        {
            NVLOGE_NO(TAG, AERIAL_THREAD_API_EVENT, "%s: thread create failed", __func__);
            return -1;
        }

        char thread_name[SHMLOG_NAME_MAX_LEN];
        int ret = snprintf(thread_name, 16, "pcap_%s", name); // Thread name length has to be <= 15 characters
        if (ret < 0) abort();
        if(pthread_setname_np(logger->tid, thread_name) != 0)
        {
            NVLOGE_NO(TAG, AERIAL_THREAD_API_EVENT, "%s: set thread name %s failed", __func__, thread_name);
        }
    }
    return 0;
}

static size_t shm_logger_round_save(shmlogger_t* logger, size_t total_offset, int size, const char* buf)
{
    if(size <= 0 || buf == NULL)
    {
        return total_offset;
    }

    size_t offset = total_offset & (logger->config.shm_cache_size - 1);

    if(offset + size <= logger->config.shm_cache_size)
    {
        memcpy(logger->shmlog->shmbuf + offset, buf, size);
    }
    else
    {
        // SHM cache overflow, round to the start
        size_t len = logger->config.shm_cache_size - offset;
        memcpy(logger->shmlog->shmbuf + offset, buf, len);
        memcpy(logger->shmlog->shmbuf, buf + len, size - len);
    }
    return total_offset + size;
}

void shmlogger_save_ipc_msg(shmlogger_t* logger, nv_ipc_msg_t* msg, int32_t flags)
{
    if(logger == NULL || msg == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: invalid parameters: msg=%p flags=%d", __func__, msg, flags);
        return;
    }

    int data_size_limit = 0; // DATA part size to save
    if(msg->data_len > 0 && msg->data_pool == NV_IPC_MEMPOOL_CPU_DATA)
    {
        data_size_limit = msg->data_len > get_pcap_max_data_size() ? get_pcap_max_data_size() : msg->data_len;
    }

    // Get the SHM buffer and copy to it
    record_t record;
    record.buf_size = msg->msg_len + data_size_limit;
    record.flags    = flags;
    gettimeofday(&record.tv, NULL);

    int    record_size = get_record_size(&record);
    size_t shm_offset  = atomic_fetch_add(&logger->shmlog->offset, record_size);
    NVLOGV(TAG, "%s: offset=0x%lX record_size=%d msg_len=%d data_len=%d", __func__, shm_offset, record_size, msg->msg_len, msg->data_len);

    size_t new_offset = shm_offset;
    // record_t header
    new_offset = shm_logger_round_save(logger, new_offset, sizeof(record_t), (const char*)&record);
    // MSG part
    new_offset = shm_logger_round_save(logger, new_offset, msg->msg_len, msg->msg_buf);
    // DATA part
    new_offset = shm_logger_round_save(logger, new_offset, data_size_limit, msg->data_buf);
    // total size
    new_offset = shm_logger_round_save(logger, new_offset, sizeof(record.buf_size), (const char*)&record.buf_size);

    if (shm_offset + record_size != new_offset)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: error round_save: old_offset=%llu new_offset=%llu size=%d", __func__, shm_offset, new_offset, record_size);
    }
    if(logger->config.save_to_file)
    {
        // If half of the SHM size was filled, save the cached half to file.
        shm_cache_check(logger, shm_offset, new_offset);
    }
}

void shmlogger_save_buffer(shmlogger_t* logger, const char* buffer, int32_t size, int32_t flags)
{
    if(buffer == NULL || size <= 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: invalid parameters: buffer=%p size=%d", __func__, buffer, size);
        return;
    }
    // Get the SHM buffer and copy to it
    record_t record;
    record.buf_size = size;
    record.flags    = flags;

    int    record_size = get_record_size(&record);
    size_t shm_offset  = atomic_fetch_add(&logger->shmlog->offset, record_size);

    NVLOGV(TAG, "%s: offset=0x%lX record_size=%d", __func__, shm_offset, record_size);

    gettimeofday(&record.tv, NULL);

    size_t new_offset = shm_offset;
    new_offset = shm_logger_round_save(logger, new_offset, sizeof(record_t), (const char*)&record);
    new_offset = shm_logger_round_save(logger, new_offset, size, buffer);
    new_offset = shm_logger_round_save(logger, new_offset, sizeof(record.buf_size), (const char*)&record.buf_size);

    if(logger->config.save_to_file)
    {
        // If half of the SHM size was filled, save the cached half to file.
        shm_cache_check(logger, shm_offset, new_offset);
    }
}

shmlogger_t* shmlogger_open(int primary, const char* name, shmlogger_config_t* cfg)
{
    int size = sizeof(shmlogger_t);

    shmlogger_t* logger = malloc(size);
    if(logger == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: malloc failed", __func__);
        return NULL;
    }
    memset(logger, 0, size);
    snprintf(logger->name, SHMLOG_NAME_MAX_LEN, "%s", (name == NULL ? SHMLOG_DEFAULT_NAME : name));

    atomic_store(&logger->log_state, SHMLOG_STATE_INIT);

    if(primary)
    {
        logger->primary = 1;
    }
    else
    {
        logger->primary = 0;
    }

    logger->module_type = logger->primary;

    logger->config.save_to_file   = cfg->save_to_file;
    logger->config.shm_cache_size = cfg->shm_cache_size;
    logger->config.max_file_size  = cfg->max_file_size;
    logger->config.cpu_core_id    = cfg->cpu_core_id;

    // Set shm_cache_size to be 2^n, n=highest_bit-1
    int highest_bit = 0;
    while(logger->config.shm_cache_size > 0)
    {
        logger->config.shm_cache_size >>= 1;
        highest_bit++;
    }
    if(highest_bit > 0)
    {
        logger->config.shm_cache_size = 1L << (highest_bit - 1);
    }

    // Open a SHM memory pool, allocate one more buffer at the end for memcpy overflow
    char shm_file_name[SHMLOG_NAME_MAX_LEN + 8];
    snprintf(shm_file_name, SHMLOG_NAME_MAX_LEN + 8, "%s", logger->name);

    size_t shm_size = sizeof(shmlogger_shm_t) + logger->config.shm_cache_size;
    logger->shmpool = nv_ipc_shm_open(logger->primary, shm_file_name, shm_size);
    if(logger->shmpool == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: nv_ipc_shm_open failed", __func__);
        free(logger);
        return NULL;
    }
    logger->shmlog = logger->shmpool->get_mapped_addr(logger->shmpool);
    if(logger->primary)
    {
        logger->file_blocks          = logger->config.max_file_size / logger->config.shm_cache_size;
        logger->config.max_file_size = (uint64_t)logger->file_blocks * logger->config.shm_cache_size;

        // Clear the old logs
        memset(logger->shmlog, 0, shm_size);
        atomic_store(&logger->shmlog->offset, 0);
        atomic_store(&logger->shmlog->counter, 0);
        atomic_store(&logger->shmlog->total_saved, 0);
        atomic_store(&logger->shmlog->max_file_size, logger->config.max_file_size);
    }
    else
    {
        logger->config.shm_cache_size = logger->shmpool->get_size(logger->shmpool) - sizeof(shmlogger_shm_t);
        logger->config.max_file_size  = atomic_load(&logger->shmlog->max_file_size);
        logger->file_blocks           = logger->config.max_file_size / logger->config.shm_cache_size;
    }

    if(logger->file_blocks < 2)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: Error: max_file_size (0x%lX) has to be at lest 2 times large than shm_cache_size (0x%lX)", __func__, logger->config.max_file_size, logger->config.shm_cache_size);
        free(logger);
        return NULL;
    }

    if(logger->config.save_to_file)
    {
        log_file_open(logger, logger->name);
    }

    if(atomic_fetch_add(&logger_counter, 1) == 0)
    {
        shmlogger_default = logger;
    }

    char           date[30] = "";
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm _tm;
    if(localtime_r(&tv.tv_sec, &_tm) != NULL)
    {
        // Total size is 28
        strftime(date, sizeof("1970-01-01 00:00:00"), "%Y-%m-%d %H:%M:%S", &_tm);
    }
    // Below log will be written to SHM
    NVLOGI(TAG, "===== SHM logger %s shm_size=0x%lX file_size=0x%lX total_offset=0x%lX total_saved=0x%lX blocks=%d opened at %s =====",
            logger->name, logger->config.shm_cache_size, logger->config.max_file_size, logger->shmlog->offset, logger->shmlog->total_saved, logger->file_blocks, date);
    return logger;
}

int shmlogger_close(shmlogger_t* logger)
{
    if(logger != NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: name=%s", __func__, logger->name);

        if(shmlogger_default == logger)
        {
            shmlogger_default = NULL;
        }

        if(logger->primary)
        {
            // Notify the log file saving thread to exit
            atomic_store(&logger->log_state, SHMLOG_STATE_CLOSED);
            sem_post(logger->sem);

            if(pthread_join(logger->tid, NULL) != 0)
            {
                NVLOGE_NO(TAG, AERIAL_THREAD_API_EVENT, "%s: task join failed", __func__);
            }

            if(logger != NULL && logger->logfile != NULL)
            {
                fclose(logger->logfile);
            }
        }

        if(logger->shmpool->close(logger->shmpool) < 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: shmpool close failed", __func__);
        }
        free(logger);
    }
    return 0;
}

/********************************************************************************************
 *  Below are for collecting /dev/shm/${name}.log and ${tmp_path}/${name.log} to one log file
 *******************************************************************************************/
static int collect_file_log(shmlogger_t* logger, const char* src_path, const char* dst_path)
{
    int   ret    = 0;
    uint64_t size = 0;
    char* buf = NULL;
    FILE* fp_src = fopen(src_path, "r+"); // The file /var/log/aerial/${prefix}_pcap
    FILE* fp_dst = fopen(dst_path, "w+");
    if(fp_src == NULL || fp_dst == NULL)
    {
        NVLOGE_NO(TAG_LOG_COLLECT, AERIAL_SYSTEM_API_EVENT, "%s: fopen failed: fp_src=0x%p fp_dst=0x%p", __func__, fp_src, fp_dst);
        goto exit_clean;
    }

    uint64_t total_saved = atomic_load(&logger->shmlog->total_saved);
    uint64_t total_shm_offset = atomic_load(&logger->shmlog->offset);
    fseek(fp_src, 0, SEEK_END);
    NVLOGC(TAG, "%s: tmp_file_size=0x%lX total_saved=0x%lX total_shm_offset=0x%lX half_cache_size=0x%lX",
            __func__, ftell(fp_src), total_saved, total_shm_offset, (logger->config.shm_cache_size >> 1));
    if (ftell(fp_src) % (logger->config.shm_cache_size >> 1) != 0)
    {
        if (total_shm_offset < total_saved + (logger->config.shm_cache_size >> 1))
        {
            NVLOGE_NO(TAG_LOG_COLLECT, AERIAL_NVIPC_API_EVENT, "%s: less than half cache remains: total_saved=0x%lX total_shm_offset=0x%lX half_cache=0x%lX",
                    __func__, total_saved, total_shm_offset, (logger->config.shm_cache_size >> 1));
        }
        else
        {
            NVLOGC(TAG_LOG_COLLECT, "%s: save the last interrupted half cache: total_saved=0x%lX total_shm_offset=0x%lX half_cache=0x%lX",
                    __func__, total_saved, total_shm_offset, (logger->config.shm_cache_size >> 1));
            logger->file_offset = total_saved;
            logger->logfile = fp_src;
            save_half_shm_cache_to_file(logger, (logger->config.shm_cache_size >> 1));
        }
    }
    fseek(fp_src, 0, SEEK_SET);

    total_saved = atomic_load(&logger->shmlog->total_saved);

    if ((buf = malloc(logger->config.shm_cache_size)) == NULL)
    {
        ret = -1;
        NVLOGE_NO(TAG, TAG_LOG_COLLECT, "%s: malloc failed", __func__);
        goto exit_clean;
    }

    // Copy the first SHM block
    size_t nbytes = fread(buf, 1, logger->config.shm_cache_size, fp_src);
    if(nbytes > 0)
    {
        fwrite(buf, 1, nbytes, fp_dst);
        size += nbytes;
        NVLOGC(TAG_LOG_COLLECT, "%s: copy block 0: nbytes=0x%X=%d MB", __func__, nbytes, nbytes >> 20);
    }

    if(total_saved > logger->config.max_file_size)
    {
        uint64_t curr_offset = get_file_offset(logger, total_saved);
        double lost        = (double)(total_saved - logger->config.max_file_size) / 1048576;
        NVLOGC(TAG_LOG_COLLECT, "===============================================================================");
        NVLOGC(TAG_LOG_COLLECT, "NOTE: %1.1f MB logs were lost. total_saved=0x%lX curr_offset=0x%lX", lost, total_saved, curr_offset);
        NVLOGC(TAG_LOG_COLLECT, "===============================================================================");

        if(fseek(fp_src, curr_offset, SEEK_SET) < 0)
        {
            NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: fseek error: log_file_offset=%ld err=%d - %s", __func__, curr_offset, errno, strerror(errno));
            ret = -1;
            goto exit_clean;
        }
        while((nbytes = fread(buf, 1, logger->config.shm_cache_size / 2, fp_src)) > 0)
        {
            fwrite(buf, 1, nbytes, fp_dst);
            size += nbytes;
            NVLOGC(TAG_LOG_COLLECT, "%s: copy block 1: nbytes=0x%X w_pos=0x%lX r_pos=0x%lX", __func__, nbytes, ftell(fp_dst) - nbytes, ftell(fp_src) - nbytes);
        }
    }

    if(fseek(fp_src, logger->config.shm_cache_size, SEEK_SET) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: fseek error: shm_cache_size=%ld err=%d - %s", __func__, logger->config.shm_cache_size, errno, strerror(errno));
        ret = -1;
        goto exit_clean;
    }

    while(size < logger->config.max_file_size)
    {
        size_t to_read = logger->config.max_file_size - size > logger->config.shm_cache_size ? logger->config.shm_cache_size : logger->config.max_file_size - size;
        if((nbytes = fread(buf, 1, to_read, fp_src)) > 0)
        {
            fwrite(buf, 1, nbytes, fp_dst);
            size += nbytes;
            NVLOGC(TAG_LOG_COLLECT, "%s: copy block 2: nbytes=0x%X w_pos=0x%lX r_pos=0x%lX to_read=0x%lX", __func__, nbytes, ftell(fp_dst) - nbytes, ftell(fp_src) - nbytes, to_read);
        }
        else
        {
            break;
        }
    }

exit_clean:
    if(fp_src != NULL)
    {
        fclose(fp_src);
    }
    if(fp_dst != NULL)
    {
        fclose(fp_dst);
    }
    if (buf != NULL)
    {
        free(buf);
    }
    NVLOGI(TAG_LOG_COLLECT, "%s: copied from %s to %s: size=%lu ret=%d", __func__, src_path, dst_path, size, ret);
    return ret;
}

int shmlogger_collect(const char* prefix, const char* type, const char* path)
{
    if(prefix == NULL || type == NULL)
    {
        NVLOGE_NO(TAG_LOG_COLLECT, AERIAL_NVIPC_API_EVENT, "%s: prefix or type is null", __func__);
        return -1;
    }

    nv_ipc_app_config_shmpool_open(0, prefix);
    load_debug_config();

    char logger_name[SHMLOG_NAME_MAX_LEN * 2];
    snprintf(logger_name, SHMLOG_NAME_MAX_LEN * 2, "%s_%s", prefix, type);

    // Copy the temporary file ${tmp_path}/${name}.log to destination
    char tmp_path[NV_PATH_MAX_LEN];
    snprintf(tmp_path, NV_PATH_MAX_LEN, "%s/%s", LOG_TEMP_FILE_PATH, logger_name);

    char record_filepath[NV_PATH_MAX_LEN];
    if(path != NULL)
    {
        snprintf(record_filepath, NV_PATH_MAX_LEN, "%s/%s", path, logger_name);
    }
    else
    {
        snprintf(record_filepath, NV_PATH_MAX_LEN, "%s", logger_name);
    }

    NVLOGI(TAG_LOG_COLLECT, "%s: save %s and /dev/shm/%s logs to %s", __func__, tmp_path, logger_name, record_filepath);

    // Copy unsaved cache from /dev/shm/${name}.log to destination
    shmlogger_config_t config;
    shmlogger_t*       logger = shmlogger_open(0, logger_name, &config);
    if(logger == NULL)
    {
        NVLOGI(TAG_LOG_COLLECT, "%s: no /dev/shm/%s_%s, logger may have been closed normally", __func__, prefix, type);
        return 0;
    }

    size_t total_saved = atomic_load(&logger->shmlog->total_saved);

    uint64_t file_page_index = get_file_page_index(logger, total_saved + logger->config.shm_cache_size);
    uint64_t break_offset = logger->shmlog->page_start_offset[file_page_index];
    for (int i = 0; i < logger->config.max_file_size / (logger->config.shm_cache_size >> 1) + 4; i++)
    {
        NVLOGC(TAG_LOG_COLLECT, "%s: page_start_offset[%d]=0x%lX", __func__, i, logger->shmlog->page_start_offset[i]);
    }
    NVLOGC(TAG_LOG_COLLECT, "%s: break_offset: page_start_offset[%lu]=0x%lX", __func__, file_page_index, break_offset);

    collect_file_log(logger, tmp_path, record_filepath);

    FILE* record_file = fopen(record_filepath, "a+");
    if(record_file == NULL)
    {
        NVLOGE_NO(TAG_LOG_COLLECT, AERIAL_SYSTEM_API_EVENT, "%s: failed to open file %s", __func__, path);
        return -1;
    }

    fseek(record_file, 0, SEEK_END);
    // size_t file_offset = ftell(dst_file);
    total_saved = atomic_load(&logger->shmlog->total_saved);

    size_t size = (atomic_load(&logger->shmlog->offset) - total_saved);
    if(size > logger->config.shm_cache_size)
    {
        NVLOGC(TAG_LOG_COLLECT, "%s: NOTE: shm_cache log had been overridden, some logs were lost.", __func__);
    }

    NVLOGC(TAG_LOG_COLLECT, "%s: tmp_file_logs=0x%lX shm_cache_logs=0x%lX shm_cache_size=0x%lX", __func__, total_saved, size, logger->config.shm_cache_size);

    size &= (logger->config.shm_cache_size - 1);

    int ret = 0;
    while(size > 0)
    {
        size_t shm_offset = total_saved & (logger->config.shm_cache_size - 1);

        size_t to_write = size;
        if(to_write > logger->config.shm_cache_size - shm_offset)
        {
            to_write = logger->config.shm_cache_size - shm_offset;
        }

        char*  start  = logger->shmlog->shmbuf + shm_offset;
        size_t nbytes = fwrite(start, 1, to_write, record_file);
        if(nbytes > 0)
        {
            size -= nbytes;
            total_saved += nbytes;
        }

        NVLOGC(TAG_LOG_COLLECT, "%s: copy block 3: nbytes=0x%lX w_pos=0x%lX shm_cache_offset=0x%lX", __func__, nbytes, ftell(record_file) - nbytes, shm_offset);

        if(nbytes != to_write)
        {
            NVLOGE_NO(TAG_LOG_COLLECT, AERIAL_NVIPC_API_EVENT, "%s: fwrite error: to_write=%ld written=%lu err=%d - %s", __func__, to_write, nbytes, errno, strerror(errno));
            ret = -1;
        }
    }

    char pcap_path[NV_PATH_MAX_LEN];
    if(path != NULL)
    {
        snprintf(pcap_path, NV_PATH_MAX_LEN, "%s/%s.%s", path, prefix, type);
    }
    else
    {
        snprintf(pcap_path, NV_PATH_MAX_LEN, "%s.%s", prefix, type);
    }

    int64_t pcap_size = nv_ipc_convert_pcap(record_file, pcap_path, logger->config.shm_cache_size, total_saved, break_offset);
    if(pcap_size >= 0)
    {
        NVLOGC(TAG_LOG_COLLECT, "%s: successfully converted pcap logs to %s, total_size=0x%lX=%lu pcap_size=%ld=%ld MB",
                __func__, record_filepath, total_saved, total_saved, pcap_size, pcap_size / 1024 / 1024);
    }
    else
    {
        NVLOGE_NO(TAG_LOG_COLLECT, AERIAL_NVIPC_API_EVENT, "%s: Failed to convert pcap logs to %s, total_size=0x%lX=%lu", __func__, record_filepath, total_saved, total_saved);
    }

    fclose(record_file);

    if(remove(record_filepath) != 0)
    {
        NVLOGE_NO(TAG_LOG_COLLECT, AERIAL_SYSTEM_API_EVENT, "%s: remove %s failed", __func__, record_filepath);
    }

#if 0
    NVLOGC(TAG_LOG_COLLECT, "%s: start create zip file. ret=%d", __func__, ret);

    // Create .tar.gz file
    char cmd[NV_PATH_MAX_LEN * 3];
    snprintf(cmd, NV_PATH_MAX_LEN * 3, "gzip -k %s", pcap_path);
    if(system(cmd) != 0)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: run system command failed: %s: err=%d - %s", __func__, path, errno, strerror(errno));
    }
    NVLOGC(TAG_LOG_COLLECT, "%s: finished create zip file. ret=%d", __func__, ret);
#endif

    return ret;
}
