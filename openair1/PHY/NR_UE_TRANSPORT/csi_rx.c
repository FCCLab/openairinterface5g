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

/***********************************************************************
*
* FILENAME    :  csi_rx.c
*
* MODULE      :
*
* DESCRIPTION :  function to receive the channel state information
*
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nr_transport_proto_ue.h"
#include "PHY/phy_extern_nr_ue.h"
#include "common/utils/nr/nr_common.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_UE_ESTIMATION/filt16a_32.h"


//#define NR_CSIRS_DEBUG

bool is_csi_rs_in_symbol(fapi_nr_dl_config_csirs_pdu_rel15_t csirs_config_pdu, int symbol) {

  bool ret = false;

  // 38.211-Table 7.4.1.5.3-1: CSI-RS locations within a slot
  switch(csirs_config_pdu.row){
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
    case 9:
      if(symbol == csirs_config_pdu.symb_l0) {
        ret = true;
      }
      break;
    case 5:
    case 7:
    case 8:
    case 10:
    case 11:
    case 12:
      if(symbol == csirs_config_pdu.symb_l0 || symbol == (csirs_config_pdu.symb_l0+1) ) {
        ret = true;
      }
      break;
    case 13:
    case 14:
    case 16:
    case 17:
      if(symbol == csirs_config_pdu.symb_l0 || symbol == (csirs_config_pdu.symb_l0+1) ||
          symbol == csirs_config_pdu.symb_l1 || symbol == (csirs_config_pdu.symb_l1+1)) {
        ret = true;
      }
      break;
    case 15:
    case 18:
      if(symbol == csirs_config_pdu.symb_l0 || symbol == (csirs_config_pdu.symb_l0+1) || symbol == (csirs_config_pdu.symb_l0+2) ) {
        ret = true;
      }
      break;
    default:
      AssertFatal(0==1, "Row %d is not valid for CSI Table 7.4.1.5.3-1\n", csirs_config_pdu.row);
  }

  return ret;
}

int nr_get_csi_rs_signal(PHY_VARS_NR_UE *ue,
                         UE_nr_rxtx_proc_t *proc,
                         fapi_nr_dl_config_csirs_pdu_rel15_t *csirs_config_pdu,
                         nr_csi_rs_info_t *nr_csi_rs_info,
                         int32_t **csi_rs_received_signal) {

  int32_t **rxdataF  =  ue->common_vars.common_vars_rx_data_per_thread[proc->thread_id].rxdataF;
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;

  for (int ant_rx = 0; ant_rx < frame_parms->nb_antennas_rx; ant_rx++) {
    memset(csi_rs_received_signal[ant_rx], 0, frame_parms->samples_per_frame_wCP*sizeof(int32_t));

    for (int rb = csirs_config_pdu->start_rb; rb < (csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs); rb++) {

      // for freq density 0.5 checks if even or odd RB
      if(csirs_config_pdu->freq_density <= 1 && csirs_config_pdu->freq_density != (rb % 2)) {
        continue;
      }

      for (int cdm_id = 0; cdm_id < nr_csi_rs_info->N_cdm_groups; cdm_id++) {
        for (int s = 0; s < nr_csi_rs_info->CDM_group_size; s++)  {

          // loop over frequency resource elements within a group
          for (int kp = 0; kp <= nr_csi_rs_info->kprime; kp++) {

            uint16_t k = (frame_parms->first_carrier_offset + (rb*NR_NB_SC_PER_RB)+nr_csi_rs_info->koverline[cdm_id] + kp) % frame_parms->ofdm_symbol_size;

            // loop over time resource elements within a group
            for (int lp = 0; lp <= nr_csi_rs_info->lprime; lp++) {
              uint16_t symb = lp + nr_csi_rs_info->loverline[cdm_id];
              uint64_t symbol_offset = symb*frame_parms->ofdm_symbol_size;
              int16_t *rx_signal = (int16_t*)&rxdataF[ant_rx][symbol_offset];
              int16_t *rx_csi_rs_signal = (int16_t*)&csi_rs_received_signal[ant_rx][symbol_offset];
              rx_csi_rs_signal[k<<1] = rx_signal[k<<1];
              rx_csi_rs_signal[(k<<1)+1] = rx_signal[(k<<1)+1];

#ifdef NR_CSIRS_DEBUG
              int dataF_offset = proc->nr_slot_rx*ue->frame_parms.samples_per_slot_wCP;
              uint16_t port_tx = s+nr_csi_rs_info->j[cdm_id]*nr_csi_rs_info->CDM_group_size;
              int16_t *tx_csi_rs_signal = (int16_t*)&nr_csi_rs_info->csi_rs_generated_signal[port_tx][symbol_offset+dataF_offset];
              LOG_I(NR_PHY, "l,k (%2d,%4d) |\tport_tx %d (%4d,%4d)\tant_rx %d (%4d,%4d)\n",
                    symb,
                    k,
                    port_tx+3000,
                    tx_csi_rs_signal[k<<1],
                    tx_csi_rs_signal[(k<<1)+1],
                    ant_rx,
                    rx_csi_rs_signal[k<<1],
                    rx_csi_rs_signal[(k<<1)+1]);
#endif
            }
          }
        }
      }
    }
  }

  return 0;
}

int get_wf(int kp, int s) {
  int wf = 0;
  if (kp == 0) {
    wf = 1;
  } else {
    wf = -2*(s%2)+1;
  }
  return wf;
}

int get_wt(int lp, int s) {
  int wt = 0;
  if (s < 2) {
    wt = 1;
  } else if (s < 4) {
    wt = -2*(lp%2)+1;
  } else if (s < 6) {
    wt = -2*(lp/2)+1;
  } else {
    if ((lp == 0) || (lp == 3)) {
      wt = 1;
    } else {
      wt = -1;
    }
  }
  return wt;
}

uint32_t calc_power_csirs(uint16_t *x, fapi_nr_dl_config_csirs_pdu_rel15_t *csirs_config_pdu) {
  uint64_t sum_x = 0;
  uint64_t sum_x2 = 0;
  uint16_t size = 0;
  for (int rb = csirs_config_pdu->start_rb; rb < (csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs); rb++) {
    if (csirs_config_pdu->freq_density <= 1 && csirs_config_pdu->freq_density != (rb % 2)) {
      continue;
    }
    sum_x = sum_x + x[rb-csirs_config_pdu->start_rb];
    sum_x2 = sum_x2 + x[rb]*x[rb-csirs_config_pdu->start_rb];
    size++;
  }
  return sum_x2/size - (sum_x/size)*(sum_x/size);
}

int nr_csi_rs_channel_estimation(PHY_VARS_NR_UE *ue,
                                 UE_nr_rxtx_proc_t *proc,
                                 fapi_nr_dl_config_csirs_pdu_rel15_t *csirs_config_pdu,
                                 nr_csi_rs_info_t *nr_csi_rs_info,
                                 int32_t **csi_rs_generated_signal,
                                 int32_t **csi_rs_received_signal,
                                 int32_t ***csi_rs_estimated_channel_freq,
                                 uint32_t *noise_power) {

  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  int dataF_offset = proc->nr_slot_rx*ue->frame_parms.samples_per_slot_wCP;
  *noise_power = 0;
  int maxh = 0;

  for (int ant_rx = 0; ant_rx < frame_parms->nb_antennas_rx; ant_rx++) {

    /// LS channel estimation

    for(uint16_t port_tx = 0; port_tx<nr_csi_rs_info->N_ports; port_tx++) {
      memset(nr_csi_rs_info->csi_rs_ls_estimated_channel[ant_rx][port_tx], 0, frame_parms->ofdm_symbol_size*sizeof(int32_t));
    }

    for (int rb = csirs_config_pdu->start_rb; rb < (csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs); rb++) {

      // for freq density 0.5 checks if even or odd RB
      if(csirs_config_pdu->freq_density <= 1 && csirs_config_pdu->freq_density != (rb % 2)) {
        continue;
      }

      for (int cdm_id = 0; cdm_id < nr_csi_rs_info->N_cdm_groups; cdm_id++) {
        for (int s = 0; s < nr_csi_rs_info->CDM_group_size; s++)  {

          uint16_t port_tx = s+nr_csi_rs_info->j[cdm_id]*nr_csi_rs_info->CDM_group_size;

          // loop over frequency resource elements within a group
          for (int kp = 0; kp <= nr_csi_rs_info->kprime; kp++) {

            uint16_t kinit = (frame_parms->first_carrier_offset + rb*NR_NB_SC_PER_RB) % frame_parms->ofdm_symbol_size;
            uint16_t k = kinit + nr_csi_rs_info->koverline[cdm_id] + kp;

            // loop over time resource elements within a group
            for (int lp = 0; lp <= nr_csi_rs_info->lprime; lp++) {
              uint16_t symb = lp + nr_csi_rs_info->loverline[cdm_id];
              uint64_t symbol_offset = symb*frame_parms->ofdm_symbol_size;
              int16_t *tx_csi_rs_signal = (int16_t*)&csi_rs_generated_signal[port_tx][symbol_offset+dataF_offset];
              int16_t *rx_csi_rs_signal = (int16_t*)&csi_rs_received_signal[ant_rx][symbol_offset];
              int16_t *csi_rs_ls_estimated_channel = (int16_t*)&nr_csi_rs_info->csi_rs_ls_estimated_channel[ant_rx][port_tx][0];

              int16_t csi_rs_ls_estimated_channel_re = (int16_t)(((int32_t)tx_csi_rs_signal[k<<1]*rx_csi_rs_signal[k<<1] + (int32_t)tx_csi_rs_signal[(k<<1)+1]*rx_csi_rs_signal[(k<<1)+1])>>nr_csi_rs_info->csi_rs_generated_signal_bits);
              int16_t csi_rs_ls_estimated_channel_im = (int16_t)(((int32_t)tx_csi_rs_signal[k<<1]*rx_csi_rs_signal[(k<<1)+1] - (int32_t)tx_csi_rs_signal[(k<<1)+1]*rx_csi_rs_signal[k<<1])>>nr_csi_rs_info->csi_rs_generated_signal_bits);

              // This is not just the LS estimation for each (k,l), but also the sum of the different contributions
              // for the sake of optimizing the memory used.
              int w = get_wf(kp,s)*get_wt(lp,s);
              csi_rs_ls_estimated_channel[kinit<<1] += w*csi_rs_ls_estimated_channel_re;
              csi_rs_ls_estimated_channel[(kinit<<1)+1] += w*csi_rs_ls_estimated_channel_im;
            }
          }
        }
      }
    }

#ifdef NR_CSIRS_DEBUG
    for(int symb = 0; symb < NR_SYMBOLS_PER_SLOT; symb++) {
      if(!is_csi_rs_in_symbol(*csirs_config_pdu,symb)) {
        continue;
      }
      for(int k = 0; k<frame_parms->ofdm_symbol_size; k++) {
        LOG_I(NR_PHY, "l,k (%2d,%4d) | ", symb, k);
        for(uint16_t port_tx = 0; port_tx<nr_csi_rs_info->N_ports; port_tx++) {
          uint64_t symbol_offset = symb*frame_parms->ofdm_symbol_size;
          int16_t *tx_csi_rs_signal = (int16_t*)&csi_rs_generated_signal[port_tx][symbol_offset+dataF_offset];
          int16_t *rx_csi_rs_signal = (int16_t*)&csi_rs_received_signal[ant_rx][symbol_offset];
          int16_t *csi_rs_ls_estimated_channel = (int16_t*)&nr_csi_rs_info->csi_rs_ls_estimated_channel[ant_rx][port_tx][0];
          printf("port_tx %d --> ant_rx %d, tx (%4d,%4d), rx (%4d,%4d), ls (%4d,%4d) | ",
                 port_tx+3000, ant_rx,
                 tx_csi_rs_signal[k<<1], tx_csi_rs_signal[(k<<1)+1],
                 rx_csi_rs_signal[k<<1], rx_csi_rs_signal[(k<<1)+1],
                 csi_rs_ls_estimated_channel[k<<1], csi_rs_ls_estimated_channel[(k<<1)+1]);
        }
        printf("\n");
      }
    }
#endif

    /// Channel interpolation

    for(uint16_t port_tx = 0; port_tx<nr_csi_rs_info->N_ports; port_tx++) {
      memset(csi_rs_estimated_channel_freq[ant_rx][port_tx], 0, frame_parms->ofdm_symbol_size*sizeof(int32_t));
    }

    for (int rb = csirs_config_pdu->start_rb; rb < (csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs); rb++) {

      // for freq density 0.5 checks if even or odd RB
      if(csirs_config_pdu->freq_density <= 1 && csirs_config_pdu->freq_density != (rb % 2)) {
        continue;
      }

      uint16_t k = (frame_parms->first_carrier_offset + rb*NR_NB_SC_PER_RB) % frame_parms->ofdm_symbol_size;
      for(uint16_t port_tx = 0; port_tx<nr_csi_rs_info->N_ports; port_tx++) {
        int16_t *csi_rs_ls_estimated_channel = (int16_t*)&nr_csi_rs_info->csi_rs_ls_estimated_channel[ant_rx][port_tx][k];
        int16_t *csi_rs_estimated_channel16 = (int16_t *)&csi_rs_estimated_channel_freq[ant_rx][port_tx][k];
        if( (k == 0) || (k == frame_parms->first_carrier_offset) ) { // Start of OFDM symbol case or first occupied subcarrier case
          multadd_real_vector_complex_scalar(filt24_start, csi_rs_ls_estimated_channel, csi_rs_estimated_channel16, 24);
        } else if( ( (k + NR_NB_SC_PER_RB) >= frame_parms->ofdm_symbol_size) ||
                   (rb == (csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs-1)) ) { // End of OFDM symbol case or Last occupied subcarrier case
          multadd_real_vector_complex_scalar(filt24_end, csi_rs_ls_estimated_channel, csi_rs_estimated_channel16 - 3*sizeof(uint64_t), 24);
        } else { // Middle case
          multadd_real_vector_complex_scalar(filt24_middle, csi_rs_ls_estimated_channel, csi_rs_estimated_channel16 - 3*sizeof(uint64_t), 24);
        }
      }
    }

    /// Power noise estimation
    uint16_t noise_real[frame_parms->nb_antennas_rx][nr_csi_rs_info->N_ports][csirs_config_pdu->nr_of_rbs];
    uint16_t noise_imag[frame_parms->nb_antennas_rx][nr_csi_rs_info->N_ports][csirs_config_pdu->nr_of_rbs];
    for (int rb = csirs_config_pdu->start_rb; rb < (csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs); rb++) {
      if (csirs_config_pdu->freq_density <= 1 && csirs_config_pdu->freq_density != (rb % 2)) {
        continue;
      }
      uint16_t k = (frame_parms->first_carrier_offset + rb*NR_NB_SC_PER_RB) % frame_parms->ofdm_symbol_size;
      for(uint16_t port_tx = 0; port_tx<nr_csi_rs_info->N_ports; port_tx++) {
        int16_t *csi_rs_ls_estimated_channel = (int16_t*)&nr_csi_rs_info->csi_rs_ls_estimated_channel[ant_rx][port_tx][k];
        int16_t *csi_rs_estimated_channel16 = (int16_t *)&csi_rs_estimated_channel_freq[ant_rx][port_tx][k];
        noise_real[ant_rx][port_tx][rb-csirs_config_pdu->start_rb] = abs(csi_rs_ls_estimated_channel[0]-csi_rs_estimated_channel16[0]);
        noise_imag[ant_rx][port_tx][rb-csirs_config_pdu->start_rb] = abs(csi_rs_ls_estimated_channel[1]-csi_rs_estimated_channel16[1]);
        maxh = cmax3(maxh, abs(csi_rs_estimated_channel16[0]), abs(csi_rs_estimated_channel16[1]));
      }
    }
    for(uint16_t port_tx = 0; port_tx<nr_csi_rs_info->N_ports; port_tx++) {
      *noise_power += (calc_power_csirs(noise_real[ant_rx][port_tx], csirs_config_pdu) + calc_power_csirs(noise_imag[ant_rx][port_tx],csirs_config_pdu));
    }

#ifdef NR_CSIRS_DEBUG
    for(int k = 0; k<frame_parms->ofdm_symbol_size; k++) {
      int rb = k >= frame_parms->first_carrier_offset ?
               (k - frame_parms->first_carrier_offset)/NR_NB_SC_PER_RB :
               (k + frame_parms->ofdm_symbol_size - frame_parms->first_carrier_offset)/NR_NB_SC_PER_RB;
      LOG_I(NR_PHY, "(k = %4d) |\t", k);
      for(uint16_t port_tx = 0; port_tx<nr_csi_rs_info->N_ports; port_tx++) {
        int16_t *csi_rs_ls_estimated_channel = (int16_t*)&nr_csi_rs_info->csi_rs_ls_estimated_channel[ant_rx][port_tx][0];
        int16_t *csi_rs_estimated_channel16 = (int16_t *)&csi_rs_estimated_channel_freq[ant_rx][port_tx][0];
        printf("Channel port_tx %d --> ant_rx %d : ls (%4d,%4d), int (%4d,%4d), noise (%4d,%4d) | ",
               port_tx+3000, ant_rx,
               csi_rs_ls_estimated_channel[k<<1], csi_rs_ls_estimated_channel[(k<<1)+1],
               csi_rs_estimated_channel16[k<<1], csi_rs_estimated_channel16[(k<<1)+1],
               rb >= csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs ? 0 : noise_real[ant_rx][port_tx][rb-csirs_config_pdu->start_rb],
               rb >= csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs ? 0 : noise_imag[ant_rx][port_tx][rb-csirs_config_pdu->start_rb]);
      }
      printf("\n");
    }
#endif

  }

  *noise_power /= (frame_parms->nb_antennas_rx*nr_csi_rs_info->N_ports);
  nr_csi_rs_info->log2_maxh = log2_approx(maxh-1);

#ifdef NR_CSIRS_DEBUG
  LOG_I(NR_PHY, "Noise power estimation based on CSI-RS: %i\n", *noise_power);
#endif

  return 0;
}

int nr_csi_rs_ri_estimation(PHY_VARS_NR_UE *ue,
                            fapi_nr_dl_config_csirs_pdu_rel15_t *csirs_config_pdu,
                            nr_csi_rs_info_t *nr_csi_rs_info,
                            int32_t ***csi_rs_estimated_channel_freq,
                            uint8_t *rank_indicator) {

  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  int16_t cond_dB_threshold = 0;
  int count = 0;
  *rank_indicator = 0;

  if (ue->frame_parms.nb_antennas_rx == 1 || ue->nr_csi_rs_info->N_ports == 1) {
    return 0;
  } else if( !(ue->frame_parms.nb_antennas_rx == 2 && ue->nr_csi_rs_info->N_ports == 2) ) {
    LOG_W(NR_PHY, "Rank indicator computation is not implemented for %i x %i system\n",
          ue->frame_parms.nb_antennas_rx, ue->nr_csi_rs_info->N_ports);
    return -1;
  }

  /* Example 2x2: Hh x H =
  *            | conjch00 conjch10 | x | ch00 ch01 | = | conjch00*ch00+conjch10*ch10 conjch00*ch01+conjch10*ch11 |
  *            | conjch01 conjch11 |   | ch10 ch11 |   | conjch01*ch00+conjch11*ch10 conjch01*ch01+conjch11*ch11 |
  */

  for(uint16_t port_tx_conjch = 0; port_tx_conjch < nr_csi_rs_info->N_ports; port_tx_conjch++) {
    for(uint16_t port_tx_ch = 0; port_tx_ch < nr_csi_rs_info->N_ports; port_tx_ch++) {
      memset(nr_csi_rs_info->csi_rs_estimated_A_MF[port_tx_conjch][port_tx_ch],0,NR_MAX_OFDM_SYMBOL_SIZE*sizeof(int32_t));
    }
  }

  for (int rb = csirs_config_pdu->start_rb; rb < (csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs); rb++) {

    if (csirs_config_pdu->freq_density <= 1 && csirs_config_pdu->freq_density != (rb % 2)) {
      continue;
    }
    uint16_t k = (frame_parms->first_carrier_offset + rb*NR_NB_SC_PER_RB) % frame_parms->ofdm_symbol_size;

    for (int ant_rx_conjch = 0; ant_rx_conjch < frame_parms->nb_antennas_rx; ant_rx_conjch++) {
      for(uint16_t port_tx_conjch = 0; port_tx_conjch < nr_csi_rs_info->N_ports; port_tx_conjch++) {
        for (int ant_rx_ch = 0; ant_rx_ch < frame_parms->nb_antennas_rx; ant_rx_ch++) {
          for(uint16_t port_tx_ch = 0; port_tx_ch < nr_csi_rs_info->N_ports; port_tx_ch++) {

            // conjch x ch computation
            nr_conjch0_mult_ch1(&csi_rs_estimated_channel_freq[ant_rx_conjch][port_tx_conjch][k],
                                &csi_rs_estimated_channel_freq[ant_rx_ch][port_tx_ch][k],
                                &nr_csi_rs_info->csi_rs_estimated_conjch_ch[ant_rx_conjch][port_tx_conjch][ant_rx_ch][port_tx_ch][k],
                                1,
                                nr_csi_rs_info->log2_maxh);

            // construct Hh x H elements
            if(ant_rx_conjch == ant_rx_ch) {
              nr_a_sum_b((__m128i *)&nr_csi_rs_info->csi_rs_estimated_A_MF[port_tx_conjch][port_tx_ch][k],
                         (__m128i *)&nr_csi_rs_info->csi_rs_estimated_conjch_ch[ant_rx_conjch][port_tx_conjch][ant_rx_ch][port_tx_ch][k],
                         1);
            }
          }
        }
      }
    }

    // compute the determinant of A_MF (denominator)
    nr_det_A_MF_2x2(&nr_csi_rs_info->csi_rs_estimated_A_MF[0][0][k],
                    &nr_csi_rs_info->csi_rs_estimated_A_MF[0][1][k],
                    &nr_csi_rs_info->csi_rs_estimated_A_MF[1][0][k],
                    &nr_csi_rs_info->csi_rs_estimated_A_MF[1][1][k],
                    &nr_csi_rs_info->csi_rs_estimated_determ_fin[k],
                    1);

    // compute the square of A_MF (numerator)
    nr_squared_matrix_element(&nr_csi_rs_info->csi_rs_estimated_A_MF[0][0][k], &nr_csi_rs_info->csi_rs_estimated_A_MF_sq[0][0][k], 1);
    nr_squared_matrix_element(&nr_csi_rs_info->csi_rs_estimated_A_MF[0][1][k], &nr_csi_rs_info->csi_rs_estimated_A_MF_sq[0][1][k], 1);
    nr_squared_matrix_element(&nr_csi_rs_info->csi_rs_estimated_A_MF[1][0][k], &nr_csi_rs_info->csi_rs_estimated_A_MF_sq[1][0][k], 1);
    nr_squared_matrix_element(&nr_csi_rs_info->csi_rs_estimated_A_MF[1][1][k], &nr_csi_rs_info->csi_rs_estimated_A_MF_sq[1][1][k], 1);
    nr_numer_2x2(&nr_csi_rs_info->csi_rs_estimated_A_MF_sq[0][0][k],
                 &nr_csi_rs_info->csi_rs_estimated_A_MF_sq[0][1][k],
                 &nr_csi_rs_info->csi_rs_estimated_A_MF_sq[1][0][k],
                 &nr_csi_rs_info->csi_rs_estimated_A_MF_sq[1][1][k],
                 &nr_csi_rs_info->csi_rs_estimated_numer_fin[k],
                 1);

    // compute the conditional number
    for (int sc_idx=0; sc_idx < NR_NB_SC_PER_RB; sc_idx++) {
      int8_t csi_rs_estimated_denum_db = dB_fixed(nr_csi_rs_info->csi_rs_estimated_determ_fin[k + sc_idx]);
      int8_t csi_rs_estimated_numer_db = dB_fixed(nr_csi_rs_info->csi_rs_estimated_numer_fin[k + sc_idx]);
      int8_t cond_db = csi_rs_estimated_numer_db - csi_rs_estimated_denum_db;
      if (cond_db < cond_dB_threshold) {
        count++;
      } else {
        count--;
      }
    }
  }

  // conditional number is lower than cond_dB_threshold in half on more REs
  if (count > 0) {
    *rank_indicator = 1;
  }

  return 0;
}

int nr_csi_rs_pmi_estimation(PHY_VARS_NR_UE *ue,
                             fapi_nr_dl_config_csirs_pdu_rel15_t *csirs_config_pdu,
                             nr_csi_rs_info_t *nr_csi_rs_info,
                             int32_t ***csi_rs_estimated_channel_freq,
                             uint8_t rank_indicator,
                             uint8_t *i1,
                             uint8_t *i2) {

  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  memset(i1,0,3*sizeof(uint8_t));
  i2[0] = 0;

  // i1 is a three-element vector in the form of [i11 i12 i13], when CodebookType is specified as 'Type1SinglePanel'.
  // Note that i13 is not applicable when the number of transmission layers is one of {1, 5, 6, 7, 8}.
  // i2, for 'Type1SinglePanel' codebook type, it is a scalar when PMIMode is specified as 'wideband', and when PMIMode
  // is specified as 'subband' or when PRGSize, the length of the i2 vector equals to the number of subbands or PRGs.
  // Note that when the number of CSI-RS ports is 2, the applicable codebook type is 'Type1SinglePanel'. In this case,
  // the precoding matrix is obtained by a single index (i2 field here) based on TS 38.214 Table 5.2.2.2.1-1.
  // The first column is applicable if the UE is reporting a Rank = 1, whereas the second column is applicable if the
  // UE is reporting a Rank = 2.

  if(ue->nr_csi_rs_info->N_ports == 1) {
    return 0;
  }

  if(rank_indicator == 0) {

    uint32_t metric[4] = {0};
    uint16_t shift = nr_csi_rs_info->log2_maxh + log2_approx(frame_parms->nb_antennas_rx) + log2_approx(csirs_config_pdu->nr_of_rbs) - 1;

    for (int rb = csirs_config_pdu->start_rb; rb < (csirs_config_pdu->start_rb+csirs_config_pdu->nr_of_rbs); rb++) {

      if (csirs_config_pdu->freq_density <= 1 && csirs_config_pdu->freq_density != (rb % 2)) {
        continue;
      }
      uint16_t k = (frame_parms->first_carrier_offset + rb * NR_NB_SC_PER_RB) % frame_parms->ofdm_symbol_size;

      for (int ant_rx = 0; ant_rx < frame_parms->nb_antennas_rx; ant_rx++) {
        int16_t *csi_rs_estimated_channel_p0 = (int16_t *) &csi_rs_estimated_channel_freq[ant_rx][0][k];
        int16_t *csi_rs_estimated_channel_p1 = (int16_t *) &csi_rs_estimated_channel_freq[ant_rx][1][k];

        // H_p0 + 1*H_p1 = (H_p0_re + H_p1_re) + 1j*(H_p0_im + H_p1_im)
        metric[0] += ((csi_rs_estimated_channel_p0[0]+csi_rs_estimated_channel_p1[0])*(csi_rs_estimated_channel_p0[0]+csi_rs_estimated_channel_p1[0])
            + (csi_rs_estimated_channel_p0[1]+csi_rs_estimated_channel_p1[1])*(csi_rs_estimated_channel_p0[1]+csi_rs_estimated_channel_p1[1]))>>shift;

        // H_p0 + 1j*H_p1 = (H_p0_re - H_p1_im) + 1j*(H_p0_im + H_p1_re)
        metric[1] += ((csi_rs_estimated_channel_p0[0]-csi_rs_estimated_channel_p1[1])*(csi_rs_estimated_channel_p0[0]-csi_rs_estimated_channel_p1[1])
                      + (csi_rs_estimated_channel_p0[1]+csi_rs_estimated_channel_p1[0])*(csi_rs_estimated_channel_p0[1]+csi_rs_estimated_channel_p1[0]))>>shift;

        // H_p0 - 1*H_p1 = (H_p0_re - H_p1_re) + 1j*(H_p0_im - H_p1_im)
        metric[2] += ((csi_rs_estimated_channel_p0[0]-csi_rs_estimated_channel_p1[0])*(csi_rs_estimated_channel_p0[0]-csi_rs_estimated_channel_p1[0])
                      + (csi_rs_estimated_channel_p0[1]-csi_rs_estimated_channel_p1[1])*(csi_rs_estimated_channel_p0[1]-csi_rs_estimated_channel_p1[1]))>>shift;

        // H_p0 - 1j*H_p1 = (H_p0_re + H_p1_im) + 1j*(H_p0_im - H_p1_re)
        metric[3] += ((csi_rs_estimated_channel_p0[0]+csi_rs_estimated_channel_p1[1])*(csi_rs_estimated_channel_p0[0]+csi_rs_estimated_channel_p1[1])
                      + (csi_rs_estimated_channel_p0[1]-csi_rs_estimated_channel_p1[0])*(csi_rs_estimated_channel_p0[1]-csi_rs_estimated_channel_p1[0]))>>shift;
      }
    }

    for(int tested_i2 = 0; tested_i2 < 4; tested_i2++) {
      if(metric[tested_i2] > metric[i2[0]]) {
        i2[0] = tested_i2;
      }
    }

  } else if(rank_indicator == 1) {

  } else {
    LOG_W(NR_PHY, "PMI computation is not implemented for rank indicator %i\n", rank_indicator);
  }

  return 0;
}

int nr_ue_csi_im_procedures(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc, uint8_t gNB_id) {
  return 0;
}

int nr_ue_csi_rs_procedures(PHY_VARS_NR_UE *ue, UE_nr_rxtx_proc_t *proc, uint8_t gNB_id) {

  if(!ue->csirs_vars[gNB_id]->active) {
    return -1;
  }

  fapi_nr_dl_config_csirs_pdu_rel15_t *csirs_config_pdu = (fapi_nr_dl_config_csirs_pdu_rel15_t*)&ue->csirs_vars[gNB_id]->csirs_config_pdu;

#ifdef NR_CSIRS_DEBUG
  LOG_I(NR_PHY, "csirs_config_pdu->subcarrier_spacing = %i\n", csirs_config_pdu->subcarrier_spacing);
  LOG_I(NR_PHY, "csirs_config_pdu->cyclic_prefix = %i\n", csirs_config_pdu->cyclic_prefix);
  LOG_I(NR_PHY, "csirs_config_pdu->start_rb = %i\n", csirs_config_pdu->start_rb);
  LOG_I(NR_PHY, "csirs_config_pdu->nr_of_rbs = %i\n", csirs_config_pdu->nr_of_rbs);
  LOG_I(NR_PHY, "csirs_config_pdu->csi_type = %i (0:TRS, 1:CSI-RS NZP, 2:CSI-RS ZP)\n", csirs_config_pdu->csi_type);
  LOG_I(NR_PHY, "csirs_config_pdu->row = %i\n", csirs_config_pdu->row);
  LOG_I(NR_PHY, "csirs_config_pdu->freq_domain = %i\n", csirs_config_pdu->freq_domain);
  LOG_I(NR_PHY, "csirs_config_pdu->symb_l0 = %i\n", csirs_config_pdu->symb_l0);
  LOG_I(NR_PHY, "csirs_config_pdu->symb_l1 = %i\n", csirs_config_pdu->symb_l1);
  LOG_I(NR_PHY, "csirs_config_pdu->cdm_type = %i\n", csirs_config_pdu->cdm_type);
  LOG_I(NR_PHY, "csirs_config_pdu->freq_density = %i (0: dot5 (even RB), 1: dot5 (odd RB), 2: one, 3: three)\n", csirs_config_pdu->freq_density);
  LOG_I(NR_PHY, "csirs_config_pdu->scramb_id = %i\n", csirs_config_pdu->scramb_id);
  LOG_I(NR_PHY, "csirs_config_pdu->power_control_offset = %i\n", csirs_config_pdu->power_control_offset);
  LOG_I(NR_PHY, "csirs_config_pdu->power_control_offset_ss = %i\n", csirs_config_pdu->power_control_offset_ss);
#endif

  nr_generate_csi_rs(ue->frame_parms,
                     ue->nr_csi_rs_info->csi_rs_generated_signal,
                     AMP,
                     ue->nr_csi_rs_info,
                     (nfapi_nr_dl_tti_csi_rs_pdu_rel15_t *) csirs_config_pdu,
                     ue->frame_parms.first_carrier_offset,
                     proc->nr_slot_rx);

  nr_get_csi_rs_signal(ue,
                       proc,
                       csirs_config_pdu,
                       ue->nr_csi_rs_info,
                       ue->nr_csi_rs_info->csi_rs_received_signal);

  nr_csi_rs_channel_estimation(ue,
                               proc,
                               csirs_config_pdu,
                               ue->nr_csi_rs_info,
                               ue->nr_csi_rs_info->csi_rs_generated_signal,
                               ue->nr_csi_rs_info->csi_rs_received_signal,
                               ue->nr_csi_rs_info->csi_rs_estimated_channel_freq,
                               ue->nr_csi_rs_info->noise_power);

  nr_csi_rs_ri_estimation(ue,
                          csirs_config_pdu,
                          ue->nr_csi_rs_info,
                          ue->nr_csi_rs_info->csi_rs_estimated_channel_freq,
                          ue->nr_csi_rs_info->rank_indicator);

  nr_csi_rs_pmi_estimation(ue,
                           csirs_config_pdu,
                           ue->nr_csi_rs_info,
                           ue->nr_csi_rs_info->csi_rs_estimated_channel_freq,
                           *ue->nr_csi_rs_info->rank_indicator,
                           ue->nr_csi_rs_info->i1,
                           ue->nr_csi_rs_info->i2);

  LOG_I(NR_PHY, "RI = %i, i1 = %i.%i.%i, i2 = %i\n",
        *ue->nr_csi_rs_info->rank_indicator + 1,
        ue->nr_csi_rs_info->i1[0], ue->nr_csi_rs_info->i1[1], ue->nr_csi_rs_info->i1[2],
        ue->nr_csi_rs_info->i2[0]);

  return 0;
}
