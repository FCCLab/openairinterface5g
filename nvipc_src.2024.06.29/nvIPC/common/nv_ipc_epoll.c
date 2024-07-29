/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <errno.h>

#include "nv_ipc_epoll.h"
#include "nv_ipc_utils.h"

#define TAG (NVLOG_TAG_BASE_NVIPC + 12) //"NVIPC.EPOLL"

nv_ipc_epoll_t* ipc_epoll_create(int max_events, int target_fd)
{
    if(max_events < 0 || target_fd < 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: invalid parameters", __func__);
        return NULL;
    }

    nv_ipc_epoll_t* ipc_epoll = malloc(
        sizeof(nv_ipc_epoll_t) + sizeof(struct epoll_event) * max_events);
    if(ipc_epoll == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: memory malloc failed", __func__);
        return NULL;
    }

    ipc_epoll->max_events = max_events;
    ipc_epoll->target_fd  = target_fd;

    ipc_epoll->epoll_fd = epoll_create1(0);
    if(ipc_epoll->epoll_fd < 0)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s epoll_create failed", __func__);
        free(ipc_epoll);
        return NULL;
    }

    struct epoll_event ev;
    ev.events  = EPOLLIN;
    ev.data.fd = ipc_epoll->target_fd;
    if(epoll_ctl(ipc_epoll->epoll_fd, EPOLL_CTL_ADD, ev.data.fd, &ev) == -1)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s epoll_ctl failed", __func__);
        close(ipc_epoll->epoll_fd);
        free(ipc_epoll);
        return NULL;
    }
    NVLOGI(TAG, "%s: OK", __func__);
    return ipc_epoll;
}

int ipc_epoll_wait(nv_ipc_epoll_t* ipc_epoll)
{
    if(ipc_epoll == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }

    static int count = 0;
    NVLOGD(TAG, "%s: start wait ... count=%d", __func__, count++);

    int nfds;
    do
    {
        // epoll_wait() may return EINTR when get unexpected signal SIGSTOP from system
        nfds = epoll_wait(ipc_epoll->epoll_fd, ipc_epoll->events, ipc_epoll->max_events, -1);
    } while(nfds == -1 && errno == EINTR);

    if(nfds < 0)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "epoll_wait failed: epoll_fd=%d nfds=%d err=%d - %s", ipc_epoll->epoll_fd, nfds, errno, strerror(errno));
        return -1;
    }

    int n = 0;
    for(n = 0; n < nfds; ++n)
    {
        if(ipc_epoll->events[n].data.fd == ipc_epoll->target_fd)
        {
            return 0;
        }
    }
    return -1;
}

int ipc_epoll_destroy(nv_ipc_epoll_t* ipc_epoll)
{
    if(ipc_epoll == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: instance not exist", __func__);
        return -1;
    }

    int ret = 0;
    if(close(ipc_epoll->epoll_fd) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: close epoll_fd failed", __func__);
        ret = -1;
    }
    free(ipc_epoll);
    return ret;
}
