/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _NV_IPC_EPOLL_H_
#define _NV_IPC_EPOLL_H_

#include <sys/epoll.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct
{
    int                epoll_fd;
    int                target_fd;
    int                max_events;
    struct epoll_event events[];
} nv_ipc_epoll_t;

nv_ipc_epoll_t* ipc_epoll_create(int max_events, int target_fd);
int             ipc_epoll_wait(nv_ipc_epoll_t* ipc_epoll);
int             ipc_epoll_destroy(nv_ipc_epoll_t* ipc_epoll);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* _NV_IPC_EPOLL_H_ */
