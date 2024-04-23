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
/*! \file nfapi/open-nFAPI/fapi/src/nr_fapi_p5_utils.c
 * \brief
 * \author Ruben S. Silva
 * \date 2024
 * \version 0.1
 * \company OpenAirInterface Software Alliance
 * \email: contact@openairinterface.org, rsilva@allbesmart.pt
 * \note
 * \warning
 */
#include "nr_fapi_p5_utils.h"

bool compare_param_request(const nfapi_nr_param_request_scf_t *unpacked_req, const nfapi_nr_param_request_scf_t *req)
{
  CMP(unpacked_req->header.message_id, req->header.message_id);
  CMP(unpacked_req->header.message_length, req->header.message_length);
  return 0;
}

bool compare_param_response(const nfapi_nr_param_response_scf_t *unpacked_req, const nfapi_nr_param_response_scf_t *req)
{
  CMP(unpacked_req->header.message_id, req->header.message_id);
  CMP(unpacked_req->header.message_length, req->header.message_length);
  CMP(unpacked_req->num_tlv, req->num_tlv);
  CMP(unpacked_req->error_code, req->error_code);

  CMP(unpacked_req->cell_param.release_capability.tl.tag, req->cell_param.release_capability.tl.tag);
  CMP(unpacked_req->cell_param.release_capability.value, req->cell_param.release_capability.value);

  CMP(unpacked_req->cell_param.phy_state.tl.tag, req->cell_param.phy_state.tl.tag);
  CMP(unpacked_req->cell_param.phy_state.value, req->cell_param.phy_state.value);

  CMP(unpacked_req->cell_param.skip_blank_dl_config.tl.tag, req->cell_param.skip_blank_dl_config.tl.tag);
  CMP(unpacked_req->cell_param.skip_blank_dl_config.value, req->cell_param.skip_blank_dl_config.value);

  CMP(unpacked_req->cell_param.skip_blank_ul_config.tl.tag, req->cell_param.skip_blank_ul_config.tl.tag);
  CMP(unpacked_req->cell_param.skip_blank_ul_config.value, req->cell_param.skip_blank_ul_config.value);

  CMP(unpacked_req->cell_param.num_config_tlvs_to_report.tl.tag, req->cell_param.num_config_tlvs_to_report.tl.tag);
  CMP(unpacked_req->cell_param.num_config_tlvs_to_report.value, req->cell_param.num_config_tlvs_to_report.value);

  CMP(unpacked_req->carrier_param.cyclic_prefix.tl.tag, req->carrier_param.cyclic_prefix.tl.tag);
  CMP(unpacked_req->carrier_param.cyclic_prefix.value, req->carrier_param.cyclic_prefix.value);

  CMP(unpacked_req->carrier_param.supported_subcarrier_spacings_dl.tl.tag,
      req->carrier_param.supported_subcarrier_spacings_dl.tl.tag);
  CMP(unpacked_req->carrier_param.supported_subcarrier_spacings_dl.value,
      req->carrier_param.supported_subcarrier_spacings_dl.value);

  CMP(unpacked_req->carrier_param.supported_bandwidth_dl.tl.tag, req->carrier_param.supported_bandwidth_dl.tl.tag);
  CMP(unpacked_req->carrier_param.supported_bandwidth_dl.value, req->carrier_param.supported_bandwidth_dl.value);

  CMP(unpacked_req->carrier_param.supported_subcarrier_spacings_ul.tl.tag,
      req->carrier_param.supported_subcarrier_spacings_ul.tl.tag);
  CMP(unpacked_req->carrier_param.supported_subcarrier_spacings_ul.value,
      req->carrier_param.supported_subcarrier_spacings_ul.value);

  CMP(unpacked_req->carrier_param.supported_bandwidth_ul.tl.tag, req->carrier_param.supported_bandwidth_ul.tl.tag);
  CMP(unpacked_req->carrier_param.supported_bandwidth_ul.value, req->carrier_param.supported_bandwidth_ul.value);

  CMP(unpacked_req->pdcch_param.cce_mapping_type.tl.tag, req->pdcch_param.cce_mapping_type.tl.tag);
  CMP(unpacked_req->pdcch_param.cce_mapping_type.value, req->pdcch_param.cce_mapping_type.value);

  CMP(unpacked_req->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.tl.tag,
      req->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.tl.tag);
  CMP(unpacked_req->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.value,
      req->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.value);

  CMP(unpacked_req->pdcch_param.coreset_precoder_granularity_coreset.tl.tag,
      req->pdcch_param.coreset_precoder_granularity_coreset.tl.tag);
  CMP(unpacked_req->pdcch_param.coreset_precoder_granularity_coreset.value,
      req->pdcch_param.coreset_precoder_granularity_coreset.value);

  CMP(unpacked_req->pdcch_param.pdcch_mu_mimo.tl.tag, req->pdcch_param.pdcch_mu_mimo.tl.tag);
  CMP(unpacked_req->pdcch_param.pdcch_mu_mimo.value, req->pdcch_param.pdcch_mu_mimo.value);

  CMP(unpacked_req->pdcch_param.pdcch_precoder_cycling.tl.tag, req->pdcch_param.pdcch_precoder_cycling.tl.tag);
  CMP(unpacked_req->pdcch_param.pdcch_precoder_cycling.value, req->pdcch_param.pdcch_precoder_cycling.value);

  CMP(unpacked_req->pdcch_param.max_pdcch_per_slot.tl.tag, req->pdcch_param.max_pdcch_per_slot.tl.tag);
  CMP(unpacked_req->pdcch_param.max_pdcch_per_slot.value, req->pdcch_param.max_pdcch_per_slot.value);

  CMP(unpacked_req->pucch_param.pucch_formats.tl.tag, req->pucch_param.pucch_formats.tl.tag);
  CMP(unpacked_req->pucch_param.pucch_formats.value, req->pucch_param.pucch_formats.value);

  CMP(unpacked_req->pucch_param.max_pucchs_per_slot.tl.tag, req->pucch_param.max_pucchs_per_slot.tl.tag);
  CMP(unpacked_req->pucch_param.max_pucchs_per_slot.value, req->pucch_param.max_pucchs_per_slot.value);

  CMP(unpacked_req->pdsch_param.pdsch_mapping_type.tl.tag, req->pdsch_param.pdsch_mapping_type.tl.tag);
  CMP(unpacked_req->pdsch_param.pdsch_mapping_type.value, req->pdsch_param.pdsch_mapping_type.value);

  CMP(unpacked_req->pdsch_param.pdsch_dmrs_additional_pos.tl.tag, req->pdsch_param.pdsch_dmrs_additional_pos.tl.tag);
  CMP(unpacked_req->pdsch_param.pdsch_dmrs_additional_pos.value, req->pdsch_param.pdsch_dmrs_additional_pos.value);

  CMP(unpacked_req->pdsch_param.pdsch_allocation_types.tl.tag, req->pdsch_param.pdsch_allocation_types.tl.tag);
  CMP(unpacked_req->pdsch_param.pdsch_allocation_types.value, req->pdsch_param.pdsch_allocation_types.value);

  CMP(unpacked_req->pdsch_param.pdsch_vrb_to_prb_mapping.tl.tag, req->pdsch_param.pdsch_vrb_to_prb_mapping.tl.tag);
  CMP(unpacked_req->pdsch_param.pdsch_vrb_to_prb_mapping.value, req->pdsch_param.pdsch_vrb_to_prb_mapping.value);

  CMP(unpacked_req->pdsch_param.pdsch_cbg.tl.tag, req->pdsch_param.pdsch_cbg.tl.tag);
  CMP(unpacked_req->pdsch_param.pdsch_cbg.value, req->pdsch_param.pdsch_cbg.value);

  CMP(unpacked_req->pdsch_param.pdsch_dmrs_config_types.tl.tag, req->pdsch_param.pdsch_dmrs_config_types.tl.tag);
  CMP(unpacked_req->pdsch_param.pdsch_dmrs_config_types.value, req->pdsch_param.pdsch_dmrs_config_types.value);

  CMP(unpacked_req->pdsch_param.max_number_mimo_layers_pdsch.tl.tag, req->pdsch_param.max_number_mimo_layers_pdsch.tl.tag);
  CMP(unpacked_req->pdsch_param.max_number_mimo_layers_pdsch.value, req->pdsch_param.max_number_mimo_layers_pdsch.value);

  CMP(unpacked_req->pdsch_param.max_mu_mimo_users_dl.tl.tag, req->pdsch_param.max_mu_mimo_users_dl.tl.tag);
  CMP(unpacked_req->pdsch_param.max_mu_mimo_users_dl.value, req->pdsch_param.max_mu_mimo_users_dl.value);

  CMP(unpacked_req->pdsch_param.pdsch_data_in_dmrs_symbols.tl.tag, req->pdsch_param.pdsch_data_in_dmrs_symbols.tl.tag);
  CMP(unpacked_req->pdsch_param.pdsch_data_in_dmrs_symbols.value, req->pdsch_param.pdsch_data_in_dmrs_symbols.value);

  CMP(unpacked_req->pdsch_param.premption_support.tl.tag, req->pdsch_param.premption_support.tl.tag);
  CMP(unpacked_req->pdsch_param.premption_support.value, req->pdsch_param.premption_support.value);

  CMP(unpacked_req->pdsch_param.pdsch_non_slot_support.tl.tag, req->pdsch_param.pdsch_non_slot_support.tl.tag);
  CMP(unpacked_req->pdsch_param.pdsch_non_slot_support.value, req->pdsch_param.pdsch_non_slot_support.value);

  CMP(unpacked_req->pusch_param.uci_mux_ulsch_in_pusch.tl.tag, req->pusch_param.uci_mux_ulsch_in_pusch.tl.tag);
  CMP(unpacked_req->pusch_param.uci_mux_ulsch_in_pusch.value, req->pusch_param.uci_mux_ulsch_in_pusch.value);

  CMP(unpacked_req->pusch_param.uci_only_pusch.tl.tag, req->pusch_param.uci_only_pusch.tl.tag);
  CMP(unpacked_req->pusch_param.uci_only_pusch.value, req->pusch_param.uci_only_pusch.value);

  CMP(unpacked_req->pusch_param.pusch_frequency_hopping.tl.tag, req->pusch_param.pusch_frequency_hopping.tl.tag);
  CMP(unpacked_req->pusch_param.pusch_frequency_hopping.value, req->pusch_param.pusch_frequency_hopping.value);

  CMP(unpacked_req->pusch_param.pusch_dmrs_config_types.tl.tag, req->pusch_param.pusch_dmrs_config_types.tl.tag);
  CMP(unpacked_req->pusch_param.pusch_dmrs_config_types.value, req->pusch_param.pusch_dmrs_config_types.value);

  CMP(unpacked_req->pusch_param.pusch_dmrs_max_len.tl.tag, req->pusch_param.pusch_dmrs_max_len.tl.tag);
  CMP(unpacked_req->pusch_param.pusch_dmrs_max_len.value, req->pusch_param.pusch_dmrs_max_len.value);

  CMP(unpacked_req->pusch_param.pusch_dmrs_additional_pos.tl.tag, req->pusch_param.pusch_dmrs_additional_pos.tl.tag);
  CMP(unpacked_req->pusch_param.pusch_dmrs_additional_pos.value, req->pusch_param.pusch_dmrs_additional_pos.value);

  CMP(unpacked_req->pusch_param.pusch_cbg.tl.tag, req->pusch_param.pusch_cbg.tl.tag);
  CMP(unpacked_req->pusch_param.pusch_cbg.value, req->pusch_param.pusch_cbg.value);

  CMP(unpacked_req->pusch_param.pusch_mapping_type.tl.tag, req->pusch_param.pusch_mapping_type.tl.tag);
  CMP(unpacked_req->pusch_param.pusch_mapping_type.value, req->pusch_param.pusch_mapping_type.value);

  CMP(unpacked_req->pusch_param.pusch_allocation_types.tl.tag, req->pusch_param.pusch_allocation_types.tl.tag);
  CMP(unpacked_req->pusch_param.pusch_allocation_types.value, req->pusch_param.pusch_allocation_types.value);

  CMP(unpacked_req->pusch_param.pusch_vrb_to_prb_mapping.tl.tag, req->pusch_param.pusch_vrb_to_prb_mapping.tl.tag);
  CMP(unpacked_req->pusch_param.pusch_vrb_to_prb_mapping.value, req->pusch_param.pusch_vrb_to_prb_mapping.value);

  CMP(unpacked_req->pusch_param.pusch_max_ptrs_ports.tl.tag, req->pusch_param.pusch_max_ptrs_ports.tl.tag);
  CMP(unpacked_req->pusch_param.pusch_max_ptrs_ports.value, req->pusch_param.pusch_max_ptrs_ports.value);

  CMP(unpacked_req->pusch_param.max_pduschs_tbs_per_slot.tl.tag, req->pusch_param.max_pduschs_tbs_per_slot.tl.tag);
  CMP(unpacked_req->pusch_param.max_pduschs_tbs_per_slot.value, req->pusch_param.max_pduschs_tbs_per_slot.value);

  CMP(unpacked_req->pusch_param.max_number_mimo_layers_non_cb_pusch.tl.tag,
      req->pusch_param.max_number_mimo_layers_non_cb_pusch.tl.tag);
  CMP(unpacked_req->pusch_param.max_number_mimo_layers_non_cb_pusch.value,
      req->pusch_param.max_number_mimo_layers_non_cb_pusch.value);

  CMP(unpacked_req->pusch_param.supported_modulation_order_ul.tl.tag, req->pusch_param.supported_modulation_order_ul.tl.tag);
  CMP(unpacked_req->pusch_param.supported_modulation_order_ul.value, req->pusch_param.supported_modulation_order_ul.value);

  CMP(unpacked_req->pusch_param.max_mu_mimo_users_ul.tl.tag, req->pusch_param.max_mu_mimo_users_ul.tl.tag);
  CMP(unpacked_req->pusch_param.max_mu_mimo_users_ul.value, req->pusch_param.max_mu_mimo_users_ul.value);

  CMP(unpacked_req->pusch_param.dfts_ofdm_support.tl.tag, req->pusch_param.dfts_ofdm_support.tl.tag);
  CMP(unpacked_req->pusch_param.dfts_ofdm_support.value, req->pusch_param.dfts_ofdm_support.value);

  CMP(unpacked_req->pusch_param.pusch_aggregation_factor.tl.tag, req->pusch_param.pusch_aggregation_factor.tl.tag);
  CMP(unpacked_req->pusch_param.pusch_aggregation_factor.value, req->pusch_param.pusch_aggregation_factor.value);

  CMP(unpacked_req->prach_param.prach_long_formats.tl.tag, req->prach_param.prach_long_formats.tl.tag);
  CMP(unpacked_req->prach_param.prach_long_formats.value, req->prach_param.prach_long_formats.value);

  CMP(unpacked_req->prach_param.prach_short_formats.tl.tag, req->prach_param.prach_short_formats.tl.tag);
  CMP(unpacked_req->prach_param.prach_short_formats.value, req->prach_param.prach_short_formats.value);

  CMP(unpacked_req->prach_param.prach_restricted_sets.tl.tag, req->prach_param.prach_restricted_sets.tl.tag);
  CMP(unpacked_req->prach_param.prach_restricted_sets.value, req->prach_param.prach_restricted_sets.value);

  CMP(unpacked_req->prach_param.max_prach_fd_occasions_in_a_slot.tl.tag, req->prach_param.max_prach_fd_occasions_in_a_slot.tl.tag);
  CMP(unpacked_req->prach_param.max_prach_fd_occasions_in_a_slot.value, req->prach_param.max_prach_fd_occasions_in_a_slot.value);

  CMP(unpacked_req->measurement_param.rssi_measurement_support.tl.tag, req->measurement_param.rssi_measurement_support.tl.tag);
  CMP(unpacked_req->measurement_param.rssi_measurement_support.value, req->measurement_param.rssi_measurement_support.value);

  return 0;
}
void free_param_request(nfapi_nr_param_request_scf_t *msg)
{
  if (msg->vendor_extension) {
    free(msg->vendor_extension);
  }
}

void free_param_response(nfapi_nr_param_response_scf_t *msg)
{
  if (msg->vendor_extension) {
    free(msg->vendor_extension);
  }

  if (msg->cell_param.config_tlvs_to_report_list) {
    free(msg->cell_param.config_tlvs_to_report_list);
  }
}
void copy_tl(const nfapi_tl_t *src, nfapi_tl_t *dst)
{
  dst->tag = src->tag;
  dst->length = src->length;
}

void copy_param_request(const nfapi_nr_param_request_scf_t *src, nfapi_nr_param_request_scf_t *dst)
{
  dst->header.message_id = src->header.message_id;
  dst->header.message_length = src->header.message_length;
  if (src->vendor_extension) {
    dst->vendor_extension = calloc(1, sizeof(nfapi_vendor_extension_tlv_t));
    dst->vendor_extension->tag = src->vendor_extension->tag;
    dst->vendor_extension->length = src->vendor_extension->length;
    // TODO: FIGURE OUT WHERE THE VENDOR EXTENSION VALUE IS
  }
}

void copy_param_response(const nfapi_nr_param_response_scf_t *src, nfapi_nr_param_response_scf_t *dst)
{
  dst->header.message_id = src->header.message_id;
  dst->header.message_length = src->header.message_length;
  if (src->vendor_extension) {
    dst->vendor_extension = calloc(1, sizeof(nfapi_vendor_extension_tlv_t));
    dst->vendor_extension->tag = src->vendor_extension->tag;
    dst->vendor_extension->length = src->vendor_extension->length;
    // TODO: FIGURE OUT WHERE THE VENDOR EXTENSION VALUE IS
  }

  dst->error_code = src->error_code;
  dst->num_tlv = src->num_tlv;

  copy_tl(&src->cell_param.release_capability.tl, &dst->cell_param.release_capability.tl);
  dst->cell_param.release_capability.value = src->cell_param.release_capability.value;

  copy_tl(&src->cell_param.phy_state.tl, &dst->cell_param.phy_state.tl);
  dst->cell_param.phy_state.value = src->cell_param.phy_state.value;

  copy_tl(&src->cell_param.skip_blank_dl_config.tl, &dst->cell_param.skip_blank_dl_config.tl);
  dst->cell_param.skip_blank_dl_config.value = src->cell_param.skip_blank_dl_config.value;

  copy_tl(&src->cell_param.skip_blank_ul_config.tl, &dst->cell_param.skip_blank_ul_config.tl);
  dst->cell_param.skip_blank_ul_config.value = src->cell_param.skip_blank_ul_config.value;

  copy_tl(&src->cell_param.num_config_tlvs_to_report.tl, &dst->cell_param.num_config_tlvs_to_report.tl);
  dst->cell_param.num_config_tlvs_to_report.value = src->cell_param.num_config_tlvs_to_report.value;

  if (src->cell_param.config_tlvs_to_report_list) {
    dst->cell_param.config_tlvs_to_report_list =
        calloc(src->cell_param.num_config_tlvs_to_report.value, sizeof(nfapi_uint8_tlv_t *));
    for (int i = 0; i < src->cell_param.num_config_tlvs_to_report.value; ++i) {
      dst->cell_param.config_tlvs_to_report_list[i].tl.tag = src->cell_param.config_tlvs_to_report_list[i].tl.tag;
      dst->cell_param.config_tlvs_to_report_list[i].tl.length = src->cell_param.config_tlvs_to_report_list[i].tl.length;
      dst->cell_param.config_tlvs_to_report_list[i].value = src->cell_param.config_tlvs_to_report_list[i].value;
    }
  }

  copy_tl(&src->carrier_param.cyclic_prefix.tl, &dst->carrier_param.cyclic_prefix.tl);
  dst->carrier_param.cyclic_prefix.value = src->carrier_param.cyclic_prefix.value;

  copy_tl(&src->carrier_param.supported_subcarrier_spacings_dl.tl, &dst->carrier_param.supported_subcarrier_spacings_dl.tl);
  dst->carrier_param.supported_subcarrier_spacings_dl.value = src->carrier_param.supported_subcarrier_spacings_dl.value;

  copy_tl(&src->carrier_param.supported_bandwidth_dl.tl, &dst->carrier_param.supported_bandwidth_dl.tl);
  dst->carrier_param.supported_bandwidth_dl.value = src->carrier_param.supported_bandwidth_dl.value;

  copy_tl(&src->carrier_param.supported_subcarrier_spacings_ul.tl, &dst->carrier_param.supported_subcarrier_spacings_ul.tl);
  dst->carrier_param.supported_subcarrier_spacings_ul.value = src->carrier_param.supported_subcarrier_spacings_ul.value;

  copy_tl(&src->carrier_param.supported_bandwidth_ul.tl, &dst->carrier_param.supported_bandwidth_ul.tl);
  dst->carrier_param.supported_bandwidth_ul.value = src->carrier_param.supported_bandwidth_ul.value;

  copy_tl(&src->pdcch_param.cce_mapping_type.tl, &dst->pdcch_param.cce_mapping_type.tl);
  dst->pdcch_param.cce_mapping_type.value = src->pdcch_param.cce_mapping_type.value;

  copy_tl(&src->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.tl,
          &dst->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.tl);
  dst->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.value =
      src->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.value;

  copy_tl(&src->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.tl,
          &dst->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.tl);
  dst->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.value =
      src->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.value;

  copy_tl(&src->pdcch_param.coreset_precoder_granularity_coreset.tl, &dst->pdcch_param.coreset_precoder_granularity_coreset.tl);
  dst->pdcch_param.coreset_precoder_granularity_coreset.value = src->pdcch_param.coreset_precoder_granularity_coreset.value;

  copy_tl(&src->pdcch_param.pdcch_mu_mimo.tl, &dst->pdcch_param.pdcch_mu_mimo.tl);
  dst->pdcch_param.pdcch_mu_mimo.value = src->pdcch_param.pdcch_mu_mimo.value;

  copy_tl(&src->pdcch_param.pdcch_precoder_cycling.tl, &dst->pdcch_param.pdcch_precoder_cycling.tl);
  dst->pdcch_param.pdcch_precoder_cycling.value = src->pdcch_param.pdcch_precoder_cycling.value;

  copy_tl(&src->pdcch_param.max_pdcch_per_slot.tl, &dst->pdcch_param.max_pdcch_per_slot.tl);
  dst->pdcch_param.max_pdcch_per_slot.value = src->pdcch_param.max_pdcch_per_slot.value;

  copy_tl(&src->pucch_param.pucch_formats.tl, &dst->pucch_param.pucch_formats.tl);
  dst->pucch_param.pucch_formats.value = src->pucch_param.pucch_formats.value;

  copy_tl(&src->pucch_param.max_pucchs_per_slot.tl, &dst->pucch_param.max_pucchs_per_slot.tl);
  dst->pucch_param.max_pucchs_per_slot.value = src->pucch_param.max_pucchs_per_slot.value;

  copy_tl(&src->pdsch_param.pdsch_mapping_type.tl, &dst->pdsch_param.pdsch_mapping_type.tl);
  dst->pdsch_param.pdsch_mapping_type.value = src->pdsch_param.pdsch_mapping_type.value;

  copy_tl(&src->pdsch_param.pdsch_dmrs_additional_pos.tl, &dst->pdsch_param.pdsch_dmrs_additional_pos.tl);
  dst->pdsch_param.pdsch_dmrs_additional_pos.value = src->pdsch_param.pdsch_dmrs_additional_pos.value;

  copy_tl(&src->pdsch_param.pdsch_allocation_types.tl, &dst->pdsch_param.pdsch_allocation_types.tl);
  dst->pdsch_param.pdsch_allocation_types.value = src->pdsch_param.pdsch_allocation_types.value;

  copy_tl(&src->pdsch_param.pdsch_vrb_to_prb_mapping.tl, &dst->pdsch_param.pdsch_vrb_to_prb_mapping.tl);
  dst->pdsch_param.pdsch_vrb_to_prb_mapping.value = src->pdsch_param.pdsch_vrb_to_prb_mapping.value;

  copy_tl(&src->pdsch_param.pdsch_cbg.tl, &dst->pdsch_param.pdsch_cbg.tl);
  dst->pdsch_param.pdsch_cbg.value = src->pdsch_param.pdsch_cbg.value;

  copy_tl(&src->pdsch_param.pdsch_dmrs_config_types.tl, &dst->pdsch_param.pdsch_dmrs_config_types.tl);
  dst->pdsch_param.pdsch_dmrs_config_types.value = src->pdsch_param.pdsch_dmrs_config_types.value;

  copy_tl(&src->pdsch_param.max_number_mimo_layers_pdsch.tl, &dst->pdsch_param.max_number_mimo_layers_pdsch.tl);
  dst->pdsch_param.max_number_mimo_layers_pdsch.value = src->pdsch_param.max_number_mimo_layers_pdsch.value;

  copy_tl(&src->pdsch_param.max_mu_mimo_users_dl.tl, &dst->pdsch_param.max_mu_mimo_users_dl.tl);
  dst->pdsch_param.max_mu_mimo_users_dl.value = src->pdsch_param.max_mu_mimo_users_dl.value;

  copy_tl(&src->pdsch_param.pdsch_data_in_dmrs_symbols.tl, &dst->pdsch_param.pdsch_data_in_dmrs_symbols.tl);
  dst->pdsch_param.pdsch_data_in_dmrs_symbols.value = src->pdsch_param.pdsch_data_in_dmrs_symbols.value;

  copy_tl(&src->pdsch_param.premption_support.tl, &dst->pdsch_param.premption_support.tl);
  dst->pdsch_param.premption_support.value = src->pdsch_param.premption_support.value;

  copy_tl(&src->pdsch_param.pdsch_non_slot_support.tl, &dst->pdsch_param.pdsch_non_slot_support.tl);
  dst->pdsch_param.pdsch_non_slot_support.value = src->pdsch_param.pdsch_non_slot_support.value;

  copy_tl(&src->pusch_param.uci_mux_ulsch_in_pusch.tl, &dst->pusch_param.uci_mux_ulsch_in_pusch.tl);
  dst->pusch_param.uci_mux_ulsch_in_pusch.value = src->pusch_param.uci_mux_ulsch_in_pusch.value;

  copy_tl(&src->pusch_param.uci_only_pusch.tl, &dst->pusch_param.uci_only_pusch.tl);
  dst->pusch_param.uci_only_pusch.value = src->pusch_param.uci_only_pusch.value;

  copy_tl(&src->pusch_param.pusch_frequency_hopping.tl, &dst->pusch_param.pusch_frequency_hopping.tl);
  dst->pusch_param.pusch_frequency_hopping.value = src->pusch_param.pusch_frequency_hopping.value;

  copy_tl(&src->pusch_param.pusch_dmrs_config_types.tl, &dst->pusch_param.pusch_dmrs_config_types.tl);
  dst->pusch_param.pusch_dmrs_config_types.value = src->pusch_param.pusch_dmrs_config_types.value;

  copy_tl(&src->pusch_param.pusch_dmrs_max_len.tl, &dst->pusch_param.pusch_dmrs_max_len.tl);
  dst->pusch_param.pusch_dmrs_max_len.value = src->pusch_param.pusch_dmrs_max_len.value;

  copy_tl(&src->pusch_param.pusch_dmrs_additional_pos.tl, &dst->pusch_param.pusch_dmrs_additional_pos.tl);
  dst->pusch_param.pusch_dmrs_additional_pos.value = src->pusch_param.pusch_dmrs_additional_pos.value;

  copy_tl(&src->pusch_param.pusch_cbg.tl, &dst->pusch_param.pusch_cbg.tl);
  dst->pusch_param.pusch_cbg.value = src->pusch_param.pusch_cbg.value;

  copy_tl(&src->pusch_param.pusch_mapping_type.tl, &dst->pusch_param.pusch_mapping_type.tl);
  dst->pusch_param.pusch_mapping_type.value = src->pusch_param.pusch_mapping_type.value;

  copy_tl(&src->pusch_param.pusch_allocation_types.tl, &dst->pusch_param.pusch_allocation_types.tl);
  dst->pusch_param.pusch_allocation_types.value = src->pusch_param.pusch_allocation_types.value;

  copy_tl(&src->pusch_param.pusch_vrb_to_prb_mapping.tl, &dst->pusch_param.pusch_vrb_to_prb_mapping.tl);
  dst->pusch_param.pusch_vrb_to_prb_mapping.value = src->pusch_param.pusch_vrb_to_prb_mapping.value;

  copy_tl(&src->pusch_param.pusch_max_ptrs_ports.tl, &dst->pusch_param.pusch_max_ptrs_ports.tl);
  dst->pusch_param.pusch_max_ptrs_ports.value = src->pusch_param.pusch_max_ptrs_ports.value;

  copy_tl(&src->pusch_param.max_pduschs_tbs_per_slot.tl, &dst->pusch_param.max_pduschs_tbs_per_slot.tl);
  dst->pusch_param.max_pduschs_tbs_per_slot.value = src->pusch_param.max_pduschs_tbs_per_slot.value;

  copy_tl(&src->pusch_param.max_number_mimo_layers_non_cb_pusch.tl, &dst->pusch_param.max_number_mimo_layers_non_cb_pusch.tl);
  dst->pusch_param.max_number_mimo_layers_non_cb_pusch.value = src->pusch_param.max_number_mimo_layers_non_cb_pusch.value;

  copy_tl(&src->pusch_param.supported_modulation_order_ul.tl, &dst->pusch_param.supported_modulation_order_ul.tl);
  dst->pusch_param.supported_modulation_order_ul.value = src->pusch_param.supported_modulation_order_ul.value;

  copy_tl(&src->pusch_param.max_mu_mimo_users_ul.tl, &dst->pusch_param.max_mu_mimo_users_ul.tl);
  dst->pusch_param.max_mu_mimo_users_ul.value = src->pusch_param.max_mu_mimo_users_ul.value;

  copy_tl(&src->pusch_param.dfts_ofdm_support.tl, &dst->pusch_param.dfts_ofdm_support.tl);
  dst->pusch_param.dfts_ofdm_support.value = src->pusch_param.dfts_ofdm_support.value;

  copy_tl(&src->pusch_param.pusch_aggregation_factor.tl, &dst->pusch_param.pusch_aggregation_factor.tl);
  dst->pusch_param.pusch_aggregation_factor.value = src->pusch_param.pusch_aggregation_factor.value;

  copy_tl(&src->prach_param.prach_long_formats.tl, &dst->prach_param.prach_long_formats.tl);
  dst->prach_param.prach_long_formats.value = src->prach_param.prach_long_formats.value;

  copy_tl(&src->prach_param.prach_short_formats.tl, &dst->prach_param.prach_short_formats.tl);
  dst->prach_param.prach_short_formats.value = src->prach_param.prach_short_formats.value;

  copy_tl(&src->prach_param.prach_restricted_sets.tl, &dst->prach_param.prach_restricted_sets.tl);
  dst->prach_param.prach_restricted_sets.value = src->prach_param.prach_restricted_sets.value;

  copy_tl(&src->prach_param.max_prach_fd_occasions_in_a_slot.tl, &dst->prach_param.max_prach_fd_occasions_in_a_slot.tl);
  dst->prach_param.max_prach_fd_occasions_in_a_slot.value = src->prach_param.max_prach_fd_occasions_in_a_slot.value;

  copy_tl(&src->measurement_param.rssi_measurement_support.tl, &dst->measurement_param.rssi_measurement_support.tl);
  dst->measurement_param.rssi_measurement_support.value = src->measurement_param.rssi_measurement_support.value;

  copy_tl(&src->nfapi_config.p7_vnf_address_ipv4.tl, &dst->nfapi_config.p7_vnf_address_ipv4.tl);
  memcpy(dst->nfapi_config.p7_vnf_address_ipv4.address,
         src->nfapi_config.p7_vnf_address_ipv4.address,
         sizeof(dst->nfapi_config.p7_vnf_address_ipv4.address));

  copy_tl(&src->nfapi_config.p7_vnf_address_ipv6.tl, &dst->nfapi_config.p7_vnf_address_ipv6.tl);
  memcpy(dst->nfapi_config.p7_vnf_address_ipv6.address,
         src->nfapi_config.p7_vnf_address_ipv6.address,
         sizeof(dst->nfapi_config.p7_vnf_address_ipv6.address));

  copy_tl(&src->nfapi_config.p7_vnf_port.tl, &dst->nfapi_config.p7_vnf_port.tl);
  dst->nfapi_config.p7_vnf_port.value = src->nfapi_config.p7_vnf_port.value;

  copy_tl(&src->nfapi_config.p7_pnf_address_ipv4.tl, &dst->nfapi_config.p7_pnf_address_ipv4.tl);
  memcpy(dst->nfapi_config.p7_pnf_address_ipv4.address,
         src->nfapi_config.p7_pnf_address_ipv4.address,
         sizeof(dst->nfapi_config.p7_pnf_address_ipv4.address));

  copy_tl(&src->nfapi_config.p7_pnf_address_ipv6.tl, &dst->nfapi_config.p7_pnf_address_ipv6.tl);
  memcpy(dst->nfapi_config.p7_pnf_address_ipv6.address,
         src->nfapi_config.p7_pnf_address_ipv6.address,
         sizeof(dst->nfapi_config.p7_pnf_address_ipv6.address));

  copy_tl(&src->nfapi_config.p7_pnf_port.tl, &dst->nfapi_config.p7_pnf_port.tl);
  dst->nfapi_config.p7_pnf_port.value = src->nfapi_config.p7_pnf_port.value;

  copy_tl(&src->nfapi_config.timing_window.tl, &dst->nfapi_config.timing_window.tl);
  dst->nfapi_config.timing_window.value = src->nfapi_config.timing_window.value;

  copy_tl(&src->nfapi_config.timing_info_mode.tl, &dst->nfapi_config.timing_info_mode.tl);
  dst->nfapi_config.timing_info_mode.value = src->nfapi_config.timing_info_mode.value;

  copy_tl(&src->nfapi_config.timing_info_period.tl, &dst->nfapi_config.timing_info_period.tl);
  dst->nfapi_config.timing_info_period.value = src->nfapi_config.timing_info_period.value;

  copy_tl(&src->nfapi_config.dl_tti_timing_offset.tl, &dst->nfapi_config.dl_tti_timing_offset.tl);
  dst->nfapi_config.dl_tti_timing_offset.value = src->nfapi_config.dl_tti_timing_offset.value;

  copy_tl(&src->nfapi_config.ul_tti_timing_offset.tl, &dst->nfapi_config.ul_tti_timing_offset.tl);
  dst->nfapi_config.ul_tti_timing_offset.value = src->nfapi_config.ul_tti_timing_offset.value;

  copy_tl(&src->nfapi_config.ul_dci_timing_offset.tl, &dst->nfapi_config.ul_dci_timing_offset.tl);
  dst->nfapi_config.ul_dci_timing_offset.value = src->nfapi_config.ul_dci_timing_offset.value;

  copy_tl(&src->nfapi_config.tx_data_timing_offset.tl, &dst->nfapi_config.tx_data_timing_offset.tl);
  dst->nfapi_config.tx_data_timing_offset.value = src->nfapi_config.tx_data_timing_offset.value;
}
