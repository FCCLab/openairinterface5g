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

#include "ran_func_slice.h"
#include "../../flexric/test/rnd/fill_rnd_data_slice.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "openair2/LAYER2/NR_MAC_gNB/slicing/nr_slicing.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#include "common/ran_context.h"
#include "common/utils/LOG/log.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

extern RAN_CONTEXT_t RC;

static const int mod_id = 0;

bool read_slice_sm(void* data)
{
  assert(data != NULL);
//  assert(data->type == SLICE_STATS_V0);

  slice_ind_data_t* slice = (slice_ind_data_t*)data;
  fill_slice_ind_data(slice);

  return true;
}

void read_slice_setup_sm(void* data)
{
  assert(data != NULL);
//  assert(data->type == SLICE_AGENT_IF_E2_SETUP_ANS_V0 );

  assert(0 !=0 && "Not supported");
}

static bool nssai_matches(nssai_t a_nssai, uint8_t b_sst, const uint32_t *b_sd)
{
  AssertFatal(b_sd == NULL || *b_sd <= 0xffffff, "illegal SD %d\n", *b_sd);
  if (b_sd == NULL) {
    return a_nssai.sst == b_sst && a_nssai.sd == 0xffffff;
  } else {
    return a_nssai.sst == b_sst && a_nssai.sd == *b_sd;
  }
}

static int add_mod_dl_slice(int mod_id,
                            slice_algorithm_e current_algo,
                            int id,
                            nssai_t nssai,
                            char* label,
                            void* params)
{
  nr_pp_impl_param_dl_t *dl = &RC.nrmac[mod_id]->pre_processor_dl;
  char *slice_algo = NULL;
  if (current_algo == NVS_SLICING) {
    nvs_nr_slice_param_t * nvs_params = (nvs_nr_slice_param_t *)params;
    if (!nvs_params) return -1;
    slice_algo = strdup("NVS_CAPACITY");
    if (dl->slices && dl->slices->num > 0 && dl->slices->s[0] && dl->slices->s[0]->algo_data) {
      float remain_pct = 1.0f - ((nvs_nr_slice_param_t *)dl->slices->s[0]->algo_data)->pct_reserved;
      nvs_params->pct_reserved = nvs_params->pct_reserved / 100.0f * remain_pct;
    }
  } else {
    assert(0 != 0 && "Unknown current_algo");
  }

  void *algo = &dl->dl_algo;
  char *l = NULL;
  if (label)
    l = strdup(label);
  LOG_W(NR_MAC, "add DL slice id %d, label %s, slice sched algo %s, pct_reserved %.2f, ue sched algo %s\n", 
        id, l ? l : "NULL", slice_algo ? slice_algo : "NULL", 
        ((nvs_nr_slice_param_t *)params)->pct_reserved, dl->dl_algo.name);
  if (slice_algo) free(slice_algo);
  return dl->addmod_slice(dl->slices, id, nssai, l, algo, params);
}

static void set_new_dl_slice_algo(int mod_id, int algo)
{
  gNB_MAC_INST *nrmac = RC.nrmac[mod_id];
  assert(nrmac);

  nr_pp_impl_param_dl_t dl = nrmac->pre_processor_dl;
  switch (algo) {
    case NVS_SLICING:
      nrmac->pre_processor_dl = nvs_nr_dl_init(mod_id);
      break;
    default:
      nrmac->pre_processor_dl.algorithm = 0;
      nrmac->pre_processor_dl = nr_init_fr1_dlsch_preprocessor(0); // assume CC_id = 0
      nrmac->pre_processor_dl.slices = NULL;
      break;
  }
  if (dl.slices)
    dl.destroy(&dl.slices);
  if (dl.dl_algo.data)
    dl.dl_algo.unset(&dl.dl_algo.data);
}

static int find_slice_idx_by_id(nr_slice_info_t *si, int slice_id)
{
  if (!si || !si->s) return -1;
  for (int i = 0; i < si->num; i++) {
    if (si->s[i] && si->s[i]->id == slice_id) {
      return i;
    }
  }
  return -1;
}

static int handle_slice_add(const slice_conf_t *slice_conf)
{
  gNB_MAC_INST *nrmac = RC.nrmac[mod_id];
  if (!nrmac) {
    LOG_E(NR_MAC, "MAC instance not available\n");
    return -1;
  }

  int current_algo = nrmac->pre_processor_dl.algorithm;
  int new_algo = NVS_SLICING;

  NR_SCHED_LOCK(&nrmac->sched_lock);
  
  if (current_algo != new_algo) {
    set_new_dl_slice_algo(mod_id, new_algo);
    current_algo = new_algo;
    LOG_D(NR_MAC, "set new slicing algorithm %d\n", current_algo);
  }

  if (!nrmac->pre_processor_dl.slices || !nrmac->pre_processor_dl.slices->s) {
    NR_SCHED_UNLOCK(&nrmac->sched_lock);
    LOG_E(NR_MAC, "slice structure not initialized\n");
    return -1;
  }

  // Process DL slices
  for (uint32_t i = 0; i < slice_conf->dl.len_slices; i++) {
    fr_slice_t *fr_slice = &slice_conf->dl.slices[i];
    
    // TODO: Extract NSSAI from slice configuration
    // The slice SM control message structure doesn't directly include NSSAI in fr_slice_t
    // For now, we use a default NSSAI. In practice, NSSAI might need to be:
    // 1. Derived from slice ID mapping
    // 2. Provided in a separate field in the control message
    // 3. Extracted from UE associations after slice creation
    // Using slice ID as SST for now (with SD=0) as a temporary mapping
    nssai_t nssai = {.sst = (uint8_t)(fr_slice->id & 0xFF), .sd = 0};
    
    char *label = NULL;
    if (fr_slice->len_label > 0 && fr_slice->label) {
      label = strndup(fr_slice->label, fr_slice->len_label);
    }
    
    // Convert slice parameters to NVS format
    nvs_nr_slice_param_t *params = malloc(sizeof(nvs_nr_slice_param_t));
    if (!params) {
      if (label) free(label);
      NR_SCHED_UNLOCK(&nrmac->sched_lock);
      LOG_E(NR_MAC, "Memory exhausted\n");
      return -1;
    }
    
    if (fr_slice->params.type == SLICE_ALG_SM_V0_NVS) {
      if (fr_slice->params.u.nvs.conf == SLICE_SM_NVS_V0_CAPACITY) {
        params->type = NVS_RES;
        params->pct_reserved = fr_slice->params.u.nvs.u.capacity.u.pct_reserved / 100.0f;
      } else if (fr_slice->params.u.nvs.conf == SLICE_SM_NVS_V0_RATE) {
        params->type = NVS_RATE;
        params->Mbps_reserved = fr_slice->params.u.nvs.u.rate.u1.mbps_required;
        params->Mbps_reference = fr_slice->params.u.nvs.u.rate.u2.mbps_reference;
      } else {
        free(params);
        if (label) free(label);
        NR_SCHED_UNLOCK(&nrmac->sched_lock);
        LOG_E(NR_MAC, "Unsupported NVS slice type\n");
        return -1;
      }
    } else {
      free(params);
      if (label) free(label);
      NR_SCHED_UNLOCK(&nrmac->sched_lock);
      LOG_E(NR_MAC, "Only NVS slicing algorithm supported, got type %d\n", fr_slice->params.type);
      return -1;
    }
    
    int rc = add_mod_dl_slice(mod_id, current_algo, fr_slice->id, nssai, label, params);
    if (rc < 0) {
      free(params);
      if (label) free(label);
      NR_SCHED_UNLOCK(&nrmac->sched_lock);
      LOG_E(NR_MAC, "error code %d while adding slice id %d\n", rc, fr_slice->id);
      return -1;
    }
    
    // Associate existing UEs with matching NSSAI
    nr_pp_impl_param_dl_t *dl = &RC.nrmac[mod_id]->pre_processor_dl;
    NR_UEs_t *UE_info = &RC.nrmac[mod_id]->UE_info;
    UE_iterator(UE_info->connected_ue_list, UE) {
      NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
      for (size_t l = 0; l < seq_arr_size(&sched_ctrl->lc_config); ++l) {
        const nr_lc_config_t *c = seq_arr_at(&sched_ctrl->lc_config, l);
        if (nssai_matches(c->nssai, nssai.sst, &nssai.sd)) {
          dl->add_UE(dl->slices, UE);
          break;
        }
      }
    }
  }
  
  NR_SCHED_UNLOCK(&nrmac->sched_lock);
  LOG_D(NR_MAC, "All slices added/modified successfully!\n");
  return 0;
}

static int handle_slice_del(const del_slice_conf_t *del_conf)
{
  gNB_MAC_INST *nrmac = RC.nrmac[mod_id];
  if (!nrmac || !nrmac->pre_processor_dl.slices) {
    LOG_E(NR_MAC, "MAC instance or slice structure not available\n");
    return -1;
  }

  NR_SCHED_LOCK(&nrmac->sched_lock);
  
  nr_pp_impl_param_dl_t *dl = &nrmac->pre_processor_dl;
  
  // Delete DL slices
  for (uint32_t i = 0; i < del_conf->len_dl; i++) {
    int slice_idx = find_slice_idx_by_id(dl->slices, del_conf->dl[i]);
    if (slice_idx < 0) {
      LOG_W(NR_MAC, "Slice ID %d not found, skipping\n", del_conf->dl[i]);
      continue;
    }
    
    // Move UEs to default slice (index 0) before deletion
    if (slice_idx != 0 && dl->slices->num > 1) {
      UE_iterator(dl->slices->s[slice_idx]->UE_list, rm_ue) {
        if (rm_ue) {
          int old_idx = slice_idx;
          int new_idx = 0;
          dl->move_UE(dl->slices, rm_ue, old_idx, new_idx);
          LOG_D(NR_MAC, "Moved UE rnti 0x%04x from slice idx %d to default slice\n", rm_ue->rnti, old_idx);
        }
      }
    }
    
    int rc = dl->remove_slice(dl->slices, slice_idx);
    if (rc <= 0) {
      LOG_W(NR_MAC, "Failed to remove slice idx %d (ID %d)\n", slice_idx, del_conf->dl[i]);
    } else {
      LOG_D(NR_MAC, "Removed slice idx %d (ID %d)\n", slice_idx, del_conf->dl[i]);
    }
  }
  
  NR_SCHED_UNLOCK(&nrmac->sched_lock);
  LOG_D(NR_MAC, "Slice deletion completed\n");
  return 0;
}

static int handle_ue_slice_assoc(const ue_slice_conf_t *ue_conf)
{
  gNB_MAC_INST *nrmac = RC.nrmac[mod_id];
  if (!nrmac || !nrmac->pre_processor_dl.slices) {
    LOG_E(NR_MAC, "MAC instance or slice structure not available\n");
    return -1;
  }

  NR_SCHED_LOCK(&nrmac->sched_lock);
  
  nr_pp_impl_param_dl_t *dl = &nrmac->pre_processor_dl;
  NR_UEs_t *UE_info = &nrmac->UE_info;
  
  for (uint32_t i = 0; i < ue_conf->len_ue_slice; i++) {
    ue_slice_assoc_t *assoc = &ue_conf->ues[i];
    rnti_t rnti = assoc->rnti;
    int target_dl_idx = find_slice_idx_by_id(dl->slices, assoc->dl_id);
    
    if (target_dl_idx < 0) {
      LOG_W(NR_MAC, "Target DL slice ID %d not found for UE rnti 0x%04x\n", assoc->dl_id, rnti);
      continue;
    }
    
    // Find UE by RNTI
    NR_UE_info_t *ue = NULL;
    UE_iterator(UE_info->connected_ue_list, ue_iter) {
      if (ue_iter->rnti == rnti) {
        ue = ue_iter;
        break;
      }
    }
    
    if (!ue) {
      LOG_W(NR_MAC, "UE rnti 0x%04x not found\n", rnti);
      continue;
    }
    
    // Find current slice index for this UE
    int old_idx = dl->get_UE_slice_idx(dl->slices, rnti);
    if (old_idx < 0) {
      // UE not in any slice, add to target slice
      dl->add_UE(dl->slices, ue);
      LOG_D(NR_MAC, "Added UE rnti 0x%04x to slice idx %d (ID %d)\n", rnti, target_dl_idx, assoc->dl_id);
    } else if (old_idx != target_dl_idx) {
      // Move UE to new slice
      dl->move_UE(dl->slices, ue, old_idx, target_dl_idx);
      LOG_D(NR_MAC, "Moved UE rnti 0x%04x from slice idx %d to slice idx %d (ID %d)\n", 
            rnti, old_idx, target_dl_idx, assoc->dl_id);
    } else {
      LOG_D(NR_MAC, "UE rnti 0x%04x already in slice idx %d (ID %d)\n", rnti, target_dl_idx, assoc->dl_id);
    }
  }
  
  NR_SCHED_UNLOCK(&nrmac->sched_lock);
  LOG_D(NR_MAC, "UE-slice association completed\n");
  return 0;
}

sm_ag_if_ans_t write_ctrl_slice_sm(void const* data)
{
  assert(data != NULL);

  slice_ctrl_req_data_t const* slice_req_ctrl = (slice_ctrl_req_data_t const*)data;
  slice_ctrl_msg_t const* msg = &slice_req_ctrl->msg;

  sm_ag_if_ans_t ans = {.type = CTRL_OUTCOME_SM_AG_IF_ANS_V0};
  ans.ctrl_out.type = SLICE_AGENT_IF_CTRL_ANS_V0;
  ans.ctrl_out.slice.ans = SLICE_CTRL_OUT_OK;
  ans.ctrl_out.slice.len_diag = 0;
  ans.ctrl_out.slice.diagnostic = NULL;

  int rc = 0;
  const char *error_msg = NULL;

  switch (msg->type) {
    case SLICE_CTRL_SM_V0_ADD: {
      LOG_I(NR_MAC, "[E2 Agent]: SLICE CONTROL ADD received\n");
      rc = handle_slice_add(&msg->u.add_mod_slice);
      if (rc < 0) {
        error_msg = "Failed to add/modify slice";
        ans.ctrl_out.slice.ans = SLICE_CTRL_OUT_ERROR;
      } else {
        error_msg = "Slice added/modified successfully";
      }
      break;
    }
    
    case SLICE_CTRL_SM_V0_DEL: {
      LOG_I(NR_MAC, "[E2 Agent]: SLICE CONTROL DEL received\n");
      rc = handle_slice_del(&msg->u.del_slice);
      if (rc < 0) {
        error_msg = "Failed to delete slice";
        ans.ctrl_out.slice.ans = SLICE_CTRL_OUT_ERROR;
      } else {
        error_msg = "Slice deleted successfully";
      }
      break;
    }
    
    case SLICE_CTRL_SM_V0_UE_SLICE_ASSOC: {
      LOG_I(NR_MAC, "[E2 Agent]: SLICE CONTROL ASSOC received\n");
      rc = handle_ue_slice_assoc(&msg->u.ue_slice);
      if (rc < 0) {
        error_msg = "Failed to associate UE with slice";
        ans.ctrl_out.slice.ans = SLICE_CTRL_OUT_ERROR;
      } else {
        error_msg = "UE-slice association completed successfully";
      }
      break;
    }
    
    default: {
      error_msg = "Unknown slice control message type";
      ans.ctrl_out.slice.ans = SLICE_CTRL_OUT_ERROR;
      LOG_E(NR_MAC, "[E2 Agent]: Unknown msg_type %d!\n", msg->type);
      break;
    }
  }

  // Set diagnostic message
  if (error_msg) {
    ans.ctrl_out.slice.len_diag = strlen(error_msg);
    ans.ctrl_out.slice.diagnostic = malloc(ans.ctrl_out.slice.len_diag);
    if (ans.ctrl_out.slice.diagnostic) {
      memcpy(ans.ctrl_out.slice.diagnostic, error_msg, ans.ctrl_out.slice.len_diag);
    }
  }

  return ans;
}


