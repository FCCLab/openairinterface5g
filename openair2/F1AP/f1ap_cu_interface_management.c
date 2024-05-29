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

/*! \file f1ap_cu_interface_management.c
 * \brief f1ap interface management for CU
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
#include "f1ap_cu_interface_management.h"
#include "f1ap_default_values.h"
#include "lib/f1ap_interface_management.h"

void dump_setup_req(f1ap_setup_req_t *msg)
{
  LOG_D(F1AP, "out->transaction_id %lu \n", msg->transaction_id);
  LOG_D(F1AP, "out->gNB_DU_id %lu \n", msg->gNB_DU_id);
  LOG_D(F1AP, "out->gNB_DU_name %s \n", msg->gNB_DU_name);
  LOG_D(F1AP, "out->num_cells_available %d \n", msg->num_cells_available);
  for (int i = 0; i < msg->num_cells_available; i++) {
    LOG_D(F1AP, "out->tac[%d] %d \n", i, *msg->cell[i].info.tac);
    LOG_D(F1AP,
          "Received nRCGI: MCC %d, MNC %d, CELL_ID %llu\n",
          msg->cell[i].info.plmn.mcc,
          msg->cell[i].info.plmn.mnc,
          (long long unsigned int)msg->cell[i].info.nr_cellid);
    LOG_D(F1AP, "out->nr_pci[%d] %d \n", i, msg->cell[i].info.nr_pci);
  }
}

/**
 * @brief F1AP Setup Request decoding (9.2.1.4 of 3GPP TS 38.473) and transfer to RRC
 */
int CU_handle_F1_SETUP_REQUEST(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  LOG_D(F1AP, "CU_handle_F1_SETUP_REQUEST\n");
  DevAssert(pdu != NULL);
  /* F1 Setup Request == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    LOG_W(F1AP, "[SCTP %d] Received f1 setup request on stream != 0 (%d)\n",
          assoc_id, stream);
  }
  f1ap_setup_req_t msg;
  /* Decode */
  if (!decode_f1ap_setup_request(pdu, &msg)) {
    LOG_E(F1AP, "cannot decode F1AP Setup Request\n");
    free_f1ap_setup_request(&msg);
    return -1;
  }
  dump_setup_req(&msg);
  /* Send to RRC (ITTI) */
  MessageDef *message_p = itti_alloc_new_message(TASK_CU_F1, 0, F1AP_SETUP_REQ);
  message_p->ittiMsgHeader.originInstance = assoc_id;
  f1ap_setup_req_t *req = &F1AP_SETUP_REQ(message_p);
  *req = msg; /* "move" message into ITTI, RRC thread will free it */
  free_f1ap_setup_request(&msg);
  itti_send_msg_to_task(TASK_RRC_GNB, GNB_MODULE_ID_TO_INSTANCE(instance), message_p);
  return 0;
}

/**
 * @brief F1AP Setup Response encoding (9.2.1.5 of 3GPP TS 38.473) and message transfer
 */
int CU_send_F1_SETUP_RESPONSE(sctp_assoc_t assoc_id, f1ap_setup_resp_t *f1ap_setup_resp)
{
  uint8_t  *buffer=NULL;
  uint32_t len = 0;

  /* Encode F1 Setup Response */
  F1AP_F1AP_PDU_t *pdu = encode_f1ap_setup_response(f1ap_setup_resp);

  /* encode */
  if (f1ap_encode_pdu(pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 setup response\n");
    ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, &pdu);
    return -1;
  }
  ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

/**
 * @brief F1 Setup Failure encoding and transmission
 */
int CU_send_F1_SETUP_FAILURE(sctp_assoc_t assoc_id, const f1ap_setup_failure_t *fail)
{
  LOG_D(F1AP, "CU_send_F1_SETUP_FAILURE\n");
  uint8_t *buffer = NULL;
  uint32_t len = 0;
  /* Encode F1 Setup Failure */
  F1AP_F1AP_PDU_t *pdu = encode_f1ap_setup_failure(fail);
  if (f1ap_encode_pdu(pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 setup failure\n");
    ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, &pdu);
    return -1;
  }
  ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

static void dump_f1ap_du_configuration_update(f1ap_gnb_du_configuration_update_t *out)
{
  LOG_D(F1AP, "F1 gNB-DU Configuration Update: out->transaction_id %lu \n", out->transaction_id);
  /* to add */
  LOG_D(F1AP, "F1 gNB-DU Configuration Update: out->num_cells_to_add %d \n", out->num_cells_to_add);
  for (int i = 0; i < out->num_cells_to_add; i++) {
    LOG_D(F1AP, "out->tac[%d] %d \n", i, *out->cell_to_add[i].info.tac);
    LOG_D(F1AP,
          "F1 gNB-DU Configuration Update: nRCGI to add = MCC %d, MNC %d, CELL_ID %llu\n",
          out->cell_to_add[i].info.plmn.mcc,
          out->cell_to_add[i].info.plmn.mnc,
          (long long unsigned int)out->cell_to_add[i].info.nr_cellid);
    LOG_D(F1AP, "F1 gNB-DU Configuration Update: out->nr_pci[%d] %d \n", i, out->cell_to_add[i].info.nr_pci);
  }
  /* to modify */
  LOG_D(F1AP, "F1 gNB-DU Configuration Update: out->num_cells_to_modify %d \n", out->num_cells_to_modify);
  for (int i = 0; i < out->num_cells_to_modify; i++) {
    LOG_D(F1AP, "out->tac[%d] %d \n", i, *out->cell_to_modify[i].info.tac);
    LOG_D(F1AP,
          "F1 gNB-DU Configuration Update: nRCGI to modify = MCC %d, MNC %d, CELL_ID %llu\n",
          out->cell_to_modify[i].info.plmn.mcc,
          out->cell_to_modify[i].info.plmn.mnc,
          (long long unsigned int)out->cell_to_modify[i].info.nr_cellid);
    LOG_D(F1AP, "out->nr_pci[%d] %d \n", i, out->cell_to_modify[i].info.nr_pci);
  }
  /* to delete */
  LOG_D(F1AP, "F1 gNB-DU Configuration Update: out->num_cells_to_delete %d \n", out->num_cells_to_delete);
  for (int i = 0; i < out->num_cells_to_delete; i++) {
    LOG_D(F1AP,
          "F1 gNB-DU Configuration Update: nRCGI to delete = MCC %d, MNC %d, CELL_ID %llu\n",
          out->cell_to_delete[i].plmn.mcc,
          out->cell_to_delete[i].plmn.mnc,
          (long long unsigned int)out->cell_to_delete[i].nr_cellid);
  }
}

/**
 * @brief Decode and send F1 gNB-DU Configuration Update message to RRC
 */
int CU_handle_gNB_DU_CONFIGURATION_UPDATE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  LOG_D(F1AP, "[SCTP %d] CU_handle_gNB_DU_CONFIGURATION_UPDATE\n", assoc_id);
  DevAssert(pdu != NULL);
  /* gNB DU Configuration Update == Non UE-related procedure -> stream 0 */
  if (stream != 0) {
    LOG_W(F1AP, "[SCTP %d] Received f1 setup request on stream != 0 (%d)\n", assoc_id, stream);
  }
  /* Decode */
  f1ap_gnb_du_configuration_update_t msg;
  if (!decode_f1ap_du_configuration_update(pdu, &msg)) {
    LOG_E(F1AP, "cannot decode F1AP gNB-DU Configuration Update\n");
    free_f1ap_du_configuration_update(&msg);
    return -1;
  }
  dump_f1ap_du_configuration_update(&msg);
  /* Send to RRC */
  MessageDef *message_p = itti_alloc_new_message(TASK_CU_F1, 0, F1AP_GNB_DU_CONFIGURATION_UPDATE);
  message_p->ittiMsgHeader.originInstance = assoc_id;
  f1ap_gnb_du_configuration_update_t *req = &F1AP_GNB_DU_CONFIGURATION_UPDATE(message_p); // RRC thread will free it
  *req = msg; // copy F1 message to ITTI
  free_f1ap_du_configuration_update(&msg);
  LOG_D(F1AP, "Sending F1AP_GNB_DU_CONFIGURATION_UPDATE ITTI message \n");
  itti_send_msg_to_task(TASK_RRC_GNB, GNB_MODULE_ID_TO_INSTANCE(instance), message_p);
  return 0;
}

int CU_send_gNB_DU_CONFIGURATION_UPDATE_ACKNOWLEDGE(
    sctp_assoc_t assoc_id,
    f1ap_gnb_du_configuration_update_acknowledge_t *GNBDUConfigurationUpdateAcknowledge)
{
  F1AP_F1AP_PDU_t pdu = {};
  uint8_t *buffer;
  uint32_t len;

  /* Create */
  /* 0. Message */

  pdu.present = F1AP_F1AP_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, succOut);
  succOut->procedureCode = F1AP_ProcedureCode_id_gNBDUConfigurationUpdate;
  succOut->criticality = F1AP_Criticality_reject;
  succOut->value.present = F1AP_SuccessfulOutcome__value_PR_GNBDUConfigurationUpdateAcknowledge;
  F1AP_GNBDUConfigurationUpdateAcknowledge_t *ack = &succOut->value.choice.GNBDUConfigurationUpdateAcknowledge;

  /* Mandatory */
  /* Transaction Id */
  asn1cSequenceAdd(ack->protocolIEs.list, F1AP_GNBDUConfigurationUpdateAcknowledgeIEs_t, ie1);
  ie1->id = F1AP_ProtocolIE_ID_id_TransactionID;
  ie1->criticality = F1AP_Criticality_reject;
  ie1->value.present = F1AP_GNBDUConfigurationUpdateAcknowledgeIEs__value_PR_TransactionID;
  ie1->value.choice.TransactionID = GNBDUConfigurationUpdateAcknowledge->transaction_id;

  /* Todo add optional fields */
  /* encode */

  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 gNB-DU CONFIGURATION UPDATE\n");
    return -1;
  }

  LOG_DUMPMSG(F1AP, LOG_DUMP_CHAR, buffer, len, "F1AP gNB-DU CONFIGURATION UPDATE : ");
  ASN_STRUCT_RESET(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

/*
    gNB-CU Configuration Update
*/

int CU_send_gNB_CU_CONFIGURATION_UPDATE(sctp_assoc_t assoc_id, f1ap_gnb_cu_configuration_update_t *f1ap_gnb_cu_configuration_update)
{
  F1AP_F1AP_PDU_t                    pdu= {0};
  uint8_t  *buffer;
  uint32_t  len;
  /* Create */
  /* 0. Message Type */
  pdu.present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, initMsg);
  initMsg->procedureCode = F1AP_ProcedureCode_id_gNBCUConfigurationUpdate;
  initMsg->criticality   = F1AP_Criticality_reject;
  initMsg->value.present = F1AP_InitiatingMessage__value_PR_GNBCUConfigurationUpdate;
  F1AP_GNBCUConfigurationUpdate_t *cfgUpdate = &pdu.choice.initiatingMessage->value.choice.GNBCUConfigurationUpdate;
  /* mandatory */
  /* c1. Transaction ID (integer value) */
  asn1cSequenceAdd(cfgUpdate->protocolIEs.list, F1AP_GNBCUConfigurationUpdateIEs_t, ieC1);
  ieC1->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ieC1->criticality               = F1AP_Criticality_reject;
  ieC1->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_TransactionID;
  ieC1->value.choice.TransactionID = F1AP_get_next_transaction_identifier(0, 0);

  // mandatory
  // c2. Cells_to_be_Activated_List
  if (f1ap_gnb_cu_configuration_update->num_cells_to_activate > 0) {
    asn1cSequenceAdd(cfgUpdate->protocolIEs.list, F1AP_GNBCUConfigurationUpdateIEs_t, ieC3);
    ieC3->id                        = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List;
    ieC3->criticality               = F1AP_Criticality_reject;
    ieC3->value.present             = F1AP_GNBCUConfigurationUpdateIEs__value_PR_Cells_to_be_Activated_List;

    for (int i=0; i<f1ap_gnb_cu_configuration_update->num_cells_to_activate; i++) {
      asn1cSequenceAdd(ieC3->value.choice.Cells_to_be_Activated_List.list,F1AP_Cells_to_be_Activated_List_ItemIEs_t,
                       cells_to_be_activated_ies);
      cells_to_be_activated_ies->id = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item;
      cells_to_be_activated_ies->criticality = F1AP_Criticality_reject;
      cells_to_be_activated_ies->value.present = F1AP_Cells_to_be_Activated_List_ItemIEs__value_PR_Cells_to_be_Activated_List_Item;
      // 2.1 cells to be Activated list item
      F1AP_Cells_to_be_Activated_List_Item_t *cells_to_be_activated_list_item=
        &cells_to_be_activated_ies->value.choice.Cells_to_be_Activated_List_Item;
      // - nRCGI
      addnRCGI(cells_to_be_activated_list_item->nRCGI, f1ap_gnb_cu_configuration_update->cells_to_activate+i);
      // optional
      // -nRPCI
      asn1cCalloc(cells_to_be_activated_list_item->nRPCI, tmp);
      *tmp = f1ap_gnb_cu_configuration_update->cells_to_activate[i].nrpci;  // int 0..1007
      // optional
      // 3.1.2 gNB-CUSystem Information
      F1AP_ProtocolExtensionContainer_10696P112_t *p = calloc(1,sizeof(*p));
      cells_to_be_activated_list_item->iE_Extensions = (struct F1AP_ProtocolExtensionContainer *) p;
      //F1AP_ProtocolExtensionContainer_154P112_t
      asn1cSequenceAdd(p->list, F1AP_Cells_to_be_Activated_List_ItemExtIEs_t,  cells_to_be_activated_itemExtIEs);
      cells_to_be_activated_itemExtIEs->id                     = F1AP_ProtocolIE_ID_id_gNB_CUSystemInformation;
      cells_to_be_activated_itemExtIEs->criticality            = F1AP_Criticality_reject;
      cells_to_be_activated_itemExtIEs->extensionValue.present = F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_GNB_CUSystemInformation;

      if (f1ap_gnb_cu_configuration_update->cells_to_activate[i].num_SI > 0) {
        F1AP_GNB_CUSystemInformation_t *gNB_CUSystemInformation =
          &cells_to_be_activated_itemExtIEs->extensionValue.choice.GNB_CUSystemInformation;
        //LOG_I(F1AP, "%s() SI %d size %d: ", __func__, i, f1ap_setup_resp->SI_container_length[i][0]);
        //for (int n = 0; n < f1ap_setup_resp->SI_container_length[i][0]; n++)
        //  printf("%02x ", f1ap_setup_resp->SI_container[i][0][n]);
        //printf("\n");

        // for (int sIBtype=2;sIBtype<33;sIBtype++) { //21 ? 33 ?
        for (int j = 0; j < sizeofArray(f1ap_gnb_cu_configuration_update->cells_to_activate[i].SI_msg); j++) {
          if (f1ap_gnb_cu_configuration_update->cells_to_activate[i].SI_msg[j].SI_container != NULL) {
            asn1cSequenceAdd(gNB_CUSystemInformation->sibtypetobeupdatedlist.list, F1AP_SibtypetobeupdatedListItem_t, sib_item);
            sib_item->sIBtype = f1ap_gnb_cu_configuration_update->cells_to_activate[i].SI_msg[j].SI_type;
            AssertFatal(sib_item->sIBtype < 6 || sib_item->sIBtype == 9, "Illegal SI type %ld\n", sib_item->sIBtype);
            OCTET_STRING_fromBuf(&sib_item->sIBmessage,
                                 (const char *)f1ap_gnb_cu_configuration_update->cells_to_activate[i].SI_msg[j].SI_container,
                                 f1ap_gnb_cu_configuration_update->cells_to_activate[i].SI_msg[j].SI_container_length);
            LOG_D(F1AP,
                  "f1ap_setup_resp->SI_container_length[%d][sIBtype %ld] = %d \n",
                  i,
                  sib_item->sIBtype,
                  f1ap_gnb_cu_configuration_update->cells_to_activate[i].SI_msg[j].SI_container_length);
          }
        }
      }
    }
  }

  /* encode */
  if (f1ap_encode_pdu(&pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 gNB-CU CONFIGURATION UPDATE\n");
    return -1;
  }

  LOG_DUMPMSG(F1AP, LOG_DUMP_CHAR, buffer, len, "F1AP gNB-CU CONFIGURATION UPDATE : ");
  ASN_STRUCT_RESET(asn_DEF_F1AP_F1AP_PDU, &pdu);
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

int CU_handle_gNB_CU_CONFIGURATION_UPDATE_ACKNOWLEDGE(instance_t instance,
                                                      sctp_assoc_t assoc_id,
                                                      uint32_t stream,
                                                      F1AP_F1AP_PDU_t *pdu)
{
  LOG_I(F1AP,"Cell Configuration ok (assoc_id %d)\n",assoc_id);
  return(0);
}
