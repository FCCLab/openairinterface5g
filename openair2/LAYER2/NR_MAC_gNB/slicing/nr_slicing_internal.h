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

/*!
 * \file   nr_slicing_internal.h
 * \brief  Internal NR slice helper functions
 * \author Robert Schmidt
 * \date   2021
 * \email  robert.schmidt@eurecom.fr
 */

#ifndef NR_SLICING_INTERNAL_H__
#define NR_SLICING_INTERNAL_H__

#include "nr_slicing.h"

void nr_slicing_add_UE(nr_slice_info_t *si, NR_UE_info_t *new_ue);

void nr_slicing_remove_UE(nr_slice_info_t *si, NR_UE_info_t* rm_ue, int idx);

void nr_slicing_move_UE(nr_slice_info_t *si, NR_UE_info_t* assoc_ue, int old_idx, int new_idx);

int nr_slicing_get_UE_slice_idx(nr_slice_info_t *si, rnti_t rnti);

int nr_slicing_get_UE_idx(nr_slice_t *si, rnti_t rnti);

nr_slice_t *_nr_add_slice(uint8_t *n, nr_slice_t **s);
nr_slice_t *_nr_remove_slice(uint8_t *n, nr_slice_t **s, int idx);

#endif /* NR_SLICING_INTERNAL_H__ */
