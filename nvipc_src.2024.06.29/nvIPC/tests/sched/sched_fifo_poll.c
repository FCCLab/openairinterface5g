/*
 * Copyright (c) 2023, NVIDIA CORPORATION.  All rights reserved.
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
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>

int nv_set_sched_fifo_priority(int priority) {
    struct sched_param param;
    param.__sched_priority = priority;
    pthread_t thread_me = pthread_self();
    if (pthread_setschedparam(thread_me, SCHED_FIFO, &param) != 0) {
        printf("%s: line %d errno=%d: %s\n", __func__, __LINE__, errno, strerror(errno));
        return -1;
    } else {
        printf("%s: OK: thread=%ld priority=%d\n", __func__, thread_me, priority);
        return 0;
    }
}

int nv_assign_thread_cpu_core(int cpu_id) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu_id, &mask);
    int ret;
    if ((ret = pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask)) != 0) {
        printf("%s: line %d ret=%d errno=%d: %s\n", __func__,
        __LINE__, ret, errno, strerror(errno));
        return -1;
    } else {
        printf("%s: OK: thread=%ld cpu_id=%d\n", __func__, pthread_self(), cpu_id);
        return 0;
    }
}

int main(int argc, char **argv) {

    printf("%s: started on CPU core %d\n", __func__, sched_getcpu());

    nv_assign_thread_cpu_core(3);

    nv_set_sched_fifo_priority(90);

    printf("%s: run SCHED_FIFO while(1) loop on core %d ...\n", __func__, sched_getcpu());

    int ret = 0;

    while (1) {
        ret += 1;
    }

    // printf("Never run to here\n");

    return ret;
}
