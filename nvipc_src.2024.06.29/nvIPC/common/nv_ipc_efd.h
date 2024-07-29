/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _NV_IPC_EFD_H_
#define _NV_IPC_EFD_H_

#if defined(__cplusplus)
extern "C" {
#endif

// The Unix domain socket path maxim length is 108, see <sys/un.h>
#define NV_UNIX_SOCKET_PATH_MAX_LEN 108

#define NV_EFD_NAME_MAX_LEN 32

typedef struct nv_ipc_efd_t nv_ipc_efd_t;
struct nv_ipc_efd_t
{
    // Get event FD for receiving select/poll/epoll
    int (*get_fd)(nv_ipc_efd_t* ipc_efd);

    int (*notify)(nv_ipc_efd_t* ipc_efd, int value);

    int (*get_value)(nv_ipc_efd_t* ipc_efd);

    int (*close)(nv_ipc_efd_t* ipc_efd);
};

// PRIMARY is responsible to create or initiate the event FD
nv_ipc_efd_t* nv_ipc_efd_open(int primary, const char* prefix);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* _NV_IPC_EFD_H_ */
