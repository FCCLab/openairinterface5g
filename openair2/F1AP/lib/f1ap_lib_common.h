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

#ifndef F1AP_LIB_COMMON_H_
#define F1AP_LIB_COMMON_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "openair3/UTILS/conversions.h"

#define CHECK_F1AP_CONDITION(condition)                                       \
  do {                                                                        \
    if (!(condition)) {                                                       \
      fprintf(stderr, "Invalid message: condition %s failed!\n", #condition); \
      return NULL;                                                            \
    }                                                                         \
  } while (0)

#define CHECK_IE_CONDITION(condition)                                    \
  do {                                                                   \
    if (!(condition)) {                                                  \
      fprintf(stderr, "Invalid IE: condition %s failed!\n", #condition); \
      return false;                                                      \
    }                                                                    \
  } while (0)

#define CHECK_CRITICALITY_REJECT(criticality)                                                           \
  do {                                                                                                  \
    if (criticality != F1AP_Criticality_reject) {                                                       \
      fprintf(stderr, "Invalid criticality, expected F1AP_Criticality_reject, got %ld\n", criticality); \
      return false;                                                                                     \
    }                                                                                                   \
  } while (0)

#define CHECK_AND_RETURN_FALSE(condition, fmt, ...) \
  do { \
    if (!(condition)) { \
      fprintf(stderr, "Condition failed %s:line %d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
      return false; \
    } \
  } while (0)

/* macro to look up IE. If mandatory and not found, macro will print
 * descriptive debug message to stderr and force exit in calling function */
#define F1AP_LIB_FIND_IE(IE_TYPE, ie, container, IE_ID, mandatory)                                   \
  do {                                                                                               \
    ie = NULL;                                                                                       \
    for (IE_TYPE **ptr = container->protocolIEs.list.array;                                          \
         ptr < &container->protocolIEs.list.array[container->protocolIEs.list.count];                \
         ptr++) {                                                                                    \
      if ((*ptr)->id == IE_ID) {                                                                     \
        ie = *ptr;                                                                                   \
        break;                                                                                       \
      }                                                                                              \
    }                                                                                                \
    if (mandatory && ie == NULL) {                                                                   \
      fprintf(stderr, "%s(): could not find element " #IE_ID " with type " #IE_TYPE "\n", __func__); \
      return false;                                                                                  \
    }                                                                                                \
  } while (0)

#define addnRCGI(nRCGi, servedCelL)                      \
  MCC_MNC_TO_PLMNID((servedCelL)->plmn.mcc,              \
                    (servedCelL)->plmn.mnc,              \
                    (servedCelL)->plmn.mnc_digit_length, \
                    &((nRCGi).pLMN_Identity));           \
  NR_CELL_ID_TO_BIT_STRING((servedCelL)->nr_cellid, &((nRCGi).nRCellIdentity));

struct f1ap_plmn_t;
bool eq_f1ap_plmn(const struct f1ap_plmn_t *a, const struct f1ap_plmn_t *b);
struct f1ap_served_cell_info_t;
bool eq_f1ap_cell_info(const struct f1ap_served_cell_info_t *a, const struct f1ap_served_cell_info_t *b);
struct f1ap_gnb_du_system_info_t;
bool eq_f1ap_sys_info(const struct f1ap_gnb_du_system_info_t *a, const struct f1ap_gnb_du_system_info_t *b);
struct f1ap_nr_frequency_info_t;
bool eq_f1ap_freq_info(const struct f1ap_nr_frequency_info_t *a, const struct f1ap_nr_frequency_info_t *b);
struct f1ap_transmission_bandwidth_t;
bool eq_f1ap_tx_bandwidth(const struct f1ap_transmission_bandwidth_t *a, const struct f1ap_transmission_bandwidth_t *b);

struct OCTET_STRING;
uint8_t *cp_octet_string(const struct OCTET_STRING *os, int *len);

#endif /* F1AP_LIB_COMMON_H_ */
