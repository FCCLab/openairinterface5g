#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_MAC_gNB/gNB_scheduler_types.h"
#include "../../flexric/src/sm/rc_sm/ie/ir/lst_ran_param.h"
#include "../../flexric/src/sm/rc_sm/ie/ir/ran_param_list.h"
#include "ran_func_rc_helper.h"
#include <stdio.h>
#include <inttypes.h>

uint32_t parse_sd_big_endian(byte_array_t ba) {
    // Ensure buffer exists and has no more than 3 bytes as SD is 24 bits.
    if (ba.buf == NULL || ba.len != 3) {
        return 0; 
    }
    uint32_t ans = 0;
    for (size_t i = 0; i < ba.len; i++) {
        int shift = (3 - i - 1) * 8;
        ans |= ((uint32_t)ba.buf[i]) << shift;
    }
    return ans;
}


slice_config_t extract_slice_config_from_ctrl_req(e2sm_rc_ctrl_msg_frmt_1_t const* frmt_1)
{
  int64_t dedicated_prb_policy_ratio_val  = 0;
  int64_t min_prb_policy_ratio_val  = 0;
  int64_t max_prb_policy_ratio_val  = 0;
  uint32_t sst_val = 0;
  uint32_t sd_val = 0;

  printf("[RC] CONTROL STYLE 2 (ric_style_type == 2): Slice-level PRB Quota (ctrl_act_id==6)\n");
  fflush(stdout);
  assert(frmt_1->sz_ran_param == 1);
  seq_ran_param_t* rrm_policy_ratio_list_seq_rp = frmt_1->ran_param;
  assert(rrm_policy_ratio_list_seq_rp->ran_param_id == 1);
  printf("[RC] ran_param_id == 1 (RRM Policy Ratio List)\n");
  fflush(stdout);
  assert(rrm_policy_ratio_list_seq_rp->ran_param_val.type == LIST_RAN_PARAMETER_VAL_TYPE);

  lst_ran_param_t const* rrm_policy_ratio_list_lrp = rrm_policy_ratio_list_seq_rp->ran_param_val.lst->lst_ran_param;
  size_t rrm_policy_ratio_list_lrp_size = rrm_policy_ratio_list_seq_rp->ran_param_val.lst->sz_lst_ran_param;
  for(size_t j = 0; j < rrm_policy_ratio_list_lrp_size; ++j){ 
    
    seq_ran_param_t* rrm_policy_ratio_group_seq_rp = rrm_policy_ratio_list_lrp[j].ran_param_struct.ran_param_struct;
    
    size_t rrm_policy_ratio_group_size = rrm_policy_ratio_group_seq_rp->ran_param_val.strct->sz_ran_param_struct;
    assert(rrm_policy_ratio_group_seq_rp->ran_param_val.type == STRUCTURE_RAN_PARAMETER_VAL_TYPE);
    assert(rrm_policy_ratio_group_size == 4);
    assert(rrm_policy_ratio_group_seq_rp->ran_param_id == 2);
    printf("[RC] ran_param_id == 2 (RRM Policy Ratio Group)\n");
    fflush(stdout);

    for(size_t k = 0 ; k < rrm_policy_ratio_group_size ; ++k){
      seq_ran_param_t rrm_policy_ratio_group_nested_seq_rp = rrm_policy_ratio_group_seq_rp->ran_param_val.strct->ran_param_struct[k];
      if(rrm_policy_ratio_group_nested_seq_rp.ran_param_id == 3){

        printf("[RC] ran_param_id == 3 (RRM Policy)\n");
        fflush(stdout);

        seq_ran_param_t* rrm_policy_seq_rp = &rrm_policy_ratio_group_nested_seq_rp;
        assert(rrm_policy_seq_rp->ran_param_val.type == STRUCTURE_RAN_PARAMETER_VAL_TYPE);
        assert(rrm_policy_seq_rp->ran_param_val.strct->sz_ran_param_struct == 1);

        seq_ran_param_t* rrm_policy_member_list_seq_rp = rrm_policy_seq_rp->ran_param_val.strct->ran_param_struct;
        assert(rrm_policy_member_list_seq_rp->ran_param_val.type == LIST_RAN_PARAMETER_VAL_TYPE);
        assert(rrm_policy_member_list_seq_rp->ran_param_val.strct->sz_ran_param_struct == 1);
        assert(rrm_policy_member_list_seq_rp->ran_param_id == 5);
        printf("[RC] ran_param_id == 5 (RRM Policy Member List)\n");
        fflush(stdout);

        lst_ran_param_t* rrm_policy_member_list_lrp = rrm_policy_member_list_seq_rp->ran_param_val.lst->lst_ran_param;
        size_t rrm_policy_member_list_size = rrm_policy_member_list_seq_rp->ran_param_val.lst->sz_lst_ran_param;
        for(size_t l = 0 ; l < rrm_policy_member_list_size ; ++l){

          seq_ran_param_t* rrm_policy_member_seq_rp = rrm_policy_member_list_lrp[l].ran_param_struct.ran_param_struct;
          size_t rrm_policy_member_size = rrm_policy_member_seq_rp->ran_param_val.strct->sz_ran_param_struct;
          assert(rrm_policy_member_seq_rp->ran_param_val.type == STRUCTURE_RAN_PARAMETER_VAL_TYPE);
          assert(rrm_policy_member_seq_rp->ran_param_id == 6);
          assert(rrm_policy_member_size == 2);
          printf("[RC] ran_param_id == 6 (RRM Policy Member)\n");
          fflush(stdout);

          printf("[RC] rrm_policy_member_size: %zu\n", rrm_policy_member_size);
          fflush(stdout);
          for(size_t m = 0 ; m < rrm_policy_member_size ; ++m){
            seq_ran_param_t rrm_policy_member_nested_seq_rp = rrm_policy_member_seq_rp->ran_param_val.strct->ran_param_struct[m];
            printf("[RC] rrm_policy_ratio_group_nested_seq_rp.ran_param_id %u\n", rrm_policy_member_nested_seq_rp.ran_param_id );
            fflush(stdout);
            if(rrm_policy_member_nested_seq_rp.ran_param_id == 7){
              seq_ran_param_t* plmn_identity_seq_rp = &rrm_policy_member_nested_seq_rp;
              assert(plmn_identity_seq_rp->ran_param_val.type == ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE);
              printf("[RC] ran_param_id == 7 (PLMN Identity)\n");
              fflush(stdout);
            } else if(rrm_policy_member_nested_seq_rp.ran_param_id == 8){
              seq_ran_param_t* snssai_seq_rp = &rrm_policy_member_nested_seq_rp;
              assert(snssai_seq_rp->ran_param_val.type == STRUCTURE_RAN_PARAMETER_VAL_TYPE);
              printf("[RC] ran_param_id == 8 (S-NSSAI)\n");

              size_t snssai_seq_rp_size = snssai_seq_rp->ran_param_val.strct->sz_ran_param_struct;
              printf("[RC] snssai_seq_rp_size size: %zu\n", snssai_seq_rp_size);
              fflush(stdout);
              for(size_t n = 0 ; n < snssai_seq_rp_size ; ++n){
                seq_ran_param_t snssai_nested_seq_rp = snssai_seq_rp->ran_param_val.strct->ran_param_struct[n];
                if(snssai_nested_seq_rp.ran_param_id == 9){
                  seq_ran_param_t* sst_seq_rp = &snssai_nested_seq_rp;
                  assert(sst_seq_rp->ran_param_val.type == ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE);
                  printf("[RC] ran_param_id == 9 (SST)\n");

                  byte_array_t sst_ba = sst_seq_rp->ran_param_val.flag_false->octet_str_ran;
                  uint8_t* sst_buf = sst_ba.buf;
                  sst_val = (uint32_t) sst_buf[0];
                  printf("[RC] sst_val: %u\n", sst_val);
                  fflush(stdout);
                  // TODO: use val
                } else if(snssai_nested_seq_rp.ran_param_id == 10){
                  seq_ran_param_t* sd_seq_rp = &snssai_nested_seq_rp;
                  assert(sd_seq_rp->ran_param_val.type == ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE);
                  printf("[RC] ran_param_id == 10 (SD)\n");

                  byte_array_t sd_ba = sd_seq_rp->ran_param_val.flag_false->octet_str_ran;
                  uint8_t* sd_buf = sd_ba.buf;
                  for(size_t _sd_idx = 0 ; _sd_idx < sd_ba.len ; _sd_idx ++){
                    printf("[RC] sd_buf[%zu]: %" PRIu8 "\n", _sd_idx, sd_buf[_sd_idx]);
                  }
                  sd_val = parse_sd_big_endian(sd_ba);

                  printf("[RC] sd_val: %u\n", sd_val);
                  fflush(stdout);
                  // TODO: use val
                }
              }
            }
          }
        }
      } else if(rrm_policy_ratio_group_nested_seq_rp.ran_param_id == 11){
        printf("[RC] ran_param_id == 11 (Min PRB Policy Ratio)\n");
        seq_ran_param_t* min_prb_policy_ratio_seq_rp = &rrm_policy_ratio_group_nested_seq_rp;
        assert(min_prb_policy_ratio_seq_rp->ran_param_val.type == ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE);
        
        min_prb_policy_ratio_val = min_prb_policy_ratio_seq_rp->ran_param_val.flag_false->int_ran;
        
        printf("[RC] min_prb_policy_ratio_val: %ld\n", min_prb_policy_ratio_val);
        fflush(stdout);
        // TODO: extract and use val
      } else if(rrm_policy_ratio_group_nested_seq_rp.ran_param_id == 12){
        printf("[RC] ran_param_id == 12 (Max PRB Policy Ratio)\n");
        seq_ran_param_t* max_prb_policy_ratio_seq_rp = &rrm_policy_ratio_group_nested_seq_rp;
        assert(max_prb_policy_ratio_seq_rp->ran_param_val.type == ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE);

        max_prb_policy_ratio_val = max_prb_policy_ratio_seq_rp->ran_param_val.flag_false->int_ran;
        printf("[RC] max_prb_policy_ratio_val: %ld\n", max_prb_policy_ratio_val);
        fflush(stdout);
        // TODO: extract and use val
      } else if(rrm_policy_ratio_group_nested_seq_rp.ran_param_id == 13){
        printf("[RC] ran_param_id == 13 (Dedicated PRB Policy Ratio)\n");
        seq_ran_param_t* dedicated_prb_policy_ratio_seq_rp = &rrm_policy_ratio_group_nested_seq_rp;
        assert(dedicated_prb_policy_ratio_seq_rp->ran_param_val.type == ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE);

        dedicated_prb_policy_ratio_val = dedicated_prb_policy_ratio_seq_rp->ran_param_val.flag_false->int_ran;
        printf("[RC] dedicated_prb_policy_ratio_val: %ld\n", dedicated_prb_policy_ratio_val);
        fflush(stdout);
        // TODO: extract and use val
      } 
      else {
        assert(0 != 0 && "Unsupported ran_param_id for ric_tyle_type == 2, ctrl_act_id == 6");
      }
    }
  }

  const float ded = (float)dedicated_prb_policy_ratio_val / 100.f;
  const float min = (float)min_prb_policy_ratio_val / 100.f;
  const float max = (float)max_prb_policy_ratio_val / 100.f;
  printf("[RC] (Dedicated PRB Policy Ratio): %f\n", (float)dedicated_prb_policy_ratio_val);
  printf("[RC] (Min PRB Policy Ratio): %f\n", (float)min_prb_policy_ratio_val);
  printf("[RC] (Max PRB Policy Ratio): %f\n", (float)max_prb_policy_ratio_val);
  printf("[RC] calculated ded: %f\n", ded);
  printf("[RC] calculated min: %f\n", min);
  printf("[RC] calculated max: %f\n", max);
  fflush(stdout);
  slice_nssai_t slice_id = {.sst=sst_val, .sd=sd_val};
  slice_config_t slice_config = {slice_id,ded,min,max,0};
  return slice_config;
}