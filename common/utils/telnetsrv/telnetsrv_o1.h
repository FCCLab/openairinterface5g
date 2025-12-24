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

#ifndef TELNETSRV_O1_H_
#define TELNETSRV_O1_H_

#include <stdint.h>

/* Handover callback function type */
typedef void (*handover_callback_func_t)(const char *status,
                                          uint32_t ue_id,
                                          uint64_t amf_ue_ngap_id,
                                          uint64_t nr_cellid,
                                          uint32_t node_id,
                                          uint16_t pci,
                                          const char *source_gnb_id,
                                          uint64_t source_cellid,
                                          uint16_t source_pci,
                                          const char *failure_cause,
                                          const char *failure_reason);

#endif /* TELNETSRV_O1_H_ */

