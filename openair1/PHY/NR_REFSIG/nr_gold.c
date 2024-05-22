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

#include "nr_refsig.h"
#include "nr_refsig_common.h"

uint32_t* nr_init_pbch_dmrs(PHY_VARS_gNB* gNB, int n_hf, int ssb)
{
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  int Nid = gNB->gNB_config.cell_config.phy_cell_id.value;

  int Lmax = fp->Lmax;
  int i_ssb = ssb & (Lmax - 1);
  int i_ssb2 = i_ssb + (n_hf << 2);
  uint32_t x2 = (1 << 11) * (i_ssb2 + 1) * ((Nid >> 2) + 1) + (1 << 6) * (i_ssb2 + 1) + (Nid & 3);
  return gold_cache(x2, NR_PBCH_DMRS_LENGTH_DWORD);
}

uint32_t* nr_init_pdcch_dmrs(PHY_VARS_gNB* gNB, int Nid, int slot, int symb)
{
  NR_DL_FRAME_PARMS* fp = &gNB->frame_parms;
  int pdcch_dmrs_init_length = (((fp->N_RB_DL << 1) * 3) >> 5) + 1;
  uint64_t temp_x2 = ((1UL << 17) * (fp->symbols_per_slot * slot + symb + 1) * ((Nid << 1) + 1) + (Nid << 1));
  uint32_t x2 = temp_x2 % (1U << 31);
  LOG_D(PHY, "PDCCH DMRS slot %d, symb %d, Nid %d, x2 %x\n", slot, symb, Nid, x2);
  return gold_cache(x2, pdcch_dmrs_init_length);
}

uint32_t* nr_gold_pdsch_dmrs(PHY_VARS_gNB* gNB, uint nscid, int Nid, int slot, int symb)
{
  NR_DL_FRAME_PARMS* fp = &gNB->frame_parms;
  int pdsch_dmrs_init_length = ((fp->N_RB_DL * 24) >> 5) + 1;
  uint64_t temp_x2 = ((1UL << 17) * (fp->symbols_per_slot * slot + symb + 1) * ((Nid << 1) + 1) + ((Nid << 1) + nscid));
  uint32_t x2 = temp_x2 % (1U << 31);
  LOG_D(PHY, "PDSCH DMRS slot %d, symb %d, Nid %d, nscid %d, x2 %x\n", slot, symb, Nid, nscid, x2);
  return gold_cache(x2, pdsch_dmrs_init_length);
}

uint32_t* nr_gold_pusch(PHY_VARS_gNB* gNB, int nscid, int ns, int l)
{
  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  int Nid = gNB->gNB_config.cell_config.phy_cell_id.value;
  int pusch_dmrs_init_length = ((fp->N_RB_UL * 12) >> 5) + 1;
  uint64_t temp_x2 = ((1UL << 17) * (fp->symbols_per_slot * ns + l + 1) * ((Nid << 1) + 1) + ((Nid << 1) + nscid));
  uint32_t x2 = temp_x2 % (1U << 31);
  LOG_D(PHY, "DMRS slot %d, symb %d, nscid %d, nid %d, x2 %x\n", ns, l, nscid, Nid, x2);
  return gold_cache(x2, pusch_dmrs_init_length);
}

uint32_t* nr_init_prs(PHY_VARS_gNB* gNB, int rsc_id, int slotNum, int symNum)
{
  int Nid = gNB->prs_vars.prs_cfg[rsc_id].NPRSID; // seed value
  LOG_I(PHY, "Initiaized NR-PRS sequence with PRS_ID %3d for resource %d\n", Nid, rsc_id);
  uint32_t pow22 = 1 << 22;
  uint32_t pow10 = 1 << 10;
  uint32_t c_init1 = pow22 * ceil(Nid / 1024);
  uint32_t c_init2 = pow10 * (slotNum + symNum + 1) * (2 * (Nid % 1024) + 1);
  uint32_t c_init3 = Nid % 1024;
  uint32_t x2 = c_init1 + c_init2 + c_init3;
  return gold_cache(x2, NR_MAX_PRS_INIT_LENGTH_DWORD);
}


