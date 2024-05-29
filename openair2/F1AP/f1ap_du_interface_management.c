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

int DU_handle_gNB_CU_CONFIGURATION_UPDATE(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  LOG_D(F1AP, "DU_handle_gNB_CU_CONFIGURATION_UPDATE\n");
  AssertFatal(pdu->present == F1AP_F1AP_PDU_PR_initiatingMessage,
              "pdu->present != F1AP_F1AP_PDU_PR_initiatingMessage\n");
  AssertFatal(pdu->choice.initiatingMessage->procedureCode  == F1AP_ProcedureCode_id_gNBCUConfigurationUpdate,
              "pdu->choice.initiatingMessage->procedureCode != F1AP_ProcedureCode_id_gNBCUConfigurationUpdate\n");
  AssertFatal(pdu->choice.initiatingMessage->criticality  == F1AP_Criticality_reject,
              "pdu->choice.initiatingMessage->criticality != F1AP_Criticality_reject\n");
  AssertFatal(pdu->choice.initiatingMessage->value.present  == F1AP_InitiatingMessage__value_PR_GNBCUConfigurationUpdate,
              "pdu->choice.initiatingMessage->value.present != F1AP_InitiatingMessage__value_PR_GNBCUConfigurationUpdate\n");
  F1AP_GNBCUConfigurationUpdate_t *in = &pdu->choice.initiatingMessage->value.choice.GNBCUConfigurationUpdate;
  F1AP_GNBCUConfigurationUpdateIEs_t *ie;
  int TransactionId = -1;
  int num_cells_to_activate = 0;
  F1AP_Cells_to_be_Activated_List_Item_t *cell;
  MessageDef *msg_p = itti_alloc_new_message (TASK_DU_F1, 0, F1AP_GNB_CU_CONFIGURATION_UPDATE);
  LOG_D(F1AP, "F1AP: gNB_CU_Configuration_Update: protocolIEs.list.count %d\n",
        in->protocolIEs.list.count);

  for (int i=0; i < in->protocolIEs.list.count; i++) {
    ie = in->protocolIEs.list.array[i];

    switch (ie->id) {
      case F1AP_ProtocolIE_ID_id_TransactionID:
        AssertFatal(ie->criticality == F1AP_Criticality_reject,
                    "ie->criticality != F1AP_Criticality_reject\n");
        AssertFatal(ie->value.present == F1AP_GNBCUConfigurationUpdateIEs__value_PR_TransactionID,
                    "ie->value.present != F1AP_GNBCUConfigurationUpdateIEs__value_PR_TransactionID\n");
        TransactionId=ie->value.choice.TransactionID;
        LOG_D(F1AP, "F1AP: GNB-CU-ConfigurationUpdate: TransactionId %d\n",
              TransactionId);
        break;

      case F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List: {
        AssertFatal(ie->criticality == F1AP_Criticality_reject,
                    "ie->criticality != F1AP_Criticality_reject\n");
        AssertFatal(ie->value.present == F1AP_GNBCUConfigurationUpdateIEs__value_PR_Cells_to_be_Activated_List,
                    "ie->value.present != F1AP_GNBCUConfigurationUpdateIEs__value_PR_Cells_to_be_Activated_List\n");
        num_cells_to_activate = ie->value.choice.Cells_to_be_Activated_List.list.count;
        LOG_D(F1AP, "F1AP: Activating %d cells\n",num_cells_to_activate);

        for (int i=0; i<num_cells_to_activate; i++) {
          F1AP_Cells_to_be_Activated_List_ItemIEs_t *cells_to_be_activated_list_item_ies = (F1AP_Cells_to_be_Activated_List_ItemIEs_t *) ie->value.choice.Cells_to_be_Activated_List.list.array[i];
          AssertFatal(cells_to_be_activated_list_item_ies->id == F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item,
                      "cells_to_be_activated_list_item_ies->id != F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item");
          AssertFatal(cells_to_be_activated_list_item_ies->criticality == F1AP_Criticality_reject,
                      "cells_to_be_activated_list_item_ies->criticality == F1AP_Criticality_reject");
          AssertFatal(cells_to_be_activated_list_item_ies->value.present == F1AP_Cells_to_be_Activated_List_ItemIEs__value_PR_Cells_to_be_Activated_List_Item,
                      "cells_to_be_activated_list_item_ies->value.present == F1AP_Cells_to_be_Activated_List_ItemIEs__value_PR_Cells_to_be_Activated_List_Item");
          cell = &cells_to_be_activated_list_item_ies->value.choice.Cells_to_be_Activated_List_Item;
          TBCD_TO_MCC_MNC(&cell->nRCGI.pLMN_Identity,
                          F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].plmn.mcc,
                          F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].plmn.mnc,
                          F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].plmn.mnc_digit_length);
          LOG_D(F1AP, "nr_cellId : %x %x %x %x %x\n",
                cell->nRCGI.nRCellIdentity.buf[0],
                cell->nRCGI.nRCellIdentity.buf[1],
                cell->nRCGI.nRCellIdentity.buf[2],
                cell->nRCGI.nRCellIdentity.buf[3],
                cell->nRCGI.nRCellIdentity.buf[4]);
          BIT_STRING_TO_NR_CELL_IDENTITY(&cell->nRCGI.nRCellIdentity,
                                         F1AP_GNB_CU_CONFIGURATION_UPDATE (msg_p).cells_to_activate[i].nr_cellid);
          F1AP_ProtocolExtensionContainer_10696P112_t *ext = (F1AP_ProtocolExtensionContainer_10696P112_t *)cell->iE_Extensions;

          if (ext==NULL)
            continue;

          for (int cnt=0; cnt<ext->list.count; cnt++) {
            F1AP_Cells_to_be_Activated_List_ItemExtIEs_t *cells_to_be_activated_list_itemExtIEs=(F1AP_Cells_to_be_Activated_List_ItemExtIEs_t *)ext->list.array[cnt];

            switch (cells_to_be_activated_list_itemExtIEs->id) {
              /*
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_NOTHING:
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_GNB_CUSystemInformation,
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_AvailablePLMNList,
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_ExtendedAvailablePLMN_List,
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_IAB_Info_IAB_donor_CU,
                            case F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_AvailableSNPN_ID_List
              */
              case F1AP_ProtocolIE_ID_id_gNB_CUSystemInformation: {
                F1AP_GNB_CU_CONFIGURATION_UPDATE (msg_p).cells_to_activate[i].nrpci = (cell->nRPCI != NULL) ? *cell->nRPCI : 0;
                F1AP_GNB_CUSystemInformation_t *gNB_CUSystemInformation = (F1AP_GNB_CUSystemInformation_t *)&cells_to_be_activated_list_itemExtIEs->extensionValue.choice.GNB_CUSystemInformation;
                F1AP_GNB_CU_CONFIGURATION_UPDATE (msg_p).cells_to_activate[i].num_SI = gNB_CUSystemInformation->sibtypetobeupdatedlist.list.count;
                AssertFatal(ext->list.count==1,"At least one SI message should be there, and only 1 for now!\n");
                LOG_D(F1AP,
                      "F1AP: Cell %d MCC %d MNC %d NRCellid %lx num_si %d\n",
                      i,
                      F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].plmn.mcc,
                      F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].plmn.mnc,
                      F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].nr_cellid,
                      F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].num_SI);

                for (int si = 0; si < gNB_CUSystemInformation->sibtypetobeupdatedlist.list.count; si++) {
                  F1AP_SibtypetobeupdatedListItem_t *sib_item = gNB_CUSystemInformation->sibtypetobeupdatedlist.list.array[si];
                  size_t size = sib_item->sIBmessage.size;
                  F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].SI_msg[si].SI_container_length = size;
                  LOG_D(F1AP, "F1AP: SI_container_length[%d][%ld] %ld bytes\n", i, sib_item->sIBtype, size);
                  F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].SI_msg[si].SI_container =
                      malloc(F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].SI_msg[si].SI_container_length);
                  memcpy((void *)F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].SI_msg[si].SI_container,
                         (void *)sib_item->sIBmessage.buf,
                         size);
                  F1AP_GNB_CU_CONFIGURATION_UPDATE(msg_p).cells_to_activate[i].SI_msg[si].SI_type = sib_item->sIBtype;
                }

                break;
              }

              case F1AP_ProtocolIE_ID_id_AvailablePLMNList:
                AssertFatal(1==0,"F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              case F1AP_ProtocolIE_ID_id_ExtendedAvailablePLMN_List:
                AssertFatal(1==0,"F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              case F1AP_ProtocolIE_ID_id_IAB_Info_IAB_donor_CU:
                AssertFatal(1==0,"F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              case F1AP_ProtocolIE_ID_id_AvailableSNPN_ID_List:
                AssertFatal(1==0,"F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              default:
                AssertFatal(1==0,"F1AP_ProtocolIE_ID_id %d unknown\n",(int)cells_to_be_activated_list_itemExtIEs->id);
                break;
            }
          } // for (cnt=...
        } // for (cells_to_activate...

        break;
      } // case F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List

      default:
        AssertFatal(1==0,"F1AP_ProtocolIE_ID_id %d unknown\n", (int)ie->id);
        break;
    } // switch ie
  } // for IE

  AssertFatal(TransactionId!=-1,"TransactionId was not sent\n");
  LOG_D(F1AP,"F1AP: num_cells_to_activate %d\n",num_cells_to_activate);
  F1AP_GNB_CU_CONFIGURATION_UPDATE (msg_p).num_cells_to_activate = num_cells_to_activate;
  LOG_D(F1AP, "Sending F1AP_GNB_CU_CONFIGURATION_UPDATE ITTI message \n");
  itti_send_msg_to_task(TASK_GNB_APP, GNB_MODULE_ID_TO_INSTANCE(assoc_id), msg_p);
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
