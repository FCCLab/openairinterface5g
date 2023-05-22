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

#ifndef MAC_RRC_DL_HANDLER_H
#define MAC_RRC_DL_HANDLER_H

#include "platform_types.h"
#include "f1ap_messages_types.h"

void ue_context_setup_request(const f1ap_ue_context_setup_t *req);

int dl_rrc_message(module_id_t module_id, const f1ap_dl_rrc_message_t *dl_rrc);

#endif /* MAC_RRC_DL_HANDLER_H */
