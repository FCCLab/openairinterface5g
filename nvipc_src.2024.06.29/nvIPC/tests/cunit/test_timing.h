/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _TEST_TIMING_H_
#define _TEST_TIMING_H_

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

int assin_cpu_for_thread(int cpu_id);
int assign_cpu_for_process(int cpu);

void test_stat_log(void);
void test_timing(void);

#if defined(__cplusplus)
}
#endif

#endif /* _TEST_TIMING_H_ */
