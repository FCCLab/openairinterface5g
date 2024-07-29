/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NV_IPC_HPP_INCLUDED_
#define NV_IPC_HPP_INCLUDED_

#include "nv_ipc.h"
#include "nv_ipc_utils.h"
#include "yaml.hpp"

int nv_ipc_parse_yaml_node(nv_ipc_config_t* cfg, yaml::node* yaml_node, nv_ipc_module_t module_type);

#endif /* NV_IPC_HPP_INCLUDED_ */
