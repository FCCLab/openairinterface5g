/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/queue.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <stdatomic.h>

#include "test_common.h"
#include "test_ipc.h"

#define TAG (NVLOG_TAG_BASE_NVIPC + 28) // "NVIPC.TEST"

/***** Stress test multiply *********/
#define STRESS 1
#define SEND_THREAD_NUM 10
#define RECV_THREAD_NUM 12

#define CPU_NOP_COUNT 10

// #define TEST_CLOCK_SOURCE_ID CLOCK_REALTIME
// #define TEST_CLOCK_SOURCE_ID CLOCK_REALTIME_COARSE
#define TEST_CLOCK_SOURCE_ID CLOCK_MONOTONIC

/***** Ring queue test *********/
#define TEST_ENQUEUE_COUNT_PER_THREAD (1000L * 100 * STRESS)
#define TEST_RING_LEN (128)
#define NUM_THREADS_ENQUEUE SEND_THREAD_NUM
#define NUM_THREADS_DEQUEUE RECV_THREAD_NUM
nv_ipc_ring_t* ring;
pthread_t      enq_ids[NUM_THREADS_ENQUEUE];
long           enq_args[NUM_THREADS_ENQUEUE];
pthread_t      deq_ids[NUM_THREADS_DEQUEUE];
long           deq_args[NUM_THREADS_DEQUEUE];

/***** CPU memory pool test *********/
#define TEST_CPU_MEMPOOL_LOOP (100 * STRESS)
#define TEST_CPU_MEMPOOL_LEN 1024

/***** GPU memory pool test *********/

/***** IPC transfer test *********/
#define TEST_LOG_TRANSFER_TIMING 1
#define TEST_SYNC_TRANSFER_COUNT (1000L * 100 * STRESS)
#define TEST_NO_SYNC_TRANSFER_COUNT (1000L * 200 * STRESS)
#define TEST_DATA_BUF_LEN 256

#define FAPI_MSG_SIZE (8000)          // Actual FAPI message size
#define FAPI_DATA_SIZE (500 * 1024L) // Actual PDU size

/***** Multiple thread IPC transfer *********/
#define TEST_COUNT_PER_SEND_THREAD (20L * 1000L * STRESS)
#define TEST_RECV_THREAD_NUM RECV_THREAD_NUM
#define TEST_SEND_THREAD_NUM SEND_THREAD_NUM

/***** Variables *********/
int              sync_mode = 0; // 0 - blocking; 1 - epoll; 3 - no sync (infinite loop polling).
char             cpu_buf_send[TEST_DATA_BUF_LEN];
char             cpu_buf_recv[TEST_DATA_BUF_LEN];
const test_msg_t msg_no_data   = {1, FAPI_MSG_SIZE, 0, -1};
const test_msg_t msg_cpu_data  = {2, FAPI_MSG_SIZE, FAPI_DATA_SIZE, NV_IPC_MEMPOOL_CPU_DATA};
const test_msg_t msg_cuda_data = {3, FAPI_MSG_SIZE, FAPI_DATA_SIZE, NV_IPC_MEMPOOL_CUDA_DATA};
const test_msg_t msg_exit      = {-1, FAPI_MSG_SIZE, 0, -1};

atomic_long send_order;
atomic_long recv_order;

#define MSG_VERIFY_BUF_LEN (1000 * 1000 * 2)
msg_verify_t msg_verify[SEND_THREAD_NUM][MSG_VERIFY_BUF_LEN];

typedef enum {
    PERF_TEST_DL = 0,
    PERF_TEST_UL = 1,
    PERF_TEST_DUPLEX = 2,
    PERF_TEST_DUPLEX_MT = 3,
    PERF_TEST_NUM = 4,
} perf_case_t;

typedef struct {
    perf_case_t perf_case;
    int64_t total_count;
    int64_t total_time;
    int64_t average_time;
} ipc_perf_t;

char perf_case_name[PERF_TEST_NUM][32] = { "DL:", "UL:", "DUPLEX:", "DUPLEX_MT:" };
ipc_perf_t perf_results[PERF_TEST_NUM];
int perf_test_count = 0;

int init_perf_result() {
    perf_test_count = 0;
    for (int i = 0; i < PERF_TEST_NUM; i ++) {
        perf_results[i].total_count = 0;
    }
    return 0;
}

int print_perf_result() {
    if (perf_test_count == 0) {
        return 0;
    }
    NVLOGC(TAG, "Performance test result:");
    for (int i = 0; i < PERF_TEST_NUM; i++) {
        if (perf_results[i].total_count > 0) {
            NVLOGC(TAG, "  %-10s msg_count:%8ld | total_time: %6ldms | average_time:%6ldns",
                    perf_case_name[i], perf_results[i].total_count,
                    perf_results[i].total_time / 1000000L, perf_results[i].average_time);
        }
    }
    return 0;
}

int update_perf_result(perf_case_t perf_case, int64_t total_count, int64_t total_time) {
    if (perf_case >= PERF_TEST_NUM) {
        return -1;
    }
    NVLOGC(TAG, "%s: perf_case=%d total_count=%ld total_time=%ld", __func__, perf_case, total_count,
            total_time);

    perf_results[perf_case].total_count = total_count;
    perf_results[perf_case].total_time = total_time;
    perf_results[perf_case].average_time = total_time / total_count;
    perf_test_count ++;
    return 0;
}

/***** Multiple thread *********/

/////////////////////////////////////////////////////////////////////
int is_msg_exit(nv_ipc_msg_t* msg)
{
    if(msg != NULL && msg->msg_id < 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static int dummy_alloc_free(nv_ipc_t* ipc)
{
    nv_ipc_msg_t send_msg2;
    send_msg2.msg_id    = msg_cpu_data.msg_id;
    send_msg2.msg_len   = msg_cpu_data.msg_len;
    send_msg2.data_len  = msg_cpu_data.data_len;
    send_msg2.data_pool = msg_cpu_data.data_pool;
    // Allocate buffer for TX message
    if(ipc->tx_allocate(ipc, &send_msg2, 0) != 0)
    {
        NVLOGV(TAG, "%s error: test allocate TX buffer failed", __func__);
        return -1;
    }

    if(ipc->tx_release(ipc, &send_msg2) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s error: test release TX buffer failed", __func__);
        return -1;
    }
    return 0;
}

// Always allocate message buffer, but allocate data buffer only when data_len > 0
int test_nv_ipc_send(nv_ipc_t* ipc, nv_ipc_msg_t* send_msg, test_msg_t* test_msg, int tid)
{
    send_msg->msg_id    = test_msg->msg_id;
    send_msg->msg_len   = test_msg->msg_len;
    send_msg->data_len  = test_msg->data_len;
    send_msg->data_pool = test_msg->data_pool;

    // Allocate buffer for TX message
    if(ipc->tx_allocate(ipc, send_msg, 0) != 0)
    {
        NVLOGV(TAG, "%s error: allocate TX buffer failed", __func__);
        return -1;
    }

    buf_obj_t* msg_obj = send_msg->msg_buf;
    msg_obj->tid       = tid;
    msg_obj->counter   = test_msg->counter;

    if(TEST_LOG_TRANSFER_TIMING)
    {
        if(clock_gettime(TEST_CLOCK_SOURCE_ID, &msg_obj->time_send) < 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s clock_gettime %s", __func__, strerror(errno));
        }
        msg_obj->order_send = atomic_fetch_add(&send_order, 1);
    }

    int try_count = 0;
    // Send the message
    while(ipc->tx_send_msg(ipc, send_msg) < 0)
    {
        cpu_nop_time(100);
        try_count++;
    }

    if(try_count > 10)
    {
        NVLOGW(TAG, "%s ring queue may be full, send try_count=%d", __func__, try_count);
    }

    return 0;
}

// Always allocate message buffer, but allocate data buffer only when data_len > 0
int test_nv_ipc_recv(nv_ipc_t* ipc, nv_ipc_msg_t* recv_msg, msg_verify_t** receive_verify)
{
    recv_msg->msg_buf  = NULL;
    recv_msg->data_buf = NULL;
    recv_msg->data_len = 1000;

    struct timespec before_recv;
    long            middle_order;
    if(TEST_LOG_TRANSFER_TIMING)
    {
        if(clock_gettime(TEST_CLOCK_SOURCE_ID, &before_recv) < 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s clock_gettime %s", __func__, strerror(errno));
        }
        middle_order = atomic_fetch_add(&recv_order, 1);
    }

    if(ipc->rx_recv_msg(ipc, recv_msg) < 0)
    {
        NVLOGV(TAG, "%s: no more message available", __func__);
        return -1;
    }

    buf_obj_t*    msg_obj = recv_msg->msg_buf;
    msg_verify_t* verify  = &msg_verify[msg_obj->tid][msg_obj->counter % MSG_VERIFY_BUF_LEN];

    if(TEST_LOG_TRANSFER_TIMING)
    {
        if(clock_gettime(TEST_CLOCK_SOURCE_ID, &verify->after_recv) < 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s clock_gettime %s", __func__, strerror(errno));
        }
        verify->order_after         = atomic_fetch_add(&recv_order, 1);
        verify->order_middle        = middle_order;
        verify->order_before        = msg_obj->order_send;
        verify->before_send.tv_sec  = msg_obj->time_send.tv_sec;
        verify->before_send.tv_nsec = msg_obj->time_send.tv_nsec;
        verify->before_recv.tv_sec  = before_recv.tv_sec;
        verify->before_recv.tv_nsec = before_recv.tv_nsec;
    }
    verify->counter = msg_obj->counter;
    verify->tid     = msg_obj->tid;

    if(ipc->rx_release(ipc, recv_msg) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: IPC release error", __func__);
    }
    *receive_verify = verify;
    return 0;
}

int send_msg_exit(void)
{
    nv_ipc_msg_t send_msg;

    test_msg_t test_msg;
    memcpy(&test_msg, &msg_exit, sizeof(test_msg_t));

    while(test_nv_ipc_send(ipc, &send_msg, &test_msg, 0) < 0)
    {
        // Retry until succeed
    }
    if(sync_mode == 1)
    {
        return ipc->tx_tti_sem_post(ipc);
    }
    else if(sync_mode == 2)
    {
        return ipc->notify(ipc, 1);
    }
    else if(sync_mode == 3)
    {
        return 0;
    }
    else
    {
        return 0;
    }
}

void test_send_task(long total_count, int tid)
{
    struct timespec now, last;

    char stat_name[32];
    sprintf(stat_name, "SEND_%02d", tid);
    stat_log_t* stat_send = stat_log_open(stat_name, STAT_MODE_TIMER, NUMBER_1E9);
    if(stat_send == NULL)
    {
        return;
    }

    long send_counter = 0, idle_count = 0;

    test_msg_t test_msg;
    memcpy(&test_msg, &msg_cpu_data, sizeof(test_msg_t));

    nv_ipc_msg_t msg;
    long         i;
    for(i = 0; i < total_count; i++)
    {
        test_msg.counter = i;
        test_msg.msg_id = i; // TODO
        while(test_nv_ipc_send(ipc, &msg, &test_msg, tid) < 0)
        {
            NVLOGV(TAG, "send ipc message failed; total_sent=%ld", send_counter);
            cpu_nop_time(CPU_NOP_COUNT);
            idle_count++;
        }
        send_counter++;
        NVLOGD(TAG, "send ipc message succeed; send_counter=%ld", send_counter);

        if(sync_mode == 1)
        {
            ipc->tx_tti_sem_post(ipc);
        }
        else if(sync_mode == 2)
        {
            ipc->notify(ipc, 1);
        }
        else if(sync_mode == 3)
        {
        }
        else
        {
        }

        if(TEST_LOG_TRANSFER_TIMING)
        {
            if(clock_gettime(TEST_CLOCK_SOURCE_ID, &now) < 0)
            {
                NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s clock_gettime %s", __func__, strerror(errno));
            }
            if(i > 0)
            {
                stat_send->add(stat_send, nvlog_timespec_interval(&last, &now));
            }
            last.tv_sec  = now.tv_sec;
            last.tv_nsec = now.tv_nsec;
        }
    }

    NVLOGC(TAG, "%s finished: idle_count=%ld send_counter=%ld", __func__, idle_count, send_counter);

    stat_send->print(stat_send);
    stat_send->close(stat_send);
}

// Dequeue order: msg1 < msg2 => Enqueue order: msg1 < msg2
static inline void assert_enqueue_order(msg_verify_t* msg1, msg_verify_t* msg2)
{
    if (ipc_transport != NV_IPC_TRANSPORT_SHM) {
        // TODO: what need to check for non-SHM IPC?
        return;
    }
    // If a RECV thread received msg1, msg2, then msg1.before_send < msg2.after_send
    // TODO: after_send can't be recorded yet, so check after_send instead
    int ret = assert_time_order(msg1->before_send, msg2->after_recv);
    // if (ret == 0 || msg2->counter - msg1->counter != 1) {
    if(ret == 0)
    {
        size_t offset1 = (msg1 - &msg_verify[0][0]);
        size_t offset2 = (msg2 - &msg_verify[0][0]);
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: counter:%ld-%ld offset=%lu-%lu time:%ld.%ld-%ld.%ld interval=%ld", __func__, msg1->counter, msg2->counter, offset1, offset2, msg1->before_send.tv_sec, msg1->before_send.tv_nsec, msg2->after_recv.tv_sec, msg2->after_recv.tv_nsec, nvlog_timespec_interval(&msg1->before_send, &msg2->after_recv));
    }

    CU_ASSERT_EQUAL(ret, 1);
}

// Enqueue order: msg1 < msg2 => Dequeue order: msg1 < msg2
static inline void assert_dequeue_order(msg_verify_t* msg1, msg_verify_t* msg2)
{
    if (ipc_transport != NV_IPC_TRANSPORT_SHM) {
        // TODO: what need to check for non-SHM IPC?
        return;
    }
    // For message sent by the same thread msg1, msg2, then msg1.before_recv < msg2.after_recv
    int ret = assert_time_order(msg1->before_recv, msg2->after_recv);
    if(ret == 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: counter:%ld-%ld time:%ld.%ld-%ld.%ld interval=%ld", __func__, msg1->counter, msg2->counter, msg1->before_recv.tv_sec, msg1->before_recv.tv_nsec, msg2->after_recv.tv_sec, msg2->after_recv.tv_nsec, nvlog_timespec_interval(&msg1->before_recv, &msg2->after_recv));
    }
    CU_ASSERT_EQUAL(ret, 1);
}

long test_recv_task(int tid)
{
    nv_ipc_msg_t    recv_msg;
    nv_ipc_epoll_t* ipc_epoll;
    if(sync_mode == 2)
    {
        ipc_epoll = ipc_epoll_create(1, ipc->get_fd(ipc));
        CU_ASSERT_NOT_EQUAL_FATAL(ipc_epoll, NULL);
    }

    char stat_name[32];
    sprintf(stat_name, "RECV_%02d", tid);
    stat_log_t* stat_recv = stat_log_open(stat_name, STAT_MODE_TIMER, NUMBER_1E9);

    msg_verify_t* recv_verify      = NULL;
    msg_verify_t* last_recv_verify = NULL;

    long recv_counter = 0, idle_count = 0;
    while(1)
    {
        if(sync_mode == 1)
        {
            ipc->rx_tti_sem_wait(ipc);
        }
        else if(sync_mode == 2)
        {
            ipc_epoll_wait(ipc_epoll);
            ipc->get_value(ipc);
        }
        else if(sync_mode == 3)
        {
        }
        else
        {
        }

        while(1)
        {
            if(test_nv_ipc_recv(ipc, &recv_msg, &recv_verify) < 0)
            {
                NVLOGV(TAG, "recv ipc message empty: recv_counter=%ld", recv_counter);
                cpu_nop_time(CPU_NOP_COUNT);
                idle_count++;
                break;
            }
            else
            {
                buf_obj_t* msg_obj = (buf_obj_t*)(recv_msg.msg_buf);

                if(is_msg_exit(&recv_msg))
                {
                    if(sync_mode == 2)
                    {
                        ipc_epoll_destroy(ipc_epoll);
                    }
                    stat_recv->print(stat_recv);
                    stat_recv->close(stat_recv);

                    NVLOGC(TAG, "%s finished: idle_count=%ld recv_counter=%ld", __func__, idle_count, recv_counter);
                    // CU_ASSERT_EQUAL(recv_counter, total_count);
                    return recv_counter;
                }

                if(TEST_LOG_TRANSFER_TIMING)
                {
                    stat_recv->add(stat_recv, nvlog_timespec_interval(&recv_verify->before_send, &recv_verify->after_recv));
                    if(recv_verify->counter >= MSG_VERIFY_BUF_LEN / 2)
                    {
                        // Check the order of previous dequeued messages
                        int count1 = (recv_verify->counter - MSG_VERIFY_BUF_LEN / 2) % MSG_VERIFY_BUF_LEN;
                        int count2 = (count1 + 1) % MSG_VERIFY_BUF_LEN;
                        int tid    = recv_verify->tid;
                        if(msg_verify[tid][count1].counter + MSG_VERIFY_BUF_LEN / 2 == recv_verify->counter && msg_verify[tid][count1].counter + 1 == msg_verify[tid][count2].counter)
                        {
                            assert_dequeue_order(&msg_verify[tid][count1], &msg_verify[tid][count2]);
                        }
                        else
                        {
                            // TODO: what should do for non-SHM IPC
                            NVLOGC(TAG, "%s message for check not received yet: %ld-%ld-%ld", __func__, msg_verify[tid][count1].counter, msg_verify[tid][count2].counter, recv_verify->counter);
                        }
                    }

                    if(last_recv_verify != NULL)
                    {
                        assert_enqueue_order(last_recv_verify, recv_verify);
                    }

                    last_recv_verify = recv_verify;
                }

                recv_counter++;
            }
        }
    }
}

// direction: 0 - DL; 1 - UL;
static void test_transfer(int mode, ipc_dir_t direction)
{
    char info[32];
    sprintf(info, "test_transfer_%d", mode);
    sync_together(info);
    atomic_store(&send_order, 0);
    atomic_store(&recv_order, 0);

    struct timespec ts_start;
    nvlog_gettime_rt(&ts_start);

    sync_mode = mode;
    long total_count;
    if(sync_mode == 3)
    {
        total_count = TEST_NO_SYNC_TRANSFER_COUNT;
    }
    else
    {
        total_count = TEST_SYNC_TRANSFER_COUNT;
    }

    if(is_primary() ^ direction)
    {
        long recv_count = test_recv_task(0);
        CU_ASSERT_EQUAL(recv_count, total_count);
    }
    else
    {
        test_send_task(total_count, 0);
        send_msg_exit();
    }

    int64_t ns = nvlog_get_interval(&ts_start);
    NVLOGC(TAG, "%s: finished, total_count=%ld total_time=%ldms average_time=%ldns", __func__,
            total_count, ns / 1000L / 1000L, ns / total_count);

    if (sync_mode == 3) {
        perf_case_t perf_case = direction;
        update_perf_result(perf_case, total_count, ns);
    }
}

void* ipc_recv_task(void* arg)
{
    thread_info_t* info = (thread_info_t*)arg;
    NVLOGC(TAG, "%s: run task %d, count=%ld", __func__, info->tid, info->count);
    info->count = test_recv_task(info->tid);
    NVLOGC(TAG, "%s: exit task %d, receive_count=%ld", __func__, info->tid, info->count);
    return NULL;
}

void* ipc_send_task(void* arg)
{
    thread_info_t* info = (thread_info_t*)arg;
    NVLOGC(TAG, "%s: run task %d, count=%ld", __func__, info->tid, info->count);
    test_send_task(info->count, info->tid);
    NVLOGC(TAG, "%s: exit task %d, send_count=%ld", __func__, info->tid, info->count);
    return NULL;
}

void* ring_enqueue_func(void* arg)
{
    long* val = (long*)arg;
    NVLOGC(TAG, "%s: start thread %ld", __func__, *val);

    long          enqueue_count = 0, idle_count = 0;
    ring_object_t obj;
    obj.msg_len    = 100;
    obj.data_len   = 200;
    obj.msg_index  = 5;
    obj.data_index = 3;
    obj.data_pool  = 1;

    long i;
    for(i = 0; i < TEST_ENQUEUE_COUNT_PER_THREAD; i++)
    {
        obj.msg_id = *val;
        while(ring->enqueue(ring, &obj))
        {
            cpu_nop_time(CPU_NOP_COUNT);
            idle_count++;
        }
        enqueue_count++;
    }

    NVLOGC(TAG, "%s: exit thread %ld", __func__, *val);
    *val = enqueue_count;
    return NULL;
}

void* ring_dequeue_func(void* arg)
{
    long* val = (long*)arg;
    NVLOGC(TAG, "%s: start thread %ld", __func__, *val);

    long          dequeue_count = 0, idle_count = 0;
    ring_object_t obj;
    do
    {
        while(ring->dequeue(ring, &obj) < 0)
        {
            cpu_nop_time(CPU_NOP_COUNT);
            idle_count++;
        }
        dequeue_count++;
    } while(obj.msg_id >= 0);

    NVLOGC(TAG, "%s: exit thread %ld, dequeue_count=%ld", __func__, *val, dequeue_count);

    *val = dequeue_count - 1;
    return NULL;
}

void multi_thread_run(void* (*func)(void*), thread_info_t info[], int num, const char* name)
{
    NVLOGC(TAG, "%s: start %d threads of %s", __func__, num, name);

    char thread_name[NVLOG_NAME_MAX_LEN + 16];

    int i;
    for(i = 0; i < num; i++)
    {
        info[i].tid = i;
        if(pthread_create(&info[i].pthread_id, NULL, func, &info[i]) != 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: pthread_create failed: i=%d", __func__, i);
        }

        snprintf(thread_name, 16, "%s_%02d", name, i);  // Thread name length has to be <= 15 characters
        if(pthread_setname_np(info[i].pthread_id, thread_name) != 0)
        {
            NVLOGE_NO(TAG, AERIAL_THREAD_API_EVENT, "%s: set thread name %s failed", __func__, thread_name);
        }
    }

    for(i = 0; i < num; i++)
    {
        if(pthread_join(info[i].pthread_id, NULL) != 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: pthread_join failed: i=%d", __func__, i);
        }
    }
    NVLOGC(TAG, "%s: %d threads of %s finished", __func__, num, name);
}

void ring_multi_thread_dequeue(long total_count)
{
    int i;
    for(i = 0; i < NUM_THREADS_DEQUEUE; i++)
    {
        deq_args[i] = i;
        if(pthread_create(&deq_ids[i], NULL, ring_dequeue_func, &deq_args[i]) != 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: pthread_create failed: i=%d", __func__, i);
        }
    }

    NVLOGC(TAG, "%s: threads created", __func__);

    for(i = 0; i < NUM_THREADS_DEQUEUE; i++)
    {
        if(pthread_join(deq_ids[i], NULL) != 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: pthread_join failed: i=%d", __func__, i);
        }
    }

    long total_dequeue_count = 0;
    for(i = 0; i < NUM_THREADS_DEQUEUE; i++)
    {
        total_dequeue_count += deq_args[i];
    }

    CU_ASSERT_EQUAL(total_dequeue_count, total_count);
    NVLOGC(TAG, "%s: finished: total_dequeue_count=%ld", __func__, total_dequeue_count);
}

void ring_multi_thread_enqueue(long total_count)
{
    int i;
    for(i = 0; i < NUM_THREADS_ENQUEUE; i++)
    {
        enq_args[i] = i;
        if(pthread_create(&enq_ids[i], NULL, ring_enqueue_func, &enq_args[i]) != 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: pthread_create failed: i=%d", __func__, i);
        }
    }

    NVLOGC(TAG, "%s: threads created", __func__);

    for(i = 0; i < NUM_THREADS_ENQUEUE; i++)
    {
        if(pthread_join(enq_ids[i], NULL) != 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: pthread_join failed: i=%d", __func__, i);
        }
    }

    // Exit message for all the dequeue threads
    ring_object_t obj;
    obj.msg_id = -1;
    for(i = 0; i < NUM_THREADS_DEQUEUE; i++)
    {
        while(ring->enqueue(ring, &obj))
        {
        }
    }

    long total_enqueue_count = 0;
    for(i = 0; i < NUM_THREADS_ENQUEUE; i++)
    {
        total_enqueue_count += enq_args[i];
    }

    CU_ASSERT_EQUAL(total_enqueue_count, total_count);
    NVLOGC(TAG, "%s: finished: total_enqueue_count=%ld", __func__, total_enqueue_count);
}

void test_ring_multi_thread(void)
{
    sync_primary_first("test cpu ring open");
    if((ring = nv_ipc_ring_open(is_primary(), "test_ring", TEST_RING_LEN, sizeof(ring_object_t))) == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: create test ring failed", __func__);
    }
    CU_ASSERT_NOT_EQUAL_FATAL(ring, NULL);
    sync_primary_end("test cpu ring open");

    long total_count = TEST_ENQUEUE_COUNT_PER_THREAD * NUM_THREADS_ENQUEUE;
    if(is_primary())
    {
        // Receiver
        ring_multi_thread_dequeue(total_count);
    }
    else
    {
        // Sender
        ring_multi_thread_enqueue(total_count);
    }

    sync_secondary_first("test cpu ring close");
    ring->close(ring);
    sync_secondary_end("test cpu ring close");
    NVLOGC(TAG, "%s: finished", __func__);
}

void test_ring_single_thread(void)
{
    sync_primary_first("test cpu ring open");
    if((ring = nv_ipc_ring_open(is_primary(), "test_ring", TEST_RING_LEN, sizeof(ring_object_t))) == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: create test ring failed", __func__);
    }
    CU_ASSERT_NOT_EQUAL_FATAL(ring, NULL);
    sync_primary_end("test cpu ring open");

    ring_object_t obj;
    obj.msg_id     = 1;
    obj.msg_len    = 100;
    obj.data_len   = 200;
    obj.msg_index  = 5;
    obj.data_index = 3;
    obj.data_pool  = 1;

    long total_count = TEST_ENQUEUE_COUNT_PER_THREAD * NUM_THREADS_ENQUEUE;
    if(is_primary())
    {
        // Receiver
        long dequeue_count = 0, idle_count = 0;
        do
        {
            while(ring->dequeue(ring, &obj) < 0)
            {
                cpu_nop_time(CPU_NOP_COUNT);
                idle_count++;
            }
            dequeue_count++;
        } while(obj.msg_id >= 0);
        NVLOGC(TAG, "%s: finished: dequeue_count=%ld idle_count=%ld", __func__, dequeue_count, idle_count);
        CU_ASSERT_EQUAL(dequeue_count, total_count);
    }
    else
    {
        long enqueue_count = 0, idle_count = 0;
        long i;
        for(i = 0; i < total_count; i++)
        {
            if(i < total_count - 1)
            {
                obj.msg_id = 1;
            }
            else
            {
                obj.msg_id = -1;
            }
            while(ring->enqueue(ring, &obj))
            {
                cpu_nop_time(CPU_NOP_COUNT);
                idle_count++;
            }
            enqueue_count++;
        }
        NVLOGC(TAG, "%s: finished: enqueue_count=%ld idle_count=%ld", __func__, enqueue_count, idle_count);
        CU_ASSERT_EQUAL(enqueue_count, total_count);
    }

    sync_secondary_first("test cpu ring close");
    ring->close(ring);
    sync_secondary_end("test cpu ring close");
}

void test_cpu_mempool(void)
{
    nv_ipc_mempool_t* mempool;

    sync_primary_first("test cpu mempool open");
    if((mempool = nv_ipc_mempool_open(is_primary(), "test_mempool", 160, TEST_CPU_MEMPOOL_LEN, -1)) == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: create test memory pool failed", __func__);
        return;
    }
    CU_ASSERT_NOT_EQUAL_FATAL(mempool, NULL);
    sync_primary_end("test cpu mempool open");
    int i;
    for(i = 0; i < TEST_CPU_MEMPOOL_LOOP * TEST_CPU_MEMPOOL_LEN; i++)
    {
        void* buf = mempool->get_addr(mempool, mempool->alloc(mempool));
        if(buf == NULL)
        {
            NVLOGD(TAG, "%s: alloc idle: i=%d", __func__, i);
        }
        else
        {
            int ret = mempool->free(mempool, mempool->get_index(mempool, buf));
            CU_ASSERT_EQUAL(ret, 0);
        }
    }

    // PRIMARY alloc (total/2) buffers, SECONDARY alloc (total-total/2) buffers, the total count may be odd value.
    void* bufs[(TEST_CPU_MEMPOOL_LEN + 1) / 2];
    int   count;
    if(is_primary())
    {
        count = TEST_CPU_MEMPOOL_LEN / 2;
    }
    else
    {
        count = TEST_CPU_MEMPOOL_LEN - TEST_CPU_MEMPOOL_LEN / 2;
    }

    NVLOGC(TAG, "%s: start allocate - free test, count=%d", __func__, count);

    int loop;
    for(loop = 0; loop < TEST_CPU_MEMPOOL_LOOP; loop++)
    {
        // Allocate all buffers
        sync_together("mempool_alloc_all");
        for(i = 0; i < count; i++)
        {
            bufs[i] = mempool->get_addr(mempool, mempool->alloc(mempool));
            CU_ASSERT(bufs[i] != NULL);
        }

        // Verify that the memory pool is empty
        sync_together("mempool_empty_verify");
        void* buf = mempool->get_addr(mempool, mempool->alloc(mempool));
        CU_ASSERT(buf == NULL);
        if(buf != NULL)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: CU_ASSERT failed: buf=%p index=%d", __func__, buf, mempool->get_index(mempool, buf));
        }

        // Free all buffers
        sync_together("full_mempool_free_all");
        for(i = 0; i < count; i++)
        {
            int ret = mempool->free(mempool, mempool->get_index(mempool, bufs[i]));
            CU_ASSERT(ret == 0);
        }
    }

    sync_secondary_first("test cpu mempool close");
    mempool->close(mempool);
    sync_secondary_end("test cpu mempool close");

    NVLOGC(TAG, "%s: finished", __func__);
}

void test_dl_blocking_transfer(void)
{
    CU_ASSERT_NOT_EQUAL_FATAL(ipc, NULL);
    test_transfer(1, IPC_DIR_DL);
}

void test_dl_epoll_transfer(void)
{
    CU_ASSERT_NOT_EQUAL_FATAL(ipc, NULL);
    test_transfer(2, IPC_DIR_DL);
}

void test_dl_no_sync_transfer(void)
{
    CU_ASSERT_NOT_EQUAL_FATAL(ipc, NULL);
    test_transfer(3, IPC_DIR_DL);
}

void test_ul_blocking_transfer(void)
{
    CU_ASSERT_NOT_EQUAL_FATAL(ipc, NULL);
    test_transfer(1, IPC_DIR_UL);
}

void test_ul_epoll_transfer(void)
{
    CU_ASSERT_NOT_EQUAL_FATAL(ipc, NULL);
    test_transfer(2, IPC_DIR_UL);
}

void test_ul_no_sync_transfer(void)
{
    CU_ASSERT_NOT_EQUAL_FATAL(ipc, NULL);
    test_transfer(3, IPC_DIR_UL);
}

void test_duplex_no_sync_transfer(void)
{
    CU_ASSERT_NOT_EQUAL_FATAL(ipc, NULL);

    int direction = IPC_DIR_DL;
    int total_count = TEST_NO_SYNC_TRANSFER_COUNT;
    set_task_name(__func__);
    sync_mode = 3;
    int64_t ns = run_duplex_tasks(ipc_send_task, ipc_recv_task, NULL, 1, 1, total_count, 1);
    update_perf_result(PERF_TEST_DUPLEX, total_count * 2, ns);
}

void test_duplex_mt_no_sync_transfer(void)
{
    CU_ASSERT_NOT_EQUAL_FATAL(ipc, NULL);

    if (pthread_setname_np(pthread_self(), "test_duplex_mt") != 0) {
        NVLOGE_NO(TAG, AERIAL_THREAD_API_EVENT, "%s: set thread name failed", __func__);
    }

    int total_count = TEST_NO_SYNC_TRANSFER_COUNT;
    set_task_name(__func__);
    sync_mode = 3;

    int send_thread_num = 4;
    int recv_thread_num = 4;

    if (ipc_transport == NV_IPC_TRANSPORT_SHM
            && task_info.free_core_num < send_thread_num + recv_thread_num) {
        // Note: CPU core number may be different on different systems, so only detect in SHM IPC
        send_thread_num = task_info.free_core_num / 2;
        recv_thread_num = task_info.free_core_num - send_thread_num;
    }

    int64_t ns = run_duplex_tasks(ipc_send_task, ipc_recv_task, NULL, send_thread_num,
            recv_thread_num, total_count, 1);
    update_perf_result(PERF_TEST_DUPLEX_MT, total_count * 2, ns);
}

void* multi_thread_transfer_task(void* arg)
{
    CU_ASSERT_NOT_EQUAL_FATAL(ipc, NULL);

    long* p_arg = (long*)arg;

    char task_name[16];
    snprintf(task_name, 16, *p_arg ? "recv_task" : "send_task");

    NVLOGC(TAG, "%s: start %s: p_arg=%ld", __func__, task_name, *p_arg);

    atomic_store(&send_order, 0);
    atomic_store(&recv_order, 0);

    long total_count = TEST_COUNT_PER_SEND_THREAD * TEST_SEND_THREAD_NUM;

    if(*p_arg) // Receiver threads
    {
        thread_info_t recv_task_info[TEST_RECV_THREAD_NUM];
        for(int i = 0; i < TEST_RECV_THREAD_NUM; i++)
        {
            recv_task_info[i].count = 0;
        }

        multi_thread_run(ipc_recv_task, recv_task_info, TEST_RECV_THREAD_NUM, "recv");

        long total_recv_count = 0;
        for(int i = 0; i < TEST_RECV_THREAD_NUM; i++)
        {
            total_recv_count += recv_task_info[i].count;
        }

        *p_arg = total_recv_count;
        NVLOGC(TAG, "%s: finished, total_recv_count=%ld expected=%ld", __func__, total_recv_count, total_count);
        CU_ASSERT_EQUAL(total_recv_count, total_count);
    }
    else // Sender threads
    {
        thread_info_t send_task_info[TEST_SEND_THREAD_NUM];
        int           i;
        for(i = 0; i < TEST_SEND_THREAD_NUM; i++)
        {
            send_task_info[i].count = TEST_COUNT_PER_SEND_THREAD;
        }

        multi_thread_run(ipc_send_task, send_task_info, TEST_SEND_THREAD_NUM, "send");

        // Send exit message to exit all receiving threads
        for(i = 0; i < TEST_RECV_THREAD_NUM; i++)
        {
            send_msg_exit();
        }
        NVLOGC(TAG, "%s: finished, total_send_count=%ld", __func__, total_count);
    }

    NVLOGC(TAG, "%s: %s finished", __func__, task_name);
    return NULL;
}

static void test_transfer_mt(int mode, int direction)
{
    CU_ASSERT_NOT_EQUAL_FATAL(ipc, NULL);
    sync_together("multiple_thread_transfer");

    struct timespec ts_start;
    nvlog_gettime_rt(&ts_start);

    sync_mode = mode;

    long total_count = TEST_COUNT_PER_SEND_THREAD * TEST_SEND_THREAD_NUM;

    if(is_primary() ^ direction)
    {
        thread_info_t recv_task_info[TEST_RECV_THREAD_NUM];
        multi_thread_run(ipc_recv_task, recv_task_info, TEST_RECV_THREAD_NUM, "recv");

        long total_recv_count = 0;
        int  i;
        for(i = 0; i < TEST_RECV_THREAD_NUM; i++)
        {
            total_recv_count += recv_task_info[i].count;
        }

        int64_t ns = nvlog_get_interval(&ts_start);
        NVLOGC(TAG, "%s: receive finished, total_recv=%ld total_time=%ldms average_time=%ldns",
                __func__, total_recv_count, ns / 1000L / 1000L, ns / total_recv_count);
        CU_ASSERT_EQUAL(total_recv_count, total_count);
    }
    else
    {
        thread_info_t send_task_info[TEST_SEND_THREAD_NUM];
        int           i;
        for(i = 0; i < TEST_SEND_THREAD_NUM; i++)
        {
            send_task_info[i].count = TEST_COUNT_PER_SEND_THREAD;
        }

        multi_thread_run(ipc_send_task, send_task_info, TEST_SEND_THREAD_NUM, "send");

        // Send exit message to exit all receiving threads
        for(i = 0; i < TEST_RECV_THREAD_NUM; i++)
        {
            send_msg_exit();
        }
        int64_t ns = nvlog_get_interval(&ts_start);
        NVLOGC(TAG, "%s: send finished, total_send=%ld total_time=%ldms average_time=%ldns",
                __func__, total_count, ns / 1000L / 1000L, ns / total_count);
    }
}

void test_dl_transfer_multi_thread(void)
{
    test_transfer_mt(3, 0);
}

void test_ul_transfer_multi_thread(void)
{
    test_transfer_mt(3, 1);
}

void test_transfer_duplex_multi_thread(void)
{
    CU_ASSERT_NOT_EQUAL_FATAL(ipc, NULL);
    sync_together("test_transfer_duplex_multi_thread");

    NVLOGC(TAG, "%s: start", __func__);

    sync_mode = 3;

    // A receive task and a send task, each contains multiple threads
    pthread_t task_ids[2];
    long      arg[2] = {1, 0};

    int i;
    for(i = 0; i < 2; i++)
    {
        if(pthread_create(&task_ids[i], NULL, multi_thread_transfer_task, &arg[i]) != 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: task create failed: i=%d", __func__, i);
        }
    }

    for(i = 0; i < 2; i++)
    {
        if(pthread_join(task_ids[i], NULL) != 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: task join failed: i=%d", __func__, i);
        }
    }
    NVLOGC(TAG, "%s: finished, tota_received_count=%ld", __func__, arg[0]);
}
