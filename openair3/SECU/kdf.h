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

<<<<<<< HEAD:openair3/SECU/kdf.h
#ifndef KEY_DERIVATION_FUNCTION_OSA_H
#define KEY_DERIVATION_FUNCTION_OSA_H

#include <stdint.h>
#include <stdlib.h>
#include "byte_array.h"

void kdf(const uint8_t key[32], byte_array_t data, size_t len, uint8_t out[len]);

#endif


=======
#include <stdio.h>
#include <stdint.h>
#define NB_R  3
void nrLDPC_cnProc_BG1_generator_AVX512(const char *, int);
void nrLDPC_cnProc_BG2_generator_AVX512(const char *, int);

const char *__asan_default_options()
{
  /* don't do leak checking in nr_ulsim, creates problems in the CI */
  return "detect_leaks=0";
}

int main(int argc, char *argv[])
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s <output-dir>\n", argv[0]);
    return 1;
  }
  const char *dir = argv[1];

  int R[NB_R]={0,1,2};
  for(int i=0; i<NB_R;i++){
    nrLDPC_cnProc_BG1_generator_AVX512(dir, R[i]);
    nrLDPC_cnProc_BG2_generator_AVX512(dir, R[i]);
  }

  return(0);
}
>>>>>>> 41ca5b1258... Rebasing openair1 folder:openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_tools/generator_cnProc_avx512/main.c

