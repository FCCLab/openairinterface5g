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

/*! \file nrppa_gNB_position_information_transfer_procedures.c
 * \brief NRPPA gNB tasks related to position information transfer
 * \author Adeel Malik
 * \email adeel.malik@eurecom.fr
 *\date 2023
 * \version 1.0
 * @ingroup _nrppa
 */

#include "intertask_interface.h"

/* todo */
#include "nrppa_common.h"
#include "nrppa_gNB_position_information_transfer_procedures.h"
#include "nrppa_gNB_itti_messaging.h"
/* todo */

/*to access SRS config*/
#include "PHY/impl_defs_nr.h" // SRS_Resource_t; SRS_ResourceSet_t;
#include "common/ran_context.h"
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "RRC/NR/nr_rrc_defs.h"
#include "PHY/defs_gNB.h"

/* PositioningInformationExchange (Parent) procedure for  PositioningInformationRequest, PositioningInformationResponse, and
 * PositioningInformationFailure*/
int nrppa_gNB_handle_PositioningInformationExchange(nrppa_gnb_ue_info_t *nrppa_msg_info, NRPPA_NRPPA_PDU_t *rx_pdu)
{
  LOG_I(NRPPA, "Processing Received PositioningInformationRequest \n");
  xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, &rx_pdu);

  // Processing Received PositioningInformationRequest
  NRPPA_PositioningInformationRequest_t *container;
  NRPPA_PositioningInformationRequest_IEs_t *ie;
  uint32_t nrppa_transaction_id;
  NRPPA_RequestedSRSTransmissionCharacteristics_t req_SRS_info;

  DevAssert(rx_pdu != NULL);

  container = &rx_pdu->choice.initiatingMessage->value.choice.PositioningInformationRequest; // IE 9.2.3 Message type (M)
  nrppa_transaction_id = rx_pdu->choice.initiatingMessage->nrppatransactionID; // IE 9.2.4 nrppatransactionID (M)

  /* IE 9.2.27 RequestedSRSTransmissionCharacteristics (O)*/
  NRPPA_FIND_PROTOCOLIE_BY_ID(NRPPA_PositioningInformationRequest_IEs_t,
                              ie,
                              container,
                              NRPPA_ProtocolIE_ID_id_RequestedSRSTransmissionCharacteristics,
                              true);
  req_SRS_info = ie->value.choice.RequestedSRSTransmissionCharacteristics;

  /* TODO Decide if gNB able to provide the information response or not */

  // Forward request to RRC
  MessageDef *msg = itti_alloc_new_message(TASK_RRC_GNB, 0, F1AP_POSITIONING_INFORMATION_REQ);
  f1ap_positioning_information_req_t *f1ap_req = &F1AP_POSITIONING_INFORMATION_REQ(msg);
  f1ap_req->gNB_CU_ue_id = nrppa_msg_info->gNB_ue_ngap_id; // UE->rrc_ue_id,
  f1ap_req->gNB_DU_ue_id = 0; // UE->rrc_ue_id //ue_data.secondary_ue uncomment after synch with latest develop
  f1ap_req->nrppa_msg_info.nrppa_transaction_id = nrppa_transaction_id;
  f1ap_req->nrppa_msg_info.instance = nrppa_msg_info->instance;
  f1ap_req->nrppa_msg_info.gNB_ue_ngap_id = nrppa_msg_info->gNB_ue_ngap_id;
  f1ap_req->nrppa_msg_info.amf_ue_ngap_id = nrppa_msg_info->amf_ue_ngap_id;
  f1ap_req->nrppa_msg_info.routing_id_buffer = nrppa_msg_info->routing_id_buffer;
  f1ap_req->nrppa_msg_info.routing_id_length = nrppa_msg_info->routing_id_length;
  f1ap_req->req_SRS_info.resourceType = req_SRS_info.resourceType;
  f1ap_req->req_SRS_info.numberOfTransmissions = req_SRS_info.numberOfTransmissions; // TODO fill up completely

  if (req_SRS_info.bandwidth.present == NRPPA_BandwidthSRS_PR_fR1) {
    LOG_I(NRPPA,
          "Procesing PositioningInformationRequest f1ap_req->req_SRS_info.bandwidth_srs.fR1=%d \n",
          f1ap_req->req_SRS_info.bandwidth_srs.fR1);
    f1ap_req->req_SRS_info.bandwidth_srs.fR1 = req_SRS_info.bandwidth.choice.fR1;
  }
  if (req_SRS_info.bandwidth.present == NRPPA_BandwidthSRS_PR_fR2) {
    f1ap_req->req_SRS_info.bandwidth_srs.fR2 = req_SRS_info.bandwidth.choice.fR2;
    LOG_I(NRPPA,
          "Procesing PositioningInformationRequest f1ap_req->req_SRS_info.bandwidth_SRS.fR2=%d \n",
          f1ap_req->req_SRS_info.bandwidth_srs.fR2);
  }

  LOG_I(NRPPA,
        "Forwarding to RRC PositioningInformationRequest gNB_CU_ue_id=%d, gNB_DU_ue_id=%d \n",
        f1ap_req->gNB_CU_ue_id,
        f1ap_req->gNB_DU_ue_id);
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
}

/* PositioningActivation (Parent) procedure for  PositioningActivationRequest, PositioningActivationResponse, and
 * PositioningActivationFailure*/
// adeel TODO fill F1AP msg for rrc
int nrppa_gNB_handle_PositioningActivation(nrppa_gnb_ue_info_t *nrppa_msg_info, NRPPA_NRPPA_PDU_t *pdu)
{
  LOG_I(NRPPA, "Processing Received PositioningActivation \n");
  xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, &pdu);

  uint32_t nrppa_transaction_id;

  /*  TODO process and fill F1AP message
  // Processing Received PositioningActivation
  NRPPA_PositioningActivationRequest_t *container;
  NRPPA_PositioningActivationRequestIEs_t *ie;

  uint32_t srs_resource_set_id;
  // NRPPA_SRSType_t                 srs_type;
  // NRPPA_SRSSpatialRelation_t      srs_spatial_relation_info;
  // NRPPA_SRSResourceTrigger_t      srs_resource_trigger;
  // NRPPA_SFNInitialisationTime_t   sfn_initialisation_time;

  DevAssert(pdu != NULL);

  container = &pdu->choice.initiatingMessage->value.choice.PositioningActivationRequest; // IE 9.2.3 Message type (M)
  nrppa_transaction_id = pdu->choice.initiatingMessage->nrppatransactionID; // IE 9.2.4 nrppatransactionID (M)

  // IE  SRSType (O)
  NRPPA_FIND_PROTOCOLIE_BY_ID(NRPPA_PositioningActivationRequestIEs_t, ie, container, NRPPA_ProtocolIE_ID_id_SRSType, true);
  // srs_type = ie->value.choice.SRSType;

  // struct NRPPA_SemipersistentSRS	*semipersistentSRS;
  // srs_resource_set_id = srs_type.present
  // NRPPA_SpatialRelationInfo_t     spatial_relation_info;

  // struct NRPPA_AperiodicSRS	*aperiodicSRS;
  // NRPPA_SRSResourceTrigger_t      srs_resource_trigger;

  // struct NRPPA_ProtocolIE_Single_Container	*sRSType_extension;

  // IE  Activation Time (O)
  NRPPA_FIND_PROTOCOLIE_BY_ID(NRPPA_PositioningActivationRequestIEs_t, ie, container, NRPPA_ProtocolIE_ID_id_ActivationTime, true);
*/

  // TODO process activation request and generate corresponding response
  // Forward request to RRC
  MessageDef *msg = itti_alloc_new_message(TASK_RRC_GNB, 0, F1AP_POSITIONING_ACTIVATION_REQ);
  f1ap_positioning_activation_req_t *f1ap_req = &F1AP_POSITIONING_ACTIVATION_REQ(msg);
  f1ap_req->gNB_CU_ue_id = nrppa_msg_info->gNB_ue_ngap_id; // UE->rrc_ue_id,
  f1ap_req->gNB_DU_ue_id = 0; // UE->rrc_ue_id //ue_data.secondary_ue uncomment after synch with latest develop
  f1ap_req->nrppa_msg_info.nrppa_transaction_id = nrppa_transaction_id;
  f1ap_req->nrppa_msg_info.instance = nrppa_msg_info->instance;
  f1ap_req->nrppa_msg_info.gNB_ue_ngap_id = nrppa_msg_info->gNB_ue_ngap_id;
  f1ap_req->nrppa_msg_info.amf_ue_ngap_id = nrppa_msg_info->amf_ue_ngap_id;
  f1ap_req->nrppa_msg_info.routing_id_buffer = nrppa_msg_info->routing_id_buffer;
  f1ap_req->nrppa_msg_info.routing_id_length = nrppa_msg_info->routing_id_length;

  LOG_I(NRPPA,
        "Forwarding to RRC PositioningActivationRequest gNB_CU_ue_id=%d, gNB_DU_ue_id=%d \n",
        f1ap_req->gNB_CU_ue_id,
        f1ap_req->gNB_DU_ue_id);
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
  return 0;
}

// adeel TODO fill F1AP msg for rrc
int nrppa_gNB_handle_PositioningDeactivation(nrppa_gnb_ue_info_t *nrppa_msg_info, NRPPA_NRPPA_PDU_t *pdu)
{
  LOG_I(NRPPA, "Processing Received PositioningDeActivation \n");
  xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, &pdu);

  uint32_t nrppa_transaction_id;
  /*  TODO process and fill F1AP message
  // Processing Received PositioningDeActivation
  NRPPA_PositioningDeactivation_t *container;
  NRPPA_PositioningDeactivationIEs_t *ie;


  uint32_t srs_resource_set_id;

  DevAssert(pdu != NULL);

  container = &pdu->choice.initiatingMessage->value.choice.PositioningDeactivation; // IE 9.2.3 Message type (M)
  nrppa_transaction_id = pdu->choice.initiatingMessage->nrppatransactionID; // IE 9.2.4 nrppatransactionID (M)

  // IE  Abort Transmission(O)
  NRPPA_FIND_PROTOCOLIE_BY_ID(NRPPA_PositioningDeactivationIEs_t, ie, container, NRPPA_ProtocolIE_ID_id_AbortTransmission, true);
  // ie->value.choice.AbortTransmission

  // srs_resource_set_id = ie->value.choice.AbortTransmission.S
  // Release_all = ie->value.choice.AbortTransmission.Re */

  // TODO process daactivation request and stop the corresponding positioning process
  // Forward request to RRC
  MessageDef *msg = itti_alloc_new_message(TASK_RRC_GNB, 0, F1AP_POSITIONING_DEACTIVATION);
  f1ap_positioning_deactivation_t *f1ap_req = &F1AP_POSITIONING_DEACTIVATION(msg);
  f1ap_req->gNB_CU_ue_id = nrppa_msg_info->gNB_ue_ngap_id; // UE->rrc_ue_id,
  f1ap_req->gNB_DU_ue_id = 0; // UE->rrc_ue_id //ue_data.secondary_ue uncomment after synch with latest develop
  f1ap_req->nrppa_msg_info.nrppa_transaction_id = nrppa_transaction_id;
  f1ap_req->nrppa_msg_info.instance = nrppa_msg_info->instance;
  f1ap_req->nrppa_msg_info.gNB_ue_ngap_id = nrppa_msg_info->gNB_ue_ngap_id;
  f1ap_req->nrppa_msg_info.amf_ue_ngap_id = nrppa_msg_info->amf_ue_ngap_id;
  f1ap_req->nrppa_msg_info.routing_id_buffer = nrppa_msg_info->routing_id_buffer;
  f1ap_req->nrppa_msg_info.routing_id_length = nrppa_msg_info->routing_id_length;

  LOG_I(NRPPA,
        "Forwarding to RRC PositioningDeactivation gNB_CU_ue_id=%d, gNB_DU_ue_id=%d \n",
        f1ap_req->gNB_CU_ue_id,
        f1ap_req->gNB_DU_ue_id);
  itti_send_msg_to_task(TASK_RRC_GNB, 0, msg);
  return 0;
}

// DOWLINK
// adeel TODO paritally filled F1AP msg for rrc
int nrppa_gNB_PositioningInformationResponse(instance_t instance, MessageDef *msg_p)
{
  f1ap_positioning_information_resp_t *resp = &F1AP_POSITIONING_INFORMATION_RESP(msg_p);
  LOG_I(NRPPA,
        "Received PositioningInformationResponse info from RRC  gNB_CU_ue_id=%d, gNB_DU_ue_id=%d  rnti= %04x\n",
        resp->gNB_CU_ue_id,
        resp->gNB_DU_ue_id,
        resp->nrppa_msg_info.ue_rnti);

  // Prepare NRPPA Position Information transfer Response
  NRPPA_NRPPA_PDU_t tx_pdu; // TODO rename
  uint8_t *buffer = NULL;
  uint32_t length = 0;

  /* Prepare the NRPPA message to encode for successfulOutcome PositioningInformationResponse */
  // IE: 9.2.3 Message Type successfulOutcome PositioningInformationResponse /* mandatory */
  memset(&tx_pdu, 0, sizeof(tx_pdu));
  tx_pdu.present = NRPPA_NRPPA_PDU_PR_successfulOutcome;
  asn1cCalloc(tx_pdu.choice.successfulOutcome, head);
  head->procedureCode = NRPPA_ProcedureCode_id_positioningInformationExchange;
  head->criticality = NRPPA_Criticality_reject;
  head->value.present = NRPPA_SuccessfulOutcome__value_PR_PositioningInformationResponse;

  // IE 9.2.4 nrppatransactionID  /* mandatory */
  head->nrppatransactionID = resp->nrppa_msg_info.nrppa_transaction_id; // nrppa_transaction_id;
  NRPPA_PositioningInformationResponse_t *out = &head->value.choice.PositioningInformationResponse;

  // IE 9.2.28 SRS Configuration (O) IE of PositioningInformationResponse
  //  Currently changing SRS configuration on the fly is not possible, therefore we add the predefined SRS configuration in NRPPA
  //  pdu
  if (1) {
    asn1cSequenceAdd(out->protocolIEs.list, NRPPA_PositioningInformationResponse_IEs_t, ie);
    ie->id = NRPPA_ProtocolIE_ID_id_SRSConfiguration;
    ie->criticality = NRPPA_Criticality_ignore;
    ie->value.present = NRPPA_PositioningInformationResponse_IEs__value_PR_SRSConfiguration;

    // Preparing SRS Carrier List an IE  of SRS Configuration
    int nb_of_srscarrier = resp->srs_configuration.srs_carrier_list
                               .srs_carrier_list_length; // gNB->max_nb_srs; // TODO find the acutal number for carrier and add here
    f1ap_srs_carrier_list_item_t *carrier_list_item = resp->srs_configuration.srs_carrier_list.srs_carrier_list_item;

    printf("\n TEST 1 [NRPPA PIR] Positioning_information_response(); nb_of_srscarrier= %d", nb_of_srscarrier);
    for (int carrier_ind = 0; carrier_ind < nb_of_srscarrier; carrier_ind++) {
      asn1cSequenceAdd(ie->value.choice.SRSConfiguration.sRSCarrier_List.list, NRPPA_SRSCarrier_List_Item_t, item);
      item->pointA = carrier_list_item->pointA; // IE of SRSCarrier_List_Item
      asn1cCalloc(item->pCI, pci);
      pci =
          carrier_list_item->pci; // IE of SRSCarrier_List_Item Optional Physical cell ID of the cell that contians the SRS carrier

      // Preparing Active UL BWP information IE of SRSCarrier_List
      item->activeULBWP.locationAndBandwidth = carrier_list_item->active_ul_bwp.locationAndBandwidth; // long
      item->activeULBWP.subcarrierSpacing = carrier_list_item->active_ul_bwp.subcarrierSpacing; // long
      item->activeULBWP.cyclicPrefix = carrier_list_item->active_ul_bwp.cyclicPrefix; // long
      item->activeULBWP.txDirectCurrentLocation = carrier_list_item->active_ul_bwp.txDirectCurrentLocation; // long
      asn1cCalloc(item->activeULBWP.shift7dot5kHz, shift7dot5kHz);
      shift7dot5kHz = carrier_list_item->active_ul_bwp.shift7dot5kHz; // long  Optional

      // Preparing sRSResource_List  IE of SRSConfig (IE of activeULBWP)
      int nb_srsresource = carrier_list_item->active_ul_bwp.sRSConfig.sRSResource_List
                               .srs_resource_list_length; // srs_config->srs_ResourceToAddModList->list.count;
      asn1cCalloc(item->activeULBWP.sRSConfig.sRSResource_List, srsresource_list);
      f1ap_srs_resource_t *res_item = carrier_list_item->active_ul_bwp.sRSConfig.sRSResource_List.srs_resource;
      for (int k = 0; k < nb_srsresource; k++) // Preparing SRS Resource List
      {
        asn1cSequenceAdd(srsresource_list->list, NRPPA_SRSResource_t, resource_item);
        resource_item->sRSResourceID = res_item->sRSResourceID; //(M)
        resource_item->nrofSRS_Ports = res_item->nrofSRS_Ports; //(M) port1	= 0, ports2	= 1, ports4	= 2
        resource_item->startPosition = res_item->startPosition; //(M)
        resource_item->nrofSymbols = res_item->nrofSymbols; //(M)  n1	= 0, n2	= 1, n4	= 2
        resource_item->repetitionFactor = res_item->repetitionFactor; //(M)  n1	= 0, n2	= 1, n4	= 2
        resource_item->freqDomainPosition = res_item->freqDomainPosition; //(M)
        resource_item->freqDomainShift = res_item->freqDomainShift; //(M)
        resource_item->c_SRS = res_item->c_SRS; //(M)
        resource_item->b_SRS = res_item->b_SRS; //(M)
        resource_item->b_hop = res_item->b_hop; //(M)
        resource_item->groupOrSequenceHopping =
            res_item->groupOrSequenceHopping; //(M) neither	= 0, groupHopping	= 1, sequenceHopping	= 2
        resource_item->slotOffset = res_item->slotOffset; //(M)
        resource_item->sequenceId = res_item->sequenceId; //(M)

        // IE transmissionComb
        if (res_item->transmissionComb.present == f1ap_transmission_comb_pr_n2) {
          resource_item->transmissionComb.present =
              NRPPA_TransmissionComb_PR_n2; // res_item->transmissionComb.n2.present NRPPA_TransmissionComb_PR_NOTHING
          asn1cCalloc(resource_item->transmissionComb.choice.n2, comb_n2);
          comb_n2->combOffset_n2 =
              res_item->transmissionComb.choice.n2.combOffset_n2; // choices n2 or n4 uint8_t	 combOffset_n2; // (M) range (0,1)
                                                                  // uint8_t	 cyclicShift_n2; // (M) range (0,1,...7)
          comb_n2->cyclicShift_n2 = res_item->transmissionComb.choice.n2.cyclicShift_n2;
        } else if (res_item->transmissionComb.present == f1ap_transmission_comb_pr_n4) {
          resource_item->transmissionComb.present =
              NRPPA_TransmissionComb_PR_n4; // res_item->transmissionComb.n2.present NRPPA_TransmissionComb_PR_NOTHING
          asn1cCalloc(resource_item->transmissionComb.choice.n4, comb_n4);
          comb_n4->combOffset_n4 =
              res_item->transmissionComb.choice.n4.combOffset_n4; // choices n2 or n4 uint8_t	 combOffset_n2; // (M) range (0,1)
                                                                  // uint8_t	 cyclicShift_n2; // (M) range (0,1,...7)
          comb_n4->cyclicShift_n4 = res_item->transmissionComb.choice.n4.cyclicShift_n4;
        } else if (res_item->transmissionComb.present == f1ap_transmission_comb_pr_nothing) {
          resource_item->transmissionComb.present =
              NRPPA_TransmissionComb_PR_NOTHING; // res_item->transmissionComb.n2.present NRPPA_TransmissionComb_PR_NOTHING
        }

        // IE resourceType
        if (res_item->resourceType.present == f1ap_resource_type_pr_periodic) {
          resource_item->resourceType.present =
              NRPPA_ResourceType_PR_periodic; // NRPPA_ResourceType_PR_NOTHING; //NRPPA_ResourceType_PR_periodic,
                                              // NRPPA_ResourceType_PR_semi_persistent,	NRPPA_ResourceType_PR_aperiodic,
          asn1cCalloc(resource_item->resourceType.choice.periodic, res_type_periodic);
          res_type_periodic->periodicity =
              res_item->resourceType.choice.periodic
                  .periodicity; //(M) choice periodic (uint8_t periodicity; uint16_t offset);    semi_persistent (uint8_t
                                // periodicity; uint16_t offset);     aperiodic (uint8_t aperiodicResourceType;);
          res_type_periodic->offset = res_item->resourceType.choice.periodic.offset; //(M)
        } else if (res_item->resourceType.present == f1ap_resource_type_pr_aperiodic) {
          resource_item->resourceType.present =
              NRPPA_ResourceType_PR_aperiodic; // NRPPA_ResourceType_PR_NOTHING; //NRPPA_ResourceType_PR_periodic,
                                               // NRPPA_ResourceType_PR_semi_persistent,	NRPPA_ResourceType_PR_aperiodic,
          asn1cCalloc(resource_item->resourceType.choice.aperiodic, res_type_aperiodic);
          res_type_aperiodic->aperiodicResourceType =
              res_item->resourceType.choice.aperiodic
                  .aperiodicResourceType; //(M) choice    aperiodic (uint8_t aperiodicResourceType;);
        } else if (res_item->resourceType.present == f1ap_resource_type_pr_semi_persistent) {
          resource_item->resourceType.present =
              NRPPA_ResourceType_PR_semi_persistent; // NRPPA_ResourceType_PR_NOTHING; //NRPPA_ResourceType_PR_periodic,
                                                     // NRPPA_ResourceType_PR_semi_persistent,	NRPPA_ResourceType_PR_aperiodic,
          asn1cCalloc(resource_item->resourceType.choice.periodic, res_type_semi_persistent);
          res_type_semi_persistent->periodicity =
              res_item->resourceType.choice.semi_persistent
                  .periodicity; //(M) choice periodic (uint8_t periodicity; uint16_t offset);    semi_persistent (uint8_t
                                // periodicity; uint16_t offset);     aperiodic (uint8_t aperiodicResourceType;);
          res_type_semi_persistent->offset = res_item->resourceType.choice.semi_persistent.offset; //(M)
        } else if (res_item->resourceType.present == f1ap_resource_type_pr_nothing) {
          resource_item->resourceType.present =
              NRPPA_ResourceType_PR_NOTHING; // NRPPA_ResourceType_PR_NOTHING; //NRPPA_ResourceType_PR_periodic,
                                             // NRPPA_ResourceType_PR_semi_persistent,	NRPPA_ResourceType_PR_aperiodic,
        }

        if (k < nb_srsresource - 1) {
          res_item++;
        }

      } // for(int k=0; k < nb_srsresource; k++)

      // Preparing posSRSResource_List IE of SRSConfig (IE of activeULBWP)
      int nb_possrsresource = carrier_list_item->active_ul_bwp.sRSConfig.posSRSResource_List
                                  .pos_srs_resource_list_length; // srs_config->srs_ResourceToAddModList->list.count;
      f1ap_pos_srs_resource_item_t *pos_res_item =
          carrier_list_item->active_ul_bwp.sRSConfig.posSRSResource_List.pos_srs_resource_item;
      asn1cCalloc(item->activeULBWP.sRSConfig.posSRSResource_List, possrsresource_list);
      printf("Test 2 Adeel:  PositioningInformationResponse nb_possrsresource=%d \n", nb_possrsresource);
      for (int p = 0; p < nb_possrsresource; p++) // Preparing posSRSResource_List
      {
        asn1cSequenceAdd(possrsresource_list->list, NRPPA_PosSRSResource_Item_t, pos_resource_item);
        pos_resource_item->srs_PosResourceId = pos_res_item->srs_PosResourceId; // (M)
        pos_resource_item->transmissionCombPos.present = NRPPA_TransmissionCombPos_PR_n2;
        asn1cCalloc(pos_resource_item->transmissionCombPos.choice.n2, combPos_n2);
        combPos_n2->combOffset_n2 = pos_res_item->transmissionCombPos.n2
                                        .combOffset_n2; // (M)  f1ap_transmission_comb_pos_n2_t n2 (combOffset_n2,cyclicShift_n2) ;
                                                        // f1ap_transmission_comb_pos_n2_t n4; f1ap_transmission_comb_pos_n8_t n8;
        combPos_n2->cyclicShift_n2 = pos_res_item->transmissionCombPos.n2.cyclicShift_n2;
        pos_resource_item->startPosition = pos_res_item->startPosition; // (M)  range (0,1,...13)
        pos_resource_item->nrofSymbols = pos_res_item->nrofSymbols; // (M)  n1	= 0, n2	= 1, n4	= 2, n8	= 3, n12 = 4
        pos_resource_item->freqDomainShift = pos_res_item->freqDomainShift; // (M)
        pos_resource_item->c_SRS = pos_res_item->c_SRS; // (M)
        pos_resource_item->groupOrSequenceHopping =
            pos_res_item->groupOrSequenceHopping; // (M)  neither	= 0, groupHopping	= 1, sequenceHopping	= 2
        pos_resource_item->resourceTypePos.present =
            NRPPA_ResourceTypePos_PR_periodic; // NRPPA_ResourceType_PR_NOTHING; //NRPPA_ResourceType_PR_periodic,
                                               // NRPPA_ResourceType_PR_semi_persistent,	NRPPA_ResourceType_PR_aperiodic,
        asn1cCalloc(pos_resource_item->resourceTypePos.choice.periodic, pos_res_type_periodic);
        pos_res_type_periodic->periodicity =
            pos_res_item->resourceTypePos.periodic
                .periodicity; // (M)    f1ap_resource_type_periodic_pos_t	  periodic;	f1ap_resource_type_semi_persistent_pos_t
                              // semi_persistent;	f1ap_resource_type_aperiodic_pos_t	        aperiodic;
        pos_res_type_periodic->offset = pos_res_item->resourceTypePos.periodic.offset; // (M)
        pos_resource_item->sequenceId = pos_res_item->sequenceId; //(M)
        // pos_resource_item->spatialRelationPos                     =pos_res_item->spatialRelationPos;	// OPTIONAl
        if (p < nb_possrsresource - 1) {
          pos_res_item++;
        }
      } // for(int p=0; p < nb_possrsresource; p++)

      // Preparing sRSResourceSet_List IE of SRSConfig (IE of activeULBWP)
      int nb_srsresourceset = carrier_list_item->active_ul_bwp.sRSConfig.sRSResourceSet_List
                                  .srs_resource_set_list_length; // srs_config->srs_ResourceSetToAddModList->list.count;
      f1ap_srs_resource_set_t *resSet_item = carrier_list_item->active_ul_bwp.sRSConfig.sRSResourceSet_List.srs_resource_set;
      printf("Test 3 Adeel:  PositioningInformationResponse nb_srsresourceset=%d \n", nb_srsresourceset);
      asn1cCalloc(item->activeULBWP.sRSConfig.sRSResourceSet_List, srsresourceset_list);
      for (int y = 0; y < nb_srsresourceset; y++) // Preparing SRS Resource Set List
      {
        asn1cSequenceAdd(srsresourceset_list->list, NRPPA_SRSResourceSet_t, srsresourceset_item);
        // IE sRSResourceSetID
        srsresourceset_item->sRSResourceSetID = resSet_item->sRSResourceSetID; //// (M)
        // IE sRSResourceID_List
        int nb_srsresourceperset = resSet_item->sRSResourceID_List.srs_resource_id_list_length;
        uint8_t *srs_res_id = resSet_item->sRSResourceID_List.srs_resource_id;
        for (int y = 0; y < nb_srsresourceperset; y++) {
          asn1cSequenceAdd(srsresourceset_item->sRSResourceID_List.list, NRPPA_SRSResourceID_t, srsresourceID);
          srsresourceID = *srs_res_id;
          srs_res_id++;
        }

        // IE resourceSetType TODO fix this format issue
        srsresourceset_item->resourceSetType.present =
            NRPPA_ResourceSetType_PR_periodic; //  NRPPA_ResourceSetType_PR_NOTHING,
                                               //  NRPPA_ResourceSetType_PR_semi_persistent,NRPPA_ResourceSetType_PR_aperiodic,
        asn1cCalloc(srsresourceset_item->resourceSetType.choice.periodic, res_set_type_periodic);
        res_set_type_periodic->periodicSet = resSet_item->resourceSetType.choice.periodic.periodicSet;
        /*if(resSet_item->resourceSetType.present== f1ap_resource_set_type_pr_periodic ){
        srsresourceset_item->resourceSetType.present= NRPPA_ResourceSetType_PR_periodic; //  NRPPA_ResourceSetType_PR_NOTHING,
        NRPPA_ResourceSetType_PR_semi_persistent,NRPPA_ResourceSetType_PR_aperiodic,
        asn1cCalloc(srsresourceset_item->resourceSetType.choice.periodic, res_set_type_periodic);
        res_set_type_periodic->periodicSet=resSet_item->resourceSetType.choice.periodic.periodicSet;
        }
        else if(resSet_item->resourceSetType.present== f1ap_resource_set_type_pr_aperiodic ){
        srsresourceset_item->resourceSetType.present= NRPPA_ResourceSetType_PR_aperiodic; //  NRPPA_ResourceSetType_PR_NOTHING,
        NRPPA_ResourceSetType_PR_semi_persistent,NRPPA_ResourceSetType_PR_aperiodic,
        asn1cCalloc(srsresourceset_item->resourceSetType.choice.aperiodic, res_set_type_aperiodic);
        res_set_type_aperiodic->sRSResourceTrigger=resSet_item->resourceSetType.choice.aperiodic.sRSResourceTrigger; // TODO verify
        IE format res_set_type_aperiodic->slotoffset=resSet_item->resourceSetType.choice.aperiodic.slotoffset; // TODO verify IE
        format
        }
        else if(resSet_item->resourceSetType.present== f1ap_resource_set_type_pr_semi_persistent ){
        srsresourceset_item->resourceSetType.present= NRPPA_ResourceSetType_PR_semi_persistent; // NRPPA_ResourceSetType_PR_NOTHING,
        NRPPA_ResourceSetType_PR_semi_persistent,NRPPA_ResourceSetType_PR_aperiodic,
        asn1cCalloc(srsresourceset_item->resourceSetType.choice.semi_persistent, res_set_type_semi_persistent);
        res_set_type_semi_persistent->semi_persistentSet=resSet_item->resourceSetType.choice.semi_persistent.semi_persistentSet;
        }
        else if(resSet_item->resourceSetType.present== f1ap_resource_set_type_pr_nothing ){
        srsresourceset_item->resourceSetType.present= NRPPA_ResourceSetType_PR_NOTHING; //  NRPPA_ResourceSetType_PR_NOTHING,
        NRPPA_ResourceSetType_PR_semi_persistent,NRPPA_ResourceSetType_PR_aperiodic,
        }*/
        if (y < nb_srsresourceset - 1) {
          resSet_item++;
        }
      } // for(int y=0; y < nb_srsresourceset; y++)

      // Preparing posSRSResourceSet_List IE of SRSConfig (IE of activeULBWP)
      int nb_possrsresourceset = carrier_list_item->active_ul_bwp.sRSConfig.posSRSResourceSet_List
                                     .pos_srs_resource_set_list_length; // srs_config->srs_ResourceToAddModList->list.count;
      asn1cCalloc(item->activeULBWP.sRSConfig.posSRSResourceSet_List, possrsresourceset_list);
      f1ap_pos_srs_resource_set_item_t *pos_res_set_item =
          carrier_list_item->active_ul_bwp.sRSConfig.posSRSResourceSet_List.pos_srs_resource_set_item;
      printf("Test 4 Adeel: NRPPA PositioningInformationResponse nb_possrsresourceset=%d \n", nb_possrsresourceset);
      for (int j = 0; j < nb_possrsresourceset; j++) // Preparing posSRSResourceSet_List
      {
        asn1cSequenceAdd(possrsresourceset_list->list, NRPPA_PosSRSResourceSet_Item_t, pos_resource_set_item);

        // IE possrsResourceSetID
        pos_resource_set_item->possrsResourceSetID = pos_res_set_item->possrsResourceSetID; // (M)
        // IE possRSResourceID_List
        int nb_srsposresourceperset = pos_res_set_item->possRSResourceID_List.pos_srs_resource_id_list_length;
        uint8_t *pos_srs_res_id = pos_res_set_item->possRSResourceID_List.srs_pos_resource_id;
        for (int y = 0; y < nb_srsposresourceperset; y++) {
          asn1cSequenceAdd(pos_resource_set_item->possRSResourceID_List.list, NRPPA_SRSPosResourceID_t, srsposresourceID);
          srsposresourceID = *pos_srs_res_id; // TODO add increment to pointer
        }

        // IE posresourceSetType
        pos_resource_set_item->posresourceSetType.present = NRPPA_PosResourceSetType_PR_periodic;
        asn1cCalloc(pos_resource_set_item->posresourceSetType.choice.periodic, pos_res_set_type_periodic);
        pos_res_set_type_periodic->posperiodicSet = pos_res_set_item->posresourceSetType.periodic.posperiodicSet;

        if (j < nb_possrsresourceset - 1) {
          pos_res_set_item++;
        }

      } // for(int j=0; j < nb_possrsresourceset; j++)*/

      //  Preparing Uplink Channel BW Per SCS List information IE of SRSCarrier_List
      int size_SpecificCarrier_list = carrier_list_item->uplink_channel_bw_per_scs_list.scs_specific_carrier_list_length;
      f1ap_scs_specific_carrier_t *scs_spe_carrier_item = carrier_list_item->uplink_channel_bw_per_scs_list.scs_specific_carrier;
      for (int b = 0; b < size_SpecificCarrier_list; b++) {
        asn1cSequenceAdd(item->uplinkChannelBW_PerSCS_List.list, NRPPA_SCS_SpecificCarrier_t, SpecificCarrier_item);
        SpecificCarrier_item->offsetToCarrier = scs_spe_carrier_item->offsetToCarrier;
        SpecificCarrier_item->subcarrierSpacing = scs_spe_carrier_item->subcarrierSpacing;
        SpecificCarrier_item->carrierBandwidth = scs_spe_carrier_item->carrierBandwidth;

        if (b < size_SpecificCarrier_list - 1) {
          scs_spe_carrier_item++;
        }

      } // for(int b=0; b < size_SpecificCarrier_list; b++)
    } // for (int i = 0; i < nb_of_srscarrier; i++)
  }

  // IE 9.2.36 SFN Initialisation Time (O)
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, NRPPA_PositioningInformationResponse_IEs_t, ie);
    ie->id = NRPPA_ProtocolIE_ID_id_SFNInitialisationTime;
    ie->criticality = NRPPA_Criticality_ignore;
    ie->value.present = NRPPA_PositioningInformationResponse_IEs__value_PR_SFNInitialisationTime;
    // TODO Retreive SFN Initialisation Time and assign
    // ie->value.choice.SFNInitialisationTime.buf = NULL ; //TODO adeel retrieve and add TYPE typedef struct BIT_STRING_s {uint8_t
    // *buf;	size_t size;	int bits_unused;} BIT_STRING_t; ie->value.choice.SFNInitialisationTime.size =4;
    // ie->value.choice.SFNInitialisationTime.bits_unused =0;
  }

  //  IE 9.2.2 CriticalityDiagnostics (O)
  if (0) {
    asn1cSequenceAdd(out->protocolIEs.list, NRPPA_PositioningInformationResponse_IEs_t, ie);
    ie->id = NRPPA_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie->criticality = NRPPA_Criticality_ignore;
    ie->value.present = NRPPA_PositioningInformationResponse_IEs__value_PR_CriticalityDiagnostics;
    // TODO Retreive CriticalityDiagnostics information and assign
    // ie->value.choice.CriticalityDiagnostics.procedureCode = 9; //TODO adeel retrieve and add
    // ie->value.choice.CriticalityDiagnostics.triggeringMessage = 1; //TODO adeel retrieve and add
    // ie->value.choice.CriticalityDiagnostics.procedureCriticality = NRPPA_Criticality_reject; //TODO adeel retrieve and add
    // ie->value.choice.CriticalityDiagnostics.nrppatransactionID =10;//nrppa_transaction_id ;
    // ie->value.choice.CriticalityDiagnostics.iEsCriticalityDiagnostics = ; //TODO adeel retrieve and add
    // ie->value.choice.CriticalityDiagnostics.iEsCriticalityDiagnostics = ; //TODO adeel retrieve and add
    // ie->value.choice.CriticalityDiagnostics.iEsCriticalityDiagnostics = ; //TODO adeel retrieve and add
    // ie->value.choice.CriticalityDiagnostics.iE_Extensions = ; //TODO adeel retrieve and add
  }

  LOG_I(NRPPA, "Calling encoder for PositioningInformationResponse \n");
  xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, &tx_pdu);

  /* Encode NRPPA message */
  if (nrppa_gNB_encode_pdu(&tx_pdu, &buffer, &length) < 0) {
    NRPPA_ERROR("Failed to encode Uplink NRPPa PositioningInformationResponse\n");
    /* Encode procedure has failed... */
    return -1;
  }

  /* Forward the NRPPA PDU to NGAP */
  if (resp->nrppa_msg_info.gNB_ue_ngap_id > 0 && resp->nrppa_msg_info.amf_ue_ngap_id > 0) //( 1) // TODO
  {
    LOG_D(NRPPA,
          "Sending UplinkUEAssociatedNRPPa (PositioningInformationResponse) to NGAP (gNB_ue_ngap_id= %d, amf_ue_ngap_id =%ld)  \n",
          resp->nrppa_msg_info.gNB_ue_ngap_id,
          resp->nrppa_msg_info.amf_ue_ngap_id);
    nrppa_gNB_itti_send_UplinkUEAssociatedNRPPa(resp->nrppa_msg_info.instance,
                                                resp->nrppa_msg_info.gNB_ue_ngap_id,
                                                resp->nrppa_msg_info.amf_ue_ngap_id,
                                                resp->nrppa_msg_info.routing_id_buffer,
                                                resp->nrppa_msg_info.routing_id_length,
                                                buffer,
                                                length);
    return length;
  } else if (resp->nrppa_msg_info.gNB_ue_ngap_id == -1 && resp->nrppa_msg_info.amf_ue_ngap_id == -1) //
  {
    LOG_D(
        NRPPA,
        "Sending UplinkNonUEAssociatedNRPPa (PositioningInformationResponse) to NGAP (gNB_ue_ngap_id= %d, amf_ue_ngap_id =%ld)  \n",
        resp->nrppa_msg_info.gNB_ue_ngap_id,
        resp->nrppa_msg_info.amf_ue_ngap_id);
    nrppa_gNB_itti_send_UplinkNonUEAssociatedNRPPa(resp->nrppa_msg_info.instance,
                                                   resp->nrppa_msg_info.routing_id_buffer,
                                                   resp->nrppa_msg_info.routing_id_length,
                                                   buffer,
                                                   length);
    return length;
  } else {
    NRPPA_ERROR("Failed to find context for Uplink NonUE/UE Associated NRPPa PositioningInformationResponse\n");
    return -1;
  }
}

// adeel TODO fill F1AP msg for rrc
int nrppa_gNB_PositioningInformationFailure(instance_t instance, MessageDef *msg_p)
{
  f1ap_positioning_information_failure_t *failure_msg = &F1AP_POSITIONING_INFORMATION_FAILURE(msg_p);
  LOG_I(NRPPA,
        "Received PositioningInformationFailure info from RRC  gNB_CU_ue_id=%d, gNB_DU_ue_id=%d  rnti= %04x\n",
        failure_msg->gNB_CU_ue_id,
        failure_msg->gNB_DU_ue_id,
        failure_msg->nrppa_msg_info.ue_rnti);

  // TODO UPDATE data from F1AP Message

  // Prepare NRPPA Position Information failure
  NRPPA_NRPPA_PDU_t tx_pdu; // TODO rename
  uint8_t *buffer = NULL;
  uint32_t length = 0;

  // IE: 9.2.3 Message Type unsuccessfulOutcome PositioningInformationFaliure /* mandatory */
  memset(&tx_pdu, 0, sizeof(tx_pdu));
  tx_pdu.present = NRPPA_NRPPA_PDU_PR_unsuccessfulOutcome;
  asn1cCalloc(tx_pdu.choice.unsuccessfulOutcome, head);
  head->procedureCode = NRPPA_ProcedureCode_id_positioningInformationExchange;
  head->criticality = NRPPA_Criticality_reject;
  head->value.present = NRPPA_UnsuccessfulOutcome__value_PR_PositioningInformationFailure;

  // IE 9.2.4 nrppatransactionID  /* mandatory */
  head->nrppatransactionID = failure_msg->nrppa_msg_info.nrppa_transaction_id;
  NRPPA_PositioningInformationFailure_t *out = &head->value.choice.PositioningInformationFailure;

  // TODO IE 9.2.1 Cause (M)
  {
    asn1cSequenceAdd(out->protocolIEs.list, NRPPA_PositioningInformationFailure_IEs_t, ie);
    ie->id = NRPPA_ProtocolIE_ID_id_Cause;
    ie->criticality = NRPPA_Criticality_ignore;
    ie->value.present = NRPPA_PositioningInformationFailure_IEs__value_PR_Cause;
    // TODO Reteive Cause and assign
    ie->value.choice.Cause.present = NRPPA_Cause_PR_misc; // IE 1
    // ie->value.choice.Cause.present = NRPPA_Cause_PR_NOTHING ; //IE 1
    ie->value.choice.Cause.choice.misc = 0; // TODO dummay response
    // ie->value.choice.Cause. =;  // IE 2 and so on
  }

  //  TODO IE 9.2.2 CriticalityDiagnostics (O)
  if (1) {
    asn1cSequenceAdd(out->protocolIEs.list, NRPPA_PositioningInformationFailure_IEs_t, ie);
    ie->id = NRPPA_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie->criticality = NRPPA_Criticality_ignore;
    ie->value.present = NRPPA_PositioningInformationFailure_IEs__value_PR_CriticalityDiagnostics;
    // TODO Retreive CriticalityDiagnostics information and assign
    // ie->value.choice.CriticalityDiagnostics.procedureCode = 9; //TODO adeel retrieve and add
    // ie->value.choice.CriticalityDiagnostics.triggeringMessage = 1; //TODO adeel retrieve and add
    // ie->value.choice.CriticalityDiagnostics.procedureCriticality; = NRPPA_Criticality_reject; //TODO adeel retrieve and add
    // ie->value.choice.CriticalityDiagnostics.nrppatransactionID =10;//nrppa_transaction_id ;
    // ie->value.choice.CriticalityDiagnostics.iEsCriticalityDiagnostics = ; //TODO adeel retrieve and add
    // ie->value.choice.CriticalityDiagnostics.iE_Extensions = ; //TODO adeel retrieve and add
    // TODO Retreive CriticalityDiagnostics information and assign
    // ie->value.choice.CriticalityDiagnostics. = ;
    // ie->value.choice.CriticalityDiagnostics. = ;
  }

  // printf("Test Adeel: nrppa pdu\n");
  // xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, &pdu); // test adeel
  /* Encode NRPPA message */
  if (nrppa_gNB_encode_pdu(&tx_pdu, &buffer, &length) < 0) {
    NRPPA_ERROR("Failed to encode Uplink NRPPa PositioningInformationFailure \n");
    /* Encode procedure has failed... */
    return -1;
  }

  /* Forward the NRPPA PDU to NGAP */
  if (failure_msg->nrppa_msg_info.gNB_ue_ngap_id > 0 && failure_msg->nrppa_msg_info.amf_ue_ngap_id > 0) {
    LOG_D(NRPPA,
          "Sending UplinkUEAssociatedNRPPa (PositioningInformationFailure) to NGAP (gNB_ue_ngap_id= %d, amf_ue_ngap_id =%ld)  \n",
          failure_msg->nrppa_msg_info.gNB_ue_ngap_id,
          failure_msg->nrppa_msg_info.amf_ue_ngap_id);
    nrppa_gNB_itti_send_UplinkUEAssociatedNRPPa(failure_msg->nrppa_msg_info.instance,
                                                failure_msg->nrppa_msg_info.gNB_ue_ngap_id,
                                                failure_msg->nrppa_msg_info.amf_ue_ngap_id,
                                                failure_msg->nrppa_msg_info.routing_id_buffer,
                                                failure_msg->nrppa_msg_info.routing_id_length,
                                                buffer,
                                                length); // tx_nrppa_pdu=buffer, nrppa_pdu_length=length
    return length;
  } else if (failure_msg->nrppa_msg_info.gNB_ue_ngap_id == -1 && failure_msg->nrppa_msg_info.amf_ue_ngap_id == -1) //
  {
    LOG_D(
        NRPPA,
        "Sending UplinkNonUEAssociatedNRPPa (PositioningInformationFailure) to NGAP (gNB_ue_ngap_id= %d, amf_ue_ngap_id =%ld)  \n",
        failure_msg->nrppa_msg_info.gNB_ue_ngap_id,
        failure_msg->nrppa_msg_info.amf_ue_ngap_id);
    nrppa_gNB_itti_send_UplinkNonUEAssociatedNRPPa(failure_msg->nrppa_msg_info.instance,
                                                   failure_msg->nrppa_msg_info.routing_id_buffer,
                                                   failure_msg->nrppa_msg_info.routing_id_length,
                                                   buffer,
                                                   length);
    return length;
  } else {
    NRPPA_ERROR("Failed to find context for Uplink NonUE/UE Associated NRPPa PositioningInformationFailure\n");

    return -1;
  }
}

// adeel TODO fill F1AP msg for rrc
int nrppa_gNB_PositioningInformationUpdate(
    instance_t instance,
    MessageDef *msg_p) //( uint32_t nrppa_transaction_id, uint8_t *buffer ) // TODO adeel define when and where to call this
                       // function and setup corresponding ITTI exchange to NGAP
{
  f1ap_positioning_information_update_t *update_msg = &F1AP_POSITIONING_INFORMATION_UPDATE(msg_p);
  LOG_I(NRPPA,
        "Received PositioningInformationUpdate from RRC gNB_CU_ue_id=%d, gNB_DU_ue_id=%d  rnti= %04x\n",
        update_msg->gNB_CU_ue_id,
        update_msg->gNB_DU_ue_id,
        update_msg->nrppa_msg_info.ue_rnti);

  // Prepare NRPPA Position Information Update
  NRPPA_NRPPA_PDU_t pdu; // TODO rename
  uint8_t *buffer = NULL;
  uint32_t length = 0;

  /* Prepare the NRPPA message to encode for initiating message PositioningInformationUpdate */

  // IE: 9.2.3 Message Type initiatingMessage PositioningInformationUpdate /* mandatory */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NRPPA_NRPPA_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu.choice.initiatingMessage, head);
  head->procedureCode = NRPPA_ProcedureCode_id_positioningInformationUpdate;
  head->criticality = NRPPA_Criticality_ignore;
  head->value.present = NRPPA_InitiatingMessage__value_PR_PositioningInformationUpdate;

  // IE 9.2.4 nrppatransactionID  /* mandatory */
  head->nrppatransactionID = update_msg->nrppa_msg_info.nrppa_transaction_id;

  NRPPA_PositioningInformationUpdate_t *out = &head->value.choice.PositioningInformationUpdate;

  // IE 9.2.28 SRS Configuration (O)
  /*  Currently changing SRS configuration on the fly is not possible, therefore we add the predefined SRS configuration in NRPPA
   * pdu */
  {
    asn1cSequenceAdd(out->protocolIEs.list, NRPPA_PositioningInformationUpdate_IEs_t, ie);
    ie->id = NRPPA_ProtocolIE_ID_id_SRSConfiguration;
    ie->criticality = NRPPA_Criticality_ignore;
    ie->value.present = NRPPA_PositioningInformationUpdate_IEs__value_PR_SRSConfiguration;
    // TODO
  } // IE 9.2.28 SRS Configuration

  // IE 9.2.36 SFN Initialisation Time (O)
  {
    asn1cSequenceAdd(out->protocolIEs.list, NRPPA_PositioningInformationUpdate_IEs_t, ie);
    ie->id = NRPPA_ProtocolIE_ID_id_SFNInitialisationTime;
    ie->criticality = NRPPA_Criticality_ignore;
    ie->value.present = NRPPA_PositioningInformationUpdate_IEs__value_PR_SFNInitialisationTime;
    // TODO Retreive SFN Initialisation Time and assign
    //        ie->value.choice.SFNInitialisationTime = "1253486"; //TODO adeel retrieve and add TYPE typedef struct BIT_STRING_s
    //        {uint8_t *buf;	size_t size;	int bits_unused;} BIT_STRING_t;
  }

  /* Encode NRPPA message */
  if (nrppa_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    NRPPA_ERROR("Failed to encode Uplink NRPPa PositioningInformationUpdate\n");
    /* Encode procedure has failed... */
    return -1;
  }

  if (update_msg->nrppa_msg_info.gNB_ue_ngap_id > 0 && update_msg->nrppa_msg_info.amf_ue_ngap_id > 0) {
    LOG_D(NRPPA,
          "Sending UplinkUEAssociatedNRPPa (PositioningInformationUpdate) to NGAP (gNB_ue_ngap_id= %d, amf_ue_ngap_id =%ld)  \n",
          update_msg->nrppa_msg_info.gNB_ue_ngap_id,
          update_msg->nrppa_msg_info.amf_ue_ngap_id);
    nrppa_gNB_itti_send_UplinkUEAssociatedNRPPa(update_msg->nrppa_msg_info.instance,
                                                update_msg->nrppa_msg_info.gNB_ue_ngap_id,
                                                update_msg->nrppa_msg_info.amf_ue_ngap_id,
                                                update_msg->nrppa_msg_info.routing_id_buffer,
                                                update_msg->nrppa_msg_info.routing_id_length,
                                                buffer,
                                                length); // tx_nrppa_pdu=buffer, nrppa_pdu_length=length
    return length;
  } else if (update_msg->nrppa_msg_info.gNB_ue_ngap_id == -1 && update_msg->nrppa_msg_info.amf_ue_ngap_id == -1) //
  {
    LOG_D(NRPPA,
          "Sending UplinkNonUEAssociatedNRPPa (PositioningInformationUpdate) to NGAP (gNB_ue_ngap_id= %d, amf_ue_ngap_id =%ld)  \n",
          update_msg->nrppa_msg_info.gNB_ue_ngap_id,
          update_msg->nrppa_msg_info.amf_ue_ngap_id);
    nrppa_gNB_itti_send_UplinkNonUEAssociatedNRPPa(update_msg->nrppa_msg_info.instance,
                                                   update_msg->nrppa_msg_info.routing_id_buffer,
                                                   update_msg->nrppa_msg_info.routing_id_length,
                                                   buffer,
                                                   length);
    return length;
  } else {
    NRPPA_ERROR("Failed to find context for Uplink NonUE/UE Associated NRPPa PositioningInformationUpdate\n");

    return -1;
  }
}

// adeel TODO fill F1AP msg for rrc
int nrppa_gNB_PositioningActivationResponse(instance_t instance,
                                            MessageDef *msg_p) //(uint32_t nrppa_transaction_id, uint8_t *buffer)
{
  f1ap_positioning_activation_resp_t *resp = &F1AP_POSITIONING_ACTIVATION_RESP(msg_p);
  LOG_I(NRPPA,
        "Received PositioningActivationResponse info from RRC  gNB_CU_ue_id=%d, gNB_DU_ue_id=%d  rnti= %04x\n",
        resp->gNB_CU_ue_id,
        resp->gNB_DU_ue_id,
        resp->nrppa_msg_info.ue_rnti);

  // Prepare NRPPA Positioning  Activation Response
  NRPPA_NRPPA_PDU_t pdu; // TODO rename
  uint8_t *buffer = NULL;
  uint32_t length = 0;

  /* Prepare the NRPPA message to encode for successfulOutcome PositioningActivationResponse */

  // IE: 9.2.3 Message Type successfulOutcome PositioningActivationResponse /* mandatory */
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NRPPA_NRPPA_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu.choice.successfulOutcome, head);
  head->procedureCode = NRPPA_ProcedureCode_id_positioningActivation;
  head->criticality = NRPPA_Criticality_reject;
  head->value.present = NRPPA_SuccessfulOutcome__value_PR_PositioningActivationResponse;

  // IE 9.2.4 nrppatransactionID  /* mandatory */
  head->nrppatransactionID = resp->nrppa_msg_info.nrppa_transaction_id;

  //  TODO IE 9.2.2 CriticalityDiagnostics (O)
  NRPPA_PositioningActivationResponse_t *out = &head->value.choice.PositioningActivationResponse;
  {
    asn1cSequenceAdd(out->protocolIEs.list, NRPPA_PositioningActivationResponseIEs_t, ie);
    ie->id = NRPPA_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie->criticality = NRPPA_Criticality_ignore;
    ie->value.present = NRPPA_PositioningActivationResponseIEs__value_PR_CriticalityDiagnostics;
    // TODO Retreive CriticalityDiagnostics information and assign
    // ie->value.choice.CriticalityDiagnostics.procedureCode = ; //TODO adeel retrieve and add
    // ie->value.choice.CriticalityDiagnostics.triggeringMessage; = ; //TODO adeel retrieve and add
    // ie->value.choice.CriticalityDiagnostics.procedureCriticality; = ; //TODO adeel retrieve and add
    ie->value.choice.CriticalityDiagnostics.nrppatransactionID = resp->nrppa_msg_info.nrppa_transaction_id;
    // ie->value.choice.CriticalityDiagnostics.iEsCriticalityDiagnostics = ; //TODO adeel retrieve and add
    // ie->value.choice.CriticalityDiagnostics.iE_Extensions = ; //TODO adeel retrieve and add
  }

  /* Encode NRPPA message */
  if (nrppa_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    NRPPA_ERROR("Failed to encode Uplink NRPPa PositioningActivationResponse\n");
    /* Encode procedure has failed... */
    return -1;
  }

  /* Forward the NRPPA PDU to NGAP */
  if (resp->nrppa_msg_info.gNB_ue_ngap_id > 0 && resp->nrppa_msg_info.amf_ue_ngap_id > 0) //( 1) // TODO
  {
    LOG_D(NRPPA,
          "Sending UplinkUEAssociatedNRPPa (PositioningActivationResponse) to NGAP (gNB_ue_ngap_id= %d, amf_ue_ngap_id =%ld)  \n",
          resp->nrppa_msg_info.gNB_ue_ngap_id,
          resp->nrppa_msg_info.amf_ue_ngap_id);
    nrppa_gNB_itti_send_UplinkUEAssociatedNRPPa(resp->nrppa_msg_info.instance,
                                                resp->nrppa_msg_info.gNB_ue_ngap_id,
                                                resp->nrppa_msg_info.amf_ue_ngap_id,
                                                resp->nrppa_msg_info.routing_id_buffer,
                                                resp->nrppa_msg_info.routing_id_length,
                                                buffer,
                                                length);
    return length;
  } else if (resp->nrppa_msg_info.gNB_ue_ngap_id == -1 && resp->nrppa_msg_info.amf_ue_ngap_id == -1) //
  {
    LOG_D(
        NRPPA,
        "Sending UplinkNonUEAssociatedNRPPa (PositioningActivationResponse) to NGAP (gNB_ue_ngap_id= %d, amf_ue_ngap_id =%ld)  \n",
        resp->nrppa_msg_info.gNB_ue_ngap_id,
        resp->nrppa_msg_info.amf_ue_ngap_id);
    nrppa_gNB_itti_send_UplinkNonUEAssociatedNRPPa(resp->nrppa_msg_info.instance,
                                                   resp->nrppa_msg_info.routing_id_buffer,
                                                   resp->nrppa_msg_info.routing_id_length,
                                                   buffer,
                                                   length);
    return length;
  } else {
    NRPPA_ERROR("Failed to find context for Uplink NonUE/UE Associated NRPPa PositioningActivationResponse\n");
    return -1;
  }
}

// adeel TODO fill F1AP msg for rrc
int nrppa_gNB_PositioningActivationFailure(instance_t instance,
                                           MessageDef *msg_p) //(uint32_t nrppa_transaction_id, uint8_t *buffer)
{
  f1ap_positioning_activation_failure_t *failure_msg = &F1AP_POSITIONING_ACTIVATION_FAILURE(msg_p);
  LOG_I(NRPPA,
        "Received PositioningActivationFailure info from RRC  gNB_CU_ue_id=%d, gNB_DU_ue_id=%d  rnti= %04x\n",
        failure_msg->gNB_CU_ue_id,
        failure_msg->gNB_DU_ue_id,
        failure_msg->nrppa_msg_info.ue_rnti);

  // Prepare NRPPA Positioning Activation Failure

  NRPPA_NRPPA_PDU_t pdu; // TODO rename
  uint8_t *buffer = NULL;
  uint32_t length = 0;

  /* Prepare the NRPPA message to encode for unsuccessfulOutcome PositioningActivationFailure */
  // IE: 9.2.3 Message Type unsuccessfulOutcome PositioningActivationFailure /* mandatory */

  // IE 9.2.3 Message type (M)
  memset(&pdu, 0, sizeof(pdu));
  pdu.present = NRPPA_NRPPA_PDU_PR_unsuccessfulOutcome;
  asn1cCalloc(pdu.choice.unsuccessfulOutcome, head);
  head->procedureCode = NRPPA_ProcedureCode_id_positioningActivation;
  head->criticality = NRPPA_Criticality_reject;
  head->value.present = NRPPA_UnsuccessfulOutcome__value_PR_PositioningActivationFailure;

  // IE 9.2.4 nrppatransactionID  /* mandatory */
  head->nrppatransactionID = failure_msg->nrppa_msg_info.nrppa_transaction_id;

  NRPPA_PositioningActivationFailure_t *out = &head->value.choice.PositioningActivationFailure;

  // TODO IE 9.2.1 Cause (M)
  {
    asn1cSequenceAdd(out->protocolIEs.list, NRPPA_PositioningActivationFailureIEs_t, ie);
    ie->id = NRPPA_ProtocolIE_ID_id_Cause;
    ie->criticality = NRPPA_Criticality_ignore;
    ie->value.present = NRPPA_PositioningActivationFailureIEs__value_PR_Cause;
    // TODO Reteive Cause and assign
    // ie->value.choice.Cause. = ; //IE 1
    // ie->value.choice.Cause. =;  // IE 2 and so on
    /* Send a dummy cause */
    // sample
    //    ie->value.present = NGAP_NASNonDeliveryIndication_IEs__value_PR_Cause;
    //   ie->value.choice.Cause.present = NGAP_Cause_PR_radioNetwork;
    //  ie->value.choice.Cause.choice.radioNetwork = NGAP_CauseRadioNetwork_radio_connection_with_ue_lost;
  }

  //  TODO IE 9.2.2 CriticalityDiagnostics (O)
  {
    asn1cSequenceAdd(out->protocolIEs.list, NRPPA_PositioningActivationFailureIEs_t, ie);
    ie->id = NRPPA_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie->criticality = NRPPA_Criticality_ignore;
    ie->value.present = NRPPA_PositioningActivationFailureIEs__value_PR_CriticalityDiagnostics;
    // TODO Retreive CriticalityDiagnostics information and assign
    // ie->value.choice.CriticalityDiagnostics. = ;
    // ie->value.choice.CriticalityDiagnostics. = ;
  }

  /* Encode NRPPA message */
  if (nrppa_gNB_encode_pdu(&pdu, &buffer, &length) < 0) {
    NRPPA_ERROR("Failed to encode Uplink NRPPa PositioningActivationFailure \n");
    /* Encode procedure has failed... */
    return -1;
  }

  /* Forward the NRPPA PDU to NGAP */
  if (failure_msg->nrppa_msg_info.gNB_ue_ngap_id > 0 && failure_msg->nrppa_msg_info.amf_ue_ngap_id > 0) {
    // printf("[NRPPA] Test  4.2  Sending ITTI UplinkUEAssociatedNRPPa pdu (PositioningInformationResponse/Failure) to NGAP
    // nrppa_pdu_length=%d \n", length); xer_fprint(stdout, &asn_DEF_NRPPA_NRPPA_PDU, &tx_pdu); // test ad
    LOG_D(NRPPA,
          "Sending UplinkUEAssociatedNRPPa (PositioningActivationFailure) to NGAP (gNB_ue_ngap_id= %d, amf_ue_ngap_id =%ld)  \n",
          failure_msg->nrppa_msg_info.gNB_ue_ngap_id,
          failure_msg->nrppa_msg_info.amf_ue_ngap_id);
    nrppa_gNB_itti_send_UplinkUEAssociatedNRPPa(failure_msg->nrppa_msg_info.instance,
                                                failure_msg->nrppa_msg_info.gNB_ue_ngap_id,
                                                failure_msg->nrppa_msg_info.amf_ue_ngap_id,
                                                failure_msg->nrppa_msg_info.routing_id_buffer,
                                                failure_msg->nrppa_msg_info.routing_id_length,
                                                buffer,
                                                length); // tx_nrppa_pdu=buffer, nrppa_pdu_length=length
    return length;
  } else if (failure_msg->nrppa_msg_info.gNB_ue_ngap_id == -1 && failure_msg->nrppa_msg_info.amf_ue_ngap_id == -1) //
  {
    LOG_D(NRPPA,
          "Sending UplinkNonUEAssociatedNRPPa (PositioningActivationFailure) to NGAP (gNB_ue_ngap_id= %d, amf_ue_ngap_id =%ld)  \n",
          failure_msg->nrppa_msg_info.gNB_ue_ngap_id,
          failure_msg->nrppa_msg_info.amf_ue_ngap_id);
    nrppa_gNB_itti_send_UplinkNonUEAssociatedNRPPa(failure_msg->nrppa_msg_info.instance,
                                                   failure_msg->nrppa_msg_info.routing_id_buffer,
                                                   failure_msg->nrppa_msg_info.routing_id_length,
                                                   buffer,
                                                   length);
    return length;
  } else {
    NRPPA_ERROR("Failed to find context for Uplink NonUE/UE Associated NRPPa PositioningActivationFailure\n");

    return -1;
  }
}
