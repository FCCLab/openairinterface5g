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

#ifndef RAN_FUNC_SM_RAN_CTRL_SUBSCRIPTION_AGENT_H
#define RAN_FUNC_SM_RAN_CTRL_SUBSCRIPTION_AGENT_H

#include "openair2/E2AP/flexric/src/sm/rc_sm/ie/rc_data_ie.h"
#include "common/utils/ds/seq_arr.h"

typedef enum {    // 8.2.1 RAN Parameters for Report Service Style 1
  E2SM_RC_RS1_UE_EVENT_ID = 1,
  E2SM_RC_RS1_NI_MESSAGE = 2,
  E2SM_RC_RS1_RRC_MESSAGE = 3,
  E2SM_RC_RS1_UE_ID = 4,
  E2SM_RC_RS1_OLD_AMF_UE_NGAP_ID = 5,
  E2SM_RC_RS1_CELL_GLOBAL_ID = 6,

  END_E2SM_RC_RS1_RAN_PARAM_ID
} report_style_1_ran_param_id_e;

typedef enum {    // 8.2.2 RAN Parameters for Report Service Style 2
  E2SM_RC_RS2_CURRENT_UE_ID = 1,
  E2SM_RC_RS2_OLD_UE_ID = 2,
  E2SM_RC_RS2_CURRENT_RRC_STATE = 3,
  E2SM_RC_RS2_OLD_RRC_STATE = 4,
  E2SM_RC_RS2_UE_CONTEXT_INFORMATION_CONTAINER = 5,
  E2SM_RC_RS2_CELL_GLOBAL_ID = 6,
  E2SM_RC_RS2_UE_INFORMATION = 7,

  END_E2SM_RC_RS2_RAN_PARAM_ID
} report_style_2_ran_param_id_e;

typedef enum {    // 8.2.3 RAN Parameters for Report Service Style 3
  E2SM_RC_RS3_CELL_CONTEXT_INFORMATION = 1,
  E2SM_RC_RS3_CELL_DELETED = 2,
  E2SM_RC_RS3_NEIGHBOUR_RELATION_TABLE = 3,

  END_E2SM_RC_RS3_RAN_PARAM_ID
} report_style_3_ran_param_id_e;

typedef enum {    // 8.2.4 RAN Parameters for Report Service Style 4
  E2SM_RC_RS4_UL_MAC_CE = 100,
  E2SM_RC_RS4_DL_MAC_CE = 101,
  E2SM_RC_RS4_DL_BUFFER_OCCUPANCY = 102,
  E2SM_RC_RS4_CURRENT_RRC_STATE = 201,
  E2SM_RC_RS4_RRC_STATE_CHANGED_TO = 202,
  E2SM_RC_RS4_RRC_MESSAGE = 203,
  E2SM_RC_RS4_OLD_UE_ID = 300,
  E2SM_RC_RS4_CURRENT_UE_ID = 301,
  E2SM_RC_RS4_NI_MESSAGE = 302,
  E2SM_RC_RS4_CELL_GLOBAL_ID = 400,

  END_E2SM_RC_RS4_RAN_PARAM_ID
} report_style_4_ran_param_id_e;

typedef enum {    // 8.2.5 RAN Parameters for Report Service Style 5
  E2SM_RC_RS5_UE_CONTEXT_INFORMATION = 1,
  E2SM_RC_RS5_CELL_CONTEXT_INFORMATION = 2,
  E2SM_RC_RS5_NEIGHBOUR_RELATION_TABLE = 3,

  END_E2SM_RC_RS5_RAN_PARAM_ID
} report_style_5_ran_param_id_e;

typedef struct ran_param_data {
  uint32_t ric_req_id;
  e2sm_rc_event_trigger_t ev_tr;
} ran_param_data_t;

typedef struct {
  seq_arr_t rs4_param202; // E2SM_RC_RS4_RRC_STATE_CHANGED_TO
} rc_subs_data_t;

void init_rc_subs_data(rc_subs_data_t *rc_subs_data);
void insert_rc_subs_data(seq_arr_t *seq_arr, ran_param_data_t *data);
void remove_rc_subs_data(rc_subs_data_t *rc_subs_data, uint32_t ric_req_id);

#endif
