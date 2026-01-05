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
 * \file   nr_slicing.c
 * \brief  Generic NR Slicing helper functions and Static Slicing Implementation
 * \author Robert Schmidt
 * \date   2021
 * \email  robert.schmidt@eurecom.fr
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <dlfcn.h>

#include "assertions.h"
#include "common/utils/LOG/log.h"
#include "common/utils/nr/nr_common.h"

#include "NR_MAC_COMMON/nr_mac_extern.h"
#include "NR_MAC_COMMON/nr_mac.h"
#include "openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/LAYER2/RLC/rlc.h"

#include "nr_slicing.h"
#include "nr_slicing_internal.h"

#include "executables/softmodem-common.h"

extern RAN_CONTEXT_t RC;

#define RET_FAIL(ret, x...) do { LOG_E(MAC, x); return ret; } while (0)

int nr_slicing_get_UE_slice_idx(nr_slice_info_t *si, rnti_t rnti)
{
  for (int s_len = 0; s_len < si->num; s_len++) {
    for (int i = 0; i < MAX_MOBILES_PER_GNB; i++) {
      if (si->s[s_len]->UE_list[i] != NULL) {
        if (si->s[s_len]->UE_list[i]->rnti == rnti) {
          return s_len;
        }
      }
    }
  }
  LOG_E(NR_MAC, "cannot find slice idx for UE rnti 0x%04x\n", rnti);
  return -99;
}

int nr_slicing_get_UE_idx(nr_slice_t *si, rnti_t rnti)
{
  for (int i = 0; i < MAX_MOBILES_PER_GNB; i++) {
    if (si->UE_list[i] != NULL) {
      LOG_D(NR_MAC, "nr_slicing_get_UE_idx: si->UE_list[%d]->rnti %x map to rnti %x\n", i, si->UE_list[i]->rnti, rnti);
      if (si->UE_list[i]->rnti == rnti)
        return i;
    }
  }
  LOG_E(NR_MAC, "cannot find ue idx for UE rnti 0x%04x\n", rnti);
  return -99;
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

void nr_slicing_add_UE(nr_slice_info_t *si, NR_UE_info_t *new_ue)
{
  AssertFatal(si->num > 0 && si->s != NULL, "no slices exists, cannot add UEs\n");
  NR_UE_sched_ctrl_t *sched_ctrl = &new_ue->UE_sched_ctrl;
  long lcid = 0;
  reset_nr_list(&new_ue->dl_id);
  for (int i = 0; i < si->num; ++i) {
    reset_nr_list(&new_ue->UE_sched_ctrl.sliceInfo[i].lcid);
    for (int l = 0; l < sched_ctrl->dl_lc_num; ++l) {
      lcid = sched_ctrl->dl_lc_ids[l];
      if (nssai_matches(sched_ctrl->dl_lc_nssai[lcid], si->s[i]->nssai.sst, &si->s[i]->nssai.sd)) {

        add_nr_list(&new_ue->UE_sched_ctrl.sliceInfo[i].lcid, lcid);
        LOG_D(NR_MAC, "add lcid %ld to slice idx %d\n", lcid, i);

        if (!check_nr_list(&new_ue->dl_id, si->s[i]->id)) {
          add_nr_list(&new_ue->dl_id, si->s[i]->id);
          LOG_D(NR_MAC, "add dl id %d to new UE rnti %x\n", si->s[i]->id, new_ue->rnti);
        }

        UE_iterator(si->s[i]->UE_list, UE) {
          if (UE->rnti == new_ue->rnti)
            break;
        }
        if (UE == NULL) {
          int num_UEs = si->s[i]->num_UEs;
          if (si->s[i]->UE_list[num_UEs] == NULL) {
            si->s[i]->UE_list[num_UEs] = new_ue;
            si->s[i]->num_UEs += 1;
            LOG_W(NR_MAC, "Matched slice, Add UE rnti 0x%04x to slice idx %d, sst %d, sd %d\n",
                  new_ue->rnti, i, si->s[i]->nssai.sst, si->s[i]->nssai.sd);
          } else {
            LOG_E(NR_MAC, "cannot add new UE rnti 0x%04x to slice idx %d, num_UEs %d\n",
                  new_ue->rnti, i, si->s[i]->num_UEs);
          }
        }
      } else {
        LOG_D(NR_MAC, "cannot find matched slice (lcid %ld <sst %d sd %d>, slice idx %d <sst %d sd %d>), do nothing for UE rnti 0x%04x\n",
              lcid, sched_ctrl->dl_lc_nssai[lcid].sst, sched_ctrl->dl_lc_nssai[lcid].sd, i, si->s[i]->nssai.sst, si->s[i]->nssai.sd, new_ue->rnti);
      }
    }
  }
}

void nr_slicing_remove_UE(nr_slice_info_t *si, NR_UE_info_t* rm_ue, int idx)
{
  for (int i = 0; i < MAX_MOBILES_PER_GNB; i++) {
    if(si->s[idx]->UE_list[i] != NULL) {
      if (si->s[idx]->UE_list[i]->rnti == rm_ue->rnti) {
        si->s[idx]->UE_list[i] = NULL;
        si->s[idx]->num_UEs -= 1;
        remove_nr_list(&rm_ue->dl_id, si->s[idx]->id);
        break;
      }
    }
  }
}

void nr_slicing_move_UE(nr_slice_info_t *si, NR_UE_info_t* assoc_ue, int old_idx, int new_idx)
{
  DevAssert(new_idx >= -1 && new_idx < si->num);
  DevAssert(old_idx >= -1 && old_idx < si->num);

  // add UE to new slice
  remove_nr_list(&assoc_ue->dl_id, si->s[old_idx]->id);
  add_nr_list(&assoc_ue->dl_id, si->s[new_idx]->id);
  int cur_idx = si->s[new_idx]->num_UEs;
  si->s[new_idx]->UE_list[cur_idx] = assoc_ue;
  si->s[new_idx]->num_UEs += 1;

  // remove from old slice
  for (int i = 0; i < MAX_MOBILES_PER_GNB; i++) {
    if(si->s[old_idx]->UE_list[i] != NULL) {
      if (si->s[old_idx]->UE_list[i]->rnti == assoc_ue->rnti) {
        si->s[old_idx]->UE_list[i] = NULL;
        si->s[old_idx]->num_UEs -= 1;
        break;
      }
    }
  }

  // reorder UE_list
  int n, m = 0;
  for (n = 0; n < MAX_MOBILES_PER_GNB; n++) {
    if (si->s[old_idx]->UE_list[n] != NULL) {
      si->s[old_idx]->UE_list[m++] = si->s[old_idx]->UE_list[n];
    }
  }
  while (m < MAX_MOBILES_PER_GNB) {
    si->s[old_idx]->UE_list[m++] = NULL;
  }
}

int _nr_exists_slice(uint8_t n, nr_slice_t **s, int id)
{
  for (int i = 0; i < n; ++i) {
    LOG_D(NR_MAC, "_nr_exists_slice(): n %d, s[%d]->id %d, id %d\n", n ,i, s[i]->id, id);
    if (s[i]->id == id)
      return i;
  }
  return -1;
}

nr_slice_t *_nr_add_slice(uint8_t *n, nr_slice_t **s)
{
  s[*n] = calloc(1, sizeof(nr_slice_t));
  if (!s[*n])
    return NULL;
  *n += 1;
  return s[*n - 1];
}

nr_slice_t *_nr_remove_slice(uint8_t *n, nr_slice_t **s, int idx)
{
  if (idx >= *n)
    return NULL;

  nr_slice_t *sr = s[idx];

  for (int i = idx + 1; i < *n; ++i)
    s[i - 1] = s[i];
  *n -= 1;
  s[*n] = NULL;

  if (sr->label)
    free(sr->label);

  return sr;
}

/************************* NVS Slicing Implementation **************************/

typedef struct {
  float exp; // exponential weight. mov. avg for weight calc
  int   rb;  // number of RBs this slice has been scheduled in last round
  float eff; // effective rate for rate slices
  float beta_eff; // averaging coeff so we average over roughly one second
  int   active;   // activity state for rate slices
} _nvs_int_t;

int _nvs_nr_admission_control(const nr_slice_info_t *si,
                              const nvs_nr_slice_param_t *p,
                              int idx)
{
  if (p->type != NVS_RATE && p->type != NVS_RES)
    RET_FAIL(-1, "%s(): invalid slice type %d\n", __func__, p->type);
  if (p->type == NVS_RATE && p->Mbps_reserved > p->Mbps_reference)
    RET_FAIL(-1,
             "%s(): a rate slice cannot reserve more than the reference rate\n",
             __func__);
  if (p->type == NVS_RES && p->pct_reserved > 1.0f)
    RET_FAIL(-1, "%s(): cannot reserve more than 1.0\n", __func__);
  float sum_req = 0.0f;
  for (int i = 0; i < si->num; ++i) {
    const nvs_nr_slice_param_t *sp = i == idx ? p : si->s[i]->algo_data;
    if (sp->type == NVS_RATE) {
      sum_req += sp->Mbps_reserved / sp->Mbps_reference;
    } else {
      sum_req += sp->pct_reserved;
    }
    LOG_D(NR_MAC, "slice idx %d, sum_req %.2f\n", i, sum_req);
  }
  if (idx < 0) { /* not an existing slice */
    if (p->type == NVS_RATE)
      sum_req += p->Mbps_reserved / p->Mbps_reference;
    else
      sum_req += p->pct_reserved;
  }
  LOG_D(NR_MAC, "slice idx %u, pct_reserved %.2f, sum_req %.2f\n", idx, p->pct_reserved, sum_req);
  if (sum_req > 1.0)
    RET_FAIL(-3,
             "%s(): admission control failed: sum of resources is %f > 1.0\n",
             __func__, sum_req);
  return 0;
}

int addmod_nvs_nr_slice_dl(nr_slice_info_t *si,
                           int id,
                           nssai_t nssai,
                           char *label,
                           void *algo,
                           void *slice_params_dl)
{
  nvs_nr_slice_param_t *dl = slice_params_dl;
  int index = _nr_exists_slice(si->num, si->s, id);
  if (index < 0 && si->num >= MAX_NVS_SLICES)
    RET_FAIL(-2, "%s(): cannot handle more than %d slices\n", __func__, MAX_NVS_SLICES);

  if (index < 0 && !dl)
    RET_FAIL(-100, "%s(): no parameters for new slice %d, aborting\n", __func__, id);

  if (dl) {
    int rc = _nvs_nr_admission_control(si, dl, index);
    if (rc < 0)
      return rc;
  }

  nr_slice_t *s = NULL;
  if (index >= 0) {
    s = si->s[index];
    if (label) {
      if (s->label) free(s->label);
      s->label = label;
    }
    if (algo && s->dl_algo.run != ((nr_dl_sched_algo_t*)algo)->run) {
      s->dl_algo.unset(&s->dl_algo.data);
      s->dl_algo = *(nr_dl_sched_algo_t*) algo;
      if (!s->dl_algo.data)
        s->dl_algo.data = s->dl_algo.setup();
    }
    if (dl) {
      free(s->algo_data);
      s->algo_data = dl;
    } else { /* we have no parameters: we are done */
      return index;
    }
    s->nssai = nssai;
  } else {
    if (!algo)
      RET_FAIL(-14, "%s(): no scheduler algorithm provided\n", __func__);

    s = _nr_add_slice(&si->num, si->s);
    if (!s)
      RET_FAIL(-4, "%s(): cannot allocate memory for slice\n", __func__);
    s->int_data = malloc(sizeof(_nvs_int_t));
    if (!s->int_data)
      RET_FAIL(-5, "%s(): cannot allocate memory for slice internal data\n", __func__);

    s->id = id;
    s->label = label;
    s->dl_algo = *(nr_dl_sched_algo_t*) algo;
    if (!s->dl_algo.data)
      s->dl_algo.data = s->dl_algo.setup();
    s->algo_data = dl;
    s->nssai = nssai;
  }

  _nvs_int_t *nvs_p = s->int_data;
  /* reset all slice-internal parameters */
  nvs_p->rb = 0;
  nvs_p->active = 0;
  if (dl->type == NVS_RATE) {
    nvs_p->exp = dl->Mbps_reserved / dl->Mbps_reference;
    nvs_p->eff = dl->Mbps_reference;
  } else {
    nvs_p->exp = dl->pct_reserved;
    nvs_p->eff = 0; // not used
  }
  // scale beta so we (roughly) average the eff rate over 1s
  nvs_p->beta_eff = BETA / nvs_p->exp;

  return index < 0 ? si->num - 1 : index;
}

int remove_nvs_nr_slice_dl(nr_slice_info_t *si, uint8_t slice_idx)
{
  if (slice_idx == 0 && si->num <= 1)
    return 0;
  UE_iterator(si->s[slice_idx]->UE_list, rm_ue) {
    nr_slicing_remove_UE(si, rm_ue, slice_idx);
    remove_nr_list(&rm_ue->dl_id, si->s[slice_idx]->id);
    LOG_D(NR_MAC, "%s(), move UE rnti 0x%04x in slice ID %d idx %d to slice ID %d idx %d\n",
          __func__, rm_ue->rnti, si->s[slice_idx]->id, slice_idx, si->s[0]->id, 0);
    for (int i = 0; i < MAX_MOBILES_PER_GNB; i++) {
      if (si->s[0]->UE_list[i] == NULL) {
        si->s[0]->UE_list[i] = rm_ue;
        break;
      }
    }
  }

  nr_slice_t *sr = _nr_remove_slice(&si->num, si->s, slice_idx);
  if (!sr)
    return 0;
  free(sr->algo_data);
  free(sr->int_data);
  sr->dl_algo.unset(&sr->dl_algo.data);
  free(sr);
  return 1;
}

static void nr_store_dlsch_buffer(module_id_t module_id, frame_t frame, sub_frame_t slot)
{
  nr_slice_info_t *si = RC.nrmac[module_id]->pre_processor_dl.slices;
  for (int s = 0; s < si->num; s++) {
    UE_iterator (si->s[s]->UE_list, UE) {
      NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
      sched_ctrl->sliceInfo[s].num_total_bytes = 0;
      sched_ctrl->sliceInfo[s].dl_pdus_total = 0;
      NR_List_Iterator(&UE->UE_sched_ctrl.sliceInfo[s].lcid, lcidP)
      {
        const int lcid = *lcidP;
        const uint16_t rnti = UE->rnti;
        LOG_D(NR_MAC, "In %s: UE %x: LCID %d\n", __FUNCTION__, rnti, lcid);
        if (lcid == DL_SCH_LCID_DTCH && sched_ctrl->rrc_processing_timer > 0) {
          continue;
        }
        start_meas(&RC.nrmac[module_id]->rlc_status_ind);
        sched_ctrl->rlc_status[lcid] =
            mac_rlc_status_ind(module_id, rnti, module_id, frame, slot, ENB_FLAG_YES, MBMS_FLAG_NO, lcid, 0, 0);
        stop_meas(&RC.nrmac[module_id]->rlc_status_ind);

        if (sched_ctrl->rlc_status[lcid].bytes_in_buffer == 0)
          continue;

        sched_ctrl->sliceInfo[s].dl_pdus_total += sched_ctrl->rlc_status[lcid].pdus_in_buffer;
        sched_ctrl->sliceInfo[s].num_total_bytes += sched_ctrl->rlc_status[lcid].bytes_in_buffer;
        LOG_D(MAC,
              "[gNB %d][%4d.%2d] %s%d->DLSCH, RLC status for UE %d, slice %d: %d bytes in buffer, total DL buffer size = %d bytes, "
              "%d total PDU bytes, %s TA command\n",
              module_id,
              frame,
              slot,
              lcid < 4 ? "DCCH" : "DTCH",
              lcid,
              UE->rnti,
              s,
              sched_ctrl->rlc_status[lcid].bytes_in_buffer,
              sched_ctrl->sliceInfo[s].num_total_bytes,
              sched_ctrl->sliceInfo[s].dl_pdus_total,
              sched_ctrl->ta_apply ? "send" : "do not send");
      }
    }
  }
}

void nvs_nr_dl(module_id_t mod_id,
               frame_t frame,
               sub_frame_t slot)
{
  gNB_MAC_INST *nrmac = RC.nrmac[mod_id];
  NR_UEs_t *UE_info = &nrmac->UE_info;
  if (UE_info->list[0] == NULL) /* no UEs at all -> don't bother */
    return;

  /* check if we are supposed to schedule something */
  NR_ServingCellConfigCommon_t *scc = RC.nrmac[mod_id]->common_channels[0].ServingCellConfigCommon;
  const int CC_id = 0;
  /* Get bwpSize and TDAfrom the first UE */
  /* This is temporary and it assumes all UEs have the same BWP and TDA*/
  NR_UE_info_t *first_UE=UE_info->list[0];
  NR_UE_sched_ctrl_t *sched_ctrl = &first_UE->UE_sched_ctrl;
  NR_UE_DL_BWP_t *current_BWP = &first_UE->current_DL_BWP;
  const int tda = get_dl_tda(RC.nrmac[mod_id], scc, slot);
  int startSymbolIndex, nrOfSymbols;
  const int coresetid = sched_ctrl->coreset->controlResourceSetId;
  const struct NR_PDSCH_TimeDomainResourceAllocationList *tdaList = get_dl_tdalist(current_BWP, coresetid, sched_ctrl->search_space->searchSpaceType->present, TYPE_C_RNTI_);
  AssertFatal(tda < tdaList->list.count, "time_domain_allocation %d>=%d\n", tda, tdaList->list.count);
  const int startSymbolAndLength = tdaList->list.array[tda]->startSymbolAndLength;
  SLIV2SL(startSymbolAndLength, &startSymbolIndex, &nrOfSymbols);

  const uint16_t bwpSize = current_BWP->BWPSize;
  const uint16_t BWPStart = current_BWP->BWPStart;

  const uint16_t slbitmap = SL_to_bitmap(startSymbolIndex, nrOfSymbols);
  uint16_t *vrb_map = RC.nrmac[mod_id]->common_channels[CC_id].vrb_map;
  uint16_t rballoc_mask[bwpSize];
  int n_rb_sched = 0;

  for (int i = 0; i < bwpSize; i++) {
    // calculate mask: init with "NOT" vrb_map:
    // if any RB in vrb_map is blocked (1), the current RBG will be 0
    rballoc_mask[i] = (~vrb_map[i+BWPStart])&0x3fff; //bitwise not and 14 symbols

    // if all the pdsch symbols are free
    if ((rballoc_mask[i]&slbitmap) == slbitmap) {
      n_rb_sched++;
    }
  }

  /* Retrieve amount of data to send */
  nr_store_dlsch_buffer(mod_id, frame, slot);

  int bw = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;
  int average_agg_level = 4; // TODO find a better estimation
  int max_sched_ues = bw / (average_agg_level * NR_NB_REG_PER_CCE);

  nr_slice_info_t *si = RC.nrmac[mod_id]->pre_processor_dl.slices;
  int bytes_last_round[MAX_NVS_SLICES] = {0};
  for (int s_idx = 0; s_idx < si->num; ++s_idx) {
    UE_iterator (si->s[s_idx]->UE_list, UE) {
      const NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
      bytes_last_round[s_idx] += UE->mac_stats.dl.current_bytes;

      /* if UE has data or retransmission, mark respective slice as active */
      const int retx_pid = sched_ctrl->retrans_dl_harq.head;
      const int retx_slice = sched_ctrl->harq_slice_map[retx_pid];
      const bool active = sched_ctrl->sliceInfo[s_idx].num_total_bytes > 0 || retx_slice == s_idx;
      ((_nvs_int_t *)si->s[s_idx]->int_data)->active |= active;
    }
  }

  float maxw = 0.0f;
  int maxidx = -1;
  for (int i = 0; i < si->num; ++i) {
    nr_slice_t *s = si->s[i];
    nvs_nr_slice_param_t *p = s->algo_data;
    _nvs_int_t *ip = s->int_data;

    float w = 0.0f;
    if (p->type == NVS_RATE) {
      /* if this slice has been marked as inactive, disable to prevent that
       * it's exp rate is uselessly driven down */
      if (!ip->active)
        continue;
      float inst = 0.0f;
      if (ip->rb > 0) { /* it was scheduled last round */
        /* inst rate: B in last round * 8(bit) / 1000000 (Mbps) * 1000 (1ms) */
        inst = (float) bytes_last_round[i] * 8 / 1000;
        ip->eff = (1.0f - ip->beta_eff) * ip->eff + ip->beta_eff * inst;
        //LOG_W(NR_MAC, "i %d slice %d ip->rb %d inst %f ip->eff %f\n", i, s->id, ip->rb, inst, ip->eff);
        ip->rb = 0;
      }
      ip->exp = (1 - BETA) * ip->exp + BETA * inst;
      const float rsv = p->Mbps_reserved * min(1.0f, ip->eff / p->Mbps_reference);
      w = rsv / ip->exp;
    } else {
      float inst = (float)ip->rb / bwpSize;
      ip->exp = (1.0f - BETA) * ip->exp + BETA * inst;
      w = p->pct_reserved / ip->exp;
    }
    //LOG_I(NR_MAC, "i %d slice %d type %d ip->exp %f w %f\n", i, s->id, p->type, ip->exp, w);
    ip->rb = 0;
    if (w > maxw + 0.001f) {
      maxw = w;
      maxidx = i;
    }
  }

  if (maxidx < 0)
    return;

  ((_nvs_int_t *)si->s[maxidx]->int_data)->rb = n_rb_sched;

  UE_iterator (si->s[maxidx]->UE_list, UE) {
    UE->UE_sched_ctrl.last_sched_slice = maxidx;
  }

  /* proportional fair scheduling algorithm */
  int rb_rem = si->s[maxidx]->dl_algo.run(mod_id,
                                          frame,
                                          slot,
                                          si->s[maxidx]->UE_list,
                                          max_sched_ues,
                                          n_rb_sched,
                                          rballoc_mask,
                                          si->s[maxidx]->dl_algo.data);
  LOG_D(NR_MAC, "%4d.%2d schedule %d RBs at slice idx %d ID %d \n", frame, slot, n_rb_sched - rb_rem, maxidx, si->s[maxidx]->id);

  if (rb_rem == n_rb_sched) // if no RBs have been used mark as inactive
    ((_nvs_int_t *)si->s[maxidx]->int_data)->active = 0;
}

void nvs_nr_destroy(nr_slice_info_t **si)
{
  const int n_dl = (*si)->num;
  (*si)->num = 0;
  for (int i = 0; i < n_dl; ++i) {
    nr_slice_t *s = (*si)->s[i];
    if (s->label)
      free(s->label);
    free(s->algo_data);
    free(s->int_data);
    free(s);
  }
  free((*si)->s);
}

nr_pp_impl_param_dl_t nvs_nr_dl_init(module_id_t mod_id)
{
  nr_slice_info_t *si = calloc(1, sizeof(nr_slice_info_t));
  DevAssert(si);

  si->num = 0;
  si->s = calloc(MAX_NVS_SLICES, sizeof(*si->s));
  DevAssert(si->s);
  for (int i = 0; i < MAX_MOBILES_PER_GNB; ++i)
    si->UE_assoc_slice[i] = -1;

  /* insert default slice, all resources */
  nvs_nr_slice_param_t *dlp = malloc(sizeof(nvs_nr_slice_param_t));
  DevAssert(dlp);
  dlp->type = NVS_RES;
  // we reserved 5% resource for RRC connection while UE is connecting before created slice or
  // PDU setup while UE is trying to connect after created slice
  dlp->pct_reserved = 0.05f;
  nr_dl_sched_algo_t *algo = &RC.nrmac[mod_id]->pre_processor_dl.dl_algo;
  algo->data = NULL;
  // default slice: sst = 1, sd = 0x000000, id = 999, label = default
  nssai_t nssai = {.sst = 0, .sd = 0};
  const int rc = addmod_nvs_nr_slice_dl(si, 99, nssai, strdup("default"), algo, dlp);
  LOG_W(NR_MAC, "Add default DL slice id 99, label default, sst %d, sd %d, slice sched algo NVS_CAPACITY, pct_reserved %.2f, ue sched algo %s\n", nssai.sst, nssai.sd, dlp->pct_reserved, algo->name);
  DevAssert(0 == rc);


  nr_pp_impl_param_dl_t nvs;
  nvs.algorithm = NVS_SLICING;
  nvs.add_UE = nr_slicing_add_UE;
  nvs.remove_UE = nr_slicing_remove_UE;
  nvs.move_UE = nr_slicing_move_UE;
  nvs.get_UE_slice_idx = nr_slicing_get_UE_slice_idx;
  nvs.get_UE_idx = nr_slicing_get_UE_idx;
  nvs.addmod_slice = addmod_nvs_nr_slice_dl;
  nvs.remove_slice = remove_nvs_nr_slice_dl;
  nvs.dl = nvs_nr_dl;
  // current DL algo becomes default scheduler
  nvs.dl_algo = *algo;
  nvs.destroy = nvs_nr_destroy;
  nvs.slices = si;

  return nvs;
}
