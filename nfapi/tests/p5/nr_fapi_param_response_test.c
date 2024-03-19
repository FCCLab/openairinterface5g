#include "nr_fapi_test.h"
// #define CMP_TLV(_a,_b) do{ if(_a != _b){ return -1; } }while(0)
#define CMP_TLV(_a, _b)  \
  do {                   \
    DevAssert(_a == _b); \
  } while (0)

uint8_t compare_param_response_tlv(const nfapi_nr_param_response_scf_t *unpacked_req, const nfapi_nr_param_response_scf_t *req)
{
  CMP_TLV(unpacked_req->cell_param.release_capability.tl.tag, req->cell_param.release_capability.tl.tag);
  CMP_TLV(unpacked_req->cell_param.release_capability.value, req->cell_param.release_capability.value);

  CMP_TLV(unpacked_req->cell_param.phy_state.tl.tag, req->cell_param.phy_state.tl.tag);
  CMP_TLV(unpacked_req->cell_param.phy_state.value, req->cell_param.phy_state.value);

  CMP_TLV(unpacked_req->cell_param.skip_blank_dl_config.tl.tag, req->cell_param.skip_blank_dl_config.tl.tag);
  CMP_TLV(unpacked_req->cell_param.skip_blank_dl_config.value, req->cell_param.skip_blank_dl_config.value);

  CMP_TLV(unpacked_req->cell_param.skip_blank_ul_config.tl.tag, req->cell_param.skip_blank_ul_config.tl.tag);
  CMP_TLV(unpacked_req->cell_param.skip_blank_ul_config.value, req->cell_param.skip_blank_ul_config.value);

  CMP_TLV(unpacked_req->cell_param.num_config_tlvs_to_report.tl.tag, req->cell_param.num_config_tlvs_to_report.tl.tag);
  CMP_TLV(unpacked_req->cell_param.num_config_tlvs_to_report.value, req->cell_param.num_config_tlvs_to_report.value);

  CMP_TLV(unpacked_req->carrier_param.cyclic_prefix.tl.tag, req->carrier_param.cyclic_prefix.tl.tag);
  CMP_TLV(unpacked_req->carrier_param.cyclic_prefix.value, req->carrier_param.cyclic_prefix.value);

  CMP_TLV(unpacked_req->carrier_param.supported_subcarrier_spacings_dl.tl.tag,
          req->carrier_param.supported_subcarrier_spacings_dl.tl.tag);
  CMP_TLV(unpacked_req->carrier_param.supported_subcarrier_spacings_dl.value,
          req->carrier_param.supported_subcarrier_spacings_dl.value);

  CMP_TLV(unpacked_req->carrier_param.supported_bandwidth_dl.tl.tag, req->carrier_param.supported_bandwidth_dl.tl.tag);
  CMP_TLV(unpacked_req->carrier_param.supported_bandwidth_dl.value, req->carrier_param.supported_bandwidth_dl.value);

  CMP_TLV(unpacked_req->carrier_param.supported_subcarrier_spacings_ul.tl.tag,
          req->carrier_param.supported_subcarrier_spacings_ul.tl.tag);
  CMP_TLV(unpacked_req->carrier_param.supported_subcarrier_spacings_ul.value,
          req->carrier_param.supported_subcarrier_spacings_ul.value);

  CMP_TLV(unpacked_req->carrier_param.supported_bandwidth_ul.tl.tag, req->carrier_param.supported_bandwidth_ul.tl.tag);
  CMP_TLV(unpacked_req->carrier_param.supported_bandwidth_ul.value, req->carrier_param.supported_bandwidth_ul.value);

  CMP_TLV(unpacked_req->pdcch_param.cce_mapping_type.tl.tag, req->pdcch_param.cce_mapping_type.tl.tag);
  CMP_TLV(unpacked_req->pdcch_param.cce_mapping_type.value, req->pdcch_param.cce_mapping_type.value);

  CMP_TLV(unpacked_req->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.tl.tag,
          req->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.tl.tag);
  CMP_TLV(unpacked_req->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.value,
          req->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.value);

  CMP_TLV(unpacked_req->pdcch_param.coreset_precoder_granularity_coreset.tl.tag,
          req->pdcch_param.coreset_precoder_granularity_coreset.tl.tag);
  CMP_TLV(unpacked_req->pdcch_param.coreset_precoder_granularity_coreset.value,
          req->pdcch_param.coreset_precoder_granularity_coreset.value);

  CMP_TLV(unpacked_req->pdcch_param.pdcch_mu_mimo.tl.tag, req->pdcch_param.pdcch_mu_mimo.tl.tag);
  CMP_TLV(unpacked_req->pdcch_param.pdcch_mu_mimo.value, req->pdcch_param.pdcch_mu_mimo.value);

  CMP_TLV(unpacked_req->pdcch_param.pdcch_precoder_cycling.tl.tag, req->pdcch_param.pdcch_precoder_cycling.tl.tag);
  CMP_TLV(unpacked_req->pdcch_param.pdcch_precoder_cycling.value, req->pdcch_param.pdcch_precoder_cycling.value);

  CMP_TLV(unpacked_req->pdcch_param.max_pdcch_per_slot.tl.tag, req->pdcch_param.max_pdcch_per_slot.tl.tag);
  CMP_TLV(unpacked_req->pdcch_param.max_pdcch_per_slot.value, req->pdcch_param.max_pdcch_per_slot.value);

  CMP_TLV(unpacked_req->pucch_param.pucch_formats.tl.tag, req->pucch_param.pucch_formats.tl.tag);
  CMP_TLV(unpacked_req->pucch_param.pucch_formats.value, req->pucch_param.pucch_formats.value);

  CMP_TLV(unpacked_req->pucch_param.max_pucchs_per_slot.tl.tag, req->pucch_param.max_pucchs_per_slot.tl.tag);
  CMP_TLV(unpacked_req->pucch_param.max_pucchs_per_slot.value, req->pucch_param.max_pucchs_per_slot.value);

  CMP_TLV(unpacked_req->pdsch_param.pdsch_mapping_type.tl.tag, req->pdsch_param.pdsch_mapping_type.tl.tag);
  CMP_TLV(unpacked_req->pdsch_param.pdsch_mapping_type.value, req->pdsch_param.pdsch_mapping_type.value);

  CMP_TLV(unpacked_req->pdsch_param.pdsch_dmrs_additional_pos.tl.tag, req->pdsch_param.pdsch_dmrs_additional_pos.tl.tag);
  CMP_TLV(unpacked_req->pdsch_param.pdsch_dmrs_additional_pos.value, req->pdsch_param.pdsch_dmrs_additional_pos.value);

  CMP_TLV(unpacked_req->pdsch_param.pdsch_allocation_types.tl.tag, req->pdsch_param.pdsch_allocation_types.tl.tag);
  CMP_TLV(unpacked_req->pdsch_param.pdsch_allocation_types.value, req->pdsch_param.pdsch_allocation_types.value);

  CMP_TLV(unpacked_req->pdsch_param.pdsch_vrb_to_prb_mapping.tl.tag, req->pdsch_param.pdsch_vrb_to_prb_mapping.tl.tag);
  CMP_TLV(unpacked_req->pdsch_param.pdsch_vrb_to_prb_mapping.value, req->pdsch_param.pdsch_vrb_to_prb_mapping.value);

  CMP_TLV(unpacked_req->pdsch_param.pdsch_cbg.tl.tag, req->pdsch_param.pdsch_cbg.tl.tag);
  CMP_TLV(unpacked_req->pdsch_param.pdsch_cbg.value, req->pdsch_param.pdsch_cbg.value);

  CMP_TLV(unpacked_req->pdsch_param.pdsch_dmrs_config_types.tl.tag, req->pdsch_param.pdsch_dmrs_config_types.tl.tag);
  CMP_TLV(unpacked_req->pdsch_param.pdsch_dmrs_config_types.value, req->pdsch_param.pdsch_dmrs_config_types.value);

  CMP_TLV(unpacked_req->pdsch_param.max_number_mimo_layers_pdsch.tl.tag, req->pdsch_param.max_number_mimo_layers_pdsch.tl.tag);
  CMP_TLV(unpacked_req->pdsch_param.max_number_mimo_layers_pdsch.value, req->pdsch_param.max_number_mimo_layers_pdsch.value);

  CMP_TLV(unpacked_req->pdsch_param.max_mu_mimo_users_dl.tl.tag, req->pdsch_param.max_mu_mimo_users_dl.tl.tag);
  CMP_TLV(unpacked_req->pdsch_param.max_mu_mimo_users_dl.value, req->pdsch_param.max_mu_mimo_users_dl.value);

  CMP_TLV(unpacked_req->pdsch_param.pdsch_data_in_dmrs_symbols.tl.tag, req->pdsch_param.pdsch_data_in_dmrs_symbols.tl.tag);
  CMP_TLV(unpacked_req->pdsch_param.pdsch_data_in_dmrs_symbols.value, req->pdsch_param.pdsch_data_in_dmrs_symbols.value);

  CMP_TLV(unpacked_req->pdsch_param.premption_support.tl.tag, req->pdsch_param.premption_support.tl.tag);
  CMP_TLV(unpacked_req->pdsch_param.premption_support.value, req->pdsch_param.premption_support.value);

  CMP_TLV(unpacked_req->pdsch_param.pdsch_non_slot_support.tl.tag, req->pdsch_param.pdsch_non_slot_support.tl.tag);
  CMP_TLV(unpacked_req->pdsch_param.pdsch_non_slot_support.value, req->pdsch_param.pdsch_non_slot_support.value);

  CMP_TLV(unpacked_req->pusch_param.uci_mux_ulsch_in_pusch.tl.tag, req->pusch_param.uci_mux_ulsch_in_pusch.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.uci_mux_ulsch_in_pusch.value, req->pusch_param.uci_mux_ulsch_in_pusch.value);

  CMP_TLV(unpacked_req->pusch_param.uci_only_pusch.tl.tag, req->pusch_param.uci_only_pusch.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.uci_only_pusch.value, req->pusch_param.uci_only_pusch.value);

  CMP_TLV(unpacked_req->pusch_param.pusch_frequency_hopping.tl.tag, req->pusch_param.pusch_frequency_hopping.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.pusch_frequency_hopping.value, req->pusch_param.pusch_frequency_hopping.value);

  CMP_TLV(unpacked_req->pusch_param.pusch_dmrs_config_types.tl.tag, req->pusch_param.pusch_dmrs_config_types.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.pusch_dmrs_config_types.value, req->pusch_param.pusch_dmrs_config_types.value);

  CMP_TLV(unpacked_req->pusch_param.pusch_dmrs_max_len.tl.tag, req->pusch_param.pusch_dmrs_max_len.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.pusch_dmrs_max_len.value, req->pusch_param.pusch_dmrs_max_len.value);

  CMP_TLV(unpacked_req->pusch_param.pusch_dmrs_additional_pos.tl.tag, req->pusch_param.pusch_dmrs_additional_pos.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.pusch_dmrs_additional_pos.value, req->pusch_param.pusch_dmrs_additional_pos.value);

  CMP_TLV(unpacked_req->pusch_param.pusch_cbg.tl.tag, req->pusch_param.pusch_cbg.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.pusch_cbg.value, req->pusch_param.pusch_cbg.value);

  CMP_TLV(unpacked_req->pusch_param.pusch_mapping_type.tl.tag, req->pusch_param.pusch_mapping_type.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.pusch_mapping_type.value, req->pusch_param.pusch_mapping_type.value);

  CMP_TLV(unpacked_req->pusch_param.pusch_allocation_types.tl.tag, req->pusch_param.pusch_allocation_types.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.pusch_allocation_types.value, req->pusch_param.pusch_allocation_types.value);

  CMP_TLV(unpacked_req->pusch_param.pusch_vrb_to_prb_mapping.tl.tag, req->pusch_param.pusch_vrb_to_prb_mapping.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.pusch_vrb_to_prb_mapping.value, req->pusch_param.pusch_vrb_to_prb_mapping.value);

  CMP_TLV(unpacked_req->pusch_param.pusch_max_ptrs_ports.tl.tag, req->pusch_param.pusch_max_ptrs_ports.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.pusch_max_ptrs_ports.value, req->pusch_param.pusch_max_ptrs_ports.value);

  CMP_TLV(unpacked_req->pusch_param.max_pduschs_tbs_per_slot.tl.tag, req->pusch_param.max_pduschs_tbs_per_slot.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.max_pduschs_tbs_per_slot.value, req->pusch_param.max_pduschs_tbs_per_slot.value);

  CMP_TLV(unpacked_req->pusch_param.max_number_mimo_layers_non_cb_pusch.tl.tag,
          req->pusch_param.max_number_mimo_layers_non_cb_pusch.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.max_number_mimo_layers_non_cb_pusch.value,
          req->pusch_param.max_number_mimo_layers_non_cb_pusch.value);

  CMP_TLV(unpacked_req->pusch_param.supported_modulation_order_ul.tl.tag, req->pusch_param.supported_modulation_order_ul.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.supported_modulation_order_ul.value, req->pusch_param.supported_modulation_order_ul.value);

  CMP_TLV(unpacked_req->pusch_param.max_mu_mimo_users_ul.tl.tag, req->pusch_param.max_mu_mimo_users_ul.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.max_mu_mimo_users_ul.value, req->pusch_param.max_mu_mimo_users_ul.value);

  CMP_TLV(unpacked_req->pusch_param.dfts_ofdm_support.tl.tag, req->pusch_param.dfts_ofdm_support.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.dfts_ofdm_support.value, req->pusch_param.dfts_ofdm_support.value);

  CMP_TLV(unpacked_req->pusch_param.pusch_aggregation_factor.tl.tag, req->pusch_param.pusch_aggregation_factor.tl.tag);
  CMP_TLV(unpacked_req->pusch_param.pusch_aggregation_factor.value, req->pusch_param.pusch_aggregation_factor.value);

  CMP_TLV(unpacked_req->prach_param.prach_long_formats.tl.tag, req->prach_param.prach_long_formats.tl.tag);
  CMP_TLV(unpacked_req->prach_param.prach_long_formats.value, req->prach_param.prach_long_formats.value);

  CMP_TLV(unpacked_req->prach_param.prach_short_formats.tl.tag, req->prach_param.prach_short_formats.tl.tag);
  CMP_TLV(unpacked_req->prach_param.prach_short_formats.value, req->prach_param.prach_short_formats.value);

  CMP_TLV(unpacked_req->prach_param.prach_restricted_sets.tl.tag, req->prach_param.prach_restricted_sets.tl.tag);
  CMP_TLV(unpacked_req->prach_param.prach_restricted_sets.value, req->prach_param.prach_restricted_sets.value);

  CMP_TLV(unpacked_req->prach_param.max_prach_fd_occasions_in_a_slot.tl.tag,
          req->prach_param.max_prach_fd_occasions_in_a_slot.tl.tag);
  CMP_TLV(unpacked_req->prach_param.max_prach_fd_occasions_in_a_slot.value,
          req->prach_param.max_prach_fd_occasions_in_a_slot.value);

  CMP_TLV(unpacked_req->measurement_param.rssi_measurement_support.tl.tag, req->measurement_param.rssi_measurement_support.tl.tag);
  CMP_TLV(unpacked_req->measurement_param.rssi_measurement_support.value, req->measurement_param.rssi_measurement_support.value);

  return 0;
}

void fill_param_response_tlv(nfapi_nr_param_response_scf_t *nfapi_resp)
{
  nfapi_resp->cell_param.release_capability.tl.tag = NFAPI_NR_PARAM_TLV_RELEASE_CAPABILITY_TAG;
  nfapi_resp->cell_param.release_capability.value = rand16();
  nfapi_resp->num_tlv++;

  nfapi_resp->cell_param.phy_state.tl.tag = NFAPI_NR_PARAM_TLV_PHY_STATE_TAG;
  nfapi_resp->cell_param.phy_state.value = rand16();
  nfapi_resp->num_tlv++;

  nfapi_resp->cell_param.skip_blank_dl_config.tl.tag = NFAPI_NR_PARAM_TLV_SKIP_BLANK_DL_CONFIG_TAG;
  nfapi_resp->cell_param.skip_blank_dl_config.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->cell_param.skip_blank_ul_config.tl.tag = NFAPI_NR_PARAM_TLV_SKIP_BLANK_UL_CONFIG_TAG;
  nfapi_resp->cell_param.skip_blank_ul_config.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->cell_param.num_config_tlvs_to_report.tl.tag = NFAPI_NR_PARAM_TLV_NUM_CONFIG_TLVS_TO_REPORT_TAG;
  nfapi_resp->cell_param.num_config_tlvs_to_report.value = rand16();
  nfapi_resp->num_tlv++;

  nfapi_resp->carrier_param.cyclic_prefix.tl.tag = NFAPI_NR_PARAM_TLV_CYCLIC_PREFIX_TAG;
  nfapi_resp->carrier_param.cyclic_prefix.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->carrier_param.supported_subcarrier_spacings_dl.tl.tag = NFAPI_NR_PARAM_TLV_SUPPORTED_SUBCARRIER_SPACINGS_DL_TAG;
  nfapi_resp->carrier_param.supported_subcarrier_spacings_dl.value = rand16();
  nfapi_resp->num_tlv++;

  nfapi_resp->carrier_param.supported_bandwidth_dl.tl.tag = NFAPI_NR_PARAM_TLV_SUPPORTED_BANDWIDTH_DL_TAG;
  nfapi_resp->carrier_param.supported_bandwidth_dl.value = rand16();
  nfapi_resp->num_tlv++;

  nfapi_resp->carrier_param.supported_subcarrier_spacings_ul.tl.tag = NFAPI_NR_PARAM_TLV_SUPPORTED_SUBCARRIER_SPACINGS_UL_TAG;
  nfapi_resp->carrier_param.supported_subcarrier_spacings_ul.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->carrier_param.supported_bandwidth_ul.tl.tag = NFAPI_NR_PARAM_TLV_SUPPORTED_BANDWIDTH_UL_TAG;
  nfapi_resp->carrier_param.supported_bandwidth_ul.value = rand16();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdcch_param.cce_mapping_type.tl.tag = NFAPI_NR_PARAM_TLV_CCE_MAPPING_TYPE_TAG;
  nfapi_resp->pdcch_param.cce_mapping_type.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.tl.tag =
      NFAPI_NR_PARAM_TLV_CORESET_OUTSIDE_FIRST_3_OFDM_SYMS_OF_SLOT_TAG;
  nfapi_resp->pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdcch_param.coreset_precoder_granularity_coreset.tl.tag = NFAPI_NR_PARAM_TLV_PRECODER_GRANULARITY_CORESET_TAG;
  nfapi_resp->pdcch_param.coreset_precoder_granularity_coreset.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdcch_param.pdcch_mu_mimo.tl.tag = NFAPI_NR_PARAM_TLV_PDCCH_MU_MIMO_TAG;
  nfapi_resp->pdcch_param.pdcch_mu_mimo.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdcch_param.pdcch_precoder_cycling.tl.tag = NFAPI_NR_PARAM_TLV_PDCCH_PRECODER_CYCLING_TAG;
  nfapi_resp->pdcch_param.pdcch_precoder_cycling.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdcch_param.max_pdcch_per_slot.tl.tag = NFAPI_NR_PARAM_TLV_MAX_PDCCHS_PER_SLOT_TAG;
  nfapi_resp->pdcch_param.max_pdcch_per_slot.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pucch_param.pucch_formats.tl.tag = NFAPI_NR_PARAM_TLV_PUCCH_FORMATS_TAG;
  nfapi_resp->pucch_param.pucch_formats.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pucch_param.max_pucchs_per_slot.tl.tag = NFAPI_NR_PARAM_TLV_MAX_PUCCHS_PER_SLOT_TAG;
  nfapi_resp->pucch_param.max_pucchs_per_slot.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdsch_param.pdsch_mapping_type.tl.tag = NFAPI_NR_PARAM_TLV_PDSCH_MAPPING_TYPE_TAG;
  nfapi_resp->pdsch_param.pdsch_mapping_type.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdsch_param.pdsch_dmrs_additional_pos.tl.tag = NFAPI_NR_PARAM_TLV_PDSCH_DMRS_ADDITIONAL_POS_TAG;
  nfapi_resp->pdsch_param.pdsch_dmrs_additional_pos.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdsch_param.pdsch_allocation_types.tl.tag = NFAPI_NR_PARAM_TLV_PDSCH_ALLOCATION_TYPES_TAG;
  nfapi_resp->pdsch_param.pdsch_allocation_types.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdsch_param.pdsch_vrb_to_prb_mapping.tl.tag = NFAPI_NR_PARAM_TLV_PDSCH_VRB_TO_PRB_MAPPING_TAG;
  nfapi_resp->pdsch_param.pdsch_vrb_to_prb_mapping.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdsch_param.pdsch_cbg.tl.tag = NFAPI_NR_PARAM_TLV_PDSCH_CBG_TAG;
  nfapi_resp->pdsch_param.pdsch_cbg.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdsch_param.pdsch_dmrs_config_types.tl.tag = NFAPI_NR_PARAM_TLV_PDSCH_DMRS_CONFIG_TYPES_TAG;
  nfapi_resp->pdsch_param.pdsch_dmrs_config_types.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdsch_param.max_number_mimo_layers_pdsch.tl.tag = NFAPI_NR_PARAM_TLV_MAX_NUMBER_MIMO_LAYERS_PDSCH_TAG;
  nfapi_resp->pdsch_param.max_number_mimo_layers_pdsch.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdsch_param.max_mu_mimo_users_dl.tl.tag = NFAPI_NR_PARAM_TLV_MAX_MU_MIMO_USERS_DL_TAG;
  nfapi_resp->pdsch_param.max_mu_mimo_users_dl.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdsch_param.pdsch_data_in_dmrs_symbols.tl.tag = NFAPI_NR_PARAM_TLV_PDSCH_DATA_IN_DMRS_SYMBOLS_TAG;
  nfapi_resp->pdsch_param.pdsch_data_in_dmrs_symbols.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdsch_param.premption_support.tl.tag = NFAPI_NR_PARAM_TLV_PREMPTION_SUPPORT_TAG;
  nfapi_resp->pdsch_param.premption_support.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pdsch_param.pdsch_non_slot_support.tl.tag = NFAPI_NR_PARAM_TLV_PDSCH_NON_SLOT_SUPPORT_TAG;
  nfapi_resp->pdsch_param.pdsch_non_slot_support.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.uci_mux_ulsch_in_pusch.tl.tag = NFAPI_NR_PARAM_TLV_UCI_MUX_ULSCH_IN_PUSCH_TAG;
  nfapi_resp->pusch_param.uci_mux_ulsch_in_pusch.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.uci_only_pusch.tl.tag = NFAPI_NR_PARAM_TLV_UCI_ONLY_PUSCH_TAG;
  nfapi_resp->pusch_param.uci_only_pusch.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.pusch_frequency_hopping.tl.tag = NFAPI_NR_PARAM_TLV_PUSCH_FREQUENCY_HOPPING_TAG;
  nfapi_resp->pusch_param.pusch_frequency_hopping.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.pusch_dmrs_config_types.tl.tag = NFAPI_NR_PARAM_TLV_PUSCH_DMRS_CONFIG_TYPES_TAG;
  nfapi_resp->pusch_param.pusch_dmrs_config_types.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.pusch_dmrs_max_len.tl.tag = NFAPI_NR_PARAM_TLV_PUSCH_DMRS_MAX_LEN_TAG;
  nfapi_resp->pusch_param.pusch_dmrs_max_len.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.pusch_dmrs_additional_pos.tl.tag = NFAPI_NR_PARAM_TLV_PUSCH_DMRS_ADDITIONAL_POS_TAG;
  nfapi_resp->pusch_param.pusch_dmrs_additional_pos.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.pusch_cbg.tl.tag = NFAPI_NR_PARAM_TLV_PUSCH_CBG_TAG;
  nfapi_resp->pusch_param.pusch_cbg.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.pusch_mapping_type.tl.tag = NFAPI_NR_PARAM_TLV_PUSCH_MAPPING_TYPE_TAG;
  nfapi_resp->pusch_param.pusch_mapping_type.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.pusch_allocation_types.tl.tag = NFAPI_NR_PARAM_TLV_PUSCH_ALLOCATION_TYPES_TAG;
  nfapi_resp->pusch_param.pusch_allocation_types.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.pusch_vrb_to_prb_mapping.tl.tag = NFAPI_NR_PARAM_TLV_PUSCH_VRB_TO_PRB_MAPPING_TAG;
  nfapi_resp->pusch_param.pusch_vrb_to_prb_mapping.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.pusch_max_ptrs_ports.tl.tag = NFAPI_NR_PARAM_TLV_PUSCH_MAX_PTRS_PORTS_TAG;
  nfapi_resp->pusch_param.pusch_max_ptrs_ports.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.max_pduschs_tbs_per_slot.tl.tag = NFAPI_NR_PARAM_TLV_MAX_PDUSCHS_TBS_PER_SLOT_TAG;
  nfapi_resp->pusch_param.max_pduschs_tbs_per_slot.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.max_number_mimo_layers_non_cb_pusch.tl.tag = NFAPI_NR_PARAM_TLV_MAX_NUMBER_MIMO_LAYERS_NON_CB_PUSCH_TAG;
  nfapi_resp->pusch_param.max_number_mimo_layers_non_cb_pusch.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.supported_modulation_order_ul.tl.tag = NFAPI_NR_PARAM_TLV_SUPPORTED_MODULATION_ORDER_UL_TAG;
  nfapi_resp->pusch_param.supported_modulation_order_ul.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.max_mu_mimo_users_ul.tl.tag = NFAPI_NR_PARAM_TLV_MAX_MU_MIMO_USERS_UL_TAG;
  nfapi_resp->pusch_param.max_mu_mimo_users_ul.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.dfts_ofdm_support.tl.tag = NFAPI_NR_PARAM_TLV_DFTS_OFDM_SUPPORT_TAG;
  nfapi_resp->pusch_param.dfts_ofdm_support.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->pusch_param.pusch_aggregation_factor.tl.tag = NFAPI_NR_PARAM_TLV_PUSCH_AGGREGATION_FACTOR_TAG;
  nfapi_resp->pusch_param.pusch_aggregation_factor.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->prach_param.prach_long_formats.tl.tag = NFAPI_NR_PARAM_TLV_PRACH_LONG_FORMATS_TAG;
  nfapi_resp->prach_param.prach_long_formats.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->prach_param.prach_short_formats.tl.tag = NFAPI_NR_PARAM_TLV_PRACH_SHORT_FORMATS_TAG;
  nfapi_resp->prach_param.prach_short_formats.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->prach_param.prach_restricted_sets.tl.tag = NFAPI_NR_PARAM_TLV_PRACH_RESTRICTED_SETS_TAG;
  nfapi_resp->prach_param.prach_restricted_sets.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->prach_param.max_prach_fd_occasions_in_a_slot.tl.tag = NFAPI_NR_PARAM_TLV_MAX_PRACH_FD_OCCASIONS_IN_A_SLOT_TAG;
  nfapi_resp->prach_param.max_prach_fd_occasions_in_a_slot.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->measurement_param.rssi_measurement_support.tl.tag = NFAPI_NR_PARAM_TLV_RSSI_MEASUREMENT_SUPPORT_TAG;
  nfapi_resp->measurement_param.rssi_measurement_support.value = rand8();
  nfapi_resp->num_tlv++;
}

int main(int n, char *v[])
{
  srand(time(NULL));
  logInit();
  set_glog(OAILOG_DISABLE);
  nfapi_nr_param_response_scf_t req;
  memset(&req, 0, sizeof(req));
  req.header.message_id = NFAPI_NR_PHY_MSG_TYPE_PARAM_RESPONSE;
  // FAPI doesn't have these 2 following parameters, dont use
  req.header.spare = 0;
  req.header.phy_id = 0;
  uint8_t msg_buf[8192];
  uint16_t msg_len = sizeof(req);
  // Fill Param response TVLs
  fill_param_response_tlv(&req);
  // first test the packing procedure
  int pack_result = fapi_nr_p5_message_pack(&req, msg_len, msg_buf, sizeof(msg_buf), NULL);
  // PARAM.response message body length is AT LEAST 10 (NFAPI_HEADER_LENGTH + 1 byte error_code + 1 byte num_tlv)
  DevAssert(pack_result >= NFAPI_HEADER_LENGTH + 1 + 1);
  // update req message_length value with value calculated in message_pack procedure
  req.header.message_length = pack_result - NFAPI_HEADER_LENGTH;
  // test the unpacking of the header
  // copy first NFAPI_HEADER_LENGTH bytes into a new buffer, to simulate SCTP PEEK
  nfapi_p4_p5_message_header_t header;
  uint32_t header_buffer_size = NFAPI_HEADER_LENGTH;
  uint8_t header_buffer[header_buffer_size];
  for (int idx = 0; idx < header_buffer_size; idx++) {
    header_buffer[idx] = msg_buf[idx];
  }
  uint8_t *pReadPackedMessage = header_buffer;
  int unpack_header_result = fapi_nr_p5_message_header_unpack(&pReadPackedMessage, NFAPI_HEADER_LENGTH, &header, sizeof(header), 0);
  DevAssert(unpack_header_result >= 0);
  DevAssert(header.message_id == req.header.message_id);
  DevAssert(header.message_length == req.header.message_length);
  // test the unpaking and compare with initial message
  nfapi_nr_param_response_scf_t unpacked_req;
  memset(&unpacked_req, 0, sizeof(unpacked_req));
  int unpack_result =
      fapi_nr_p5_message_unpack(msg_buf, header.message_length + NFAPI_HEADER_LENGTH, &unpacked_req, sizeof(unpacked_req), NULL);
  DevAssert(unpack_result >= 0);
  DevAssert(unpacked_req.header.message_id == req.header.message_id);
  DevAssert(unpacked_req.header.message_length == req.header.message_length);
  DevAssert(unpacked_req.num_tlv == req.num_tlv);
  DevAssert(unpacked_req.error_code == req.error_code);
  DevAssert(compare_param_response_tlv(&unpacked_req, &req) == 0);

  // All tests successful!
  return 0;
}
