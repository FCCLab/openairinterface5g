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
/*! \file nfapi/open-nFAPI/fapi/inc/nr_fapi_p5_utils.h
 * \brief
 * \author Ruben S. Silva
 * \date 2024
 * \version 0.1
 * \company OpenAirInterface Software Alliance
 * \email: contact@openairinterface.org, rsilva@allbesmart.pt
 * \note
 * \warning
 */

#ifndef OPENAIRINTERFACE_NR_FAPI_P5_UTILS_H
#define OPENAIRINTERFACE_NR_FAPI_P5_UTILS_H
#include "stdio.h"
#include "stdint.h"
#include "nr_fapi.h"

#define CMP(_a,_b) do{ if((_a) != (_b)){ return -1; } }while(0)

bool compare_param_request(const nfapi_nr_param_request_scf_t *unpacked_req, const nfapi_nr_param_request_scf_t *req);
bool compare_param_response(const nfapi_nr_param_response_scf_t *unpacked_req, const nfapi_nr_param_response_scf_t *req);

void free_param_request(nfapi_nr_param_request_scf_t *msg);
void free_param_response(nfapi_nr_param_response_scf_t *msg);

void copy_param_request(const nfapi_nr_param_request_scf_t *src,  nfapi_nr_param_request_scf_t *dst);
void copy_param_response(const nfapi_nr_param_response_scf_t *src,  nfapi_nr_param_response_scf_t *dst);

#endif // OPENAIRINTERFACE_NR_FAPI_P5_UTILS_H
