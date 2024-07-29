/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _TEST_COMMON_H_
#define _TEST_COMMON_H_

#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <CUnit/Automated.h>
#include <CUnit/Console.h>

#include "array_queue.h"
#include "nv_ipc.h"
#include "nv_ipc_shm.h"
#include "nv_ipc_efd.h"
#include "nv_ipc_sem.h"
#include "nv_ipc_epoll.h"
#include "nv_ipc_ring.h"
#include "nv_ipc_mempool.h"
#include "nv_ipc_cudapool.h"

#include "test_cuda.h"
#include "stat_log.h"
#include "nv_ipc_utils.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define NUMBER_1E6 (1000L * 1000)
#define NUMBER_1E9 (1000L * 1000 * 1000)

#define IPC_MSG_ID_END -1;

extern int TAG;

extern nv_ipc_transport_t ipc_transport;
extern nv_ipc_module_t    module_type;
extern nv_ipc_t*          ipc;
extern nv_ipc_config_t    cfg;

/***** Multiple thread *********/
#define TASK_NAME_MAX_LEN 256
#define MAX_CPU_CORE_NUM 128
#define MAX_SEND_THREAD_NUM 128
#define MAX_RECV_THREAD_NUM 128

typedef struct {
    int tid;        // Thread ID allocated by user
    int cpu_core;   // CPU core to bind
    long count;
    long param;
    pthread_t pthread_id; // pthread_t ID allocated by system
    void* (*func)(void*);
    void *arg;
    char name[32];
} thread_info_t;

typedef struct {
    atomic_long expected_total_count;
    long total_recv_count;
    int send_threads_num;
    int recv_threads_num;
    int free_core_num;
    int need_exit_msg;
    int cpu_cores[MAX_CPU_CORE_NUM];
    char task_name[TASK_NAME_MAX_LEN];
    thread_info_t send_threads[MAX_SEND_THREAD_NUM];
    thread_info_t recv_threads[MAX_RECV_THREAD_NUM];
} task_info_t;

extern task_info_t task_info;

// The CUDA device ID. Can set to -1 to fall back to CPU memory IPC
extern int test_cuda_device_id;

// Total CPU core number
extern int nproc_num;

void detect_cpu_cores(void);

void set_task_name(const char* task_name);

int assin_cpu_for_thread(int cpu_id);

int is_primary(void);

int assign_cpu_for_process(int cpu);

int send_msg_exit(void);

void sync_primary_first(const char* info);
void sync_primary_end(const char* info);
void sync_secondary_first(const char* info);
void sync_secondary_end(const char* info);
void sync_together(const char* info);

void test_lockless_queue(void);
void test_assign_cpu(void);

typedef enum {
    IPC_DIR_DL = 0,
    IPC_DIR_UL = 1,
    IPC_DIR_DUPLEX = 2,
    IPC_DIR_MAX = 3,
} ipc_dir_t;

void set_task_name(const char* task_name);
int64_t run_duplex_tasks(void* (*send_func)(void*), void* (*recv_func)(void*), void *arg,
        int send_threads_num, int recv_threads_num, long total_count, int need_exit_msg);

int init_perf_result();
int print_perf_result();

static inline void cpu_nop_time(int count)
{
    volatile int i = 0;
    volatile int j = 0;
    for(i = 0; i < count; i++)
    {
        j++;
    }
}

#if defined(__cplusplus)
}
#endif

#endif /* _TEST_COMMON_H_ */
