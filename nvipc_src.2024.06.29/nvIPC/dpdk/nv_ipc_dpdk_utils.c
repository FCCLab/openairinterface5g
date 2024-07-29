/*
 * Copyright (c) 2019-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

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
#include <stdatomic.h>

#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_ring.h>
#include <rte_log.h>
#include <rte_mempool.h>
#include <rte_common.h>
#include <rte_memory.h>
#include <rte_launch.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_mbuf.h>

#include "nv_ipc.h"
#include "nv_ipc_dpdk.h"
#include "nv_ipc_utils.h"

#define TAG (NVLOG_TAG_BASE_NVIPC + 21) //"NVIPC.DPDK_UTILS"

#define MAX_EAL_ARGC 64
#define MAX_EAL_ARG_BUG_SIZE 2048

typedef struct
{
    int   argc;
    int   offset;
    char* argv[MAX_EAL_ARGC];
    char  buf[MAX_EAL_ARG_BUG_SIZE];
} eal_args_t;

static void eal_arg_add(eal_args_t* args, const char* arg)
{
    args->argv[args->argc++] = args->buf + args->offset;
    args->offset += snprintf(args->buf + args->offset, 128, "%s", arg) + 1;
}
static int print_args(int argc, char** argv)
{
    char cmd[MAX_EAL_ARG_BUG_SIZE];
    int  offset = 0;
    for(int i = 0; i < argc; i++)
    {
        // NVLOGI(TAG, "CMD[%d-%i]: [%s]", argc, i, argv[i]);
        offset += snprintf(cmd + offset, 128, "%s ", argv[i]);
    }
    NVLOGC(TAG, "CMD[%d]: %s", argc, cmd);
    return 0;
}

int nv_ipc_dpdk_init(const char* argv0, const nv_ipc_config_dpdk_t* cfg)
{
    uint16_t main_core = cfg->lcore_id - 1;
    char     core_params[16];
    snprintf(core_params, 16, "%u-%u", main_core, main_core + 1);

    eal_args_t eal_args;
    eal_args.argc   = 0;
    eal_args.offset = 0;
    eal_arg_add(&eal_args, argv0);
    eal_arg_add(&eal_args, "-l");
    eal_arg_add(&eal_args, core_params);
    eal_arg_add(&eal_args, "--file-prefix=cuphycontroller");
    //    eal_arg_add(&eal_args, "-n");
    //    eal_arg_add(&eal_args, "4");
    eal_arg_add(&eal_args, "--proc-type=auto");
    eal_arg_add(&eal_args, "-a");
    eal_arg_add(&eal_args, "0000:00:0.0");
    //    eal_arg_add(&eal_args, "-a");
    //    eal_arg_add(&eal_args, "b5:00.0");
    //    eal_arg_add(&eal_args, "--file-prefix");
    //    eal_arg_add(&eal_args, "nvipc_eal");
    //    if (cfg->primary)
    //    {
    //        eal_arg_add(&eal_args, "--proc-type=primary");
    //    }
    //    else
    //    {
    //        eal_arg_add(&eal_args, "--proc-type=secondary");
    //    }

    print_args(eal_args.argc, eal_args.argv);
    if(rte_eal_init(eal_args.argc, eal_args.argv) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_DPDK_API_EVENT, "%s: Error: Cannot init EAL", __func__);
        return -1;
    }
    else
    {
        return 0;
    }
}

void dpdk_print_lcore(const char* info)
{
    unsigned lcore_id = rte_lcore_id();
    NVLOGC(TAG, "%s: lcore_id=%u", info, lcore_id);
}

#if 0

// TODO: get NIC_PORT_MTU and calculate MBUF_NVIPC_PAYLOAD_SIZE from configuration or mbuf
#define NIC_PORT_MTU 1536
#define MBUF_DATA_ROOM_SIZE (NIC_PORT_MTU - 18 + 128)
#define MBUF_DATA_LEN_MAX (NIC_PORT_MTU - 128)
#define MBUF_NVIPC_PAYLOAD_SIZE (MBUF_DATA_LEN_MAX - sizeof(nvipc_hdr_t))

int memcpy_from_nvipc(const nv_ipc_msg_t* src, uint8_t* dst, uint32_t size)
{
    struct rte_mbuf* mbuf_head = get_rte_mbuf(src->msg_buf);
    nvipc_hdr_t*     head      = get_nvipc_hdr(mbuf_head);
    nvipc_hdr_t*     curr      = get_nvipc_hdr(head->next);

    uint32_t n, index;
    uint8_t* ptr = (uint8_t*)src->data_buf;
    for(index = 1; size != 0 && index < head->seg_num; index++)
    {
        n = size > MBUF_NVIPC_PAYLOAD_SIZE ? MBUF_NVIPC_PAYLOAD_SIZE : size;
        memcpy(dst, ptr, n);
        size -= n;
        ptr += n;
        dst += n;
        curr = get_nvipc_hdr(head->next);
    }
    if(size != 0 || index != head->seg_num)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: size doesn't match: seg_num=%u size=%u", __func__, head->seg_num, size);
        return -1;
    }
    else
    {
        return 0;
    }
}

int memcpy_to_nvipc(nv_ipc_msg_t* dst, const uint8_t* src, uint32_t size)
{
    struct rte_mbuf* mbuf_head = get_rte_mbuf(dst->msg_buf);
    nvipc_hdr_t*     head      = get_nvipc_hdr(mbuf_head);
    nvipc_hdr_t*     curr      = get_nvipc_hdr(head->next);

    uint32_t n, index;
    uint8_t* ptr_data = (uint8_t*)dst->data_buf;
    for(index = 1; size != 0 && index < head->seg_num; index++)
    {
        n = size > MBUF_NVIPC_PAYLOAD_SIZE ? MBUF_NVIPC_PAYLOAD_SIZE : size;
        memcpy(ptr_data, src, n);
        size -= n;
        ptr_data += n;
        src += n;
        curr = get_nvipc_hdr(head->next);
    }
    if(size != 0 || index != head->seg_num)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: size doesn't match: seg_num=%u size=%u", __func__, head->seg_num, size);
        return -1;
    }
    else
    {
        return 0;
    }
}
#endif

int create_dpdk_task(int (*func)(void* arg), void* arg, uint16_t lcore_id)
{
    NVLOGC(TAG, "%s: lcore_id=%u start", __func__, lcore_id);

    int ret = rte_eal_remote_launch(func, arg, lcore_id);
    if(ret != 0)
    {
        NVLOGE_NO(TAG, AERIAL_DPDK_API_EVENT, "%s: failed: lcore_id=%u ret=%d", __func__, lcore_id, ret);
    }
    NVLOGC(TAG, "%s: lcore_id=%u finished", __func__, lcore_id);

    return 0;
}
