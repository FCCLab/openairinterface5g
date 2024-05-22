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

uint32_t *nr_init_csi_rs(const NR_DL_FRAME_PARMS *fp, int slot, int symb, uint32_t Nid)
{
  int csi_dmrs_init_length =  ((fp->N_RB_DL<<4)>>5)+1;
  uint32_t x2 = ((1<<10) * (fp->symbols_per_slot*slot+symb+1) * ((Nid<<1)+1) + (Nid));
  return gold_cache(x2, csi_dmrs_init_length);
}

uint32_t* init_nr_gold_prs(int Nid, int slotNum, int symNum)
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
