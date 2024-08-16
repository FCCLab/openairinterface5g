#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "openair2/LAYER2/NR_MAC_gNB/slicing/nr_slicing.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#include "rc_ctrl_service_style_2.h"

static int add_mod_dl_slice(int mod_id,
                     slice_algorithm_e current_algo,
                     int id,
                     nssai_t nssai,
                     char* label,
                     void* params)
{
  nr_pp_impl_param_dl_t *dl = &RC.nrmac[mod_id]->pre_processor_dl;
  char *slice_algo = NULL;
  nvs_nr_slice_param_t * nvs_params = (nvs_nr_slice_param_t *)params;
  float remain_pct = 1-((nvs_nr_slice_param_t *)dl->slices->s[0]->algo_data)->pct_reserved;
  
  if (current_algo == NVS_SLICING) {
    if (!nvs_params) return -1;
    slice_algo = strdup("NVS_CAPACITY");
    nvs_params->pct_reserved = nvs_params->pct_reserved/100.0*remain_pct;
  } else {
    assert(0 != 0 && "Unknow current_algo");
  }

  void *algo = &dl->dl_algo;
  char *l = NULL;
  if (label)
    l = strdup(label);
  LOG_W(NR_MAC, "add DL slice id %d, label %s, slice sched algo %s, pct_reserved %.2f/remain_pct %.2f, ue sched algo %s\n", id, l, slice_algo, ((nvs_nr_slice_param_t *)params)->pct_reserved, remain_pct, dl->dl_algo.name);
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

static bool nssai_matches(nssai_t a_nssai, uint8_t b_sst, const uint32_t *b_sd)
{
  AssertFatal(b_sd == NULL || *b_sd <= 0xffffff, "illegal SD %d\n", *b_sd);
  if (b_sd == NULL) {
    return a_nssai.sst == b_sst && a_nssai.sd == 0xffffff;
  } else {
    return a_nssai.sst == b_sst && a_nssai.sd == *b_sd;
  }
}

bool add_mod_rc_slice(int mod_id, size_t slices_len, ran_param_list_t* lst)
{
  gNB_MAC_INST *nrmac = RC.nrmac[mod_id];
  assert(nrmac);

  int current_algo = nrmac->pre_processor_dl.algorithm;
  // use NVS algorithm by default
  int new_algo = NVS_SLICING;

  pthread_mutex_lock(&nrmac->UE_info.mutex);
  if (current_algo != new_algo) {
    set_new_dl_slice_algo(mod_id, new_algo);
    current_algo = new_algo;
    if (new_algo > 0)
      LOG_D(NR_MAC, "set new algorithm %d\n", current_algo);
    else
      LOG_W(NR_MAC, "reset slicing algorithm as NONE\n");
  }

  for (size_t i = 0; i < slices_len; ++i) {
    lst_ran_param_t* RRM_Policy_Ratio_Group = &lst->lst_ran_param[i];
    //Bug in rc_enc_asn.c:1003, asn didn't define ran_param_id for lst_ran_param_t...
    //assert(RRM_Policy_Ratio_Group->ran_param_id == RRM_Policy_Ratio_Group_8_4_3_6 && "wrong RRM_Policy_Ratio_Group id");
    assert(RRM_Policy_Ratio_Group->ran_param_struct.sz_ran_param_struct == 4 && "wrong RRM_Policy_Ratio_Group->ran_param_struct.sz_ran_param_struct");
    assert(RRM_Policy_Ratio_Group->ran_param_struct.ran_param_struct != NULL && "NULL RRM_Policy_Ratio_Group->ran_param_struct.ran_param_struct");

    seq_ran_param_t* RRM_Policy = &RRM_Policy_Ratio_Group->ran_param_struct.ran_param_struct[0];
    assert(RRM_Policy->ran_param_id == RRM_Policy_8_4_3_6 && "wrong RRM_Policy id");
    assert(RRM_Policy->ran_param_val.type == STRUCTURE_RAN_PARAMETER_VAL_TYPE && "wrong RRM_Policy type");
    assert(RRM_Policy->ran_param_val.strct != NULL && "NULL RRM_Policy->ran_param_val.strct");
    assert(RRM_Policy->ran_param_val.strct->sz_ran_param_struct == 1 && "wrong RRM_Policy->ran_param_val.strct->sz_ran_param_struct");
    assert(RRM_Policy->ran_param_val.strct->ran_param_struct != NULL && "NULL RRM_Policy->ran_param_val.strct->ran_param_struct");

    seq_ran_param_t* RRM_Policy_Member_List = &RRM_Policy->ran_param_val.strct->ran_param_struct[0];
    assert(RRM_Policy_Member_List->ran_param_id == RRM_Policy_Member_List_8_4_3_6 && "wrong RRM_Policy_Member_List id");
    assert(RRM_Policy_Member_List->ran_param_val.type == LIST_RAN_PARAMETER_VAL_TYPE && "wrong RRM_Policy_Member_List type");
    assert(RRM_Policy_Member_List->ran_param_val.lst != NULL && "NULL RRM_Policy_Member_List->ran_param_val.lst");
    assert(RRM_Policy_Member_List->ran_param_val.lst->sz_lst_ran_param == 1 && "wrong RRM_Policy_Member_List->ran_param_val.lst->sz_lst_ran_param");
    assert(RRM_Policy_Member_List->ran_param_val.lst->lst_ran_param != NULL && "NULL RRM_Policy_Member_List->ran_param_val.lst->lst_ran_param");

    lst_ran_param_t* RRM_Policy_Member = &RRM_Policy_Member_List->ran_param_val.lst->lst_ran_param[0];
    //Bug in rc_enc_asn.c:1003, asn didn't define ran_param_id for lst_ran_param_t ...
    //assert(RRM_Policy_Member->ran_param_id == RRM_Policy_Member_8_4_3_6 && "wrong RRM_Policy_Member id");
    assert(RRM_Policy_Member->ran_param_struct.sz_ran_param_struct == 2 && "wrong RRM_Policy_Member->ran_param_struct.sz_ran_param_struct");
    assert(RRM_Policy_Member->ran_param_struct.ran_param_struct != NULL && "NULL RRM_Policy_Member->ran_param_struct.ran_param_struct");

    seq_ran_param_t* PLMN_Identity = &RRM_Policy_Member->ran_param_struct.ran_param_struct[0];
    assert(PLMN_Identity->ran_param_id == PLMN_Identity_8_4_3_6 && "wrong PLMN_Identity id");
    assert(PLMN_Identity->ran_param_val.type == ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE && "wrong PLMN_Identity type");
    assert(PLMN_Identity->ran_param_val.flag_false != NULL && "NULL PLMN_Identity->ran_param_val.flag_false");
    assert(PLMN_Identity->ran_param_val.flag_false->type == OCTET_STRING_RAN_PARAMETER_VALUE && "wrong PLMN_Identity->ran_param_val.flag_false->type");
    ///// GET RC PLMN ////
    char* plmn_str = cp_ba_to_str(PLMN_Identity->ran_param_val.flag_false->octet_str_ran);
    int RC_mnc, RC_mcc = 0;
    if (strlen(plmn_str) == 6)
      sscanf(plmn_str, "%3d%2d", &RC_mcc, &RC_mnc);
    else
      sscanf(plmn_str, "%3d%3d", &RC_mcc, &RC_mnc);
    LOG_D(NR_MAC, "RC PLMN %s, MCC %d, MNC %d\n", plmn_str, RC_mcc, RC_mnc);
    free(plmn_str);

    seq_ran_param_t* S_NSSAI = &RRM_Policy_Member->ran_param_struct.ran_param_struct[1];
    assert(S_NSSAI->ran_param_id == S_NSSAI_8_4_3_6 && "wrong S_NSSAI id");
    assert(S_NSSAI->ran_param_val.type == STRUCTURE_RAN_PARAMETER_VAL_TYPE && "wrong S_NSSAI type");
    assert(S_NSSAI->ran_param_val.strct != NULL && "NULL S_NSSAI->ran_param_val.strct");
    assert(S_NSSAI->ran_param_val.strct->sz_ran_param_struct == 2 && "wrong S_NSSAI->ran_param_val.strct->sz_ran_param_struct");
    assert(S_NSSAI->ran_param_val.strct->ran_param_struct != NULL && "NULL S_NSSAI->ran_param_val.strct->ran_param_struct");

    seq_ran_param_t* SST = &S_NSSAI->ran_param_val.strct->ran_param_struct[0];
    assert(SST->ran_param_id == SST_8_4_3_6 && "wrong SST id");
    assert(SST->ran_param_val.type == ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE && "wrong SST type");
    assert(SST->ran_param_val.flag_false != NULL && "NULL SST->ran_param_val.flag_false");
    assert(SST->ran_param_val.flag_false->type == OCTET_STRING_RAN_PARAMETER_VALUE && "wrong SST->ran_param_val.flag_false type");
    seq_ran_param_t* SD = &S_NSSAI->ran_param_val.strct->ran_param_struct[1];
    assert(SD->ran_param_id == SD_8_4_3_6 && "wrong SD id");
    assert(SD->ran_param_val.type == ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE && "wrong SD type");
    assert(SD->ran_param_val.flag_false != NULL && "NULL SD->ran_param_val.flag_false");
    assert(SD->ran_param_val.flag_false->type == OCTET_STRING_RAN_PARAMETER_VALUE && "wrong SD->ran_param_val.flag_false type");
    ///// GET RC NSSAI ////
    char* rc_sst_str = cp_ba_to_str(SST->ran_param_val.flag_false->octet_str_ran);
    char* rc_sd_str = cp_ba_to_str(SD->ran_param_val.flag_false->octet_str_ran);
    nssai_t RC_nssai = {.sst = atoi(rc_sst_str), .sd = atoi(rc_sd_str)};
    LOG_D(NR_MAC, "RC (oct) SST %s, SD %s -> (uint) SST %d, SD %d\n", rc_sst_str, rc_sd_str, RC_nssai.sst, RC_nssai.sd);
    ///// SLICE LABEL NAME /////
    char* sst_str = "SST";
    char* sd_str = "SD";
    size_t label_nssai_len = strlen(sst_str) + strlen(rc_sst_str) + strlen(sd_str) + strlen(rc_sd_str) + 1;
    char* label_nssai = (char*)malloc(label_nssai_len);
    assert(label_nssai != NULL && "Memory exhausted");
    sprintf(label_nssai, "%s%s%s%s", sst_str, rc_sst_str, sd_str, rc_sd_str);
    free(rc_sst_str);
    free(rc_sd_str);

    ///// SLICE NVS CAP /////
    seq_ran_param_t* Min_PRB_Policy_Ratio = &RRM_Policy_Ratio_Group->ran_param_struct.ran_param_struct[1];
    assert(Min_PRB_Policy_Ratio->ran_param_id == Min_PRB_Policy_Ratio_8_4_3_6 && "wrong Min_PRB_Policy_Ratio id");
    assert(Min_PRB_Policy_Ratio->ran_param_val.type == ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE && "wrong Min_PRB_Policy_Ratio type");
    assert(Min_PRB_Policy_Ratio->ran_param_val.flag_false != NULL && "NULL Min_PRB_Policy_Ratio->ran_param_val.flag_false");
    assert(Min_PRB_Policy_Ratio->ran_param_val.flag_false->type == INTEGER_RAN_PARAMETER_VALUE && "wrong Min_PRB_Policy_Ratio->ran_param_val.flag_false type");
    int64_t min_prb_ratio = Min_PRB_Policy_Ratio->ran_param_val.flag_false->int_ran;
    LOG_I(NR_MAC, "configure slice %ld, label %s, Min_PRB_Policy_Ratio %ld\n", i, label_nssai, min_prb_ratio);

    seq_ran_param_t* Dedicated_PRB_Policy_Ratio = &RRM_Policy_Ratio_Group->ran_param_struct.ran_param_struct[3];
    assert(Dedicated_PRB_Policy_Ratio->ran_param_id == Dedicated_PRB_Policy_Ratio_8_4_3_6 && "wrong Dedicated_PRB_Policy_Ratio id");
    assert(Dedicated_PRB_Policy_Ratio->ran_param_val.type == ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE && "wrong Dedicated_PRB_Policy_Ratio type");
    assert(Dedicated_PRB_Policy_Ratio->ran_param_val.flag_false != NULL && "NULL Dedicated_PRB_Policy_Ratio->ran_param_val.flag_false");
    assert(Dedicated_PRB_Policy_Ratio->ran_param_val.flag_false->type == INTEGER_RAN_PARAMETER_VALUE && "wrong Dedicated_PRB_Policy_Ratio->ran_param_val.flag_false type");
    int64_t dedicated_prb_ratio = Dedicated_PRB_Policy_Ratio->ran_param_val.flag_false->int_ran;
    LOG_I(NR_MAC, "configure slice %ld, label %s, Dedicated_PRB_Policy_Ratio %ld\n", i, label_nssai, dedicated_prb_ratio);
    // TODO: could be extended to support max prb ratio in the MAC scheduling algorithm
    //seq_ran_param_t* Max_PRB_Policy_Ratio = &RRM_Policy_Ratio_Group->ran_param_struct.ran_param_struct[2];
    //assert(Max_PRB_Policy_Ratio->ran_param_id == Max_PRB_Policy_Ratio_8_4_3_6 && "wrong Max_PRB_Policy_Ratio id");
    //assert(Max_PRB_Policy_Ratio->ran_param_val.type == ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE && "wrong Max_PRB_Policy_Ratio type");
    //assert(Max_PRB_Policy_Ratio->ran_param_val.flag_false != NULL && "NULL Max_PRB_Policy_Ratio->ran_param_val.flag_false");
    //assert(Max_PRB_Policy_Ratio->ran_param_val.flag_false->type == INTEGER_RAN_PARAMETER_VALUE && "wrong Max_PRB_Policy_Ratio->ran_param_val.flag_false type");
    //int64_t max_prb_ratio = Max_PRB_Policy_Ratio->ran_param_val.flag_false->int_ran;
    //LOG_I(NR_MAC, "configure slice %ld, label %s, Max_PRB_Policy_Ratio %ld\n", i, label_nssai, max_prb_ratio);

    ///// ADD SLICE /////
    /* Resource-based */
    void *params = malloc(sizeof(nvs_nr_slice_param_t));
    ((nvs_nr_slice_param_t *)params)->type = NVS_RES;
    ((nvs_nr_slice_param_t *)params)->pct_reserved = dedicated_prb_ratio;
    const int rc = add_mod_dl_slice(mod_id, current_algo, i+1, RC_nssai, label_nssai, params);
    free(label_nssai);
    if (rc < 0) {
      pthread_mutex_unlock(&nrmac->UE_info.mutex);
      LOG_E(NR_MAC, "error code %d while updating slices\n", rc);
      return false;
    }
    /* TODO: Bandwidth-based */

    /// ASSOC SLICE ///
    if (nrmac->pre_processor_dl.algorithm <= 0)
      LOG_E(NR_MAC, "current slice algo is NONE, no UE can be associated\n");

    if (nrmac->UE_info.list[0] == NULL)
      LOG_E(NR_MAC, "no UE connected\n");


    nr_pp_impl_param_dl_t *dl = &RC.nrmac[mod_id]->pre_processor_dl;
    NR_UEs_t *UE_info = &RC.nrmac[mod_id]->UE_info;
    UE_iterator(UE_info->list, UE) {
      LOG_I(NR_MAC, "UE %04x --- start \n", UE->rnti);
      rnti_t rnti = UE->rnti;
      NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
      long lcid = 0;
      for (int l = 0; l < sched_ctrl->dl_lc_num; ++l) {
        lcid = sched_ctrl->dl_lc_ids[l];
        LOG_I(NR_MAC, "l %d, lcid %ld, sst %d, sd %d\n", l, lcid, sched_ctrl->dl_lc_nssai[lcid].sst, sched_ctrl->dl_lc_nssai[lcid].sd);
        if (nssai_matches(sched_ctrl->dl_lc_nssai[lcid], RC_nssai.sst, &RC_nssai.sd)) {
          rrc_gNB_ue_context_t* rrc_ue_context_list = rrc_gNB_get_ue_context_by_rnti_any_du(RC.nrrrc[mod_id], rnti);
          if (!rrc_ue_context_list)
            continue;
          uint16_t UE_mcc = rrc_ue_context_list->ue_context.ue_guami.mcc;
          uint16_t UE_mnc = rrc_ue_context_list->ue_context.ue_guami.mnc;

          uint8_t UE_sst = sched_ctrl->dl_lc_nssai[lcid].sst;
          uint32_t UE_sd = sched_ctrl->dl_lc_nssai[lcid].sd;
          LOG_I(NR_MAC, "UE: mcc %d mnc %d, sst %d sd %d, RC: mcc %d mnc %d, sst %d sd %d\n",
                UE_mcc, UE_mnc, UE_sst, UE_sd, RC_mcc, RC_mnc, RC_nssai.sst, RC_nssai.sd);

          if (UE_mcc == RC_mcc && UE_mnc == RC_mnc && UE_sst == RC_nssai.sst && UE_sd == RC_nssai.sd) {
            dl->add_UE(dl->slices, UE);
          } else {
            LOG_W(NR_MAC, "cannot find specified PLMN (mcc %d mnc %d) NSSAI (sst %d sd %d) from the existing UE PLMN (mcc %d mnc %d) NSSAI (sst %d sd %d) \n",
                  RC_mcc, RC_mnc, RC_nssai.sst, RC_nssai.sd, UE_mcc, UE_mnc, UE_sst, UE_sd);
          }
        }
      }
      LOG_I(NR_MAC, "UE %04x --- stop \n", UE->rnti);
    }

  }

  pthread_mutex_unlock(&nrmac->UE_info.mutex);
  LOG_D(NR_MAC, "All slices add/mod successfully!\n");
  return true;
}
