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

/*! \file f1ap_du_interface_management.c
 * \brief f1ap interface management for DU
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */

#include "f1ap_common.h"
#include "f1ap_encoder.h"
#include "f1ap_itti_messaging.h"
#include "f1ap_du_interface_management.h"
#include "lib/f1ap_interface_management.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_rrc_dl_handler.h"
#include "assertions.h"

#include "GNB_APP/gnb_paramdef.h"

int to_NRNRB(int nrb) {
  for (int i=0; i<sizeofArray(nrb_lut); i++)
    if (nrb_lut[i] == nrb)
      return i;

  if(!RC.nrrrc)
    return 0;

  AssertFatal(1==0,"nrb %d is not in the list of possible NRNRB\n",nrb);
}

/**
 * @brief F1AP Setup Request
 */
int DU_send_F1_SETUP_REQUEST(sctp_assoc_t assoc_id, const f1ap_setup_req_t *setup_req)
{
  F1AP_F1AP_PDU_t *pdu = encode_f1ap_setup_request(setup_req);
  uint8_t  *buffer;
  uint32_t  len;
  /* encode */
  if (f1ap_encode_pdu(pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 setup request\n");
    /* free PDU */
    ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, &pdu);
    return -1;
  }
  /* free PDU */
  ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, &pdu);
  /* Send to ITTI */
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

void dump_f1ap_setup_response(f1ap_setup_resp_t *resp)
{
  LOG_D(F1AP, "F1AP Setup Response: num_cells_to_activate = %d \n", resp->num_cells_to_activate);
  LOG_D(F1AP, "F1AP Setup Response: TransactionId %ld\n", resp->transaction_id);
  LOG_D(F1AP, "F1AP Setup Response: gNB_CU_name %s\n", resp->gNB_CU_name);
  if (resp->num_cells_to_activate) {
    for (int i = 0; i < resp->num_cells_to_activate; i++) {
      LOG_D(F1AP, "F1AP Setup Response: nr_cellid %ld\n", resp->cells_to_activate[i].nr_cellid);
      LOG_D(F1AP, "F1AP Setup Response: nrpci %d\n", resp->cells_to_activate[i].nrpci);
      LOG_D(F1AP, "F1AP Setup Response: plmn.mcc %d\n", resp->cells_to_activate[i].plmn.mcc);
      LOG_D(F1AP, "F1AP Setup Response: plmn.mnc %d\n", resp->cells_to_activate[i].plmn.mnc);
      LOG_D(F1AP, "F1AP Setup Response: num_SI %d\n", resp->cells_to_activate[i].num_SI);
    }
  }
}

int DU_handle_F1_SETUP_RESPONSE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  LOG_D(F1AP, "DU_handle_F1_SETUP_RESPONSE\n");
  /* Decode */
  f1ap_setup_resp_t resp;
  if (!decode_f1ap_setup_response(pdu, &resp)) {
    LOG_E(F1AP, "cannot decode F1AP Setup Request\n");
    free_f1ap_setup_response(&resp);
    return -1;
  }
  dump_f1ap_setup_response(&resp);
  LOG_D(F1AP, "Sending F1AP_SETUP_RESP ITTI message\n");
  f1_setup_response(&resp);
  free_f1ap_setup_response(&resp);
  return 0;
}

/**
 * @brief F1 Setup Failure handler (DU)
 */
int DU_handle_F1_SETUP_FAILURE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  f1ap_setup_failure_t fail;
  if (!decode_f1ap_setup_failure(pdu, &fail)) {
    LOG_E(F1AP, "Failed to decode F1AP Setup Failure\n");
    return -1;
  }
  f1_setup_failure(&fail);
  return 0;
}

/**
 * @brief F1 gNB-DU Configuration Update: encoding and ITTI transmission
 */
int DU_send_gNB_DU_CONFIGURATION_UPDATE(sctp_assoc_t assoc_id, f1ap_gnb_du_configuration_update_t *msg)
{
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  /* encode F1AP message */
  F1AP_F1AP_PDU_t *pdu = encode_f1ap_du_configuration_update(msg);
  /* encode F1AP pdu */
  if (f1ap_encode_pdu(pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 gNB-DU CONFIGURATION UPDATE\n");
    free_f1ap_du_configuration_update(msg);
    return -1;
  }
  /* transfer the message */
  ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

void dump_f1ap_gnb_cu_configuration_update(f1ap_gnb_cu_configuration_update_t *msg)
{
  LOG_D(F1AP, "F1 gNB-CU Configuration Update: TxId %ld, Activating %d cells", msg->transaction_id, msg->num_cells_to_activate);
  for (int i = 0; i < msg->num_cells_to_activate; i++) {
    LOG_D(F1AP,
          "F1 gNB-CU Configuration Update: Cell %d - MCC %d, MNC %d, NRCell 0x%lx, SI %d",
          i,
          msg->cells_to_activate[i].plmn.mcc,
          msg->cells_to_activate[i].plmn.mnc,
          msg->cells_to_activate[i].nr_cellid,
          msg->cells_to_activate[i].num_SI);
    for (int s = 0; s < msg->cells_to_activate[i].num_SI; s++) {
      LOG_D(F1AP,
            "F1 gNB-CU Configuration Update: SI[%d][%d] %d bytes",
            s,
            msg->cells_to_activate[i].SI_msg[s].SI_type,
            msg->cells_to_activate[i].SI_msg[s].SI_container_length);
    }
  }
}
/**
 * @brief F1 gNB-CU Configuration Update decoding and message transfer
 */
int DU_handle_gNB_CU_CONFIGURATION_UPDATE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  LOG_D(F1AP, "DU_handle_gNB_CU_CONFIGURATION_UPDATE\n");
  f1ap_gnb_cu_configuration_update_t in;
  if (!decode_f1ap_cu_configuration_update(pdu, &in)) {
    LOG_E(F1AP, "Failed to decode F1AP Setup Failure\n");
    free_f1ap_cu_configuration_update(&in);
    return -1;
  }
  dump_f1ap_gnb_cu_configuration_update(&in);
  MessageDef *msg_p = itti_alloc_new_message (TASK_DU_F1, 0, F1AP_GNB_CU_CONFIGURATION_UPDATE);
  f1ap_gnb_cu_configuration_update_t *msg = &F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p); // RRC thread will free it
  *msg = in; // copy F1 message to ITTI
  LOG_D(F1AP, "Sending F1AP_GNB_CU_CONFIGURATION_UPDATE ITTI message \n");
  itti_send_msg_to_task(TASK_GNB_APP, GNB_MODULE_ID_TO_INSTANCE(assoc_id), msg_p);
  free_f1ap_cu_configuration_update(&in);
  return 0;
}

int DU_send_gNB_CU_CONFIGURATION_UPDATE_FAILURE(sctp_assoc_t assoc_id,
    f1ap_gnb_cu_configuration_update_failure_t *GNBCUConfigurationUpdateFailure) {
  AssertFatal(1==0,"received gNB CU CONFIGURATION UPDATE FAILURE with cause %d\n",
              GNBCUConfigurationUpdateFailure->cause);
}

int DU_send_gNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(
    sctp_assoc_t assoc_id,
    f1ap_gnb_cu_configuration_update_acknowledge_t *GNBCUConfigurationUpdateAcknowledge)
{
  AssertFatal(GNBCUConfigurationUpdateAcknowledge->num_cells_failed_to_be_activated == 0,
              "%d cells failed to activate\n",
              GNBCUConfigurationUpdateAcknowledge->num_cells_failed_to_be_activated);
  AssertFatal(GNBCUConfigurationUpdateAcknowledge->noofTNLAssociations_to_setup == 0,
              "%d TNLAssociations to setup, handle this ...\n",
              GNBCUConfigurationUpdateAcknowledge->noofTNLAssociations_to_setup);
  AssertFatal(GNBCUConfigurationUpdateAcknowledge->noofTNLAssociations_failed == 0,
              "%d TNLAssociations failed\n",
              GNBCUConfigurationUpdateAcknowledge->noofTNLAssociations_failed);
  AssertFatal(GNBCUConfigurationUpdateAcknowledge->noofDedicatedSIDeliveryNeededUEs == 0,
              "%d DedicatedSIDeliveryNeededUEs\n",
              GNBCUConfigurationUpdateAcknowledge->noofDedicatedSIDeliveryNeededUEs);
  F1AP_F1AP_PDU_t           pdu= {0};
  uint8_t  *buffer=NULL;
  uint32_t  len=0;
  /* Create */
  /* 0. pdu Type */
  pdu.present = F1AP_F1AP_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_gNBCUConfigurationUpdate;
  tmp->criticality   = F1AP_Criticality_reject;
  tmp->value.present = F1AP_SuccessfulOutcome__value_PR_GNBCUConfigurationUpdateAcknowledge;
  F1AP_GNBCUConfigurationUpdateAcknowledge_t *out = &tmp->value.choice.GNBCUConfigurationUpdateAcknowledge;
  /* mandatory */
  /* c1. Transaction ID (integer value)*/
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_GNBCUConfigurationUpdateAcknowledgeIEs_t, ie);
  ie->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ie->criticality               = F1AP_Criticality_reject;
  ie->value.present             = F1AP_GNBCUConfigurationUpdateAcknowledgeIEs__value_PR_TransactionID;
  ie->value.choice.TransactionID = F1AP_get_next_transaction_identifier(0, 0);

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode GNB-DU-Configuration-Update-Acknowledge\n");
    return -1;
  }

  ASN_STRUCT_RESET(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}
