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

/*! \file nrppa_gNB_handlers.c
* \brief NRPPA messages handlers for gNB
* \author Adeel Malik
* \email adeel.malik@eurecom.fr
*\date 2023
* \version 1.0
* @ingroup _nrppa
*/


#include <stdint.h>


/* TODO */
#include "intertask_interface.h"
#include "nrppa_gNB_handlers.h"
#include "nrppa_gNB_decoder.h"
#include "nrppa_common.h"
#include "nrppa_gNB_position_information_transfer_procedures.h"
#include "nrppa_gNB_TRP_information_transfer_procedures.h"
#include "nrppa_gNB_measurement_information_transfer_procedures.h"

static void allocCopy(ngap_pdu_t *out, OCTET_STRING_t in)
{
    if (in.size)
    {
        out->buffer = malloc(in.size);
        memcpy(out->buffer, in.buf, in.size);
    }
    out->length = in.size;
}

/* TODO ad**l */
char *nrppa_direction2String(int nrppa_dir)
{
    static char *nrppa_direction_String[] =
    {
        "", /* Nothing */
        "Originating message", /* originating message */
        "Successfull outcome", /* successfull outcome */
        "UnSuccessfull outcome", /* successfull outcome */
    };
    return(nrppa_direction_String[nrppa_dir]);
}

/* TODO ad**l */






/* Handlers matrix. Only gNB related procedure present here */
nrppa_message_decoded_callback nrppa_messages_callback[][3] =
{
    {0, 0, 0}, // NRPPA_ProcedureCode_id_errorIndication	((NRPPA_ProcedureCode_t)0)
    {0, 0, 0}, // NRPPA_ProcedureCode_id_privateMessage	((NRPPA_ProcedureCode_t)1)
    {0, 0, 0}, // NRPPA_ProcedureCode_id_e_CIDMeasurementInitiation	((NRPPA_ProcedureCode_t)2)
    {0, 0, 0}, // NRPPA_ProcedureCode_id_e_CIDMeasurementFailureIndication	((NRPPA_ProcedureCode_t)3)
    {0, 0, 0}, // NRPPA_ProcedureCode_id_e_CIDMeasurementReport	((NRPPA_ProcedureCode_t)4)
    {0, 0, 0}, // NRPPA_ProcedureCode_id_e_CIDMeasurementTermination	((NRPPA_ProcedureCode_t)5)
    {0, 0, 0}, // NRPPA_ProcedureCode_id_oTDOAInformationExchange	((NRPPA_ProcedureCode_t)6)
    {0, 0, 0}, // NRPPA_ProcedureCode_id_assistanceInformationControl	((NRPPA_ProcedureCode_t)7)
    {0, 0, 0}, // NRPPA_ProcedureCode_id_assistanceInformationFeedback	((NRPPA_ProcedureCode_t)8)
    {nrppa_gNB_handle_PositioningInformationExchange, 0, 0}, // NRPPA_ProcedureCode_id_positioningInformationExchange	((NRPPA_ProcedureCode_t)9) /* PositioningInformationRequest */  // todo  nrppa_gNB_handle_PositioningInformationExchange
    {0, 0, 0}, // NRPPA_ProcedureCode_id_positioningInformationUpdate	((NRPPA_ProcedureCode_t)10)
    {nrppa_gNB_handle_Measurement, 0, 0}, // NRPPA_ProcedureCode_id_Measurement	((NRPPA_ProcedureCode_t)11) /* MeasurementRequest */  // todo  nrppa_gNB_handle_Measurement
    {0, 0, 0},// NRPPA_ProcedureCode_id_MeasurementReport	((NRPPA_ProcedureCode_t)12)
    {nrppa_gNB_handle_MeasurementUpdate, 0, 0},  // NRPPA_ProcedureCode_id_MeasurementUpdate	((NRPPA_ProcedureCode_t)13) // todo  nrppa_gNB_handle_MeasurementUpdate
    {nrppa_gNB_handle_MeasurementAbort, 0, 0},  // NRPPA_ProcedureCode_id_MeasurementAbort	((NRPPA_ProcedureCode_t)14) // todo  nrppa_gNB_handle_MeasurementAbort
    {0, 0, 0},// NRPPA_ProcedureCode_id_MeasurementFailureIndication	((NRPPA_ProcedureCode_t)15)
    {nrppa_gNB_handle_TRPInformationExchange, 0, 0}, // NRPPA_ProcedureCode_id_tRPInformationExchange	((NRPPA_ProcedureCode_t)16) /* TRPInformationRequest */  // todo  nrppa_gNB_handle_TRPInformationExchange
    {nrppa_gNB_handle_PositioningActivation, 0, 0},// NRPPA_ProcedureCode_id_positioningActivation	((NRPPA_ProcedureCode_t)17) /* PositioningActivationRequest */ // todo  nrppa_gNB_handle_PositioningActivation
    {nrppa_gNB_handle_PositioningDeactivation, 0, 0}, // NRPPA_ProcedureCode_id_positioningDeactivation	((NRPPA_ProcedureCode_t)18) /* PositioningDeactivation */ // todo  nrppa_gNB_handle_PositioningDeactivation
};



//Processing DownLINK UE ASSOCIATED NRPPA TRANSPORT
int nrppa_handle_DownlinkUEAssociatedNRPPaTransport(instance_t instance, ngap_DownlinkUEAssociatedNRPPa_t *ngap_DownlinkUEAssociatedNRPPa_p)
{
//int nrppa_handle_DownlinkUEAssociatedNRPPaTransport(ngap_DownlinkUEAssociatedNRPPa_t *ngap_DownlinkUEAssociatedNRPPa_p){
// instance_t instance
//ngap_DownlinkUEAssociatedNRPPa_p->amf_ue_ngap_id //NGAP_RAN_UE_NGAP_ID_t            gnb_ue_ngap_id;
//ngap_DownlinkUEAssociatedNRPPa_p->gNB_ue_ngap_id //uint64_t                         amf_ue_ngap_id;
//uint8_t *const routing_id_buff = ngap_DownlinkUEAssociatedNRPPa_p-->routing_id.buffer;
//const uint32_t routing_id_buff_len = ngap_DownlinkUEAssociatedNRPPa_p-->routing_id.length;
 printf("[NRPPA] Test 3 Adeel:  nrppa_handle_DownlinkUEAssociatedNRPPaTransport Handling case NGAP_DOWNLINKUEASSOCIATEDNRPPA \n");
    uint8_t *const data = ngap_DownlinkUEAssociatedNRPPa_p->nrppa_pdu.buffer;
    const uint32_t data_length= ngap_DownlinkUEAssociatedNRPPa_p->nrppa_pdu.length;
    NRPPA_NRPPA_PDU_t pdu;
    int ret;
    DevAssert(data != NULL);
    memset(&pdu, 0, sizeof(pdu));

printf("\n TEST 3: nrppa_handle_DownlinkUEAssociatedNRPPaTransport Nrppa pdu size =%d and value is \n ", data_length);
/*uint8_t *nrppa_pdu_buffer= ngap_DownlinkUEAssociatedNRPPa_p->nrppa_pdu.buffer;
for (int i = 0; i < data_length; i++){
printf("%02x ", *nrppa_pdu_buffer++);
//printf("%d ", *nrppa_pdu_buffer++);
//printf("%p ", nrppa_pdu_buffer++);
}*/
printf("\n");

    if (nrppa_gNB_decode_pdu(&pdu, data, data_length) < 0)    // todo nrppa_gNB_decode_pdu
    {
        NRPPA_ERROR("Failed to decode PDU\n");
        return -1;
    }
//printf("Test 3 Adeel:  nrppa_handle_DownlinkUEAssociatedNRPPaTransport calling right handler \n");
    /* Checking procedure Code and direction of message*/
    //printf("Test 4 Adeel:  nrppa_handle_DownlinkUEAssociatedNRPPaTransport  calling right handler sizeof(nrppa_messages_callback)%d (3 * sizeof(nrppaa_message_decoded_callback)%d \n", sizeof(nrppa_messages_callback), 3*sizeof(nrppa_message_decoded_callback));
    printf("[NRPPA] Test 4 Adeel: procedureCode is %ld and direction %d is \n", pdu.choice.initiatingMessage->procedureCode, pdu.present);
    if (pdu.choice.initiatingMessage->procedureCode >= sizeof(nrppa_messages_callback) / (3 * sizeof(nrppa_message_decoded_callback)) || (pdu.present > NRPPA_NRPPA_PDU_PR_unsuccessfulOutcome))
    {
//    NRPPA_ERROR("[NGAP %d] Either procedureCode %ld or direction %d exceed expected\n", assoc_id, pdu.choice.initiatingMessage->procedureCode, pdu.present); ad**l todo
        NRPPA_ERROR("[NGAP] Either procedureCode %ld or direction %d exceed expected\n", pdu.choice.initiatingMessage->procedureCode, pdu.present);
        ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NRPPA_NRPPA_PDU, &pdu);
        return -1;
    }

    /* No handler present.
     * This can mean not implemented or no procedure for gNB (wrong direction).*/
     printf("Test 5 Adeel: procedureCode is %ld and direction %d is \n", pdu.choice.initiatingMessage->procedureCode, pdu.present);
    if (nrppa_messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1] == NULL)
    {
//    NRPPA_ERROR("[NGAP %d] No handler for procedureCode %ld in %s\n", assoc_id, pdu.choice.initiatingMessage->procedureCode, nrppa_direction2String(pdu.present - 1));  ad**l todo
        NRPPA_ERROR("[NGAP] No handler for procedureCode %ld in %s\n", pdu.choice.initiatingMessage->procedureCode, nrppa_direction2String(pdu.present - 1));
        ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NRPPA_NRPPA_PDU, &pdu);
        return -1;
    }
    /* Calling the right handler*/
    nrppa_gnb_ue_info_t nrppa_msg_info;
    nrppa_msg_info.instance= instance;
    nrppa_msg_info.gNB_ue_ngap_id =ngap_DownlinkUEAssociatedNRPPa_p->gNB_ue_ngap_id;
    nrppa_msg_info.amf_ue_ngap_id =ngap_DownlinkUEAssociatedNRPPa_p->amf_ue_ngap_id;
    nrppa_msg_info.routing_id_buffer =ngap_DownlinkUEAssociatedNRPPa_p->routing_id.buffer;
    nrppa_msg_info.routing_id_length =ngap_DownlinkUEAssociatedNRPPa_p->routing_id.length;

    ret = (*nrppa_messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1])(&nrppa_msg_info, &pdu); // ad**l

    /*  ret = (*nrppa_messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1])(instance,
                                                                                                     ngap_DownlinkUEAssociatedNRPPa_p->gNB_ue_ngap_id,
                                                                                                     ngap_DownlinkUEAssociatedNRPPa_p->amf_ue_ngap_id,
                                                                                                     ngap_DownlinkUEAssociatedNRPPa_p->routing_id.buffer,
                                                                                                     ngap_DownlinkUEAssociatedNRPPa_p->routing_id.length,
                                                                                                     &pdu); // ad**l*/
    printf("Test 8 Adeel: procedureCode is %ld and direction %d is \n", pdu.choice.initiatingMessage->procedureCode, pdu.present);
    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NRPPA_NRPPA_PDU, &pdu);
    return ret;
}



//Processing DOWNLINK NON UE ASSOCIATED NRPPA TRANSPORT (9.2.9.4 of TS 38.413 Version 16.0.0.0 Release 16)
int nrppa_handle_DownlinkNonUEAssociatedNRPPaTransport(instance_t instance, ngap_DownlinkNonUEAssociatedNRPPa_t *ngap_DownlinkNonUEAssociatedNRPPa_p)
{
//int nrppa_handle_DownlinkNonUEAssociatedNRPPaTransport(ngap_DownlinkNonUEAssociatedNRPPa_t *ngap_DownlinkNonUEAssociatedNRPPa_p){
    /*TODO*/
//ngap_DownlinkNonUEAssociatedNRPPa_p->routing_id
//uint8_t *const routing_id_buff = ngap_DownlinkNonUEAssociatedNRPPa_p-->routing_id.buffer;
//const uint32_t routing_id_buff_len = ngap_DownlinkNonUEAssociatedNRPPa_p-->routing_id.length;

    uint8_t *const data = &ngap_DownlinkNonUEAssociatedNRPPa_p->nrppa_pdu.buffer;
    const uint32_t data_length= ngap_DownlinkNonUEAssociatedNRPPa_p->nrppa_pdu.length;
    NRPPA_NRPPA_PDU_t pdu;
    int ret;
    DevAssert(data != NULL);
    memset(&pdu, 0, sizeof(pdu));

    if (nrppa_gNB_decode_pdu(&pdu, data, data_length) < 0)    // todo nrppa_gNB_decode_pdu
    {
        NRPPA_ERROR("Failed to decode Downlink Non UE Associated NRPPa PDU\n");
        return -1;
    }

    /* Checking procedure Code and direction of message*/
    if (pdu.choice.initiatingMessage->procedureCode >= sizeof(nrppa_messages_callback) / (3 * sizeof(nrppa_message_decoded_callback)) || (pdu.present > NRPPA_NRPPA_PDU_PR_unsuccessfulOutcome))
    {
//    NRPPA_ERROR("[NGAP %d] Either procedureCode %ld or direction %d exceed expected\n", assoc_id, pdu.choice.initiatingMessage->procedureCode, pdu.present); ad**l todo
        NRPPA_ERROR("[NGAP] Either procedureCode %ld or direction %d exceed expected\n", pdu.choice.initiatingMessage->procedureCode, pdu.present);
        ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NRPPA_NRPPA_PDU, &pdu);
        return -1;
    }

    /* No handler present.
     * This can mean not implemented or no procedure for gNB (wrong direction).*/

    if (nrppa_messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1] == NULL)
    {
//    NRPPA_ERROR("[NGAP %d] No handler for procedureCode %ld in %s\n", assoc_id, pdu.choice.initiatingMessage->procedureCode, nrppa_direction2String(pdu.present - 1));  ad**l todo
        NRPPA_ERROR("[NGAP] No handler for procedureCode %ld in %s\n", pdu.choice.initiatingMessage->procedureCode, nrppa_direction2String(pdu.present - 1));
        ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NRPPA_NRPPA_PDU, &pdu);
        return -1;
    }

    /* Calling the right handler*/
//  ret = (*nrppa_messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1])(assoc_id, stream, &pdu);
    //ret = (*nrppa_messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1])(, &pdu); // ad**l
    /*ret = (*nrppa_messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1])(instance,
                                                                                                   0,
                                                                                                   0,
                                                                                                   ngap_DownlinkNonUEAssociatedNRPPa_p->routing_id.buffer,
                                                                                                   ngap_DownlinkNonUEAssociatedNRPPa_p->routing_id.length,
                                                                                                   &pdu); // ad**l*/
    nrppa_gnb_ue_info_t nrppa_msg_info;
    nrppa_msg_info.instance= instance;
    nrppa_msg_info.gNB_ue_ngap_id =-1;//%0; TODO check
    nrppa_msg_info.amf_ue_ngap_id =-1;//%0;
    nrppa_msg_info.routing_id_buffer =ngap_DownlinkNonUEAssociatedNRPPa_p->routing_id.buffer;
    nrppa_msg_info.routing_id_length =ngap_DownlinkNonUEAssociatedNRPPa_p->routing_id.length;

    ret = (*nrppa_messages_callback[pdu.choice.initiatingMessage->procedureCode][pdu.present - 1])(&nrppa_msg_info, &pdu); // ad**l


    ASN_STRUCT_FREE_CONTENTS_ONLY(asn_DEF_NRPPA_NRPPA_PDU, &pdu);
    return ret;

}







