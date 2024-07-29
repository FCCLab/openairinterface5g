/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

// #define _GNU_SOURCE /* For CPU_ZERO, CPU_SET, ... */

#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include "nv_ipc.h"
#include <memory>
#include "yaml.hpp"
#include "nv_phy_mac_transport.hpp"

using namespace std;
using namespace nv;

#include <exception>

#define TAG "NVIPC:FORWARDER"

#include "nv_ipc_utils.h"

void usage()
{
    printf("Usage: altran_ue_configure_phy <file>.yaml\n");
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
        NVLOGI(TAG, "%s: thread_id=%lu cpu_id=%d OK", __func__, pthread_self(), cpu_id)
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
    }
    else
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: thread %ld is NOT running on core %d", __func__, pthread_self(), cpu_id);
    }

    return 0;
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
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "Set CPU affinity failue, ERROR:%s", strerror(errno));
        return -1;
    }

    struct timespec wait_time = {0, 1000000000};
    nanosleep(&wait_time, 0);

    CPU_ZERO(&get);
    if(sched_getaffinity(getpid(), sizeof(get), &get) == -1)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "get CPU affinity failue, ERROR:%s", strerror(errno));
        return -1;
    }

    int i;
    for(i = 0; i < cpu_num; i++)
    {
        if(CPU_ISSET(i, &get))
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "this process %d of running processor: %d", getpid(), i);
        }
    }
    return 0;
}

class forwarder {
public:
    phy_mac_transport* sender;
    phy_mac_transport* receiver;
    char               name[NV_NAME_MAX_LEN];
    int                core_id;

    forwarder(phy_mac_transport* s, phy_mac_transport* r)
    {
        sender   = s;
        receiver = r;
        core_id  = 0;
    }
};

void* forwarder_thread(void* arg)
{
    forwarder* fw = (forwarder*)arg;
    NVLOGI(TAG, "%s start thread", __func__);

    assin_cpu_for_thread(fw->core_id);
    long sync_counter = 0;
    long fw_counter   = 0;
    while(1)
    {
        try
        {
            int msg_counter = 0;
            NVLOGD(TAG, "%s: ======== tti wait start ============", fw->name);
            fw->receiver->rx_wait();
            //if(fw->receiver->rx_wait() < 0)
            //{
            //    NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: rx_wait failed", __func__);
            //}
            NVLOGD(TAG, "%s: ------- tti wait end: sync_counter=%ld------------", fw->name, sync_counter);

            phy_mac_msg_desc recv_msg, send_msg;
            while(fw->receiver->rx_recv(recv_msg) >= 0)
            {
                send_msg.msg_id    = recv_msg.msg_id;
                send_msg.msg_len   = recv_msg.msg_len;
                send_msg.data_len  = recv_msg.data_len;
                send_msg.data_pool = recv_msg.data_pool;

                fw->sender->tx_alloc(send_msg);
                //if(fw->sender->tx_alloc(send_msg) < 0)
                //{
                //    NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: tx_alloc failed", fw->name);
                //}

                NVLOGD(TAG, "%s: msg_id=%d msg_len=%d data_len=%d data_pool=%d", fw->name, send_msg.msg_id, send_msg.msg_len, send_msg.data_len, send_msg.data_pool);

                if(sync_counter % 1000 == 0)
                {
                    NVLOGI(TAG, "%s: sync_counter=%ld msg_counter=%ld", fw->name, sync_counter, msg_counter);
                }

                if(recv_msg.msg_len > 0)
                {
                    memcpy(send_msg.msg_buf, recv_msg.msg_buf, recv_msg.msg_len);
                }
                else
                {
                    memcpy(send_msg.msg_buf, recv_msg.msg_buf, 512);
                }

                if(recv_msg.data_len > 0)
                {
                    memcpy(send_msg.data_buf, recv_msg.data_buf, recv_msg.data_len);
                }

                fw->receiver->rx_release(recv_msg);
                //if(fw->receiver->rx_release(recv_msg) < 0)
                //{
                //    NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: rx_release failed", fw->name);
                //}

                fw->sender->tx_send(send_msg);
                //if(fw->sender->tx_send(send_msg) < 0)
                //{
                //    NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: tx_send failed", fw->name);
                //}
                msg_counter++;
                fw_counter++;
            }

            fw->sender->tx_post();
            //if(fw->sender->tx_post() < 0)
            //{
            //    NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: tx_post failed", fw->name);
            //}
            NVLOGD(TAG, "%s: ------- fw post end: msg_counter=%d------------", fw->name, msg_counter);
            sync_counter++;
        }
        catch(std::exception& e)
        {
            NVLOGW(TAG, "EXCEPTION: %s", e.what());
        }
        catch(...)
        {
            NVLOGW(TAG, "Unknown exception");
        }
    }

    return NULL;
}

int main(int argc, const char* argv[])
{
    int return_value = 0;

    NVLOGI(TAG, "%s ", __func__);
    try
    {
        //--------------------------------------------------------------
        // Parse command line arguments
        if(argc < 2)
        {
            usage();
            exit(1);
        }

        nvlog_open(1, "nvipc_fw", nullptr);
        nvlog_set_shm_log_level(nullptr, NVLOG_INFO);

        yaml::file_parser fp(argv[1]);

        yaml::document    doc     = fp.next_document();
        phy_mac_transport gnb_phy = phy_mac_transport(doc.root()["transport"],
                                                      NV_IPC_MODULE_PRIMARY);

        doc                      = fp.next_document();
        phy_mac_transport ue_phy = phy_mac_transport(doc.root()["transport"], NV_IPC_MODULE_SECONDARY);

        forwarder fw1 = forwarder(&gnb_phy, &ue_phy);
        snprintf(fw1.name, NV_NAME_MAX_LEN, "%s", "DL");
        fw1.core_id   = 14;
        forwarder fw2 = forwarder(&ue_phy, &gnb_phy);
        snprintf(fw2.name, NV_NAME_MAX_LEN, "%s", "UL");
        fw2.core_id = 15;

        NVLOGI(TAG, "%s created transports", __func__);

        pthread_t thread_id1, thread_id2;
        if(pthread_create(&thread_id1, NULL, forwarder_thread, &fw1))
        {
            NVLOGI(TAG, "pthread_create failed ");
        }
        if(pthread_create(&thread_id2, NULL, forwarder_thread, &fw2))
        {
            NVLOGI(TAG, "pthread_create failed ");
        }

        if(pthread_join(thread_id1, NULL) != 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: pthread_join failed: thread_id1=%ld", __func__, thread_id1);
        }
        if(pthread_join(thread_id2, NULL) != 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: pthread_join failed: thread_id1=%ld", __func__, thread_id2);
        }
    }
    catch(std::exception& e)
    {
        fprintf(stderr, "EXCEPTION: %s", e.what());
        return_value = 1;
    }
    catch(...)
    {
        fprintf(stderr, "UNKNOWN EXCEPTION");
        return_value = 2;
    }
    return return_value;
}
