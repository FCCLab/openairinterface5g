/*
 * Copyright (c) 2019-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#define _GNU_SOURCE             /* See feature_test_macros(7) */

#include <stdio.h>
#include <string.h>
#include <stddef.h>
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
#include <stdatomic.h>
#include <pthread.h>
#include <sched.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>

#include <doca_version.h>
#include <doca_dev.h>
#include <doca_dma.h>
#include <doca_mmap.h>
#include <doca_error.h>

#include <doca_buf.h>
#include <doca_buf_inventory.h>
#include <doca_ctx.h>
#include <doca_dev.h>
#include <doca_dma.h>
#include <doca_mmap.h>

#include "nv_ipc.h"
#include "nv_ipc_doca.h"
#include "nv_ipc_utils.h"

// #include "dma_copy_core.h"

#define TAG (NVLOG_TAG_BASE_NVIPC + 19) //"NVIPC.DOCA_UTILS"

#define BLOCKING_INIT 1

struct cc_msg_dma_status {
    bool is_success; /* Indicate success or failure for last message sent */
};

#define MAX_COMM_ARGC 64
#define MAX_COMM_ARG_BUG_SIZE 2048

#define STATUS_SUCCESS true    /* Successful status */
#define STATUS_FAILURE false       /* Unsuccessful status */

typedef struct {
    int argc;
    int offset;
    char *argv[MAX_COMM_ARGC];
    char buf[MAX_COMM_ARG_BUG_SIZE];
} comm_args_t;

static void comm_arg_add(comm_args_t *args, const char *arg) {
    if (args != NULL && arg != NULL) {
        args->argv[args->argc++] = args->buf + args->offset;
        args->offset += snprintf(args->buf + args->offset, 128, "%s", arg) + 1;
    }
}
static int print_args(int argc, char **argv) {
    char cmd[MAX_COMM_ARG_BUG_SIZE];
    int offset = 0;
    for (int i = 0; i < argc; i++) {
        // NVLOGI(TAG, "CMD[%d-%i]: [%s]", argc, i, argv[i]);
        offset += snprintf(cmd + offset, 128, "%s ", argv[i]);
    }
    NVLOGC(TAG, "CMD[%d]: %s", argc, cmd);
    return 0;
}

void print_doca_sdk_version() {
    NVLOGC(TAG, "DOCA SDK     Version (Compilation): %s", doca_version());
    NVLOGC(TAG, "DOCA Runtime Version (Runtime):     %s", doca_version_runtime());
}

static doca_error_t open_doca_device_with_pci(char *pcie_value, struct doca_dev **retval) {
    struct doca_devinfo **dev_list;
    uint32_t nb_devs;
    uint8_t is_addr_equal = 0;
    doca_error_t res;
    size_t i;

    /* Set default return value */
    *retval = NULL;

    res = doca_devinfo_create_list(&dev_list, &nb_devs);
    if (res != DOCA_SUCCESS) {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to load doca devices list. Doca_error value: %d",
                res);
        return res;
    }

    /* Search */
    for (i = 0; i < nb_devs; i++) {
        res = doca_devinfo_is_equal_pci_addr(dev_list[i], pcie_value, &is_addr_equal);
        if (res == DOCA_SUCCESS && is_addr_equal) { // && buf.raw == value->raw) { ???
            /* If any special capabilities are needed */
            // jobs_check func,
//            if (func != NULL && func(dev_list[i]) != DOCA_SUCCESS)
//                continue;
            /* if device can be opened */
            res = doca_dev_open(dev_list[i], retval);
            if (res == DOCA_SUCCESS) {
                uint32_t max_send_queue = 0;
                uint32_t max_recv_queue = 0;
                if ((res = doca_comm_channel_get_max_send_queue_size(dev_list[i],
                        &max_send_queue)) != DOCA_SUCCESS) {
                    NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to get_max_send_queue_size");
                    return res;
                }
                if ((res = doca_comm_channel_get_max_recv_queue_size(dev_list[i],
                        &max_recv_queue)) != DOCA_SUCCESS) {
                    NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to get_max_send_queue_size");
                    return res;
                }
                NVLOGC(TAG, "%s: open cc_dev, max_send_queue=%u max_recv_queue=%u", __func__,
                        max_send_queue, max_recv_queue);
                doca_devinfo_destroy_list(dev_list);
                return res;
            }
        }
    }

    NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Matching device not found.");
    res = DOCA_ERROR_NOT_FOUND;

    doca_devinfo_destroy_list(dev_list);
    return res;
}

static doca_error_t open_doca_device_rep_with_pci(struct doca_dev *local, enum doca_devinfo_rep_filter filter,
        const char *pcie_value, struct doca_dev_rep **retval) {
    uint32_t nb_rdevs = 0;
    struct doca_devinfo_rep **rep_dev_list = NULL;
   	uint8_t is_addr_equal = 0;
    doca_error_t result;
    size_t i;

    *retval = NULL;

    /* Search */
    result = doca_devinfo_rep_create_list(local, filter, &rep_dev_list, &nb_rdevs);
    if (result != DOCA_SUCCESS) {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT,
                "Failed to create devinfo representors list. Representor devices are available only on DPU, do not run on Host.");
        return DOCA_ERROR_INVALID_VALUE;
    }

    for (i = 0; i < nb_rdevs; i++) {
        result = doca_devinfo_rep_is_equal_pci_addr(rep_dev_list[i], pcie_value, &is_addr_equal);
        if (result == DOCA_SUCCESS && is_addr_equal /*queried_pci_bdf.raw == pci_bdf->raw */
                && doca_dev_rep_open(rep_dev_list[i], retval) == DOCA_SUCCESS) {
            doca_devinfo_rep_destroy_list(rep_dev_list);
            return DOCA_SUCCESS;
        }
    }

    NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Matching device not found.");
    doca_devinfo_rep_destroy_list(rep_dev_list);
    return DOCA_ERROR_NOT_FOUND;
}

static doca_error_t set_cc_properties(enum dma_copy_mode mode, struct doca_comm_channel_ep_t *ep,
        struct doca_dev *dev, struct doca_dev_rep *dev_rep) {
    doca_error_t result;

    result = doca_comm_channel_ep_set_device(ep, dev);
    if (result != DOCA_SUCCESS) {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to set DOCA device property");
        return result;
    }

    result = doca_comm_channel_ep_set_max_msg_size(ep, NVIPC_DOCA_CC_MAX_MSG_SIZE);
    if (result != DOCA_SUCCESS) {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to set max_msg_size property");
        return result;
    }

    result = doca_comm_channel_ep_set_send_queue_size(ep, CC_MAX_QUEUE_SIZE);
    if (result != DOCA_SUCCESS) {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to set snd_queue_size property");
        return result;
    }

    result = doca_comm_channel_ep_set_recv_queue_size(ep, CC_MAX_QUEUE_SIZE);
    if (result != DOCA_SUCCESS) {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to set rcv_queue_size property");
        return result;
    }

    if (mode == DMA_COPY_MODE_DPU) {
        result = doca_comm_channel_ep_set_device_rep(ep, dev_rep);
        if (result != DOCA_SUCCESS)
            NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to set DOCA device representor property");
    }

    return result;
}

int doca_comm_channel_init(doca_info_t *di) {
    doca_error_t result;
    struct timespec ts = { .tv_nsec = SLEEP_IN_NANOS, };

    result = doca_devinfo_create_list(&di->dev_list, &di->nb_devs);
    if (result != DOCA_SUCCESS) {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to load doca devices list. Doca_error value: %d",
                result);
        return result;
    }

    NVLOGC(TAG, "%s: doca_devinfo_create_list get %u devices", __func__, di->nb_devs);

    result = doca_comm_channel_ep_create(&di->ep);
    if (result != DOCA_SUCCESS) {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to create Comm Channel endpoint: %s",
                doca_error_get_descr(result));
        return result;
    }

    result = open_doca_device_with_pci(di->dev_pci_str, &di->cc_dev);
    if (result != DOCA_SUCCESS) {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT,
                "Failed to open Comm Channel DOCA device based on PCI address");
        doca_comm_channel_ep_destroy(di->ep);
        return result;
    }

    /* Open DOCA device representor on DPU side */
    if (di->mode == DMA_COPY_MODE_DPU) {
        result = open_doca_device_rep_with_pci(di->cc_dev, DOCA_DEVINFO_REP_FILTER_NET, di->rep_pci_str,
                &di->cc_dev_rep);
        if (result != DOCA_SUCCESS) {
            NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT,
                    "Failed to open Comm Channel DOCA device representor based on PCI address");
            doca_comm_channel_ep_destroy(di->ep);
            doca_dev_close(di->cc_dev);
            return result;
        }
    }

    result = set_cc_properties(di->mode, di->ep, di->cc_dev, di->cc_dev_rep);
    if (result != DOCA_SUCCESS) {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to set Comm Channel properties");
        doca_comm_channel_ep_destroy(di->ep);
        if (di->mode == DMA_COPY_MODE_DPU)
            doca_dev_rep_close(di->cc_dev_rep);
        doca_dev_close(di->cc_dev);
    }

    if (di->mode == DMA_COPY_MODE_DPU) {
        result = doca_comm_channel_ep_listen(di->ep, di->name);
        if (result != DOCA_SUCCESS) {
            NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Comm Channel endpoint couldn't start listening: %s",
                    doca_error_get_descr(result));
            return result;
        }
    } else {
        do {
            result = doca_comm_channel_ep_connect(di->ep, di->name, &di->peer_addr);
            if (BLOCKING_INIT) {
                NVLOGC(TAG, "Waiting for nvipc server to start ...");
                sleep(1);
            } else {
                NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to establish a connection with the DPU: %s",
                          doca_error_get_descr(result));
                return result;
            }
        } while (BLOCKING_INIT && result != DOCA_SUCCESS);

        while ((result = doca_comm_channel_peer_addr_update_info(di->peer_addr))
                == DOCA_ERROR_CONNECTION_INPROGRESS) {
            NVLOGC(TAG, "%s: DOCA_ERROR_CONNECTION_INPROGRESS", __func__);
            nanosleep(&ts, &ts);
        }

        if (result != DOCA_SUCCESS) {
            NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to validate the connection with the DPU: %s",
                    doca_error_get_descr(result));
            return result;
        }
    }
    return result;
}

doca_error_t wait_for_successful_status_msg(struct doca_comm_channel_ep_t *ep,
        struct doca_comm_channel_addr_t **peer_addr) {
    struct cc_msg_dma_status msg_status;
    doca_error_t result;
    size_t msg_len, status_msg_len = sizeof(struct cc_msg_dma_status);
    struct timespec ts = { .tv_nsec = SLEEP_IN_NANOS, };

    msg_len = status_msg_len;
    while ((result = doca_comm_channel_ep_recvfrom(ep, (void*) &msg_status, &msg_len,
            DOCA_CC_MSG_FLAG_NONE, peer_addr)) == DOCA_ERROR_AGAIN) {
        nanosleep(&ts, &ts);
        msg_len = status_msg_len;
    }
    if (result != DOCA_SUCCESS) {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Status message was not received: %s",
                doca_error_get_descr(result));
        return result;
    }

    if (!msg_status.is_success) {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failure status received");
        return DOCA_ERROR_INVALID_VALUE;
    }

    return DOCA_SUCCESS;
}

/*
 * Send status message
 *
 * @ep [in]: Comm Channel endpoint
 * @peer_addr [in]: Comm Channel peer address
 * @status [in]: Status to send
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t send_status_msg(struct doca_comm_channel_ep_t *ep,
        struct doca_comm_channel_addr_t **peer_addr, bool status) {
    struct cc_msg_dma_status status_msg;
    doca_error_t result;
    struct timespec ts = { .tv_nsec = SLEEP_IN_NANOS, };

    status_msg.is_success = status;

    while ((result = doca_comm_channel_ep_sendto(ep, &status_msg, sizeof(struct cc_msg_dma_status),
            DOCA_CC_MSG_FLAG_NONE, *peer_addr)) == DOCA_ERROR_AGAIN)
        nanosleep(&ts, &ts);

    if (result != DOCA_SUCCESS) {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to send status message: %s",
                doca_error_get_descr(result));
        return result;
    }

    return DOCA_SUCCESS;
}

int doca_cc_send(doca_info_t *di, cc_msg_t *cmsg) {
    nvlog_gettime_rt(&cmsg->ts_send);

    NVLOGD(TAG, "DOCA_CC SEND: type=%d len=%d msg_id=0x%02X", cmsg->type, cmsg->len,
            ((packet_info_t* )cmsg)->msg_id);

    doca_error_t result;
    if ((result = doca_comm_channel_ep_sendto(di->ep, cmsg, cmsg->len, DOCA_CC_MSG_FLAG_NONE,
            di->peer_addr)) != DOCA_SUCCESS) {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to send status message: %s",
                doca_error_get_descr(result));
        return -1;
    } else {
        return 0;
    }
}

int doca_cc_recv(doca_info_t *di, cc_msg_t *cmsg) {
    size_t cc_len = NVIPC_DOCA_CC_MAX_MSG_SIZE;
    doca_error_t result = doca_comm_channel_ep_recvfrom(di->ep, (void*) cmsg, &cc_len,
            DOCA_CC_MSG_FLAG_NONE, &di->peer_addr);

    if (result == DOCA_SUCCESS) {
        int payload_len = cmsg->type < CC_MSG_NO_PAYLOAD ? 0 : cmsg->len;
        if (cc_len != cmsg->len) {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "RECV CC: type=%d len=%d cc_len=%lu",
                    cmsg->type, cmsg->len, cc_len);
            return -2;
        }
        return 0;
    } else if (DOCA_ERROR_AGAIN) {
        NVLOGV(TAG, "%s: doca_comm recv queue is empty", __func__);
        return -1;
    } else {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "%s: doca_comm recv failed: ", __func__,
                doca_error_get_descr(result));
        return -1;
    }
}

int doca_comm_recv(doca_info_t *doca_info, void *buf, size_t *p_size) {
    doca_error_t result = DOCA_SUCCESS;

    struct timespec ts = { .tv_nsec = 1000, };

    *p_size = NVIPC_DOCA_CC_MAX_MSG_SIZE;
    while ((result = doca_comm_channel_ep_recvfrom(doca_info->ep, (void*) buf, p_size,
            DOCA_CC_MSG_FLAG_NONE, &doca_info->peer_addr)) == DOCA_ERROR_AGAIN) {
        // nanosleep(&ts, &ts);
        *p_size = NVIPC_DOCA_CC_MAX_MSG_SIZE;
        // msg_len = sizeof(struct cc_msg_dma_direction);
        // NVLOGC(TAG, "%s: DOCA_ERROR_AGAIN msg_len=%lu", __func__, *p_size);
    }
    // NVLOGC(TAG, "%s: result=%lu msg_len=%lu", __func__, result, *p_size);

    if (result != DOCA_SUCCESS) {
        NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to receive: %s", doca_error_get_descr(result));
    }
    return result;
}


int doca_comm_send(doca_info_t *di, void *buf, size_t size) {
    doca_error_t result = DOCA_SUCCESS;
    struct timespec ts = { .tv_nsec = SLEEP_IN_NANOS, };

    // NVLOGC(TAG, "%s: size=%lu", __func__, size);

    while ((result = doca_comm_channel_ep_sendto(di->ep, buf, size, DOCA_CC_MSG_FLAG_NONE,
            di->peer_addr)) == DOCA_ERROR_AGAIN)
        // nanosleep(&ts, &ts);

        if (result != DOCA_SUCCESS) {
            NVLOGE_NO(TAG, AERIAL_DOCA_API_EVENT, "Failed to send status message: %s",
                    doca_error_get_descr(result));
            return result;
        }
    return result;
}

void doca_print_cpu_core() {

    char thread_name[32];
    pthread_getname_np(pthread_self(), thread_name, 32);

    int nproc_num = get_nprocs();
    if (nproc_num > 100) {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: thread %lu get_nprocs failed: %s", __func__,
                pthread_self(), strerror(errno));
    }

    cpu_set_t get;
    CPU_ZERO(&get);
    if (pthread_getaffinity_np(pthread_self(), sizeof(get), &get) < 0) {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: pthread_getaffinity_np failed for %s %s",
                __func__, thread_name, strerror(errno));
    }

    int core_num = 0;
    for (int cpu_id = 0; cpu_id < nproc_num; cpu_id++) {
        if (CPU_ISSET(cpu_id, &get)) {
            NVLOGC(TAG, "%s: thread %s - %ld is running on core %d", __func__, thread_name,
                    pthread_self(), cpu_id);
            core_num ++;
        }
    }
    if (core_num != 1) {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: thread %s core_num=%d", __func__, thread_name,
                core_num);
    }
}
