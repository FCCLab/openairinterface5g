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

uint32_t* gold_cache(uint32_t key, int length)
{
  typedef struct {
    int key;
    int length;
    int usage;
    uint32_t* sequence;
  } gold_cache_t;
#define GOLD_CACHE_SZ PAGE_SIZE / sizeof(gold_cache_t)
  static __thread gold_cache_t table[GOLD_CACHE_SZ] = {0};
  static __thread int calls = 0;
  calls++;
  // check if already cached
  for (int i = 0; i < GOLD_CACHE_SZ; i++)
    if (table[i].length && table[i].key == key) {
      if (table[i].length >= length) {
        table[i].usage++;
        return table[i].sequence;
      } else {
        // cached, but too short, let's recompute it
        free(table[i].sequence);
        table[i].length = 0;
      }
    }

  // cleanup unused entries since last adding
  if (calls > GOLD_CACHE_SZ)
    for (int i = 0; i < GOLD_CACHE_SZ; i++) {
      if (table[i].length && !table[i].usage) {
        free(table[i].sequence);
        table[i].length = 0;
      }
      table[i].usage = 0;
    }
  calls = 0;

  // We will add a new entry
  for (int i = 0; i < GOLD_CACHE_SZ; i++)
    if (table[i].length == 0) {
      table[i].key = key;
      table[i].length = length;
      table[i].usage = 1;
      table[i].sequence = malloc(length * sizeof(*table[i].sequence));
      unsigned int x1 = 0, x2 = key;
      table[i].sequence[0] = lte_gold_generic(&x1, &x2, 1);
      for (int n = 1; n < length; n++)
        table[i].sequence[n] = lte_gold_generic(&x1, &x2, 0);
      LOG_D(PHY, "created a gold sequence, start %d; len %d\n", key, length);
      return table[i].sequence;
    }
  AssertFatal(PHY, "gold sequence table full\n");
}

uint32_t* nr_gold_pbch(int Lmax, int Nid, int n_hf, int l)
{
  int i_ssb = l & (Lmax - 1);
  int i_ssb2 = i_ssb + (n_hf << 2);
  uint32_t x2 = (1 << 11) * (i_ssb2 + 1) * ((Nid >> 2) + 1) + (1 << 6) * (i_ssb2 + 1) + (Nid & 3);
  return gold_cache(x2, NR_PBCH_DMRS_LENGTH_DWORD);
}

uint32_t* nr_gold_pdcch(int N_RB_DL, int symbols_per_slot, unsigned short nid, int ns, int l)
{
  int pdcch_dmrs_init_length = (((N_RB_DL << 1) * 3) >> 5) + 1;
  uint64_t x2tmp0 = ((symbols_per_slot * ns + l + 1) * ((nid << 1) + 1));
  x2tmp0 <<= 17;
  x2tmp0 += (nid << 1);
  uint32_t x2 = x2tmp0 % (1U << 31); // cinit
  LOG_D(PHY, "PDCCH DMRS slot %d, symb %d, Nid %d, x2 %x\n", ns, l, nid, x2);
  return gold_cache(x2, pdcch_dmrs_init_length);
}

uint32_t* nr_gold_pdsch(int N_RB_DL, int symbols_per_slot, int nid, int nscid, int slot, int symbol)
{
  int pdsch_dmrs_init_length = ((N_RB_DL * 24) >> 5) + 1;
  uint64_t x2tmp0 = ((symbols_per_slot * slot + symbol + 1) * ((nid << 1) + 1)) << 17;
  uint32_t x2 = (x2tmp0 + (nid << 1) + nscid) % (1U << 31); // cinit
  LOG_D(PHY, "UE DMRS slot %d, symb %d, nscid %d, x2 %x\n", slot, symbol, nscid, x2);
  return gold_cache(x2, pdsch_dmrs_init_length);
}

uint32_t* nr_gold_pusch(int N_RB_UL, int symbols_per_slot, int Nid, int nscid, int ns, int l)
{
  int pusch_dmrs_init_length = ((N_RB_UL * 12) >> 5) + 1;
  uint64_t temp_x2 = ((1UL << 17) * (symbols_per_slot * ns + l + 1) * ((Nid << 1) + 1) + ((Nid << 1) + nscid));
  uint32_t x2 = temp_x2 % (1U << 31);
  LOG_D(PHY, "DMRS slot %d, symb %d, nscid %d, nid %d, x2 %x\n", ns, l, nscid, Nid, x2);
  return gold_cache(x2, pusch_dmrs_init_length);
}

uint32_t* nr_gold_csi_rs(const NR_DL_FRAME_PARMS* fp, int slot, int symb, uint32_t Nid)
{
  int csi_dmrs_init_length =  ((fp->N_RB_DL<<4)>>5)+1;
  uint32_t x2 = ((1<<10) * (fp->symbols_per_slot*slot+symb+1) * ((Nid<<1)+1) + (Nid));
  return gold_cache(x2, csi_dmrs_init_length);
}

uint32_t* nr_gold_prs(int Nid, int slotNum, int symNum)
{
  LOG_I(PHY, "Initialised NR-PRS sequence for PCI %d\n", Nid);
  // initial x2 for prs as ts138.211
  uint32_t pow22 = 1 << 22;
  uint32_t pow10 = 1 << 10;
  uint32_t c_init1 = pow22 * ceil(Nid / 1024);
  uint32_t c_init2 = pow10 * (slotNum + symNum + 1) * (2 * (Nid % 1024) + 1);
  uint32_t c_init3 = Nid % 1024;
  uint32_t x2 = c_init1 + c_init2 + c_init3;
  return gold_cache(x2, NR_MAX_PRS_INIT_LENGTH_DWORD);
}
