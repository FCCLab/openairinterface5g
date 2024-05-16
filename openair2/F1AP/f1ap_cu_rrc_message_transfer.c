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

/*! \file f1ap_cu_rrc_message_transfer.c
 * \brief f1ap rrc message transfer for CU
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
#include "f1ap_ids.h"
#include "f1ap_itti_messaging.h"
#include "f1ap_cu_rrc_message_transfer.h"
#include "common/ran_context.h"
#include "openair3/UTILS/conversions.h"
#include "LAYER2/nr_pdcp/nr_pdcp_oai_api.h"

#include "f1ap_rrc_message_transfer.h"

/*
    Initial UL RRC Message Transfer
*/

int CU_handle_INITIAL_UL_RRC_MESSAGE_TRANSFER(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  f1ap_initial_ul_rrc_message_t msg;
  if (!decode_initial_ul_rrc_message_transfer(pdu, &msg)) {
    LOG_E(F1AP, "cannot decode F1 initial UL RRC message Transfer\n");
    return -1;
  }

  // create an ITTI message and copy SDU
  MessageDef *message_p = itti_alloc_new_message(TASK_CU_F1, 0, F1AP_INITIAL_UL_RRC_MESSAGE);
  message_p->ittiMsgHeader.originInstance = assoc_id;
  f1ap_initial_ul_rrc_message_t *ul_rrc = &F1AP_INITIAL_UL_RRC_MESSAGE(message_p);
  *ul_rrc = msg; /* "move" message into ITTI, RRC thread will free it */
  itti_send_msg_to_task(TASK_RRC_GNB, instance, message_p);

  return 0;
}

/*
    DL RRC Message Transfer.
*/
int CU_send_DL_RRC_MESSAGE_TRANSFER(sctp_assoc_t assoc_id, f1ap_dl_rrc_message_t *msg)
{
  /* encode DL RRC Message Transfer */
  F1AP_F1AP_PDU_t *pdu = encode_dl_rrc_message_transfer(msg);
  if (pdu == NULL) {
    LOG_E(F1AP, "Failed to encode F1 DL RRC MESSAGE TRANSFER: can't send message!\n");
    ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);
    return -1;
  }
  /* encode F1AP PDU */
  uint8_t *buffer = NULL;
  uint32_t len = 0;
  if (f1ap_encode_pdu(pdu, &buffer, &len) < 0) {
    LOG_E(F1AP, "Failed to encode F1 DL RRC MESSAGE TRANSFER \n");
    ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);
    return -1;
  }
  /* Free F1AP PDU */
  ASN_STRUCT_FREE(asn_DEF_F1AP_F1AP_PDU, pdu);
  /* Send to DU */
  LOG_D(F1AP, "CU send DL_RRC_MESSAGE_TRANSFER \n");
  f1ap_itti_send_sctp_data_req(assoc_id, buffer, len);
  return 0;
}

/*
    UL RRC Message Transfer
*/
int CU_handle_UL_RRC_MESSAGE_TRANSFER(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, F1AP_F1AP_PDU_t *pdu)
{
  LOG_D(F1AP, "CU_handle_UL_RRC_MESSAGE_TRANSFER \n");
  F1AP_ULRRCMessageTransfer_t    *container;
  F1AP_ULRRCMessageTransferIEs_t *ie;
  uint64_t        cu_ue_f1ap_id;
  uint64_t        du_ue_f1ap_id;
  uint64_t        srb_id;
  DevAssert(pdu != NULL);

  if (stream != 0) {
    LOG_E(F1AP, "[SCTP %d] Received F1 on stream != 0 (%d)\n",
          assoc_id, stream);
    return -1;
  }

  container = &pdu->choice.initiatingMessage->value.choice.ULRRCMessageTransfer;
  /* GNB_CU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_ULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_CU_UE_F1AP_ID, true);
  cu_ue_f1ap_id = ie->value.choice.GNB_CU_UE_F1AP_ID;
  /* GNB_DU_UE_F1AP_ID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_ULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_gNB_DU_UE_F1AP_ID, true);
  du_ue_f1ap_id = ie->value.choice.GNB_DU_UE_F1AP_ID;

  if (!cu_exists_f1_ue_data(cu_ue_f1ap_id)) {
    LOG_E(F1AP, "unknown CU UE ID %ld\n", cu_ue_f1ap_id);
    return 1;
  }

  /* the RLC-PDCP does not transport the DU UE ID (yet), so we drop it here.
   * For the moment, let's hope this won't become relevant; to sleep in peace,
   * let's put an assert to check that it is the expected DU UE ID. */
  f1_ue_data_t ue_data = cu_get_f1_ue_data(cu_ue_f1ap_id);
  if (ue_data.secondary_ue != du_ue_f1ap_id) {
    LOG_E(F1AP, "unexpected DU UE ID %u received, expected it to be %lu\n", ue_data.secondary_ue, du_ue_f1ap_id);
    return 1;
  }

  /* mandatory */
  /* SRBID */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_ULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_SRBID, true);
  srb_id = ie->value.choice.SRBID;

  if (srb_id < 1 )
    LOG_E(F1AP, "Unexpected UL RRC MESSAGE for srb_id %lu \n", srb_id);
  else
    LOG_D(F1AP, "UL RRC MESSAGE for srb_id %lu in DCCH \n", srb_id);

  // issue in here
  /* mandatory */
  /* RRC Container */
  F1AP_FIND_PROTOCOLIE_BY_ID(F1AP_ULRRCMessageTransferIEs_t, ie, container,
                             F1AP_ProtocolIE_ID_id_RRCContainer, true);
  protocol_ctxt_t ctxt={0};
  ctxt.instance = instance;
  ctxt.module_id = instance;
  ctxt.rntiMaybeUEid = cu_ue_f1ap_id;
  ctxt.enb_flag = 1;
  ctxt.eNB_index = 0;
  uint8_t *mb = malloc16(ie->value.choice.RRCContainer.size);
  memcpy(mb, ie->value.choice.RRCContainer.buf, ie->value.choice.RRCContainer.size);
  LOG_D(F1AP, "Calling pdcp_data_ind for UE RNTI %lx srb_id %lu with size %ld (DCCH) \n", ctxt.rntiMaybeUEid, srb_id, ie->value.choice.RRCContainer.size);
  //for (int i = 0; i < ie->value.choice.RRCContainer.size; i++)
  //  printf("%02x ", mb->data[i]);
  //printf("\n");
  pdcp_data_ind (&ctxt,
                 1, // srb_flag
                 0, // embms_flag
                 srb_id,
                 ie->value.choice.RRCContainer.size,
                 mb, NULL, NULL);
  return 0;
}
