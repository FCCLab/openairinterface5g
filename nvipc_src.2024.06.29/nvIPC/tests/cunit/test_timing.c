/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#define _GNU_SOURCE /* For CPU_ZERO, CPU_SET, ... */

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
#include <sched.h>
#include <assert.h>

#include "test_common.h"

#define TAG (NVLOG_TAG_BASE_NVIPC + 24) // "NVIPC.TIMING"
//static int threadSetRtPriority(int policy, int priority) {
//    struct sched_param sp = { 0 };
//    sp.sched_priority = priority;
//    return sched_setscheduler(syscall(SYS_gettid), policy, &sp);
//}

static int get_schedule_policy(pthread_attr_t* attr)
{
    int policy;
    if(pthread_attr_getschedpolicy(attr, &policy) != 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: line %d errno=%d: %s", __func__, __LINE__, errno, strerror(errno));
        return -1;
    }
    else
    {
        return policy;
    }
}

static int get_thread_priority(pthread_attr_t* attr)
{
    struct sched_param param;
    if(pthread_attr_getschedparam(attr, &param) != 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: line %d errno=%d - %s", __func__, __LINE__, errno, strerror(errno));
        return -1;
    }
    else
    {
        return param.__sched_priority;
    }
}

static int get_priority_range(pthread_attr_t* attr, int policy)
{
    int priority = sched_get_priority_max(policy);
    assert(priority != -1);
    NVLOGI(TAG, "max_priority=%d", priority);
    priority = sched_get_priority_min(policy);
    assert(priority != -1);
    NVLOGI(TAG, "min_priority=%d", priority);
    return 0;
}

static void set_thread_policy(pthread_attr_t* attr, int policy)
{
    int rs = pthread_attr_setschedpolicy(attr, policy);
    assert(rs == 0);
    get_schedule_policy(attr);
}

static int set_sched_policy_and_priority(int policy, int priority)
{
    struct sched_param sp = {0};
    sp.sched_priority     = priority;
    if(sched_setscheduler(getpid(), policy, &sp) != 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: line %d errno=%d: %s", __func__, __LINE__, errno, strerror(errno));
        return -1;
    }
    else
    {
        return 0;
    }
}

int assin_cpu_for_thread(int cpu_id)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);

    if(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: thread_id=%lu cpu_id=%d failed: %s", __func__, pthread_self(), cpu_id, strerror(errno));
        return -1;
    }
    else
    {
        NVLOGI(TAG, "%s: thread_id=%lu cpu_id=%d OK", __func__, pthread_self(), cpu_id);
    }

    cpu_set_t get;
    CPU_ZERO(&get);
    if(pthread_getaffinity_np(pthread_self(), sizeof(get), &get) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: pthread_getaffinity_np thread_id=%lu cpu_id=%d failed: %s", __func__, pthread_self(), cpu_id, strerror(errno));
    }

    if(CPU_ISSET(cpu_id, &get))
    {
        NVLOGI(TAG, "%s: thread %ld is running on core %d", __func__, pthread_self(), cpu_id);
        return 0;
    }
    else
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: thread %ld is NOT running on core %d", __func__, pthread_self(), cpu_id);
        return -1;
    }
}

int assign_cpu_for_process(int cpu)
{
    cpu_set_t mask;
    cpu_set_t get;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);

    int cpu_num = sysconf(_SC_NPROCESSORS_CONF);

    if(sched_setaffinity(getpid(), sizeof(mask), &mask) == -1)
    {
        NVLOGE_NO(TAG, AERIAL_THREAD_API_EVENT, "Set CPU affinity failue, ERROR:%s", strerror(errno));
        return -1;
    }

    struct timespec wait_time = {0, 1000000000};
    nanosleep(&wait_time, 0);

    CPU_ZERO(&get);
    if(sched_getaffinity(getpid(), sizeof(get), &get) == -1)
    {
        NVLOGE_NO(TAG, AERIAL_THREAD_API_EVENT, "get CPU affinity failue, ERROR:%s", strerror(errno));
        return -1;
    }

    int i;
    for(i = 0; i < cpu_num; i++)
    {
        if(CPU_ISSET(i, &get))
        {
            NVLOGC(TAG, "this process %d of running processor: %d", getpid(), i);
        }
    }
    return 0;
}

void test_stat_log(void)
{
    // Test log add
    stat_log_t* tti = stat_log_open("TTI", STAT_MODE_COUNTER, 10000);
    if(tti == NULL)
    {
        return;
    }
    long i = 0;
    for(i = 0; i < 3 * 10000; i++)
    {
        if(i > 0)
        {
            tti->add(tti, 500000);
        }
    }
    tti->close(tti);

    // Test log time_interval
    stat_log_t* stat_rt = stat_log_open("CLK_RT", STAT_MODE_COUNTER, 1000 * 1000);
    if(stat_rt == NULL)
    {
        return;
    }
    stat_rt->set_clock_source(stat_rt, CLOCK_REALTIME);
    stat_rt->set_limit(stat_rt, 10, 1000 * 1000);
    for(i = 0; i < 1000 * 1000 + 1; i++)
    {
        stat_rt->time_interval(stat_rt);
    }
    stat_rt->close(stat_rt);

    stat_log_t* stat_mono = stat_log_open("CLK_MONO", STAT_MODE_COUNTER, 1000 * 1000);
    if(stat_mono == NULL)
    {
        return;
    }
    stat_mono->set_clock_source(stat_mono, CLOCK_MONOTONIC);
    stat_mono->set_limit(stat_mono, 10, 1000 * 1000);
    for(i = 0; i < 1000 * 1000 + 1; i++)
    {
        stat_mono->time_interval(stat_mono);
    }
    stat_mono->close(stat_mono);

    stat_log_t* stat_raw = stat_log_open("CLK_RAW", STAT_MODE_COUNTER, 1000 * 1000);
    if(stat_raw == NULL)
    {
        return;
    }
    stat_raw->set_clock_source(stat_raw, CLOCK_MONOTONIC_RAW);
    stat_raw->set_limit(stat_raw, 10, 1000 * 1000);
    for(i = 0; i < 1000 * 1000 + 1; i++)
    {
        stat_raw->time_interval(stat_raw);
    }
    stat_raw->close(stat_raw);

    //    uint64_t start = __rdtsc();// nv_ipc_get_rdtsc();
    //    usleep(1000);
    //    uint64_t end = __rdtsc();//nv_ipc_get_rdtsc();
    //    NVLOGI(TAG, "%s CPU ticks of 1000us: %llu", __func__, end-start);
}

#define TTI_INTERVAL (1000L * 500) // nanoseconds

static inline void add_timespec_interval(struct timespec* ts, long interval)
{
    ts->tv_nsec += interval;
    if(ts->tv_nsec >= NUMBER_1E9)
    {
        ts->tv_nsec -= NUMBER_1E9;
        ts->tv_sec++;
    }
}

static inline int get_timesepc_order(struct timespec* ts_old, struct timespec* ts_new)
{
    if(ts_old->tv_sec < ts_new->tv_sec)
    {
        return 1;
    }
    else if(ts_old->tv_sec > ts_new->tv_sec)
    {
        return 0;
    }
    else
    {
        if(ts_old->tv_nsec <= ts_new->tv_nsec)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
}

// Test logging performance
void test_logger_performance(void)
{
    int j;
    for(j = 0; j < 10; j++)
    {
        struct timespec last, now;
        clock_gettime(CLOCK_REALTIME, &last);

        int i;
        for(i = 0; i < 1000 * 1000; i++)
        {
            NVLOGI(TAG, "test %d", i);
        }
        clock_gettime(CLOCK_REALTIME, &now);
        NVLOGC(TAG, "log interval: %d, avg=%ld", i, nvlog_timespec_interval(&last, &now) / i);
    }
}

static void set_sched_fifo_priority(int priority)
{
    struct sched_param param;
    param.__sched_priority = priority;
    pthread_t thread_me    = pthread_self();
    if(pthread_setschedparam(thread_me, SCHED_FIFO, &param) != 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: line %d errno=%d: %s", __func__, __LINE__, errno, strerror(errno));
    }
    else
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: OK: thread=%ld priority=%d", __func__, thread_me, priority);
    }
}

void test_timing(void)
{
    // test_logger_performance();

    int            policy, priority;
    pthread_attr_t attr;
    //set_thread_policy(&attr,SCHED_FIFO);
    //policy = get_schedule_policy(&attr);
    //priority = get_thread_priority(&attr);
    //NVLOGI(TAG, "Before set: policy=%d priority=%d", __func__, policy, priority);
    //
    //set_sched_policy_and_priority(SCHED_FIFO, 99);
    //
    //policy = get_schedule_policy(&attr);
    //priority = get_thread_priority(&attr);
    //NVLOGI(TAG, "After set: policy=%d priority=%d", __func__, policy, priority);

    set_sched_fifo_priority(1);

    stat_log_t* stat = stat_log_open("TTI", STAT_MODE_COUNTER, 1000L * 1000 * 100);
    if(stat == NULL)
    {
        return;
    }
    stat->set_clock_source(stat, CLOCK_REALTIME);
    stat->set_limit(stat, 0, 1000 * 100);

    struct timespec target, now, last;
    nvlog_gettime_rt(&target);

    nvlog_gettime_rt(&last);

    while(1)
    {
        //add_timespec_interval(&target, TTI_INTERVAL);
        //while (1) {
        //    nvlog_gettime_rt(&now);
        //    if (get_timesepc_order(&target, &now)) {
        //        break;
        //    }
        //}
        nvlog_gettime_rt(&now);
        long tti = nvlog_timespec_interval(&last, &now);
        //        stat->add(stat, tti);

        if(stat->add(stat, tti) == 1)
        {
            nvlog_gettime_rt(&last);
        }
        else
        {
            last.tv_sec  = now.tv_sec;
            last.tv_nsec = now.tv_nsec;
        }
    }
}
