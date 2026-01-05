/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*!
 * \file   nr_slicing.h
 * \brief  General NR slice definition and helper parameters
 * \author Robert Schmidt
 * \date   2021
 * \email  robert.schmidt@eurecom.fr
 */

#ifndef NR_SLICING_H__
#define NR_SLICING_H__

#include "openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h"

typedef struct nr_slice_s {
  /// Arbitrary ID
  slice_id_t id;
  /// Arbitrary label
  char *label;

  nr_dl_sched_algo_t dl_algo;

  /// A specific algorithm's implementation parameters
  void *algo_data;
  /// Internal data that might be kept alongside a slice's params
  void *int_data;

  // list of users in this slice
  int num_UEs;
  NR_UE_info_t *UE_list[MAX_MOBILES_PER_GNB+1];

  nssai_t nssai;
} nr_slice_t;

typedef struct nr_slice_info_s {
  uint8_t num;
  nr_slice_t **s;
  uint8_t UE_assoc_slice[MAX_MOBILES_PER_GNB+1];
} nr_slice_info_t;

#define NVS_SLICING 20
/* arbitrary upper limit, increase if you want to instantiate more slices */
#define MAX_NVS_SLICES NR_MAX_NUM_SLICES
/* window for slice weight averaging -> 1s for fine granularity */
#define BETA 0.001f
typedef struct {
  enum nvs_type {NVS_RATE, NVS_RES} type;
  union {
    struct { float Mbps_reserved; float Mbps_reference; };
    struct { float pct_reserved; };
  };
} nvs_nr_slice_param_t;
nr_pp_impl_param_dl_t nvs_nr_dl_init(module_id_t mod_id);

#endif /* NR_SLICING_H__ */
