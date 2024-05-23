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

#include "PHY/defs_gNB.h"

#ifndef __NR_ULSCH_DECODING_INTERFACE__H__
#define __NR_ULSCH_DECODING_INTERFACE__H__

typedef int32_t(nr_ulsch_decoding_init_t)(void);
typedef int32_t(nr_ulsch_decoding_shutdown_t)(void);
/**
 * \brief slot decoding function interface
 * \param gNB PHY layers variables of gNB
 * \param frame_parms informations on the frame
 * \param frame_rx RX frame index
 * \param slot_rx RX slot index
 * \param G arrays of G
 */
typedef int32_t(nr_ulsch_decoding_decoder_t)(PHY_VARS_gNB *gNB, NR_DL_FRAME_PARMS *frame_parms, int frame_rx, int slot_rx, uint32_t *G);

typedef int32_t(nr_ulsch_decoding_encoder_t)
  (PHY_VARS_gNB *gNB,
   processingData_L1tx_t *msgTx,
   int frame_tx,
   uint8_t slot_tx,
   NR_DL_FRAME_PARMS* frame_parms,
   unsigned char ** output,
   time_stats_t *tinput,
   time_stats_t *tprep,
   time_stats_t *tparity,
   time_stats_t *toutput,
   time_stats_t *dlsch_rate_matching_stats,
   time_stats_t *dlsch_interleaving_stats,
   time_stats_t *dlsch_segmentation_stats);

typedef struct nr_ulsch_decoding_interface_s {
  nr_ulsch_decoding_init_t *nr_ulsch_decoding_init;
  nr_ulsch_decoding_shutdown_t *nr_ulsch_decoding_shutdown;
  nr_ulsch_decoding_decoder_t *nr_ulsch_decoding_decoder;
  nr_ulsch_decoding_encoder_t *nr_ulsch_decoding_encoder;
} nr_ulsch_decoding_interface_t;

int load_nr_ulsch_decoding_interface(char *version, nr_ulsch_decoding_interface_t *interface);
int free_nr_ulsch_decoding_interface(nr_ulsch_decoding_interface_t *interface);

//TODO replace the global structure below
// Global var to limit the rework of the dirty legacy code
extern nr_ulsch_decoding_interface_t nr_ulsch_decoding_interface;

#endif
