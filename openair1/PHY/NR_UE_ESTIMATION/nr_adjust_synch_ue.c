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

#include "PHY/types.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"
#include "PHY/impl_defs_top.h"

#include "executables/softmodem-common.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

//#define DEBUG_PHY

// Adjust location synchronization point to account for drift
// The adjustment is performed once per frame based on the
// last channel estimate of the receiver

int nr_adjust_synch_ue(NR_DL_FRAME_PARMS *frame_parms,
                       PHY_VARS_NR_UE *ue,
                       module_id_t gNB_id,
                       const int estimateSz,
                       struct complex16 dl_ch_estimates_time[][estimateSz],
                       uint8_t frame,
                       uint8_t subframe,
                       unsigned char clear,
                       short coef)
{
  int max_val = 0, max_pos = 0;
  const int sync_pos = 0;
  uint8_t sync_offset = 0;
  int sampleShift = 0;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_ADJUST_SYNCH, VCD_FUNCTION_IN);

  short ncoef = 32767 - coef;

  // search for maximum position within the cyclic prefix
  for (int i = -frame_parms->nb_prefix_samples/2; i < frame_parms->nb_prefix_samples/2; i++) {
    int temp = 0;

    int j = (i < 0) ? (i + frame_parms->ofdm_symbol_size) : i;
    for (int aa = 0; aa < frame_parms->nb_antennas_rx; aa++) {
      int Re = dl_ch_estimates_time[aa][j].r;
      int Im = dl_ch_estimates_time[aa][j].i;
      temp += (Re*Re/2) + (Im*Im/2);
    }

    if (temp > max_val) {
      max_pos = i;
      max_val = temp;
    }
  }

  // filter position to reduce jitter
  if (clear == 1)
    ue->max_pos_fil = max_pos << 15;
  else
    ue->max_pos_fil = ((ue->max_pos_fil * coef) >> 15) + (max_pos * ncoef);

  // do not filter to have proactive timing adjustment
  //ue->max_pos_fil = max_pos << 15;

  int diff = (ue->max_pos_fil >> 15) - sync_pos;

  if (frame_parms->freq_range==nr_FR2) 
    sync_offset = 2;
  else
    sync_offset = 0;

  if (abs(diff) > (NR_SYNCH_HYST + sync_offset))
    sampleShift = diff;

  const int sample_shift = -(sampleShift >> 1);
  // reset IIR filter for next offset calculation
  ue->max_pos_fil += sample_shift * 32768;

  LOG_D(PHY,
        "AbsSubframe %d: diff = %i, rx_offset (final) = %i : clear = %d, max_pos = %d, max_pos_fil = %ld, max_val = %d, sync_pos "
        "%d\n",
        subframe,
        diff,
        sampleShift,
        clear,
        max_pos,
        ue->max_pos_fil,
        max_val,
        sync_pos);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_ADJUST_SYNCH, VCD_FUNCTION_OUT);
  return sampleShift;
}
