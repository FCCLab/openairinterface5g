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
#include "executables/nr-uesoftmodem.h"

//#define DEBUG_PHY

// Adjust location synchronization point to account for drift
// The adjustment is performed once per frame based on the
// last channel estimate of the receiver

void nr_adjust_synch_ue(NR_DL_FRAME_PARMS *frame_parms,
                        PHY_VARS_NR_UE *ue,
                        module_id_t gNB_id,
                        const int estimateSz,
                        struct complex16 dl_ch_estimates_time[][estimateSz],
                        uint8_t frame,
                        uint8_t subframe,
                        unsigned char clear,
                        short coef)
{

  static int count_max_pos_ok = 0;
  static int first_time = 1;
  int max_val = 0, max_pos = 0;
  const int sync_pos = 0;
  //uint8_t sync_offset = 0;
  static int flagInitIScaling = 0;

  static int64_t TO_I_Ctrl = 0; //Integral controller for TO
  static int frameLast = 0; //frame number of last call of nr_adjust_synch_ue()
  static int FirstFlag = 1; //indicate the first call of nr_adjust_synch_ue()

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_ADJUST_SYNCH, VCD_FUNCTION_IN);

  short ncoef = 32767 - coef;

  LOG_D(PHY,"AbsSubframe %d: rx_offset (before) = %d\n",subframe,ue->rx_offset);

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
  int FrameDiff = (frame-frameLast+1024)%1024;

  if (FirstFlag)
    FirstFlag = 0;
  else
    diff /= FrameDiff; //scaled by the frame number

  frameLast = frame; //save the last frame number
  
  if ( (((diff < -4) || (diff > 4)) && (flagInitIScaling == 0)))
  {
	  TO_I_Ctrl = TO_IScalingInit/TO_IScaling;
	  flagInitIScaling = 1;
  }

  ue->TO_I_Ctrl += diff; //integral of all offsets
  ue->rx_offset = diff;
  ue->rx_offset_TO = (TO_PScaling*diff) + (ue->TO_I_Ctrl*TO_IScaling); //PI controller
  ue->rx_offset_slot = 1;
  ue->rx_offset_comp = 0;

  LOG_D(PHY, "Frame: %u, ue->rx_offset: %d, ue->rx_offset_TO: %d\n", frame, ue->rx_offset, ue->rx_offset_TO);

  if (tdriftComp == 0)
  {
    ue->rx_offset = 0;
    ue->rx_offset_TO = 0; //PI controller
    ue->rx_offset_slot = 1;
    ue->rx_offset_comp = 0;
  }

  LOG_D(PHY, "Frame: %u, diff: %d, rx_offset_TO: %d, PScaling: %f, IScaling: %f, TA: %d, TO_I_Ctrl: %d \n", frame, ue->rx_offset, ue->rx_offset_TO, TO_PScaling, TO_IScaling, ue->timing_advance,ue->TO_I_Ctrl);

  if(abs(diff)<5)
    count_max_pos_ok ++;
  else
    count_max_pos_ok = 0;
      
  //printf("adjust sync count_max_pos_ok = %d\n",count_max_pos_ok);

  if(count_max_pos_ok > 10 && first_time == 1) {
    first_time = 0;
    ue->time_sync_cell = 1;
  }

#ifdef DEBUG_PHY
  LOG_I(PHY,"AbsSubframe %d: diff = %i, rx_offset (final) = %i : clear = %d, max_pos = %d, max_pos_fil = %d, max_val = %d, sync_pos %d\n",
        subframe,
        diff,
        ue->rx_offset,
        clear,
        max_pos,
        ue->max_pos_fil,
        max_val,
        sync_pos);
#endif //DEBUG_PHY

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_ADJUST_SYNCH, VCD_FUNCTION_OUT);
}
