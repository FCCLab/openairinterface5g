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

bool compare_config_request(const nfapi_nr_config_request_scf_t *unpacked_req, const nfapi_nr_config_request_scf_t *req)
{
  CMP(unpacked_req->header.message_id, req->header.message_id);
  CMP(unpacked_req->header.message_length, req->header.message_length);
  CMP(unpacked_req->num_tlv, req->num_tlv);

  CMP(unpacked_req->carrier_config.dl_bandwidth.tl.tag, req->carrier_config.dl_bandwidth.tl.tag);
  CMP(unpacked_req->carrier_config.dl_bandwidth.value, req->carrier_config.dl_bandwidth.value);

  CMP(unpacked_req->carrier_config.dl_frequency.tl.tag, req->carrier_config.dl_frequency.tl.tag);
  CMP(unpacked_req->carrier_config.dl_frequency.value, req->carrier_config.dl_frequency.value);

  for (int i = 0; i < 5; ++i) {
    CMP(unpacked_req->carrier_config.dl_k0[i].tl.tag, req->carrier_config.dl_k0[i].tl.tag);
    CMP(unpacked_req->carrier_config.dl_k0[i].value, req->carrier_config.dl_k0[i].value);

    CMP(unpacked_req->carrier_config.dl_grid_size[i].tl.tag, req->carrier_config.dl_grid_size[i].tl.tag);
    CMP(unpacked_req->carrier_config.dl_grid_size[i].value, req->carrier_config.dl_grid_size[i].value);
  }

  CMP(unpacked_req->carrier_config.num_tx_ant.tl.tag, req->carrier_config.num_tx_ant.tl.tag);
  CMP(unpacked_req->carrier_config.num_tx_ant.value, req->carrier_config.num_tx_ant.value);

  CMP(unpacked_req->carrier_config.uplink_bandwidth.tl.tag, req->carrier_config.uplink_bandwidth.tl.tag);
  CMP(unpacked_req->carrier_config.uplink_bandwidth.value, req->carrier_config.uplink_bandwidth.value);

  CMP(unpacked_req->carrier_config.uplink_frequency.tl.tag, req->carrier_config.uplink_frequency.tl.tag);
  CMP(unpacked_req->carrier_config.uplink_frequency.value, req->carrier_config.uplink_frequency.value);

  for (int i = 0; i < 5; ++i) {
    CMP(unpacked_req->carrier_config.ul_k0[i].tl.tag, req->carrier_config.ul_k0[i].tl.tag);
    CMP(unpacked_req->carrier_config.ul_k0[i].value, req->carrier_config.ul_k0[i].value);

    CMP(unpacked_req->carrier_config.ul_grid_size[i].tl.tag, req->carrier_config.ul_grid_size[i].tl.tag);
    CMP(unpacked_req->carrier_config.ul_grid_size[i].value, req->carrier_config.ul_grid_size[i].value);
  }

  CMP(unpacked_req->carrier_config.num_rx_ant.tl.tag, req->carrier_config.num_rx_ant.tl.tag);
  CMP(unpacked_req->carrier_config.num_rx_ant.value, req->carrier_config.num_rx_ant.value);

  CMP(unpacked_req->carrier_config.frequency_shift_7p5khz.tl.tag, req->carrier_config.frequency_shift_7p5khz.tl.tag);
  CMP(unpacked_req->carrier_config.frequency_shift_7p5khz.value, req->carrier_config.frequency_shift_7p5khz.value);

  CMP(unpacked_req->cell_config.phy_cell_id.tl.tag, req->cell_config.phy_cell_id.tl.tag);
  CMP(unpacked_req->cell_config.phy_cell_id.value, req->cell_config.phy_cell_id.value);

  CMP(unpacked_req->cell_config.frame_duplex_type.tl.tag, req->cell_config.frame_duplex_type.tl.tag);
  CMP(unpacked_req->cell_config.frame_duplex_type.value, req->cell_config.frame_duplex_type.value);

  CMP(unpacked_req->ssb_config.ss_pbch_power.tl.tag, req->ssb_config.ss_pbch_power.tl.tag);
  CMP(unpacked_req->ssb_config.ss_pbch_power.value, req->ssb_config.ss_pbch_power.value);

  CMP(unpacked_req->ssb_config.bch_payload.tl.tag, req->ssb_config.bch_payload.tl.tag);
  CMP(unpacked_req->ssb_config.bch_payload.value, req->ssb_config.bch_payload.value);

  CMP(unpacked_req->ssb_config.scs_common.tl.tag, req->ssb_config.scs_common.tl.tag);
  CMP(unpacked_req->ssb_config.scs_common.value, req->ssb_config.scs_common.value);

  CMP(unpacked_req->prach_config.prach_sequence_length.tl.tag, req->prach_config.prach_sequence_length.tl.tag);
  CMP(unpacked_req->prach_config.prach_sequence_length.value, req->prach_config.prach_sequence_length.value);

  CMP(unpacked_req->prach_config.prach_sub_c_spacing.tl.tag, req->prach_config.prach_sub_c_spacing.tl.tag);
  CMP(unpacked_req->prach_config.prach_sub_c_spacing.value, req->prach_config.prach_sub_c_spacing.value);

  CMP(unpacked_req->prach_config.restricted_set_config.tl.tag, req->prach_config.restricted_set_config.tl.tag);
  CMP(unpacked_req->prach_config.restricted_set_config.value, req->prach_config.restricted_set_config.value);

  CMP(unpacked_req->prach_config.num_prach_fd_occasions.tl.tag, req->prach_config.num_prach_fd_occasions.tl.tag);
  CMP(unpacked_req->prach_config.num_prach_fd_occasions.value, req->prach_config.num_prach_fd_occasions.value);

  CMP(unpacked_req->prach_config.prach_ConfigurationIndex.tl.tag, req->prach_config.prach_ConfigurationIndex.tl.tag);
  CMP(unpacked_req->prach_config.prach_ConfigurationIndex.value, req->prach_config.prach_ConfigurationIndex.value);

  for (int i = 0; i < unpacked_req->prach_config.num_prach_fd_occasions.value; i++) {
    nfapi_nr_num_prach_fd_occasions_t unpacked_prach_fd_occasion = unpacked_req->prach_config.num_prach_fd_occasions_list[i];
    nfapi_nr_num_prach_fd_occasions_t req_prach_fd_occasion = req->prach_config.num_prach_fd_occasions_list[i];

    CMP(unpacked_prach_fd_occasion.prach_root_sequence_index.tl.tag, req_prach_fd_occasion.prach_root_sequence_index.tl.tag);
    CMP(unpacked_prach_fd_occasion.prach_root_sequence_index.value, req_prach_fd_occasion.prach_root_sequence_index.value);

    CMP(unpacked_prach_fd_occasion.num_root_sequences.tl.tag, req_prach_fd_occasion.num_root_sequences.tl.tag);
    CMP(unpacked_prach_fd_occasion.num_root_sequences.value, req_prach_fd_occasion.num_root_sequences.value);

    CMP(unpacked_prach_fd_occasion.k1.tl.tag, req_prach_fd_occasion.k1.tl.tag);
    CMP(unpacked_prach_fd_occasion.k1.value, req_prach_fd_occasion.k1.value);

    CMP(unpacked_prach_fd_occasion.prach_zero_corr_conf.tl.tag, req_prach_fd_occasion.prach_zero_corr_conf.tl.tag);
    CMP(unpacked_prach_fd_occasion.prach_zero_corr_conf.value, req_prach_fd_occasion.prach_zero_corr_conf.value);

    CMP(unpacked_prach_fd_occasion.num_unused_root_sequences.tl.tag, req_prach_fd_occasion.num_unused_root_sequences.tl.tag);
    CMP(unpacked_prach_fd_occasion.num_unused_root_sequences.value, req_prach_fd_occasion.num_unused_root_sequences.value);
    for (int k = 0; k < unpacked_prach_fd_occasion.num_unused_root_sequences.value; k++) {
      CMP(unpacked_prach_fd_occasion.unused_root_sequences_list[k].tl.tag,
          req_prach_fd_occasion.unused_root_sequences_list[k].tl.tag);
      CMP(unpacked_prach_fd_occasion.unused_root_sequences_list[k].value,
          req_prach_fd_occasion.unused_root_sequences_list[k].value);
    }
  }

  CMP(unpacked_req->prach_config.ssb_per_rach.tl.tag, req->prach_config.ssb_per_rach.tl.tag);
  CMP(unpacked_req->prach_config.ssb_per_rach.value, req->prach_config.ssb_per_rach.value);

  CMP(unpacked_req->prach_config.prach_multiple_carriers_in_a_band.tl.tag,
      req->prach_config.prach_multiple_carriers_in_a_band.tl.tag);
  CMP(unpacked_req->prach_config.prach_multiple_carriers_in_a_band.value,
      req->prach_config.prach_multiple_carriers_in_a_band.value);

  CMP(unpacked_req->ssb_table.ssb_offset_point_a.tl.tag, req->ssb_table.ssb_offset_point_a.tl.tag);
  CMP(unpacked_req->ssb_table.ssb_offset_point_a.value, req->ssb_table.ssb_offset_point_a.value);

  CMP(unpacked_req->ssb_table.ssb_period.tl.tag, req->ssb_table.ssb_period.tl.tag);
  CMP(unpacked_req->ssb_table.ssb_period.value, req->ssb_table.ssb_period.value);

  CMP(unpacked_req->ssb_table.ssb_subcarrier_offset.tl.tag, req->ssb_table.ssb_subcarrier_offset.tl.tag);
  CMP(unpacked_req->ssb_table.ssb_subcarrier_offset.value, req->ssb_table.ssb_subcarrier_offset.value);

  CMP(unpacked_req->ssb_table.MIB.tl.tag, req->ssb_table.MIB.tl.tag);
  CMP(unpacked_req->ssb_table.MIB.value, req->ssb_table.MIB.value);

  CMP(unpacked_req->ssb_table.ssb_mask_list[0].ssb_mask.tl.tag, req->ssb_table.ssb_mask_list[0].ssb_mask.tl.tag);
  CMP(unpacked_req->ssb_table.ssb_mask_list[0].ssb_mask.value, req->ssb_table.ssb_mask_list[0].ssb_mask.value);

  CMP(unpacked_req->ssb_table.ssb_mask_list[1].ssb_mask.tl.tag, req->ssb_table.ssb_mask_list[1].ssb_mask.tl.tag);
  CMP(unpacked_req->ssb_table.ssb_mask_list[1].ssb_mask.value, req->ssb_table.ssb_mask_list[1].ssb_mask.value);

  for (int i = 0; i < 64; i++) {
    CMP(unpacked_req->ssb_table.ssb_beam_id_list[i].beam_id.tl.tag, req->ssb_table.ssb_beam_id_list[i].beam_id.tl.tag);
    CMP(unpacked_req->ssb_table.ssb_beam_id_list[i].beam_id.value, req->ssb_table.ssb_beam_id_list[i].beam_id.value);
  }

  CMP(unpacked_req->tdd_table.tdd_period.tl.tag, req->tdd_table.tdd_period.tl.tag);
  CMP(unpacked_req->tdd_table.tdd_period.value, req->tdd_table.tdd_period.value);

  const uint8_t slotsperframe[5] = {10, 20, 40, 80, 160};
  // Assuming always CP_Normal, because Cyclic prefix is not included in CONFIG.request 10.02, but is present in 10.04
  uint8_t cyclicprefix = 1;
  bool normal_CP = cyclicprefix ? false : true;
  // 3GPP 38.211 Table 4.3.2.1 & Table 4.3.2.2
  uint8_t number_of_symbols_per_slot = normal_CP ? 14 : 12;

  for (int i = 0; i < slotsperframe[unpacked_req->ssb_config.scs_common.value]; i++) {
    for (int k = 0; k < number_of_symbols_per_slot; k++) {
      CMP(unpacked_req->tdd_table.max_tdd_periodicity_list[i].max_num_of_symbol_per_slot_list[k].slot_config.tl.tag,
          req->tdd_table.max_tdd_periodicity_list[i].max_num_of_symbol_per_slot_list[k].slot_config.tl.tag);
      CMP(unpacked_req->tdd_table.max_tdd_periodicity_list[i].max_num_of_symbol_per_slot_list[k].slot_config.value,
          req->tdd_table.max_tdd_periodicity_list[i].max_num_of_symbol_per_slot_list[k].slot_config.value);
    }
  }

  CMP(unpacked_req->measurement_config.rssi_measurement.tl.tag, req->measurement_config.rssi_measurement.tl.tag);
  CMP(unpacked_req->measurement_config.rssi_measurement.value, req->measurement_config.rssi_measurement.value);

  CMP(unpacked_req->nfapi_config.p7_vnf_address_ipv4.tl.tag, req->nfapi_config.p7_vnf_address_ipv4.tl.tag);
  for (int i = 0; i < NFAPI_IPV4_ADDRESS_LENGTH; ++i) {
    CMP(unpacked_req->nfapi_config.p7_vnf_address_ipv4.address[i], req->nfapi_config.p7_vnf_address_ipv4.address[i]);
  }

  CMP(unpacked_req->nfapi_config.p7_vnf_address_ipv6.tl.tag, req->nfapi_config.p7_vnf_address_ipv6.tl.tag);
  for (int i = 0; i < NFAPI_IPV6_ADDRESS_LENGTH; ++i) {
    CMP(unpacked_req->nfapi_config.p7_vnf_address_ipv6.address[i], req->nfapi_config.p7_vnf_address_ipv6.address[i]);
  }

  CMP(unpacked_req->nfapi_config.p7_vnf_port.tl.tag, req->nfapi_config.p7_vnf_port.tl.tag);
  CMP(unpacked_req->nfapi_config.p7_vnf_port.value, req->nfapi_config.p7_vnf_port.value);

  CMP(unpacked_req->nfapi_config.timing_window.tl.tag, req->nfapi_config.timing_window.tl.tag);
  CMP(unpacked_req->nfapi_config.timing_window.value, req->nfapi_config.timing_window.value);

  CMP(unpacked_req->nfapi_config.timing_info_mode.tl.tag, req->nfapi_config.timing_info_mode.tl.tag);
  CMP(unpacked_req->nfapi_config.timing_info_mode.value, req->nfapi_config.timing_info_mode.value);

  CMP(unpacked_req->nfapi_config.timing_info_period.tl.tag, req->nfapi_config.timing_info_period.tl.tag);
  CMP(unpacked_req->nfapi_config.timing_info_period.value, req->nfapi_config.timing_info_period.value);
  return 0;
}

static bool compare_config_response_tlv_lists(nfapi_nr_generic_tlv_scf_t *unpacked_list,
                                              nfapi_nr_generic_tlv_scf_t *req_list,
                                              uint8_t size)
{
  for (int i = 0; i < size; ++i) {
    nfapi_nr_generic_tlv_scf_t *unpacked_element = &(unpacked_list[i]);
    nfapi_nr_generic_tlv_scf_t *req_element = &(req_list[i]);

    CMP(unpacked_element->tl.tag, req_element->tl.tag);
    CMP(unpacked_element->tl.length, req_element->tl.length);
    // check according to value type
    switch (unpacked_element->tl.length) {
      case UINT_8:
        CMP(unpacked_element->value.u8, req_element->value.u8);
        break;
      case UINT_16:
        CMP(unpacked_element->value.u16, req_element->value.u16);
        break;
      case UINT_32:
        CMP(unpacked_element->value.u32, req_element->value.u32);
        break;
      case ARRAY_UINT_16:
        for (int j = 0; j < 5; ++j) {
          CMP(unpacked_element->value.array_u16[j], req_element->value.array_u16[j]);
        }
        break;
      default:
        printf("unknown length %d\n", unpacked_element->tl.length);
        DevAssert(1 == 0);
        break;
    }
  }
  return 0;
}

bool compare_config_response(const nfapi_nr_config_response_scf_t *unpacked_req, const nfapi_nr_config_response_scf_t *req)
{
  CMP(unpacked_req->header.message_id, req->header.message_id);
  CMP(unpacked_req->header.message_length, req->header.message_length);

  CMP(unpacked_req->error_code, req->error_code);
  CMP(unpacked_req->num_invalid_tlvs, req->num_invalid_tlvs);
  CMP(unpacked_req->num_invalid_tlvs_configured_in_idle, req->num_invalid_tlvs_configured_in_idle);
  CMP(unpacked_req->num_invalid_tlvs_configured_in_running, req->num_invalid_tlvs_configured_in_running);
  CMP(unpacked_req->num_missing_tlvs, req->num_missing_tlvs);
  // compare the list elements
  CMP(compare_config_response_tlv_lists(unpacked_req->invalid_tlvs_list, req->invalid_tlvs_list, req->num_invalid_tlvs), 0);
  CMP(compare_config_response_tlv_lists(unpacked_req->invalid_tlvs_configured_in_idle_list,
                                        req->invalid_tlvs_configured_in_idle_list,
                                        req->num_invalid_tlvs_configured_in_idle),
      0);
  CMP(compare_config_response_tlv_lists(unpacked_req->invalid_tlvs_configured_in_running_list,
                                        req->invalid_tlvs_configured_in_running_list,
                                        req->num_invalid_tlvs_configured_in_running),
      0);
  CMP(compare_config_response_tlv_lists(unpacked_req->missing_tlvs_list, req->missing_tlvs_list, req->num_missing_tlvs), 0);
  return 0;
}

bool compare_start_request(const nfapi_nr_start_request_scf_t *unpacked_req, const nfapi_nr_start_request_scf_t *req)
{
  CMP(unpacked_req->header.message_id, req->header.message_id);
  CMP(unpacked_req->header.message_length, req->header.message_length);
  return 0;
}

bool compare_start_response(const nfapi_nr_start_response_scf_t *unpacked_req, const nfapi_nr_start_response_scf_t *req)
{
  CMP(unpacked_req->header.message_id, req->header.message_id);
  CMP(unpacked_req->header.message_length, req->header.message_length);
  CMP(unpacked_req->error_code, req->error_code);
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

void free_config_request(nfapi_nr_config_request_scf_t *msg)
{
  if (msg->vendor_extension) {
    free(msg->vendor_extension);
  }

  if (msg->prach_config.num_prach_fd_occasions_list) {
    for (int i = 0; i < msg->prach_config.num_prach_fd_occasions.value; i++) {
      nfapi_nr_num_prach_fd_occasions_t *prach_fd_occasion = &(msg->prach_config.num_prach_fd_occasions_list[i]);
      if(prach_fd_occasion->unused_root_sequences_list){
        free(prach_fd_occasion->unused_root_sequences_list);
      }
    }
    free(msg->prach_config.num_prach_fd_occasions_list);
  }
  const uint8_t slotsperframe[5] = {10, 20, 40, 80, 160};
  if (msg->tdd_table.max_tdd_periodicity_list) {
    for (int i = 0; i < slotsperframe[msg->ssb_config.scs_common.value]; i++) {
      free(msg->tdd_table.max_tdd_periodicity_list[i].max_num_of_symbol_per_slot_list);
    }
    free(msg->tdd_table.max_tdd_periodicity_list);
  }

  if (msg->pmi_list.pmi_pdu) {
    free(msg->pmi_list.pmi_pdu);
  }
}

void free_config_response(nfapi_nr_config_response_scf_t *msg)
{
  if (msg->vendor_extension) {
    free(msg->vendor_extension);
  }

  if (msg->invalid_tlvs_list) {
    free(msg->invalid_tlvs_list);
  }

  if (msg->invalid_tlvs_configured_in_idle_list) {
    free(msg->invalid_tlvs_configured_in_idle_list);
  }

  if (msg->invalid_tlvs_configured_in_running_list) {
    free(msg->invalid_tlvs_configured_in_running_list);
  }

  if (msg->missing_tlvs_list) {
    free(msg->missing_tlvs_list);
  }
}

void free_start_request(nfapi_nr_start_request_scf_t *msg)
{
  if (msg->vendor_extension) {
    free(msg->vendor_extension);
  }
}

void free_start_response(nfapi_nr_start_response_scf_t *msg)
{
  if (msg->vendor_extension) {
    free(msg->vendor_extension);
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

void copy_config_request(const nfapi_nr_config_request_scf_t *src, nfapi_nr_config_request_scf_t *dst)
{
  dst->header.message_id = src->header.message_id;
  dst->header.message_length = src->header.message_length;
  if (src->vendor_extension) {
    dst->vendor_extension = calloc(1, sizeof(nfapi_vendor_extension_tlv_t));
    dst->vendor_extension->tag = src->vendor_extension->tag;
    dst->vendor_extension->length = src->vendor_extension->length;
    // TODO: FIGURE OUT WHERE THE VENDOR EXTENSION VALUE IS
  }

  dst->num_tlv = src->num_tlv;

  copy_tl(&src->carrier_config.dl_bandwidth.tl, &dst->carrier_config.dl_bandwidth.tl);
  dst->carrier_config.dl_bandwidth.value = src->carrier_config.dl_bandwidth.value;

  copy_tl(&src->carrier_config.dl_frequency.tl, &dst->carrier_config.dl_frequency.tl);
  dst->carrier_config.dl_frequency.value = src->carrier_config.dl_frequency.value;

  for (int i = 0; i < 5; ++i) {
    copy_tl(&src->carrier_config.dl_k0[i].tl, &dst->carrier_config.dl_k0[i].tl);
    dst->carrier_config.dl_k0[i].value = src->carrier_config.dl_k0[i].value;

    copy_tl(&src->carrier_config.dl_grid_size[i].tl, &dst->carrier_config.dl_grid_size[i].tl);
    dst->carrier_config.dl_grid_size[i].value = src->carrier_config.dl_grid_size[i].value;
  }

  copy_tl(&src->carrier_config.num_tx_ant.tl, &dst->carrier_config.num_tx_ant.tl);
  dst->carrier_config.num_tx_ant.value = src->carrier_config.num_tx_ant.value;

  copy_tl(&src->carrier_config.uplink_bandwidth.tl, &dst->carrier_config.uplink_bandwidth.tl);
  dst->carrier_config.uplink_bandwidth.value = src->carrier_config.uplink_bandwidth.value;

  copy_tl(&src->carrier_config.uplink_frequency.tl, &dst->carrier_config.uplink_frequency.tl);
  dst->carrier_config.uplink_frequency.value = src->carrier_config.uplink_frequency.value;

  copy_tl(&src->carrier_config.uplink_frequency.tl, &dst->carrier_config.uplink_frequency.tl);
  dst->carrier_config.uplink_frequency.value = src->carrier_config.uplink_frequency.value;

  for (int i = 0; i < 5; ++i) {
    copy_tl(&src->carrier_config.ul_k0[i].tl, &dst->carrier_config.ul_k0[i].tl);
    dst->carrier_config.ul_k0[i].value = src->carrier_config.ul_k0[i].value;

    copy_tl(&src->carrier_config.ul_grid_size[i].tl, &dst->carrier_config.ul_grid_size[i].tl);
    dst->carrier_config.ul_grid_size[i].value = src->carrier_config.ul_grid_size[i].value;
  }

  copy_tl(&src->carrier_config.num_rx_ant.tl, &dst->carrier_config.num_rx_ant.tl);
  dst->carrier_config.num_rx_ant.value = src->carrier_config.num_rx_ant.value;

  copy_tl(&src->carrier_config.frequency_shift_7p5khz.tl, &dst->carrier_config.frequency_shift_7p5khz.tl);
  dst->carrier_config.frequency_shift_7p5khz.value = src->carrier_config.frequency_shift_7p5khz.value;

  copy_tl(&src->cell_config.phy_cell_id.tl, &dst->cell_config.phy_cell_id.tl);
  dst->cell_config.phy_cell_id.value = src->cell_config.phy_cell_id.value;

  copy_tl(&src->cell_config.frame_duplex_type.tl, &dst->cell_config.frame_duplex_type.tl);
  dst->cell_config.frame_duplex_type.value = src->cell_config.frame_duplex_type.value;

  copy_tl(&src->ssb_config.ss_pbch_power.tl, &dst->ssb_config.ss_pbch_power.tl);
  dst->ssb_config.ss_pbch_power.value = src->ssb_config.ss_pbch_power.value;

  copy_tl(&src->ssb_config.bch_payload.tl, &dst->ssb_config.bch_payload.tl);
  dst->ssb_config.bch_payload.value = src->ssb_config.bch_payload.value;

  copy_tl(&src->ssb_config.scs_common.tl, &dst->ssb_config.scs_common.tl);
  dst->ssb_config.scs_common.value = src->ssb_config.scs_common.value;

  copy_tl(&src->prach_config.prach_sequence_length.tl, &dst->prach_config.prach_sequence_length.tl);
  dst->prach_config.prach_sequence_length.value = src->prach_config.prach_sequence_length.value;

  copy_tl(&src->prach_config.prach_sub_c_spacing.tl, &dst->prach_config.prach_sub_c_spacing.tl);
  dst->prach_config.prach_sub_c_spacing.value = src->prach_config.prach_sub_c_spacing.value;

  copy_tl(&src->prach_config.restricted_set_config.tl, &dst->prach_config.restricted_set_config.tl);
  dst->prach_config.restricted_set_config.value = src->prach_config.restricted_set_config.value;

  copy_tl(&src->prach_config.num_prach_fd_occasions.tl, &dst->prach_config.num_prach_fd_occasions.tl);
  dst->prach_config.num_prach_fd_occasions.value = src->prach_config.num_prach_fd_occasions.value;

  copy_tl(&src->prach_config.prach_ConfigurationIndex.tl, &dst->prach_config.prach_ConfigurationIndex.tl);
  dst->prach_config.prach_ConfigurationIndex.value = src->prach_config.prach_ConfigurationIndex.value;

  copy_tl(&src->prach_config.prach_ConfigurationIndex.tl, &dst->prach_config.prach_ConfigurationIndex.tl);
  dst->prach_config.prach_ConfigurationIndex.value = src->prach_config.prach_ConfigurationIndex.value;

  dst->prach_config.num_prach_fd_occasions_list = (nfapi_nr_num_prach_fd_occasions_t *)malloc(
      dst->prach_config.num_prach_fd_occasions.value * sizeof(nfapi_nr_num_prach_fd_occasions_t));
  for (int i = 0; i < dst->prach_config.num_prach_fd_occasions.value; i++) {
    nfapi_nr_num_prach_fd_occasions_t *src_prach_fd_occasion = &(src->prach_config.num_prach_fd_occasions_list[i]);
    nfapi_nr_num_prach_fd_occasions_t *dst_prach_fd_occasion = &(dst->prach_config.num_prach_fd_occasions_list[i]);

    copy_tl(&src_prach_fd_occasion->prach_root_sequence_index.tl, &dst_prach_fd_occasion->prach_root_sequence_index.tl);
    dst_prach_fd_occasion->prach_root_sequence_index.value = src_prach_fd_occasion->prach_root_sequence_index.value;

    copy_tl(&src_prach_fd_occasion->num_root_sequences.tl, &dst_prach_fd_occasion->num_root_sequences.tl);
    dst_prach_fd_occasion->num_root_sequences.value = src_prach_fd_occasion->num_root_sequences.value;

    copy_tl(&src_prach_fd_occasion->k1.tl, &dst_prach_fd_occasion->k1.tl);
    dst_prach_fd_occasion->k1.value = src_prach_fd_occasion->k1.value;

    copy_tl(&src_prach_fd_occasion->prach_zero_corr_conf.tl, &dst_prach_fd_occasion->prach_zero_corr_conf.tl);
    dst_prach_fd_occasion->prach_zero_corr_conf.value = src_prach_fd_occasion->prach_zero_corr_conf.value;

    copy_tl(&src_prach_fd_occasion->num_unused_root_sequences.tl, &dst_prach_fd_occasion->num_unused_root_sequences.tl);
    dst_prach_fd_occasion->num_unused_root_sequences.value = src_prach_fd_occasion->num_unused_root_sequences.value;

    dst_prach_fd_occasion->unused_root_sequences_list =
        (nfapi_uint8_tlv_t *)malloc(dst_prach_fd_occasion->num_unused_root_sequences.value * sizeof(nfapi_uint8_tlv_t));
    for (int k = 0; k < dst_prach_fd_occasion->num_unused_root_sequences.value; k++) {
      copy_tl(&src_prach_fd_occasion->unused_root_sequences_list[k].tl, &dst_prach_fd_occasion->unused_root_sequences_list[k].tl);
      dst_prach_fd_occasion->unused_root_sequences_list[k].value = src_prach_fd_occasion->unused_root_sequences_list[k].value;
    }
  }

  copy_tl(&src->prach_config.ssb_per_rach.tl, &dst->prach_config.ssb_per_rach.tl);
  dst->prach_config.ssb_per_rach.value = src->prach_config.ssb_per_rach.value;

  copy_tl(&src->prach_config.prach_multiple_carriers_in_a_band.tl, &dst->prach_config.prach_multiple_carriers_in_a_band.tl);
  dst->prach_config.prach_multiple_carriers_in_a_band.value = src->prach_config.prach_multiple_carriers_in_a_band.value;

  copy_tl(&src->ssb_table.ssb_offset_point_a.tl, &dst->ssb_table.ssb_offset_point_a.tl);
  dst->ssb_table.ssb_offset_point_a.value = src->ssb_table.ssb_offset_point_a.value;

  copy_tl(&src->ssb_table.ssb_period.tl, &dst->ssb_table.ssb_period.tl);
  dst->ssb_table.ssb_period.value = src->ssb_table.ssb_period.value;

  copy_tl(&src->ssb_table.ssb_subcarrier_offset.tl, &dst->ssb_table.ssb_subcarrier_offset.tl);
  dst->ssb_table.ssb_subcarrier_offset.value = src->ssb_table.ssb_subcarrier_offset.value;

  copy_tl(&src->ssb_table.MIB.tl, &dst->ssb_table.MIB.tl);
  dst->ssb_table.MIB.value = src->ssb_table.MIB.value;

  copy_tl(&src->ssb_table.ssb_mask_list[0].ssb_mask.tl, &dst->ssb_table.ssb_mask_list[0].ssb_mask.tl);
  dst->ssb_table.ssb_mask_list[0].ssb_mask.value = src->ssb_table.ssb_mask_list[0].ssb_mask.value;

  copy_tl(&src->ssb_table.ssb_mask_list[1].ssb_mask.tl, &dst->ssb_table.ssb_mask_list[1].ssb_mask.tl);
  dst->ssb_table.ssb_mask_list[1].ssb_mask.value = src->ssb_table.ssb_mask_list[1].ssb_mask.value;

  for (int i = 0; i < 64; i++) {
    copy_tl(&src->ssb_table.ssb_beam_id_list[i].beam_id.tl, &dst->ssb_table.ssb_beam_id_list[i].beam_id.tl);
    dst->ssb_table.ssb_beam_id_list[i].beam_id.value = src->ssb_table.ssb_beam_id_list[i].beam_id.value;
  }

  copy_tl(&src->tdd_table.tdd_period.tl, &dst->tdd_table.tdd_period.tl);
  dst->tdd_table.tdd_period.value = src->tdd_table.tdd_period.value;
  const uint8_t slotsperframe[5] = {10, 20, 40, 80, 160};
  // Assuming always CP_Normal, because Cyclic prefix is not included in CONFIG.request 10.02, but is present in 10.04
  uint8_t cyclicprefix = 1;
  bool normal_CP = cyclicprefix ? false : true;
  // 3GPP 38.211 Table 4.3.2.1 & Table 4.3.2.2
  uint8_t number_of_symbols_per_slot = normal_CP ? 14 : 12;
  dst->tdd_table.max_tdd_periodicity_list = (nfapi_nr_max_tdd_periodicity_t *)malloc(slotsperframe[dst->ssb_config.scs_common.value]
                                                                                     * sizeof(nfapi_nr_max_tdd_periodicity_t));

  for (int i = 0; i < slotsperframe[dst->ssb_config.scs_common.value]; i++) {
    dst->tdd_table.max_tdd_periodicity_list[i].max_num_of_symbol_per_slot_list =
        (nfapi_nr_max_num_of_symbol_per_slot_t *)malloc(number_of_symbols_per_slot * sizeof(nfapi_nr_max_num_of_symbol_per_slot_t));
  }
  for (int i = 0; i < slotsperframe[dst->ssb_config.scs_common.value]; i++) { // TODO check right number of slots
    for (int k = 0; k < number_of_symbols_per_slot; k++) { // TODO can change?
      copy_tl(&src->tdd_table.max_tdd_periodicity_list[i].max_num_of_symbol_per_slot_list[k].slot_config.tl,
              &dst->tdd_table.max_tdd_periodicity_list[i].max_num_of_symbol_per_slot_list[k].slot_config.tl);
      dst->tdd_table.max_tdd_periodicity_list[i].max_num_of_symbol_per_slot_list[k].slot_config.value =
          src->tdd_table.max_tdd_periodicity_list[i].max_num_of_symbol_per_slot_list[k].slot_config.value;
    }
  }

  copy_tl(&src->measurement_config.rssi_measurement.tl, &dst->measurement_config.rssi_measurement.tl);
  dst->measurement_config.rssi_measurement.value = src->measurement_config.rssi_measurement.value;

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

void copy_config_response(const nfapi_nr_config_response_scf_t *src, nfapi_nr_config_response_scf_t *dst)
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
  dst->num_invalid_tlvs = src->num_invalid_tlvs;
  dst->num_invalid_tlvs_configured_in_idle = src->num_invalid_tlvs_configured_in_idle;
  dst->num_invalid_tlvs_configured_in_running = src->num_invalid_tlvs_configured_in_running;
  dst->num_missing_tlvs = src->num_missing_tlvs;

  dst->invalid_tlvs_list =
      (nfapi_nr_generic_tlv_scf_t *)malloc(dst->num_invalid_tlvs * sizeof(nfapi_nr_generic_tlv_scf_t));
  dst->invalid_tlvs_configured_in_idle_list =
      (nfapi_nr_generic_tlv_scf_t *)malloc(dst->num_invalid_tlvs_configured_in_idle * sizeof(nfapi_nr_generic_tlv_scf_t));
  dst->invalid_tlvs_configured_in_running_list =
      (nfapi_nr_generic_tlv_scf_t *)malloc(dst->num_invalid_tlvs_configured_in_running * sizeof(nfapi_nr_generic_tlv_scf_t));
  dst->missing_tlvs_list =
      (nfapi_nr_generic_tlv_scf_t *)malloc(dst->num_missing_tlvs * sizeof(nfapi_nr_generic_tlv_scf_t));

  for (int i = 0; i < dst->num_invalid_tlvs; ++i) {
    copy_tl(&src->invalid_tlvs_list[i].tl,&dst->invalid_tlvs_list[i].tl);
    dst->invalid_tlvs_list[i].value = src->invalid_tlvs_list[i].value;
  }

  for (int i = 0; i < dst->num_invalid_tlvs_configured_in_idle; ++i) {
    copy_tl(&src->invalid_tlvs_configured_in_idle_list[i].tl,&dst->invalid_tlvs_configured_in_idle_list[i].tl);
    dst->invalid_tlvs_configured_in_idle_list[i].value = src->invalid_tlvs_configured_in_idle_list[i].value;
  }

  for (int i = 0; i < dst->num_invalid_tlvs_configured_in_running; ++i) {
    copy_tl(&src->invalid_tlvs_configured_in_running_list[i].tl,&dst->invalid_tlvs_configured_in_running_list[i].tl);
    dst->invalid_tlvs_configured_in_running_list[i].value = src->invalid_tlvs_configured_in_running_list[i].value;
  }

  for (int i = 0; i < dst->num_missing_tlvs; ++i) {
    copy_tl(&src->missing_tlvs_list[i].tl,&dst->missing_tlvs_list[i].tl);
    dst->missing_tlvs_list[i].value = src->missing_tlvs_list[i].value;
  }

}

void copy_start_request(const nfapi_nr_start_request_scf_t *src, nfapi_nr_start_request_scf_t *dst)
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

void copy_start_response(const nfapi_nr_start_response_scf_t *src, nfapi_nr_start_response_scf_t *dst)
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
}
