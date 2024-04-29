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

#include "f1ap_lib_common.h"
#include "f1ap_messages_types.h"

#include "OCTET_STRING.h"

#include "common/utils/assertions.h"

bool eq_f1ap_plmn(const f1ap_plmn_t *a, const f1ap_plmn_t *b)
{
  return a->mcc == b->mcc && a->mnc == b->mnc && a->mnc_digit_length == b->mnc_digit_length;
}

uint8_t *cp_octet_string(const OCTET_STRING_t *os, int *len)
{
  uint8_t *buf = calloc(os->size, sizeof(*buf));
  AssertFatal(buf != NULL, "out of memory\n");
  memcpy(buf, os->buf, os->size);
  *len = os->size;
  return buf;
}
