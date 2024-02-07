/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file PHY/NR_TRANSPORT/nr_ulsch_decoding.c
* \brief Top-level routines for decoding  LDPC (ULSCH) transport channels from 38.212, V15.4.0 2018-12
* \author Ahmed Hussein
* \date 2019
* \version 0.1
* \company Fraunhofer IIS
* \email: ahmed.hussein@iis.fraunhofer.de
* \note
* \warning
*/


// [from gNB coding]
#include "PHY/defs_gNB.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "PHY/CODING/nrLDPC_extern.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "SCHED_NR/sched_nr.h"
#include "SCHED_NR/fapi_nr_l1.h"
#include "defs.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/LOG/log.h"
#include <syscall.h>
//#define DEBUG_ULSCH_DECODING
//#define gNB_DEBUG_TRACE

#define OAI_UL_LDPC_MAX_NUM_LLR 27000//26112 // NR_LDPC_NCOL_BG1*NR_LDPC_ZMAX = 68*384
//#define DEBUG_CRC
#ifdef DEBUG_CRC
#define PRINT_CRC_CHECK(a) a
#else
#define PRINT_CRC_CHECK(a)
#endif

//extern double cpuf;

#include "PHY/defs_gNB.h"
//#include "sched_nr.h"
//#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
//#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
//#include "PHY/CODING/nr_ulsch_decoding_interface.h"
#include "PHY/CODING/nrLDPC_extern.h"
//#include "PHY/NR_TRANSPORT/nr_dci.h"
//#include "PHY/NR_ESTIMATION/nr_ul_estimation.h"
#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_interface.h"
#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_nr_interface.h"
//#include "fapi_nr_l1.h"
//#include "common/utils/LOG/log.h"
//#include "common/utils/LOG/vcd_signal_dumper.h"
//#include "PHY/INIT/nr_phy_init.h"
//#include "PHY/MODULATION/nr_modulation.h"
//#include "PHY/NR_UE_TRANSPORT/srs_modulation_nr.h"
//#include "T.h"
//#include "executables/nr-softmodem.h"
//#include "executables/softmodem-common.h"
//#include "nfapi/oai_integration/vendor_ext.h"
//#include "NR_SRS-ResourceSet.h"
//#include "assertions.h"
//#include <time.h>

// Global var to limit the rework of the dirty legacy code
ldpc_interface_t ldpc_interface_demo, ldpc_interface_offload_demo;

static void nr_processULSegment_demo(void *arg)
{
  ldpcDecode_t *rdata = (ldpcDecode_t *)arg;
  NR_UL_gNB_HARQ_t *ulsch_harq = rdata->ulsch_harq;
  t_nrLDPC_dec_params *p_decoderParms = &rdata->decoderParms;
  const int Kr = ulsch_harq->K;
  const int Kr_bytes = Kr >> 3;
  const int K_bits_F = Kr - ulsch_harq->F;
  const int r = rdata->segment_r;
  const int A = rdata->A;
  const int E = rdata->E;
  const int Qm = rdata->Qm;
  const int rv_index = rdata->rv_index;
  const int r_offset = rdata->r_offset;
  const uint8_t kc = rdata->Kc;
  short *ulsch_llr = rdata->ulsch_llr;
  const int max_ldpc_iterations = p_decoderParms->numMaxIter;
  int8_t llrProcBuf[OAI_UL_LDPC_MAX_NUM_LLR] __attribute__((aligned(32)));

  t_nrLDPC_time_stats procTime = {0};
  t_nrLDPC_time_stats *p_procTime = &procTime;

  ////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////// nr_deinterleaving_ldpc ///////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////

  //////////////////////////// ulsch_llr =====> ulsch_harq->e //////////////////////////////

  /// code blocks after bit selection in rate matching for LDPC code (38.212 V15.4.0 section 5.4.2.1)
  int16_t harq_e[E];

  nr_deinterleaving_ldpc(E, Qm, harq_e, ulsch_llr + r_offset);

  // for (int i =0; i<16; i++)
  //          printf("rx output deinterleaving w[%d]= %d r_offset %d\n", i,ulsch_harq->w[r][i], r_offset);


  //////////////////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////// nr_rate_matching_ldpc_rx ////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////

  ///////////////////////// ulsch_harq->e =====> ulsch_harq->d /////////////////////////


  if (nr_rate_matching_ldpc_rx(rdata->tbslbrm,
                               p_decoderParms->BG,
                               p_decoderParms->Z,
                               ulsch_harq->d[r],
                               harq_e,
                               ulsch_harq->C,
                               rv_index,
                               ulsch_harq->d_to_be_cleared[r],
                               E,
                               ulsch_harq->F,
                               Kr - ulsch_harq->F - 2 * (p_decoderParms->Z))
      == -1) {

    LOG_E(PHY, "ulsch_decoding.c: Problem in rate_matching\n");
    rdata->decodeIterations = max_ldpc_iterations + 1;
    return;
  }

  ulsch_harq->d_to_be_cleared[r] = false;

  memset(ulsch_harq->c[r], 0, Kr_bytes);
  p_decoderParms->crc_type = crcType(ulsch_harq->C, A);
  p_decoderParms->E = lenWithCrc(ulsch_harq->C, A);
  // start_meas(&phy_vars_gNB->ulsch_ldpc_decoding_stats);

  // set first 2*Z_c bits to zeros

  int16_t z[68 * 384 + 16] __attribute__((aligned(16)));

  memset(z, 0, 2 * ulsch_harq->Z * sizeof(*z));
  // set Filler bits
  memset(z + K_bits_F, 127, ulsch_harq->F * sizeof(*z));
  // Move coded bits before filler bits
  memcpy(z + 2 * ulsch_harq->Z, ulsch_harq->d[r], (K_bits_F - 2 * ulsch_harq->Z) * sizeof(*z));
  // skip filler bits
  memcpy(z + Kr, ulsch_harq->d[r] + (Kr - 2 * ulsch_harq->Z), (kc * ulsch_harq->Z - Kr) * sizeof(*z));
  // Saturate coded bits before decoding into 8 bits values
  simde__m128i *pv = (simde__m128i *)&z;
  int8_t l[68 * 384 + 16] __attribute__((aligned(16)));
  simde__m128i *pl = (simde__m128i *)&l;
  for (int i = 0, j = 0; j < ((kc * ulsch_harq->Z) >> 4) + 1; i += 2, j++) {
    pl[j] = simde_mm_packs_epi16(pv[i], pv[i + 1]);
  }
  //////////////////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////// nrLDPC_decoder /////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////// pl =====> llrProcBuf //////////////////////////////////
  rdata->decodeIterations =
      ldpc_interface_demo.LDPCdecoder(p_decoderParms, 0, 0, 0, l, llrProcBuf, p_procTime, &ulsch_harq->abort_decode);

  if (rdata->decodeIterations <= p_decoderParms->numMaxIter)
    memcpy(ulsch_harq->c[r],llrProcBuf,  Kr>>3);
}

int decode_offload_demo(PHY_VARS_gNB *phy_vars_gNB,
                   uint8_t ULSCH_id,
                   short *ulsch_llr,
                   nfapi_nr_pusch_pdu_t *pusch_pdu,
                   t_nrLDPC_dec_params *decParams,
                   uint8_t harq_pid,
                   uint32_t G)
{
  NR_gNB_ULSCH_t *ulsch = &phy_vars_gNB->ulsch[ULSCH_id];
  NR_UL_gNB_HARQ_t *harq_process = ulsch->harq_process;
  int16_t z_ol[LDPC_MAX_CB_SIZE] __attribute__((aligned(16)));
  int8_t l_ol[LDPC_MAX_CB_SIZE] __attribute__((aligned(16)));
  uint8_t Qm = pusch_pdu->qam_mod_order;
  uint8_t n_layers = pusch_pdu->nrOfLayers;
  const int Kr = harq_process->K;
  const int Kr_bytes = Kr >> 3;
  uint32_t A = (harq_process->TBS) << 3;
  const int kc = decParams->BG == 2 ? 52 : 68;
  ulsch->max_ldpc_iterations = 20;
  int decodeIterations = 2;
  int r_offset = 0, offset = 0;
  for (int r = 0; r < harq_process->C; r++) {
    int E = nr_get_E(G, harq_process->C, Qm, n_layers, r);
    memset(harq_process->c[r], 0, Kr_bytes);
    decParams->R = nr_get_R_ldpc_decoder(pusch_pdu->pusch_data.rv_index,
                                         E,
                                         decParams->BG,
                                         decParams->Z,
                                         &harq_process->llrLen,
                                         harq_process->round);

    memcpy(z_ol, ulsch_llr + r_offset, E * sizeof(short));
    simde__m128i *pv_ol128 = (simde__m128i *)&z_ol;
    simde__m128i *pl_ol128 = (simde__m128i *)&l_ol;
    for (int i = 0, j = 0; j < ((kc * harq_process->Z) >> 4) + 1; i += 2, j++) {
      pl_ol128[j] = simde_mm_packs_epi16(pv_ol128[i], pv_ol128[i + 1]);
    }
    decParams->E = E;
    decParams->rv = pusch_pdu->pusch_data.rv_index;
    decParams->F = harq_process->F;
    decParams->Qm = Qm;
    decodeIterations =
        ldpc_interface_offload_demo
            .LDPCdecoder(decParams, harq_pid, ULSCH_id, r, (int8_t *)&pl_ol128[0], (int8_t *)harq_process->c[r], NULL, NULL);
    if (decodeIterations < 0) {
      LOG_E(PHY, "ulsch_decoding.c: Problem in LDPC decoder offload\n");
      return -1;
    }
    bool decodeSuccess = check_crc((uint8_t *)harq_process->c[r], lenWithCrc(harq_process->C, A), crcType(harq_process->C, A));
    if (decodeSuccess) {
      memcpy(harq_process->b + offset, harq_process->c[r], Kr_bytes - (harq_process->F >> 3) - ((harq_process->C > 1) ? 3 : 0));
      offset += (Kr_bytes - (harq_process->F >> 3) - ((harq_process->C > 1) ? 3 : 0));
      harq_process->processedSegments++;
    } else {
      LOG_D(PHY, "uplink segment error %d/%d\n", r, harq_process->C);
      LOG_D(PHY, "ULSCH %d in error\n", ULSCH_id);
    }
    r_offset += E;
  }
  bool crc_valid = false;
  if (harq_process->processedSegments == harq_process->C) {
    // When the number of code blocks is 1 (C = 1) and ulsch_harq->processedSegments = 1, we can assume a good TB because of the
    // CRC check made by the LDPC for early termination, so, no need to perform CRC check twice for a single code block
    crc_valid = true;
    if (harq_process->C > 1) {
      crc_valid = check_crc(harq_process->b, lenWithCrc(1, A), crcType(1, A));
    }
  }
  if (crc_valid) {
    LOG_D(PHY, "ULSCH: Setting ACK for slot %d TBS %d\n", ulsch->slot, harq_process->TBS);
    ulsch->active = false;
    harq_process->round = 0;
    LOG_D(PHY, "ULSCH received ok \n");
    nr_fill_indication(phy_vars_gNB, ulsch->frame, ulsch->slot, ULSCH_id, harq_pid, 0, 0);
  } else {
    LOG_D(PHY,
        "[gNB %d] ULSCH: Setting NAK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d)\n",
        phy_vars_gNB->Mod_id,
        ulsch->frame,
        ulsch->slot,
        harq_pid,
        ulsch->active,
        harq_process->round,
        harq_process->TBS);
    ulsch->handled = 1;
    decodeIterations = ulsch->max_ldpc_iterations + 1;
    LOG_D(PHY, "ULSCH %d in error\n", ULSCH_id);
    nr_fill_indication(phy_vars_gNB, ulsch->frame, ulsch->slot, ULSCH_id, harq_pid, 1, 0);
  }

  ulsch->last_iteration_cnt = decodeIterations;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_gNB_ULSCH_DECODING,0);
  return 0;
}

int nr_ulsch_decoding_demo(PHY_VARS_gNB *phy_vars_gNB,
                      uint8_t ULSCH_id,
                      short *ulsch_llr,
                      NR_DL_FRAME_PARMS *frame_parms,
                      nfapi_nr_pusch_pdu_t *pusch_pdu,
                      uint32_t frame,
                      uint8_t nr_tti_rx,
                      uint8_t harq_pid,
                      uint32_t G)
{
  if (!ulsch_llr) {
    LOG_E(PHY, "ulsch_decoding.c: NULL ulsch_llr pointer\n");
    return -1;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_gNB_ULSCH_DECODING, 1);

  NR_gNB_ULSCH_t *ulsch = &phy_vars_gNB->ulsch[ULSCH_id];
  NR_gNB_PUSCH *pusch = &phy_vars_gNB->pusch_vars[ULSCH_id];
  NR_UL_gNB_HARQ_t *harq_process = ulsch->harq_process;

  if (!harq_process) {
    LOG_E(PHY, "ulsch_decoding.c: NULL harq_process pointer\n");
    return -1;
  }

  // ------------------------------------------------------------------
  const uint16_t nb_rb = pusch_pdu->rb_size;
  const uint8_t Qm = pusch_pdu->qam_mod_order;
  const uint8_t mcs = pusch_pdu->mcs_index;
  const uint8_t n_layers = pusch_pdu->nrOfLayers;
  // ------------------------------------------------------------------

  harq_process->processedSegments = 0;
  harq_process->TBS = pusch_pdu->pusch_data.tb_size;

  t_nrLDPC_dec_params decParams = {.check_crc = check_crc};
  decParams.BG = pusch_pdu->maintenance_parms_v3.ldpcBaseGraph;
  const uint32_t A = (harq_process->TBS) << 3;
  NR_gNB_PHY_STATS_t *stats = get_phy_stats(phy_vars_gNB, ulsch->rnti);
  if (stats) {
    stats->frame = frame;
    stats->ulsch_stats.round_trials[harq_process->round]++;
    for (int aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
      stats->ulsch_stats.power[aarx] = dB_fixed_x10(pusch->ulsch_power[aarx]);
      stats->ulsch_stats.noise_power[aarx] = dB_fixed_x10(pusch->ulsch_noise_power[aarx]);
    }
    if (!harq_process->harq_to_be_cleared) {
      stats->ulsch_stats.current_Qm = Qm;
      stats->ulsch_stats.current_RI = n_layers;
      stats->ulsch_stats.total_bytes_tx += harq_process->TBS;
    }
  }

  LOG_D(PHY,
        "ULSCH Decoding, harq_pid %d rnti %x TBS %d G %d mcs %d Nl %d nb_rb %d, Qm %d, Coderate %f RV %d round %d new RX %d\n",
        harq_pid,
        ulsch->rnti,
        A,
        G,
        mcs,
        n_layers,
        nb_rb,
        Qm,
        pusch_pdu->target_code_rate / 10240.0f,
        pusch_pdu->pusch_data.rv_index,
        harq_process->round,
        harq_process->harq_to_be_cleared);

  // [hna] Perform nr_segmenation with input and output set to NULL to calculate only (C, K, Z, F)
  nr_segmentation(NULL,
                  NULL,
                  lenWithCrc(1, A), // size in case of 1 segment
                  &harq_process->C,
                  &harq_process->K,
                  &harq_process->Z, // [hna] Z is Zc
                  &harq_process->F,
                  decParams.BG);

  uint16_t a_segments = MAX_NUM_NR_ULSCH_SEGMENTS_PER_LAYER * n_layers; // number of segments to be allocated
  if (harq_process->C > a_segments) {
    LOG_E(PHY, "nr_segmentation.c: too many segments %d, A %d\n", harq_process->C, A);
    return(-1);
  }
  if (nb_rb != 273) {
    a_segments = a_segments*nb_rb;
    a_segments = a_segments/273 +1;
  }
  if (harq_process->C > a_segments) {
    LOG_E(PHY,"Illegal harq_process->C %d > %d\n",harq_process->C,a_segments);
    return -1;
  }

#ifdef DEBUG_ULSCH_DECODING
  printf("ulsch decoding nr segmentation Z %d\n", harq_process->Z);
  if (!frame % 100)
    printf("K %d C %d Z %d \n", harq_process->K, harq_process->C, harq_process->Z);
  printf("Segmentation: C %d, K %d\n",harq_process->C,harq_process->K);
#endif

  decParams.Z = harq_process->Z;
  decParams.numMaxIter = ulsch->max_ldpc_iterations;
  decParams.outMode = 0;
  decParams.setCombIn = !harq_process->harq_to_be_cleared;
  if (harq_process->harq_to_be_cleared) {
    for (int r = 0; r < harq_process->C; r++)
      harq_process->d_to_be_cleared[r] = true;
    harq_process->harq_to_be_cleared = false;
  }

  if (phy_vars_gNB->ldpc_offload_flag)
    return decode_offload_demo(phy_vars_gNB, ULSCH_id, ulsch_llr, pusch_pdu, &decParams, harq_pid, G);

  uint32_t offset = 0, r_offset = 0;
  set_abort(&harq_process->abort_decode, false);
  for (int r = 0; r < harq_process->C; r++) {
    int E = nr_get_E(G, harq_process->C, Qm, n_layers, r);
    union ldpcReqUnion id = {.s = {ulsch->rnti, frame, nr_tti_rx, 0, 0}};
    notifiedFIFO_elt_t *req = newNotifiedFIFO_elt(sizeof(ldpcDecode_t), id.p, &phy_vars_gNB->respDecode, &nr_processULSegment_demo);
    ldpcDecode_t *rdata = (ldpcDecode_t *)NotifiedFifoData(req);
    decParams.R = nr_get_R_ldpc_decoder(pusch_pdu->pusch_data.rv_index,
                                        E,
                                        decParams.BG,
                                        decParams.Z,
                                        &harq_process->llrLen,
                                        harq_process->round);
    rdata->gNB = phy_vars_gNB;
    rdata->ulsch_harq = harq_process;
    rdata->decoderParms = decParams;
    rdata->ulsch_llr = ulsch_llr;
    rdata->Kc = decParams.BG == 2 ? 52 : 68;
    rdata->harq_pid = harq_pid;
    rdata->segment_r = r;
    rdata->nbSegments = harq_process->C;
    rdata->E = E;
    rdata->A = A;
    rdata->Qm = Qm;
    rdata->r_offset = r_offset;
    rdata->Kr_bytes = harq_process->K >> 3;
    rdata->rv_index = pusch_pdu->pusch_data.rv_index;
    rdata->offset = offset;
    rdata->ulsch = ulsch;
    rdata->ulsch_id = ULSCH_id;
    rdata->tbslbrm = pusch_pdu->maintenance_parms_v3.tbSizeLbrmBytes;
    pushTpool(&phy_vars_gNB->threadPool, req);
    LOG_D(PHY, "Added a block to decode, in pipe: %d\n", r);
    r_offset += E;
    offset += ((harq_process->K >> 3) - (harq_process->F >> 3) - ((harq_process->C > 1) ? 3 : 0));
  }
  return harq_process->C;
}

int32_t nr_ulsch_procedures_init(void){

  load_LDPClib(NULL, &ldpc_interface_demo);

  load_LDPClib("_t2", &ldpc_interface_offload_demo);

  return 0;

}

int32_t nr_ulsch_procedures_shutdown(void){

  free_LDPClib(&ldpc_interface_demo);

  free_LDPClib(&ldpc_interface_offload_demo);

  return 0;

}

int32_t nr_ulsch_procedures_decoder(PHY_VARS_gNB *gNB, NR_DL_FRAME_PARMS *frame_parms, int frame_rx, int slot_rx, uint32_t *G){

  int nbDecode = 0;
  for (int ULSCH_id = 0; ULSCH_id < gNB->max_nb_pusch; ULSCH_id++) {
    NR_gNB_ULSCH_t *ulsch = &gNB->ulsch[ULSCH_id];
    uint8_t harq_pid = ulsch->harq_pid; 
    nfapi_nr_pusch_pdu_t *pusch_pdu = &gNB->ulsch[ULSCH_id].harq_process->ulsch_pdu;
    nbDecode += nr_ulsch_decoding_demo(gNB, ULSCH_id, gNB->pusch_vars[ULSCH_id].llr, frame_parms, pusch_pdu, frame_rx, slot_rx, harq_pid, G[ULSCH_id]);
  }

  return nbDecode;

}

