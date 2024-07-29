/*
 * Copyright (c) 2021-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "nv_ipc.h"
#include "nvlog.h"
#include "nv_ipc_config.h"

#include "nvlog.hpp"
#include "nv_ipc.hpp"

#define TAG "NVIPC:YAML"

int nv_ipc_parse_yaml_node(nv_ipc_config_t* cfg, yaml::node* yaml_node, nv_ipc_module_t module_type)
{
    yaml::node& node_config    = *yaml_node;
    std::string transport_type = node_config["type"].as<std::string>();
    //------------------------------------------------------------------
    // Set up default values. The set_nv_ipc_default_config() function
    // requires the transport type to be set before calling.
    if(0 == strcasecmp(transport_type.c_str(), "udp"))
    {
        cfg->ipc_transport = NV_IPC_TRANSPORT_UDP;
    }
    else if(0 == strcasecmp(transport_type.c_str(), "shm"))
    {
        cfg->ipc_transport = NV_IPC_TRANSPORT_SHM;
    }
    else if(0 == strcasecmp(transport_type.c_str(), "dpdk"))
    {
        cfg->ipc_transport = NV_IPC_TRANSPORT_DPDK;
    }
    else if(0 == strcasecmp(transport_type.c_str(), "doca"))
    {
        cfg->ipc_transport = NV_IPC_TRANSPORT_DOCA;
    }
    else
    {
        NVLOGE_NO_FMT(TAG, AERIAL_CONFIG_EVENT, "Unexpected YAML transport type: {}", transport_type, transport_type.c_str());
    }

    set_nv_ipc_default_config(cfg, module_type);

    int pcap_max_msg_size = 8192;
    //------------------------------------------------------------------
    // Populate fields from YAML nodes, using default values set above
    // it the YAML node does not provide them
    if(NV_IPC_TRANSPORT_UDP == cfg->ipc_transport)
    {
        yaml::node udp_config = node_config["udp_config"];
        if(udp_config.has_key("local_port"))
        {
            cfg->transport_config.udp.local_port = udp_config["local_port"].as<int32_t>();
        }
        if(udp_config.has_key("remote_port"))
        {
            cfg->transport_config.udp.remote_port = udp_config["remote_port"].as<int32_t>();
        }
        if(udp_config.has_key("local_addr"))
        {
            nvlog_safe_strncpy(cfg->transport_config.udp.local_addr, udp_config["local_addr"].as<std::string>().c_str(), NV_IPV4_STRING_LEN);
        }
        if(udp_config.has_key("remote_addr"))
        {
            nvlog_safe_strncpy(cfg->transport_config.udp.remote_addr, udp_config["remote_addr"].as<std::string>().c_str(), NV_IPV4_STRING_LEN);
        }
        if(udp_config.has_key("msg_buf_size"))
        {
            cfg->transport_config.udp.msg_buf_size = udp_config["msg_buf_size"].as<int32_t>();
        }
        if(udp_config.has_key("data_buf_size"))
        {
            cfg->transport_config.udp.data_buf_size = udp_config["data_buf_size"].as<int32_t>();
        }
        pcap_max_msg_size = cfg->transport_config.udp.msg_buf_size;
    }
    else if(NV_IPC_TRANSPORT_SHM == cfg->ipc_transport)
    {
        yaml::node shm_config = node_config["shm_config"];
        if(shm_config.has_key("primary"))
        {
            // "primary" value is decided by module_type
            // cfg->transport_config.shm.primary = shm_config["primary"].as<int>();
        }
        std::string prefix = shm_config["prefix"].as<std::string>();
        strncpy(cfg->transport_config.shm.prefix, prefix.c_str(), NV_NAME_MAX_LEN);
        cfg->transport_config.shm.prefix[NV_NAME_MAX_LEN - 1] = '\0';
        cfg->transport_config.shm.cuda_device_id              = shm_config["cuda_device_id"].as<int>();
        cfg->transport_config.shm.ring_len                    = shm_config["ring_len"].as<int32_t>();

		yaml::node mempoolsize = shm_config["mempool_size"];
        yaml::node cpu_msg = shm_config["mempool_size"]["cpu_msg"];
        cfg->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_CPU_MSG].buf_size =
            cpu_msg["buf_size"].as<int32_t>();
        cfg->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_CPU_MSG].pool_len =
            cpu_msg["pool_len"].as<int32_t>();

        yaml::node cpu_data = shm_config["mempool_size"]["cpu_data"];
        cfg->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_CPU_DATA].buf_size =
            cpu_data["buf_size"].as<int32_t>();
        cfg->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_CPU_DATA].pool_len =
            cpu_data["pool_len"].as<int32_t>();

        yaml::node cuda_data = shm_config["mempool_size"]["cuda_data"];
        cfg->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_CUDA_DATA].buf_size =
            cuda_data["buf_size"].as<int32_t>();
        cfg->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_CUDA_DATA].pool_len =
            cuda_data["pool_len"].as<int32_t>();

        if(mempoolsize.has_key("gpu_data")) {
            yaml::node gpu_data = shm_config["mempool_size"]["gpu_data"];
            cfg->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_GPU_DATA].buf_size =
                gpu_data["buf_size"].as<int32_t>();
            cfg->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_GPU_DATA].pool_len =
                gpu_data["pool_len"].as<int32_t>();
        }

        pcap_max_msg_size = cfg->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_CPU_MSG].buf_size;
    }
    else if(NV_IPC_TRANSPORT_DPDK == cfg->ipc_transport)
    {
        yaml::node dpdk_config = node_config["dpdk_config"];
        if(dpdk_config.has_key("primary"))
        {
            // "primary" value is decided by module_type
            // cfg->transport_config.dpdk.primary = dpdk_config["primary"].as<int>();
        }
        std::string prefix        = dpdk_config["prefix"].as<std::string>();
        std::string local_nic_pci = dpdk_config["local_nic_pci"].as<std::string>();
        std::string peer_nic_mac  = dpdk_config["peer_nic_mac"].as<std::string>();
        nvlog_safe_strncpy(cfg->transport_config.dpdk.prefix, prefix.c_str(), NV_NAME_MAX_LEN);
        nvlog_safe_strncpy(cfg->transport_config.dpdk.local_nic_pci, local_nic_pci.c_str(), NV_NAME_MAX_LEN);
        nvlog_safe_strncpy(cfg->transport_config.dpdk.peer_nic_mac, peer_nic_mac.c_str(), NV_NAME_MAX_LEN);

        if(dpdk_config.has_key("nic_mtu"))
        {
            cfg->transport_config.dpdk.nic_mtu = dpdk_config["nic_mtu"].as<int>();
        }
        else
        {
            // Set default MTU to 1536
            cfg->transport_config.dpdk.nic_mtu = 1536;
        }

        cfg->transport_config.dpdk.cuda_device_id = dpdk_config["cuda_device_id"].as<int>();
        cfg->transport_config.dpdk.lcore_id       = dpdk_config["lcore_id"].as<uint16_t>();
        cfg->transport_config.dpdk.need_eal_init  = dpdk_config["need_eal_init"].as<uint16_t>();

        yaml::node cpu_msg = dpdk_config["mempool_size"]["cpu_msg"];
        cfg->transport_config.dpdk.mempool_size[NV_IPC_MEMPOOL_CPU_MSG].buf_size =
            cpu_msg["buf_size"].as<int32_t>();
        cfg->transport_config.dpdk.mempool_size[NV_IPC_MEMPOOL_CPU_MSG].pool_len =
            cpu_msg["pool_len"].as<int32_t>();

        yaml::node cpu_data = dpdk_config["mempool_size"]["cpu_data"];
        cfg->transport_config.dpdk.mempool_size[NV_IPC_MEMPOOL_CPU_DATA].buf_size =
            cpu_data["buf_size"].as<int32_t>();
        cfg->transport_config.dpdk.mempool_size[NV_IPC_MEMPOOL_CPU_DATA].pool_len =
            cpu_data["pool_len"].as<int32_t>();

        yaml::node cuda_data = dpdk_config["mempool_size"]["cuda_data"];
        cfg->transport_config.dpdk.mempool_size[NV_IPC_MEMPOOL_CUDA_DATA].buf_size =
            cuda_data["buf_size"].as<int32_t>();
        cfg->transport_config.dpdk.mempool_size[NV_IPC_MEMPOOL_CUDA_DATA].pool_len =
            cuda_data["pool_len"].as<int32_t>();

        pcap_max_msg_size = cfg->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_CPU_MSG].buf_size;
    }
    else if(NV_IPC_TRANSPORT_DOCA == cfg->ipc_transport)
    {
        yaml::node doca_config = node_config["doca_config"];
        if(doca_config.has_key("primary"))
        {
            // "primary" value is decided by module_type
            // cfg->transport_config.doca.primary = doca_config["primary"].as<int>();
        }
        std::string prefix        = doca_config["prefix"].as<std::string>();
        std::string local_nic_pci = doca_config["host_pci"].as<std::string>();
        std::string peer_nic_mac  = doca_config["dpu_pci"].as<std::string>();
        nvlog_safe_strncpy(cfg->transport_config.doca.prefix, prefix.c_str(), NV_NAME_MAX_LEN);
        nvlog_safe_strncpy(cfg->transport_config.doca.host_pci, local_nic_pci.c_str(), NV_NAME_MAX_LEN);
        nvlog_safe_strncpy(cfg->transport_config.doca.dpu_pci, peer_nic_mac.c_str(), NV_NAME_MAX_LEN);

        cfg->transport_config.doca.cuda_device_id = doca_config["cuda_device_id"].as<int>();
        cfg->transport_config.doca.cpu_core       = doca_config["cpu_core"].as<uint16_t>();

        yaml::node cpu_msg = doca_config["mempool_size"]["cpu_msg"];
        cfg->transport_config.doca.mempool_size[NV_IPC_MEMPOOL_CPU_MSG].buf_size =
            cpu_msg["buf_size"].as<int32_t>();
        cfg->transport_config.doca.mempool_size[NV_IPC_MEMPOOL_CPU_MSG].pool_len =
            cpu_msg["pool_len"].as<int32_t>();

        yaml::node cpu_data = doca_config["mempool_size"]["cpu_data"];
        cfg->transport_config.doca.mempool_size[NV_IPC_MEMPOOL_CPU_DATA].buf_size =
            cpu_data["buf_size"].as<int32_t>();
        cfg->transport_config.doca.mempool_size[NV_IPC_MEMPOOL_CPU_DATA].pool_len =
            cpu_data["pool_len"].as<int32_t>();

        yaml::node cuda_data = doca_config["mempool_size"]["cuda_data"];
        cfg->transport_config.doca.mempool_size[NV_IPC_MEMPOOL_CUDA_DATA].buf_size =
            cuda_data["buf_size"].as<int32_t>();
        cfg->transport_config.doca.mempool_size[NV_IPC_MEMPOOL_CUDA_DATA].pool_len =
            cuda_data["pool_len"].as<int32_t>();

        pcap_max_msg_size = cfg->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_CPU_MSG].buf_size;
    }
    else
    {
        throw std::runtime_error(std::string("nv_phy_mac_transport: Unexpected type '") +
                                 transport_type +
                                 std::string("'"));
    }

    // Open APP configuration SHM pool if not exist
    nv_ipc_app_config_open(cfg);

    if(node_config.has_key("app_config") && (module_type == NV_IPC_MODULE_PHY || module_type == NV_IPC_MODULE_PRIMARY))
    {
        yaml::node app_config = node_config["app_config"];
        if(app_config.has_key("grpc_forward"))
        {
            nv_ipc_app_config_set(NV_IPC_CFG_FORWARD_ENABLE, app_config["grpc_forward"].as<int32_t>());
        }
        if(app_config.has_key("debug_timing"))
        {
            nv_ipc_app_config_set(NV_IPC_CFG_DEBUG_TIMING, app_config["debug_timing"].as<int32_t>());
        }
        if(app_config.has_key("pcap_enable"))
        {
            nv_ipc_app_config_set(NV_IPC_CFG_PCAP_ENABLE, app_config["pcap_enable"].as<int32_t>());
        }

        nv_ipc_app_config_set(NV_IPC_CFG_PCAP_CPU_CORE, app_config["pcap_cpu_core"].as<int32_t>());
        nv_ipc_app_config_set(NV_IPC_CFG_PCAP_CACHE_BIT, app_config["pcap_cache_size_bits"].as<int32_t>());
        nv_ipc_app_config_set(NV_IPC_CFG_PCAP_FILE_BIT, app_config["pcap_file_size_bits"].as<int32_t>());
        nv_ipc_app_config_set(NV_IPC_CFG_PCAP_MAX_DATA_SIZE, app_config["pcap_max_data_size"].as<int32_t>());

        nv_ipc_app_config_set(NV_IPC_CFG_PCAP_MAX_MSG_SIZE, pcap_max_msg_size);
    }

    return 0;
}

int load_nv_ipc_yaml_config(nv_ipc_config_t* cfg, const char* yaml_path, nv_ipc_module_t module_type)
{
    NVLOGI_FMT(TAG, "{}: {}", __FUNCTION__, yaml_path);
    yaml::file_parser fp(yaml_path);
    yaml::document    doc        = fp.next_document();
    yaml::node        yaml_root  = doc.root();

    if (yaml_root.has_key("nvipc_log")) {
        yaml::node nvlog_node = yaml_root["nvipc_log"];
        std::string log_file = nvlog_node["fmt_log_path"].as<std::string>();
        log_file.append(nvlog_node["fmt_log_name"].as<std::string>());
        if (module_type == NV_IPC_MODULE_PHY || module_type == NV_IPC_MODULE_SECONDARY) {
            log_file.append("_secondary.log");
        } else {
            log_file.append("_primary.log");
        }

        nvlog_c_init(log_file.c_str());
        nvlog_set_log_level(nvlog_node["log_level"].as<int>());
        nvlog_set_max_file_size(nvlog_node["fmt_log_max_size"].as<uint64_t>() * 1024 * 1024);
    }

    yaml::node nvipc_node = yaml_root["transport"];
    nv_ipc_parse_yaml_node(cfg, &nvipc_node, module_type);
    return 0;
}
