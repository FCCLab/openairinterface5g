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

/*****************************************************************************
Source      user_defs.h

Version     0.1

Date        2016/07/01

Product     NAS stack

Subsystem   NAS main process

Author      Frederic Leroy

Description NAS type definition to manage a user equipment

*****************************************************************************/
#ifndef __USER_DEFS_H__
#define __USER_DEFS_H__

#include "nas_proc_defs.h"
#include "esmData.h"
#include "esm_pt_defs.h"
#include "EMM/emm_fsm_defs.h"
#include "EMM/emmData.h"
#include "EMM/Authentication.h"
#include "EMM/IdleMode_defs.h"
#include "EMM/LowerLayer_defs.h"
#include "API/USIM/usim_api.h"
#include "API/USER/user_api_defs.h"
#include "SecurityModeControl.h"
#include "userDef.h"
#include "at_response.h"
#include "as_message.h"

enum ue_type {
  UE_LTE,
  UE_NR,
};

#define MAX_PDP_CONTEXTS 8

struct nas_user_s;

typedef struct nas_user_s {
  int ueid; /* UE lower layer identifier */
  proc_data_t proc;
  // Eps Session Management
  esm_data_t *esm_data; // ESM internal data (used within ESM only)
  esm_pt_data_t *esm_pt_data;
  esm_ebr_data_t *esm_ebr_data;  // EPS bearer contexts
  default_eps_bearer_context_data_t *default_eps_bearer_context_data;
  // Eps Mobility Management
  emm_fsm_state_t emm_fsm_status; // Current EPS Mobility Management status
  emm_data_t *emm_data; // EPS mobility management data
  const char *emm_nvdata_store;
  emm_plmn_list_t *emm_plmn_list; // list of PLMN identities
  authentication_data_t *authentication_data;
  security_data_t *security_data; //Internal data used for security mode control procedure
  // Hardware persistent storage
  usim_data_t usim_data; // USIM application data
  const char *usim_data_store; // USIM application data filename
  user_nvdata_t *nas_user_nvdata; //UE parameters stored in the UE's non-volatile memory device
  const char *user_nvdata_store; //UE parameters stored in the UE's non-volatile memory device
  //
  nas_user_context_t *nas_user_context;
  at_response_t *at_response; // data structure returned to the user as the result of NAS procedure function call
  //
  user_at_commands_t *user_at_commands; //decoded data received from the user application layer
  user_api_id_t *user_api_id;
  lowerlayer_data_t *lowerlayer_data;
  enum ue_type nas_ue_type;
  nr_nas_pdp_context_t pdp_context[MAX_PDP_CONTEXTS];
  int (*nas_reset_pdn)(struct nas_user_s *user, int cid);
  int (*nas_set_pdn)(struct nas_user_s *user,
                     int cid,
                     int type,
                     const char *apn,
                     int ipv4_addr,
                     int emergency,
                     int p_cscf,
                     int im_cn_signal,
                     const char *nssai);
  int (*nas_get_pdn)(struct nas_user_s *user, int *cids, int *types, const char **apns, const char **nssai, int n_pdn_max);
  int (*nas_get_pdn_range)(struct nas_user_s *user);
  int (*nas_deactivate_pdn)(struct nas_user_s *user, int cid);
  int (*nas_activate_pdn)(struct nas_user_s *user, int cid);
  int (*nas_get_pdn_status)(struct nas_user_s *user, int *cid, int *status, int n_pdn_max);
} nas_user_t;

#endif
