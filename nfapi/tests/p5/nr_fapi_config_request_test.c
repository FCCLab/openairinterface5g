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
/*! \file nfapi/tests/p5/nr_fapi_config_request_test.c
 * \brief
 * \author Ruben S. Silva
 * \date 2024
 * \version 0.1
 * \company OpenAirInterface Software Alliance
 * \email: contact@openairinterface.org, rsilva@allbesmart.pt
 * \note
 * \warning
 */
#include "nfapi/tests/nr_fapi_test.h"
#include "nr_fapi_p5_utils.h"

void fill_config_request_tlv(nfapi_nr_config_request_scf_t *nfapi_resp)
{
  nfapi_resp->carrier_config.dl_bandwidth.tl.tag = NFAPI_NR_CONFIG_DL_BANDWIDTH_TAG;
  nfapi_resp->carrier_config.dl_bandwidth.value = rand16();
  nfapi_resp->num_tlv++;

  nfapi_resp->carrier_config.dl_frequency.tl.tag = NFAPI_NR_CONFIG_DL_FREQUENCY_TAG;
  nfapi_resp->carrier_config.dl_frequency.value = rand32();
  nfapi_resp->num_tlv++;

  for (int i = 0; i < 5; ++i) {
    nfapi_resp->carrier_config.dl_k0[i].tl.tag = NFAPI_NR_CONFIG_DL_K0_TAG;
    nfapi_resp->carrier_config.dl_k0[i].value = rand16();
  }
  // these 5 are 1 tlv
  nfapi_resp->num_tlv++;
  for (int i = 0; i < 5; ++i) {
    nfapi_resp->carrier_config.dl_grid_size[i].tl.tag = NFAPI_NR_CONFIG_DL_GRID_SIZE_TAG;
    nfapi_resp->carrier_config.dl_grid_size[i].value = rand16();
  }
  // these 5 are 1 tlv
  nfapi_resp->num_tlv++;

  nfapi_resp->carrier_config.num_tx_ant.tl.tag = NFAPI_NR_CONFIG_NUM_TX_ANT_TAG;
  nfapi_resp->carrier_config.num_tx_ant.value = rand16();
  nfapi_resp->num_tlv++;

  nfapi_resp->carrier_config.uplink_bandwidth.tl.tag = NFAPI_NR_CONFIG_UPLINK_BANDWIDTH_TAG;
  nfapi_resp->carrier_config.uplink_bandwidth.value = rand16();
  nfapi_resp->num_tlv++;

  nfapi_resp->carrier_config.uplink_frequency.tl.tag = NFAPI_NR_CONFIG_UPLINK_FREQUENCY_TAG;
  nfapi_resp->carrier_config.uplink_frequency.value = rand16();
  nfapi_resp->num_tlv++;

  for (int i = 0; i < 5; ++i) {
    nfapi_resp->carrier_config.ul_k0[i].tl.tag = NFAPI_NR_CONFIG_UL_K0_TAG;
    nfapi_resp->carrier_config.ul_k0[i].value = rand16();
  }
  // these 5 are 1 tlv
  nfapi_resp->num_tlv++;
  for (int i = 0; i < 5; ++i) {
    nfapi_resp->carrier_config.ul_grid_size[i].tl.tag = NFAPI_NR_CONFIG_UL_GRID_SIZE_TAG;
    nfapi_resp->carrier_config.ul_grid_size[i].value = rand16();
  }
  // these 5 are 1 tlv
  nfapi_resp->num_tlv++;

  nfapi_resp->carrier_config.num_rx_ant.tl.tag = NFAPI_NR_CONFIG_NUM_RX_ANT_TAG;
  nfapi_resp->carrier_config.num_rx_ant.value = rand16();
  nfapi_resp->num_tlv++;

  nfapi_resp->carrier_config.frequency_shift_7p5khz.tl.tag = NFAPI_NR_CONFIG_FREQUENCY_SHIFT_7P5KHZ_TAG;
  nfapi_resp->carrier_config.frequency_shift_7p5khz.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->cell_config.phy_cell_id.tl.tag = NFAPI_NR_CONFIG_PHY_CELL_ID_TAG;
  nfapi_resp->cell_config.phy_cell_id.value = rand16();
  nfapi_resp->num_tlv++;

  nfapi_resp->cell_config.frame_duplex_type.tl.tag = NFAPI_NR_CONFIG_FRAME_DUPLEX_TYPE_TAG;
  nfapi_resp->cell_config.frame_duplex_type.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->ssb_config.ss_pbch_power.tl.tag = NFAPI_NR_CONFIG_SS_PBCH_POWER_TAG;
  nfapi_resp->ssb_config.ss_pbch_power.value = (int32_t)rand32();
  nfapi_resp->num_tlv++;

  nfapi_resp->ssb_config.bch_payload.tl.tag = NFAPI_NR_CONFIG_BCH_PAYLOAD_TAG;
  nfapi_resp->ssb_config.bch_payload.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->ssb_config.scs_common.tl.tag = NFAPI_NR_CONFIG_SCS_COMMON_TAG;
  nfapi_resp->ssb_config.scs_common.value =
      rand8_range(0, 4); // NOTE: MUST be between 0 & 4 ( inclusive ) , since this value is used as index to obtain slots per frame
  nfapi_resp->num_tlv++;

  nfapi_resp->prach_config.prach_sequence_length.tl.tag = NFAPI_NR_CONFIG_PRACH_SEQUENCE_LENGTH_TAG;
  nfapi_resp->prach_config.prach_sequence_length.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->prach_config.prach_sub_c_spacing.tl.tag = NFAPI_NR_CONFIG_PRACH_SUB_C_SPACING_TAG;
  nfapi_resp->prach_config.prach_sub_c_spacing.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->prach_config.restricted_set_config.tl.tag = NFAPI_NR_CONFIG_RESTRICTED_SET_CONFIG_TAG;
  nfapi_resp->prach_config.restricted_set_config.value = rand8();
  nfapi_resp->num_tlv++;
  /*Number of RACH frequency domain
    occasions. Corresponds to the
    parameter 𝑀𝑀 in [38.211, sec 6.3.3.2]
    which equals the higher layer
    parameter msg1FDM
    Value: 1,2,4,8*/
  const uint8_t num_prach_fd_ocasions[] = {1, 2, 4, 8};
  nfapi_resp->prach_config.num_prach_fd_occasions.tl.tag = NFAPI_NR_CONFIG_NUM_PRACH_FD_OCCASIONS_TAG;
  nfapi_resp->prach_config.num_prach_fd_occasions.value = num_prach_fd_ocasions[rand8_range(0, sizeof(num_prach_fd_ocasions) - 1)];
  nfapi_resp->num_tlv++;

  nfapi_resp->prach_config.prach_ConfigurationIndex.tl.tag = NFAPI_NR_CONFIG_PRACH_CONFIG_INDEX_TAG;
  nfapi_resp->prach_config.prach_ConfigurationIndex.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->prach_config.num_prach_fd_occasions_list = (nfapi_nr_num_prach_fd_occasions_t *)malloc(
      nfapi_resp->prach_config.num_prach_fd_occasions.value * sizeof(nfapi_nr_num_prach_fd_occasions_t));
  for (int i = 0; i < nfapi_resp->prach_config.num_prach_fd_occasions.value; i++) {
    nfapi_nr_num_prach_fd_occasions_t *prach_fd_occasion = &(nfapi_resp->prach_config.num_prach_fd_occasions_list[i]);

    prach_fd_occasion->prach_root_sequence_index.tl.tag = NFAPI_NR_CONFIG_PRACH_ROOT_SEQUENCE_INDEX_TAG;
    prach_fd_occasion->prach_root_sequence_index.value = rand16();
    nfapi_resp->num_tlv++;

    prach_fd_occasion->num_root_sequences.tl.tag = NFAPI_NR_CONFIG_NUM_ROOT_SEQUENCES_TAG;
    prach_fd_occasion->num_root_sequences.value = rand8();
    nfapi_resp->num_tlv++;

    prach_fd_occasion->k1.tl.tag = NFAPI_NR_CONFIG_K1_TAG;
    prach_fd_occasion->k1.value = rand8();
    nfapi_resp->num_tlv++;

    prach_fd_occasion->prach_zero_corr_conf.tl.tag = NFAPI_NR_CONFIG_PRACH_ZERO_CORR_CONF_TAG;
    prach_fd_occasion->prach_zero_corr_conf.value = rand8();
    nfapi_resp->num_tlv++;

    prach_fd_occasion->num_unused_root_sequences.tl.tag = NFAPI_NR_CONFIG_NUM_UNUSED_ROOT_SEQUENCES_TAG;
    // This doesn't make sense to be more than the total num_root_sequences
    /* SCF 222.10.02 : Number of unused sequences available
       for noise estimation per FD occasion. At
       least one unused root sequence is
       required per FD occasion.  */
    if(prach_fd_occasion->num_root_sequences.value != 0){
      prach_fd_occasion->num_unused_root_sequences.value = rand8_range(1, prach_fd_occasion->num_root_sequences.value);
    }else{
      prach_fd_occasion->num_unused_root_sequences.value = 1;
    }
    nfapi_resp->num_tlv++;

    prach_fd_occasion->unused_root_sequences_list =
        (nfapi_uint8_tlv_t *)malloc(prach_fd_occasion->num_unused_root_sequences.value * sizeof(nfapi_uint8_tlv_t));
    for (int k = 0; k < prach_fd_occasion->num_unused_root_sequences.value; k++) {
      prach_fd_occasion->unused_root_sequences_list[k].tl.tag = NFAPI_NR_CONFIG_UNUSED_ROOT_SEQUENCES_TAG;
      prach_fd_occasion->unused_root_sequences_list[k].value = rand16();
    }
  }

  nfapi_resp->prach_config.ssb_per_rach.tl.tag = NFAPI_NR_CONFIG_SSB_PER_RACH_TAG;
  nfapi_resp->prach_config.ssb_per_rach.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->prach_config.prach_multiple_carriers_in_a_band.tl.tag = NFAPI_NR_CONFIG_PRACH_MULTIPLE_CARRIERS_IN_A_BAND_TAG;
  nfapi_resp->prach_config.prach_multiple_carriers_in_a_band.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->ssb_table.ssb_offset_point_a.tl.tag = NFAPI_NR_CONFIG_SSB_OFFSET_POINT_A_TAG;
  nfapi_resp->ssb_table.ssb_offset_point_a.value = rand16();
  nfapi_resp->num_tlv++;

  nfapi_resp->ssb_table.ssb_period.tl.tag = NFAPI_NR_CONFIG_SSB_PERIOD_TAG;
  nfapi_resp->ssb_table.ssb_period.value = rand16();
  nfapi_resp->num_tlv++;

  nfapi_resp->ssb_table.ssb_subcarrier_offset.tl.tag = NFAPI_NR_CONFIG_SSB_SUBCARRIER_OFFSET_TAG;
  nfapi_resp->ssb_table.ssb_subcarrier_offset.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->ssb_table.MIB.tl.tag = NFAPI_NR_CONFIG_MIB_TAG;
  nfapi_resp->ssb_table.MIB.value = rand32();
  nfapi_resp->num_tlv++;

  nfapi_resp->ssb_table.ssb_mask_list[0].ssb_mask.tl.tag = NFAPI_NR_CONFIG_SSB_MASK_TAG;
  nfapi_resp->ssb_table.ssb_mask_list[0].ssb_mask.value = rand32();
  nfapi_resp->num_tlv++;

  nfapi_resp->ssb_table.ssb_mask_list[1].ssb_mask.tl.tag = NFAPI_NR_CONFIG_SSB_MASK_TAG;
  nfapi_resp->ssb_table.ssb_mask_list[1].ssb_mask.value = rand32();
  nfapi_resp->num_tlv++;

  for (int i = 0; i < 64; i++) {
    nfapi_resp->ssb_table.ssb_beam_id_list[i].beam_id.tl.tag = NFAPI_NR_CONFIG_BEAM_ID_TAG;
    nfapi_resp->ssb_table.ssb_beam_id_list[i].beam_id.value = rand8();
    nfapi_resp->num_tlv++;
  }

  nfapi_resp->tdd_table.tdd_period.tl.tag = NFAPI_NR_CONFIG_TDD_PERIOD_TAG;
  nfapi_resp->tdd_table.tdd_period.value = rand8();
  nfapi_resp->num_tlv++;
  const uint8_t slotsperframe[5] = {10, 20, 40, 80, 160};
  // Assuming always CP_Normal, because Cyclic prefix is not included in CONFIG.request 10.02, but is present in 10.04
  uint8_t cyclicprefix = 1;
  bool normal_CP = cyclicprefix ? false : true;
  // 3GPP 38.211 Table 4.3.2.1 & Table 4.3.2.2
  uint8_t number_of_symbols_per_slot = normal_CP ? 14 : 12;

  nfapi_resp->tdd_table.max_tdd_periodicity_list = (nfapi_nr_max_tdd_periodicity_t *)malloc(
      slotsperframe[nfapi_resp->ssb_config.scs_common.value] * sizeof(nfapi_nr_max_tdd_periodicity_t));

  for (int i = 0; i < slotsperframe[nfapi_resp->ssb_config.scs_common.value]; i++) {
    nfapi_resp->tdd_table.max_tdd_periodicity_list[i].max_num_of_symbol_per_slot_list =
        (nfapi_nr_max_num_of_symbol_per_slot_t *)malloc(number_of_symbols_per_slot * sizeof(nfapi_nr_max_num_of_symbol_per_slot_t));
  }

  for (int i = 0; i < slotsperframe[nfapi_resp->ssb_config.scs_common.value]; i++) { // TODO check right number of slots
    for (int k = 0; k < number_of_symbols_per_slot; k++) { // TODO can change?
      nfapi_resp->tdd_table.max_tdd_periodicity_list[i].max_num_of_symbol_per_slot_list[k].slot_config.tl.tag =
          NFAPI_NR_CONFIG_SLOT_CONFIG_TAG;
      nfapi_resp->tdd_table.max_tdd_periodicity_list[i].max_num_of_symbol_per_slot_list[k].slot_config.value = rand8();
      nfapi_resp->num_tlv++;
    }
  }

  nfapi_resp->measurement_config.rssi_measurement.tl.tag = NFAPI_NR_CONFIG_RSSI_MEASUREMENT_TAG;
  nfapi_resp->measurement_config.rssi_measurement.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->nfapi_config.p7_vnf_address_ipv4.tl.tag = NFAPI_NR_NFAPI_P7_VNF_ADDRESS_IPV4_TAG;
  for (int i = 0; i < NFAPI_IPV4_ADDRESS_LENGTH; ++i) {
    nfapi_resp->nfapi_config.p7_vnf_address_ipv4.address[i] = rand8();
  }
  nfapi_resp->num_tlv++;

  nfapi_resp->nfapi_config.p7_vnf_address_ipv6.tl.tag = NFAPI_NR_NFAPI_P7_VNF_ADDRESS_IPV6_TAG;
  for (int i = 0; i < NFAPI_IPV6_ADDRESS_LENGTH; ++i) {
    nfapi_resp->nfapi_config.p7_vnf_address_ipv6.address[i] = rand8();
  }
  nfapi_resp->num_tlv++;

  nfapi_resp->nfapi_config.p7_vnf_port.tl.tag = NFAPI_NR_NFAPI_P7_VNF_PORT_TAG;
  nfapi_resp->nfapi_config.p7_vnf_port.value = rand16();
  nfapi_resp->num_tlv++;

  nfapi_resp->nfapi_config.timing_window.tl.tag = NFAPI_NR_NFAPI_TIMING_WINDOW_TAG;
  nfapi_resp->nfapi_config.timing_window.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->nfapi_config.timing_info_mode.tl.tag = NFAPI_NR_NFAPI_TIMING_INFO_MODE_TAG;
  nfapi_resp->nfapi_config.timing_info_mode.value = rand8();
  nfapi_resp->num_tlv++;

  nfapi_resp->nfapi_config.timing_info_period.tl.tag = NFAPI_NR_NFAPI_TIMING_INFO_PERIOD_TAG;
  nfapi_resp->nfapi_config.timing_info_period.value = rand8();
  nfapi_resp->num_tlv++;
}

void test_pack_unpack(nfapi_nr_config_request_scf_t *req){
  uint8_t msg_buf[65535];
  uint16_t msg_len = sizeof(*req);

  // first test the packing procedure
  int pack_result = fapi_nr_p5_message_pack(req, msg_len, msg_buf, sizeof(msg_buf), NULL);
  // PARAM.request message body length is 0
  DevAssert(pack_result >= 0 + NFAPI_HEADER_LENGTH);
  // update req message_length value with value calculated in message_pack procedure
  req->header.message_length = pack_result - NFAPI_HEADER_LENGTH;
  // test the unpacking of the header
  // copy first NFAPI_HEADER_LENGTH bytes into a new buffer, to simulate SCTP PEEK
  fapi_message_header_t header;
  uint32_t header_buffer_size = NFAPI_HEADER_LENGTH;
  uint8_t header_buffer[header_buffer_size];
  for (int idx = 0; idx < header_buffer_size; idx++) {
    header_buffer[idx] = msg_buf[idx];
  }
  uint8_t *pReadPackedMessage = header_buffer;
  int unpack_header_result = fapi_nr_p5_message_header_unpack(&pReadPackedMessage, NFAPI_HEADER_LENGTH, &header, sizeof(header), 0);
  DevAssert(unpack_header_result >= 0);
  DevAssert(header.message_id == req->header.message_id);
  DevAssert(header.message_length == req->header.message_length);
  // test the unpacking and compare with initial message
  nfapi_nr_config_request_scf_t unpacked_req = {0};
  int unpack_result =
      fapi_nr_p5_message_unpack(msg_buf, header.message_length + NFAPI_HEADER_LENGTH, &unpacked_req, sizeof(unpacked_req), NULL);
  DevAssert(unpack_result >= 0);
  DevAssert(compare_config_request(&unpacked_req, req) == 0);
  free_config_request(&unpacked_req);
}

void test_copy(const nfapi_nr_config_request_scf_t *msg)
{
  //Test copy function
  nfapi_nr_config_request_scf_t copy = {0};
  copy_config_request(msg,&copy);
  DevAssert(compare_config_request(msg, &copy) == 0);
  free_config_request(&copy);
}

int main(int n, char *v[])
{
  fapi_test_init();
  nfapi_nr_config_request_scf_t req = {.header.message_id = NFAPI_NR_PHY_MSG_TYPE_CONFIG_REQUEST};
  // Fill CONFIG.request TVLs
  fill_config_request_tlv(&req);
  // Perform tests
  test_pack_unpack(&req);
  test_copy(&req);
  // All tests successful!
  free_config_request(&req);
  return 0;
}
