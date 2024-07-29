/*
 * Copyright (c) 2020, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef _FD_SHARE_H_
#define _FD_SHARE_H_

#include <sys/socket.h>
#include <sys/un.h>

int send_fd(int fd, int fd_to_send);

int recv_fd(int fd);

void unix_sock_address_init(struct sockaddr_un* addr, char* path);

int unix_sock_create(struct sockaddr_un* addr);

int unix_sock_listen_and_accept(int listen_fd, struct sockaddr_un* client_addr);

int unix_sock_connect(int sock_fd, struct sockaddr_un* server_addr);

#endif /* _FD_SHARE_H_ */
