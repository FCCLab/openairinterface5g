/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */
#ifndef _TEST_IPC_H_
#define _TEST_IPC_H_

#include "stdint.h"
#include "test_cuda.h"
#include "nv_ipc.h"

#define TEST_DUPLEX_TRANSFER 0

// Configure whether to sync by TTI or sync by one single message.
#define CONFIG_SYNC_BY_TTI 1

#define TEST_MSG_COUNT 3
#define MAX_EVENTS 10
#define TEST_DATA_BUF_LEN 256

typedef struct
{
    int             tid;          // Thread ID
    long            counter;      // Message counter
    struct timespec before_send;  // Time before enqueue
    struct timespec after_send;   // Time after enqueue
    struct timespec before_recv;  // Time before dequeue
    struct timespec after_recv;   // Time after dequeue
    long            order_before; // Time after dequeue
    long            order_middle; // Time after dequeue
    long            order_after;  // Time after dequeue
} msg_verify_t;

typedef struct
{
    int             tid;     // Thread ID
    long            counter; // Message counter
    long            order_send;
    struct timespec time_send; // Send time
} buf_obj_t;

typedef struct
{
    int32_t msg_id;
    int32_t msg_len;
    int32_t data_len;
    int32_t data_pool;
    long    counter;
} test_msg_t;

typedef struct
{
    int32_t msg_id;
    int32_t msg_len;
    int32_t data_len;
    int32_t msg_index;
    int32_t data_index;
    int32_t data_pool;
} ring_object_t;

void test_dl_blocking_transfer(void);
void test_dl_epoll_transfer(void);
void test_dl_no_sync_transfer(void);

void test_ul_blocking_transfer(void);
void test_ul_epoll_transfer(void);
void test_ul_no_sync_transfer(void);
void test_duplex_no_sync_transfer(void);
void test_duplex_mt_no_sync_transfer(void);

void test_cpu_mempool(void);

void test_ring_single_thread(void);
void test_ring_multi_thread(void);
void test_dl_transfer_multi_thread(void);
void test_ul_transfer_multi_thread(void);
void test_transfer_duplex_multi_thread(void);

#endif /* _TEST_IPC_H_ */
