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

#include "refsig_defs_ue.h"
#include "nr_refsig.h"
#include "nr_refsig_common.h"

uint32_t* nr_gold_pbch(PHY_VARS_NR_UE* ue, int n_hf, int l)
{
  int Nid = ue->frame_parms.Nid_cell;
  int Lmax = ue->frame_parms.Lmax;
  int i_ssb = l & (Lmax - 1);
  int i_ssb2 = i_ssb + (n_hf << 2);
  uint32_t x2 = (1 << 11) * (i_ssb2 + 1) * ((Nid >> 2) + 1) + (1 << 6) * (i_ssb2 + 1) + (Nid & 3);
  return gold_cache(x2, NR_PBCH_DMRS_LENGTH_DWORD);
}

uint32_t* nr_gold_pdcch(PHY_VARS_NR_UE* ue, unsigned short nid, int ns, int l)
{
  int pdcch_dmrs_init_length = (((ue->frame_parms.N_RB_DL << 1) * 3) >> 5) + 1;
  uint64_t x2tmp0 = ((ue->frame_parms.symbols_per_slot * ns + l + 1) * ((nid << 1) + 1));
  x2tmp0 <<= 17;
  x2tmp0 += (nid << 1);
  uint32_t x2 = x2tmp0 % (1U << 31); // cinit
  LOG_D(PHY, "PDCCH DMRS slot %d, symb %d, Nid %d, x2 %x\n", ns, l, nid, x2);
  return gold_cache(x2, pdcch_dmrs_init_length);
}

uint32_t* nr_gold_pdsch(PHY_VARS_NR_UE* ue, int nscid, int slot, int symbol)
{
  int pdsch_dmrs_init_length = ((ue->frame_parms.N_RB_DL * 12) >> 5) + 1;
  int nid = ue->frame_parms.Nid_cell;
  uint64_t x2tmp0 = ((ue->frame_parms.symbols_per_slot * slot + symbol + 1) * ((nid << 1) + 1)) << 17;
  uint32_t x2 = (x2tmp0 + (nid << 1) + nscid) % (1U << 31); // cinit
  LOG_D(PHY, "UE DMRS slot %d, symb %d, nscid %d, x2 %x\n", slot, symbol, nscid, x2);
  return gold_cache(x2, pdsch_dmrs_init_length);
}

uint32_t* nr_init_pusch_dmrs(PHY_VARS_NR_UE* ue, uint n_scid, uint N_n_scid, int slot, int symb)
{
  NR_DL_FRAME_PARMS* fp = &ue->frame_parms;
  int pusch_dmrs_init_length = ((fp->N_RB_UL * 12) >> 5) + 1;
  uint64_t t_x2 = ((1UL << 17) * (fp->symbols_per_slot * slot + symb + 1) * ((N_n_scid << 1) + 1) + ((N_n_scid << 1) + n_scid));
  uint32_t x2 = t_x2 % (1U << 31);
  LOG_D(PHY, "DMRS slot %d, symb %d, N_n_scid %d, n_scid %d, x2 %x\n", slot, symb, N_n_scid, n_scid, x2);
  return gold_cache(x2, pusch_dmrs_init_length);
}

void sl_init_psbch_dmrs_gold_sequences(PHY_VARS_NR_UE *UE)
{
  unsigned int x1, x2;
  uint16_t slss_id;
  uint8_t reset;

  for (slss_id = 0; slss_id < SL_NR_NUM_SLSS_IDs; slss_id++) {
    reset = 1;
    x2 = slss_id;

#ifdef SL_DEBUG_INIT
    printf("\nPSBCH DMRS GOLD SEQ for SLSSID :%d  :\n", slss_id);
#endif

    for (uint8_t n = 0; n < SL_NR_NUM_PSBCH_DMRS_RE_DWORD; n++) {
      UE->SL_UE_PHY_PARAMS.init_params.psbch_dmrs_gold_sequences[slss_id][n] = lte_gold_generic(&x1, &x2, reset);
      reset = 0;

#ifdef SL_DEBUG_INIT_DATA
      printf("%x\n", SL_UE_INIT_PARAMS.sl_psbch_dmrs_gold_sequences[slss_id][n]);
#endif
    }
  }
}
