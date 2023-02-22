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

#include <string.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "common/config/config_userapi.h"
#include "common/utils/load_module_shlib.h"
#include "common/utils/LOG/log.h"
#include "common/ran_context.h"
#include "PHY/types.h"
#include "PHY/defs_nr_common.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/defs_gNB.h"
#include "PHY/INIT/phy_init.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/MODULATION/modulation_eNB.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "SCHED_NR/sched_nr.h"
#include "openair1/SIMULATION/TOOLS/sim.h"
#include "openair1/SIMULATION/RF/rf.h"
#include "openair1/SIMULATION/NR_PHY/nr_unitary_defs.h"
#include "openair1/SIMULATION/NR_PHY/nr_dummy_functions.c"
#include "common/utils/threadPool/thread-pool.h"
#include "openair2/LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "executables/nr-uesoftmodem.h"

//#define DEBUG_NR_SLSCHSIM
#define HNA_SIZE 6 * 68 * 384 // [hna] 16 segments, 68*Zc
RAN_CONTEXT_t RC;
double cpuf;
uint16_t NB_UE_INST = 1;
openair0_config_t openair0_cfg[MAX_CARDS];
uint8_t const nr_rv_round_map[4] = {0, 2, 3, 1};

uint64_t get_softmodem_optmask(void) {return 0;}
static softmodem_params_t softmodem_params;
softmodem_params_t *get_softmodem_params(void) {
  return &softmodem_params;
}

nrUE_params_t nrUE_params;
nrUE_params_t *get_nrUE_params(void) {
  return &nrUE_params;
}

void init_downlink_harq_status(NR_DL_UE_HARQ_t *dl_harq) {}


typedef struct {
  uint8_t priority;
  uint8_t freq_res;
  uint8_t time_res;
  uint8_t period;
  uint16_t dmrs_pattern;
  uint8_t mcs;
  uint8_t beta_offset;
  uint8_t dmrs_port;
} SCI_1_A;

typedef struct {
  double scs;
  double bw;
  double fs;
} BW;

<<<<<<< HEAD
double snr0 =- 2.0;
double snr1 = 2.0;
uint8_t snr1set = 0;
int n_trials = 1;
uint8_t n_tx = 1;
uint8_t n_rx = 1;
int ssb_subcarrier_offset = 0;
=======
THREAD_STRUCT thread_struct;
PHY_VARS_NR_UE *txUE;
PHY_VARS_NR_UE *rxUE;
RAN_CONTEXT_t RC;
int32_t uplink_frequency_offset[MAX_NUM_CCs][4];
uint64_t downlink_frequency[MAX_NUM_CCs][4];

SCM_t channel_model = AWGN;  //Rayleigh1_anticorr;
uint16_t N_RB = 106, mu = 1, nb_symb_sch = 12;
uint8_t nb_re_dmrs = 6;
int slot = 0;
>>>>>>> episys/master-sl
FILE *input_fd = NULL;
SCM_t channel_model = AWGN;
int nb_rb = 50;
int N_RB_UL = 106;
int N_RB_DL = 106;
int mu = 1;
int loglvl = OAILOG_WARNING;
int seed = 0;

static void get_sim_cl_opts(int argc, char **argv)
{
    char c;
    while ((c = getopt(argc, argv, "F:g:hIL:m:M:n:N:o:O:p:P:r:R:s:S:x:y:z:")) != -1) {
    switch (c) {
      case 'F':
        input_fd = fopen(optarg, "r");
        if (input_fd == NULL) {
          printf("Problem with filename %s. Exiting.\n", optarg);
          exit(-1);
        }
        break;

      case 'g':
        switch((char)*optarg) {
          case 'A':
            channel_model=SCM_A;
            break;

          case 'B':
            channel_model=SCM_B;
            break;

          case 'C':
            channel_model=SCM_C;
            break;

          case 'D':
            channel_model=SCM_D;
            break;

          case 'E':
            channel_model=EPA;
            break;

          case 'F':
            channel_model=EVA;
            break;

          case 'G':
            channel_model=ETU;
            break;

          default:
            printf("Unsupported channel model! Exiting.\n");
            exit(-1);
          }
        break;

      case 'L':
        loglvl = atoi(optarg);
        break;

      case 'm':
        mu = atoi(optarg);
        break;

      case 'n':
        n_trials = atoi(optarg);
        break;

      case 'R':
        N_RB_UL = atoi(optarg);
        break;

      case 'r':
        nb_rb = atoi(optarg);
        break;

      case 's':
        snr0 = atof(optarg);
        break;

      case 'S':
        snr1 = atof(optarg);
        snr1set = 1;
        break;

      case 'y':
        n_tx = atoi(optarg);
        if ((n_tx == 0) || (n_tx > 2)) {
          printf("Unsupported number of TX antennas %d. Exiting.\n", n_tx);
          exit(-1);
        }
        break;

      case 'z':
        n_rx = atoi(optarg);
        if ((n_rx == 0) || (n_rx > 2)) {
          printf("Unsupported number of RX antennas %d. Exiting.\n", n_rx);
          exit(-1);
        }
        break;

      default:
      case 'h':
          printf("%s -h(elp) -g channel_model -n n_frames -s snr0 -S snr1 -p(extended_prefix) -y TXant -z RXant -M -N cell_id -R -F input_filename -m -l -r\n", argv[0]);
          //printf("%s -h(elp) -p(extended_prefix) -N cell_id -f output_filename -F input_filename -g channel_model -n n_frames -t Delayspread -s snr0 -S snr1 -x transmission_mode -y TXant -z RXant -i Intefrence0 -j Interference1 -A interpolation_file -C(alibration offset dB) -N CellId\n", argv[0]);
          printf("-h This message\n");
          printf("-g [A,B,C,D,E,F,G] Use 3GPP SCM (A,B,C,D) or 36-101 (E-EPA,F-EVA,G-ETU) models (ignores delay spread and Ricean factor)\n");
          printf("-n Number of frames to simulate\n");
          printf("-s Starting SNR, runs from SNR0 to SNR0 + 5 dB.  If n_frames is 1 then just SNR is simulated\n");
          printf("-S Ending SNR, runs from SNR0 to SNR1\n");
          printf("-p Use extended prefix mode\n");
          printf("-y Number of TX antennas used in eNB\n");
          printf("-z Number of RX antennas used in UE\n");
          printf("-W number of layer\n");
          printf("-R N_RB_UL\n");
          printf("-r nb_rb\n");
          printf("-F Input filename (.txt format) for RX conformance testing\n");
          printf("-m MCS\n");
          printf("-l number of symbol\n");
          printf("-r number of RB\n");
        exit (-1);
        break;
    }
  }
}


void nr_phy_config_request_psschsim(PHY_VARS_NR_UE *ue,
                                    int N_RB_UL,
                                    int N_RB_DL,
                                    int mu,
                                    uint64_t position_in_burst)
{
  NR_DL_FRAME_PARMS *fp                 = &ue->frame_parms;
  fapi_nr_config_request_t *nrUE_config = &ue->nrUE_config;
  uint64_t rev_burst=0;
  for (int i = 0; i < 64; i++)
    rev_burst |= (((position_in_burst >> (63 - i)) & 0x01) << i);

  nrUE_config->ssb_config.scs_common               = mu;
  nrUE_config->ssb_table.ssb_subcarrier_offset     = ssb_subcarrier_offset;
  nrUE_config->ssb_table.ssb_offset_point_a        = (N_RB_UL - 20) >> 1;
  nrUE_config->ssb_table.ssb_mask_list[1].ssb_mask = (rev_burst)&(0xFFFFFFFF);
  nrUE_config->ssb_table.ssb_mask_list[0].ssb_mask = (rev_burst >> 32)&(0xFFFFFFFF);
  nrUE_config->cell_config.frame_duplex_type       = TDD;
  nrUE_config->ssb_table.ssb_period                = 1; //10ms
  nrUE_config->carrier_config.dl_grid_size[mu]     = N_RB_DL;
  nrUE_config->carrier_config.ul_grid_size[mu]     = N_RB_UL;
  nrUE_config->carrier_config.num_tx_ant           = fp->nb_antennas_tx;
  nrUE_config->carrier_config.num_rx_ant           = fp->nb_antennas_rx;
  nrUE_config->tdd_table.tdd_period                = 0;

  ue->mac_enabled = 1;
  if (mu == 0) {
    fp->dl_CarrierFreq = 2600000000;
    fp->ul_CarrierFreq = 2600000000;
    fp->nr_band = 38;
  } else if (mu == 1) {
    fp->dl_CarrierFreq = 3600000000;
    fp->ul_CarrierFreq = 3600000000;
    fp->sl_CarrierFreq = 2600000000;
    fp->nr_band = 78;
  } else if (mu == 3) {
    fp->dl_CarrierFreq = 27524520000;
    fp->ul_CarrierFreq = 27524520000;
    fp->nr_band = 261;
  }

  fp->threequarter_fs = 0;
  nrUE_config->carrier_config.dl_bandwidth = config_bandwidth(mu, N_RB_DL, fp->nr_band);

  nr_init_frame_parms_ue(fp, nrUE_config, fp->nr_band);
  fp->ofdm_offset_divisor = UINT_MAX;
  ue->configured = 1;
  LOG_I(NR_PHY, "tx UE configured\n");
}

void set_sci(SCI_1_A *sci) {
  sci->period = 0;
  sci->dmrs_pattern = 0b0001000001000; // LSB is slot 1 and MSB is slot 13
  sci->beta_offset = 0;
  sci->dmrs_port = 0;
  sci->priority = 0;
  sci->freq_res = 1;
  sci->time_res = 1;
  sci->mcs = 0;
}

void set_fs_bw(PHY_VARS_NR_UE *UE, int mu, int N_RB, BW *bw_setting) {
  double scs, fs, bw;
  switch (mu) {
    case 1:
      scs = 30000;
      UE->frame_parms.Lmax = 1;
      if (N_RB == 217) {
        fs = 122.88e6;
        bw = 80e6;
      }
      else if (N_RB == 245) {
        fs = 122.88e6;
        bw = 90e6;
      }
      else if (N_RB == 273) {
        fs = 122.88e6;
        bw = 100e6;
      }
      else if (N_RB == 106) {
        fs = 61.44e6;
        bw = 40e6;
      }
      else AssertFatal(1 == 0, "Unsupported numerology for mu %d, N_RB %d\n", mu, N_RB);
      break;
    case 3:
      UE->frame_parms.Lmax = 64;
      scs = 120000;
      if (N_RB == 66) {
        fs = 122.88e6;
        bw = 100e6;
      }
      else AssertFatal(1 == 0,"Unsupported numerology for mu %d, N_RB %d\n", mu, N_RB);
      break;
  }
  bw_setting->scs = scs;
  bw_setting->fs = fs;
  bw_setting->bw = bw;
  return;
}

int main(int argc, char **argv)
{
  get_softmodem_params()->sl_mode = 2;
  if (load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY) == 0) {
    exit_fun("[NR_PSBCHSIM] Error, configuration module init failed\n");
  }
  get_sim_cl_opts(argc, argv);
  randominit(seed);
  logInit();
  set_glog(loglvl);

  PHY_VARS_NR_UE *txUE = malloc(sizeof(PHY_VARS_NR_UE));
  txUE->frame_parms.N_RB_DL = N_RB_DL;
  txUE->frame_parms.N_RB_UL = N_RB_UL;
  txUE->frame_parms.Ncp = NORMAL;
  txUE->frame_parms.nb_antennas_tx = 1;
  txUE->frame_parms.nb_antennas_rx = n_rx;
  txUE->max_ldpc_iterations = 5;

<<<<<<< HEAD
  PHY_VARS_NR_UE *rxUE = malloc(sizeof(PHY_VARS_NR_UE));
  rxUE->frame_parms.nb_antennas_tx = n_tx;
  rxUE->frame_parms.nb_antennas_rx = 1;

  uint64_t burst_position = 0x01;
  nr_phy_config_request_psschsim(txUE, N_RB_UL, N_RB_DL, mu, burst_position);
  nr_phy_config_request_psschsim(rxUE, N_RB_UL, N_RB_DL, mu, burst_position);

  BW *bw_setting = malloc(sizeof(BW));
  set_fs_bw(txUE, mu, N_RB_UL, bw_setting);

  double DS_TDL = 300e-9; //.03;
  channel_desc_t *UE2UE = new_channel_desc_scm(n_tx, n_rx, channel_model,
                                               bw_setting->fs,
                                               bw_setting->bw,
                                               DS_TDL,
                                               0, 0, 0, 0);
=======
  txUE = malloc(sizeof(PHY_VARS_NR_UE));
  rxUE = malloc(sizeof(PHY_VARS_NR_UE));
  frame_parms = &txUE->frame_parms; //to be initialized I suppose (maybe not necessary for PBCH)
  frame_parms->N_RB_DL = N_RB;
  frame_parms->N_RB_UL = N_RB;
  frame_parms->Ncp = extended_prefix_flag ? EXTENDED : NORMAL;
  txUE->max_ldpc_iterations = max_ldpc_iterations;

  memcpy(&txUE->frame_parms, frame_parms, sizeof(NR_DL_FRAME_PARMS));

  txUE->frame_parms.nb_antennas_tx = 1;
  txUE->frame_parms.nb_antennas_rx = n_rx;

  nr_phy_config_request_psschsim(txUE, N_RB, mu, burst_position);
  memcpy(&rxUE->frame_parms, frame_parms, sizeof(NR_DL_FRAME_PARMS));

  rxUE->frame_parms.nb_antennas_tx = n_tx;
  rxUE->frame_parms.nb_antennas_rx = 1;

  BW *bw_setting = malloc(sizeof(BW));
  set_fs_bw(txUE, mu, N_RB, bw_setting);

  UE2UE = new_channel_desc_scm(n_tx,
                               n_rx,
                               channel_model,
                               bw_setting->fs, //N_RB2sampling_rate(N_RB_DL),
                               bw_setting->bw, //N_RB2channel_bandwidth(N_RB_DL),
                               DS_TDL,
                               0, 0, 0, 0);
>>>>>>> episys/master-sl

  if (UE2UE == NULL) {
    printf("Problem generating channel model. Exiting.\n");
    free(bw_setting);
    exit(-1);
  }

  if (init_nr_ue_signal(txUE, 1) != 0 || init_nr_ue_signal(rxUE, 1) != 0) {
    printf("Error at UE NR initialisation.\n");
    free(bw_setting);
    free(txUE);
    free(rxUE);
    exit(-1);
  }
<<<<<<< HEAD
#ifdef DEBUG_NR_SLSCHSIM
  for (int sf = 0; sf < 2; sf++) {
=======
  /*
  for (sf = 0; sf < 2; sf++) {
>>>>>>> episys/master-sl
    txUE->slsch[sf][0] = new_nr_ue_ulsch(N_RB, 8, frame_parms);
    if (!txUE->slsch[sf][0]) {
      printf("Can't get ue ulsch structures.\n");
      exit(-1);
    }
  }
<<<<<<< HEAD
#endif
  init_nr_ue_transport(txUE);
  init_nr_ue_transport(rxUE);
=======
  */
  init_nr_ue_transport(&txUE);
  init_nr_ue_transport(&rxUE);
>>>>>>> episys/master-sl

  uint8_t nb_re_dmrs = 6;
  uint8_t Nl = 1; // number of layers
  if ((Nl == 4) || (Nl == 3))
    nb_re_dmrs = nb_re_dmrs * 2;
  uint8_t Imcs = 9;
  uint8_t mod_order = nr_get_Qm_ul(Imcs, 0);
  uint16_t code_rate = nr_get_code_rate_ul(Imcs, 0);
  uint8_t length_dmrs = 1;
<<<<<<< HEAD
  uint16_t nb_symb_sch = 12;
  unsigned int available_bits = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs, mod_order, Nl);
  unsigned int TBS = nr_compute_tbs(mod_order, code_rate, nb_rb, nb_symb_sch, nb_re_dmrs * length_dmrs, 0, 0, Nl);
  printf("\nAvailable bits %u TBS %u mod_order %d\n", available_bits, TBS, mod_order);

  unsigned char harq_pid = 0;
  NR_UE_DLSCH_t *slsch_ue_rx = rxUE->slsch_rx[0][0][0];
  slsch_ue_rx->harq_processes[harq_pid]->Nl = Nl;
  slsch_ue_rx->harq_processes[harq_pid]->Qm = mod_order;
  nfapi_nr_pssch_pdu_t *rel16_sl_rx = &slsch_ue_rx->harq_processes[harq_pid]->pssch_pdu;
  rel16_sl_rx->mcs_index            = Imcs;
  rel16_sl_rx->pssch_data.rv_index  = 0;
  rel16_sl_rx->target_code_rate     = code_rate;
  rel16_sl_rx->pssch_data.tb_size   = TBS >> 3;
  rel16_sl_rx->maintenance_parms_v3.ldpcBaseGraph = get_BG(TBS, code_rate);

  NR_UL_UE_HARQ_t *harq_process_txUE = txUE->slsch[0][0]->harq_processes[harq_pid];
  DevAssert(harq_process_txUE);
  uint8_t N_PRB_oh = 0;
  uint16_t N_RE_prime = NR_NB_SC_PER_RB * nb_symb_sch - nb_re_dmrs - N_PRB_oh;
  uint8_t nb_codewords = 1;
  harq_process_txUE->pssch_pdu.mcs_index = Imcs;
  harq_process_txUE->pssch_pdu.nrOfLayers = Nl;
  harq_process_txUE->pssch_pdu.rb_size = nb_rb;
  harq_process_txUE->pssch_pdu.nr_of_symbols = nb_symb_sch;
  harq_process_txUE->num_of_mod_symbols = N_RE_prime * nb_rb * nb_codewords;
  harq_process_txUE->pssch_pdu.pssch_data.rv_index = 0;
  harq_process_txUE->pssch_pdu.pssch_data.tb_size  = TBS >> 3;
=======
  uint8_t N_PRB_oh;
  uint16_t N_RE_prime,code_rate;
  unsigned char mod_order;
  uint8_t rvidx = 0;
  uint8_t UE_id = 0;

  NR_UE_DLSCH_t *dlsch_rxUE = rxUE->ulsch[UE_id];
  NR_DL_UE_HARQ_t *harq_process_rxUE = dlsch_rxUE->harq_processes[harq_pid];
  nfapi_nr_pssch_pdu_t *rel16_ul = &harq_process_rxUE->pssch_pdu;
  NR_UE_DLSCH_t *dlsch0_ue = rxUE->dlsch[0][0][0];
  NR_UE_ULSCH_t *slsch_ue = txUE->slsch[0][0];

  if ((Nl == 4)||(Nl == 3))
    nb_re_dmrs = nb_re_dmrs*2;

  mod_order = nr_get_Qm_ul(Imcs, 0);
  code_rate = nr_get_code_rate_ul(Imcs, 0);
  available_bits = nr_get_G(N_RB, nb_symb_sch, nb_re_dmrs, length_dmrs, mod_order, Nl);
  TBS = nr_compute_tbs(mod_order,code_rate, N_RB, nb_symb_sch, nb_re_dmrs*length_dmrs, 0, 0, Nl);

  printf("\nAvailable bits %u TBS %u mod_order %d\n", available_bits, TBS, mod_order);

  /////////// setting rel15_ul parameters ///////////
  rel16_ul->mcs_index           = Imcs;
  rel16_ul->pssch_data.rv_index = rvidx;
  rel16_ul->target_code_rate    = code_rate;
  rel16_ul->pssch_data.tb_size  = TBS>>3;
  rel16_ul->maintenance_parms_v3.ldpcBaseGraph = get_BG(TBS, code_rate);
  ///////////////////////////////////////////////////

  double modulated_input[16 * 68 * 384]; // [hna] 16 segments, 68*Zc
  short channel_output_fixed[16 * 68 * 384];
  short channel_output_uncoded[16 * 68 * 384];
  unsigned int errors_bit_uncoded = 0;
  unsigned int errors_bit = 0;

  unsigned char test_input_bit[16 * 68 * 384];
  unsigned char estimated_output_bit[16 * 68 * 384];

  /////////////////////////[adk] preparing UL harq_process parameters/////////////////////////
  ///////////
  NR_UL_UE_HARQ_t *harq_process_txUE = slsch_ue->harq_processes[harq_pid];
  DevAssert(harq_process_txUE);

  N_PRB_oh   = 0; // higher layer (RRC) parameter xOverhead in PUSCH-ServingCellConfig
  N_RE_prime = NR_NB_SC_PER_RB*nb_symb_sch - nb_re_dmrs - N_PRB_oh;
  harq_process_txUE->pssch_pdu.mcs_index = Imcs;
  harq_process_txUE->pssch_pdu.nrOfLayers = Nl;
  harq_process_txUE->pssch_pdu.rb_size = N_RB;
  harq_process_txUE->pssch_pdu.nr_of_symbols = nb_symb_sch;
  harq_process_txUE->num_of_mod_symbols = N_RE_prime*N_RB*nb_codewords;
  harq_process_txUE->pssch_pdu.pssch_data.rv_index = rvidx;
  harq_process_txUE->pssch_pdu.pssch_data.tb_size  = TBS>>3;
>>>>>>> episys/master-sl
  harq_process_txUE->pssch_pdu.target_code_rate = code_rate;
  harq_process_txUE->pssch_pdu.qam_mod_order = mod_order;
  unsigned char *test_input = harq_process_txUE->a;

  crcTableInit();
<<<<<<< HEAD
  for (int i = 0; i < TBS / 8; i++)
=======

  ///////////
  ////////////////////////////////////////////////////////////////////////////////////////////

  for (i = 0; i < TBS / 8; i++)
>>>>>>> episys/master-sl
    test_input[i] = (unsigned char) rand();

#ifdef DEBUG_NR_ULSCHSIM
  for (int i = 0; i < TBS / 8; i++) printf("i = %d / %d test_input[i]  =%hhu \n", i, TBS / 8, test_input[i]);
#endif

<<<<<<< HEAD
  unsigned int G = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs, mod_order, Nl);
  NR_UE_ULSCH_t *slsch_ue = txUE->slsch[0][0];
  nr_slsch_encoding(txUE, slsch_ue, &txUE->frame_parms, harq_pid, G);
=======
  /////////////////////////SLSCH coding/////////////////////////
  ///////////
  unsigned int G = nr_get_G(N_RB, nb_symb_sch, nb_re_dmrs, length_dmrs, mod_order, Nl);

  if (input_fd == NULL) {
    // [TODO] encoding tx data at tx UE
    nr_slsch_encoding(txUE, slsch_ue, frame_parms, harq_pid, G);
  }

  printf("\n");
>>>>>>> episys/master-sl

  #if 0
  unsigned int errors_bit_uncoded = 0;
  unsigned int errors_bit = 0;
  unsigned int n_errors = 0;
  unsigned int n_false_positive = 0;
  double modulated_input[HNA_SIZE];
  unsigned char test_input_bit[HNA_SIZE];
  short channel_output_fixed[HNA_SIZE];
  short channel_output_uncoded[HNA_SIZE];
  unsigned char estimated_output_bit[HNA_SIZE];
  snr1 = snr1set == 0 ? snr0 + 10 : snr1;
  for (double SNR = snr0; SNR < snr1; SNR += 0.2) {
    n_errors = 0;
    n_false_positive = 0;
    for (int trial = 0; trial < n_trials; trial++) {
      errors_bit_uncoded = 0;
<<<<<<< HEAD
      for (int i = 0; i < available_bits; i++) {
=======
      for (i = 0; i < available_bits; i++) {
>>>>>>> episys/master-sl
        if (slsch_ue->harq_processes[harq_pid]->f[i] == 0)
          modulated_input[i] = 1.0;        ///sqrt(2);  //QPSK
        else
          modulated_input[i] = -1.0;        ///sqrt(2);

        double SNR_lin = pow(10, SNR / 10.0);
        double sigma = 1.0 / sqrt(2 * SNR_lin);
        unsigned char qbits = 8;
        channel_output_fixed[i] = (short) quantize(sigma / 4.0 / 4.0,
                                                   modulated_input[i] + sigma * gaussdouble(0.0, 1.0),
                                                   qbits);
        if (channel_output_fixed[i] < 0)
          channel_output_uncoded[i] = 1;  //QPSK demod
        else
          channel_output_uncoded[i] = 0;
<<<<<<< HEAD
=======

>>>>>>> episys/master-sl
        if (channel_output_uncoded[i] != slsch_ue->harq_processes[harq_pid]->f[i])
          errors_bit_uncoded = errors_bit_uncoded + 1;
      }

      int frame = 0;
      int slot = 0;
      UE_nr_rxtx_proc_t proc;
      int ret = nr_dlsch_decoding(rxUE, &proc, 0, channel_output_fixed, &rxUE->frame_parms,
                                  slsch_ue_rx, slsch_ue_rx->harq_processes[0], frame, nb_symb_sch,
                                  slot, harq_pid, 0);

      if (ret)
        n_errors++;

      errors_bit = 0;
      for (int i = 0; i < TBS; i++) {
        test_input_bit[i] = (test_input[i / 8] & (1 << (i & 7))) >> (i & 7);
        if (estimated_output_bit[i] != test_input_bit[i]) {
          errors_bit++;
        }
      }

      printf("*****************************************\n");
      printf("SNR %f, BLER %f (false positive %f)\n", SNR,
            (float) n_errors / (float) n_trials,
            (float) n_false_positive / (float) n_trials);
      printf("*****************************************\n");
      printf("\n");

      if (n_errors == 0) {
        printf("PUSCH test OK\n");
        printf("\n");
        break;
      }
      printf("\n");
    }
  }
  #endif
  for (int sf = 0; sf < 2; sf++) {
    free_nr_ue_slsch(&txUE->slsch[sf][0], N_RB_UL, &txUE->frame_parms);
    free_nr_ue_dlsch(&rxUE->slsch_rx[sf][0][0], N_RB_DL);
  }
<<<<<<< HEAD
  term_nr_ue_transport(txUE);
  term_nr_ue_transport(rxUE);
  term_nr_ue_signal(rxUE, 1);
=======

  for (sf = 0; sf < 2; sf++)
    free_nr_ue_slsch(&txUE->slsch[sf][0], N_RB, frame_parms);

>>>>>>> episys/master-sl
  term_nr_ue_signal(txUE, 1);
  free(txUE);
  free(rxUE);

  free_channel_desc_scm(UE2UE);
  free(bw_setting);
<<<<<<< HEAD
=======

  if (output_fd)
    fclose(output_fd);

  if (input_fd)
    fclose(input_fd);
>>>>>>> episys/master-sl

  loader_reset();
  logTerm();
  return (0);
}
