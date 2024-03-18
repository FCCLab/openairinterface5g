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

/*! \file open-nFAPI/fapi/inc/nr_fapi_p5.h
* \brief Defines the interface for packing/unpacking FAPI P5 messages (SCF 222)
* \author Ruben S. Silva
* \date 2024
* \version 0.1
* \company OpenAirInterface Software Alliance
* \email: contact@openairinterface.org, rsilva@allbesmart.pt
* \note
* \warning
 */

#ifndef OPENAIRINTERFACE_NR_FAPI_P5_H
#define OPENAIRINTERFACE_NR_FAPI_P5_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/wait.h>
#include <unistd.h>

#include "nfapi_nr_interface.h"
#include "nfapi_interface.h"
#include "nfapi_nr_interface_scf.h"
#include "nfapi.h"

#include "common/utils/LOG/log.h"
#include "debug.h"

uint8_t pack_nr_param_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config);
uint8_t unpack_nr_param_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config);
#endif // OPENAIRINTERFACE_NR_FAPI_P5_H
