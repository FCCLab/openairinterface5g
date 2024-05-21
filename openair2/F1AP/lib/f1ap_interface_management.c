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

#include <string.h>

#include "common/utils/assertions.h"
#include "openair3/UTILS/conversions.h"
#include "common/utils/oai_asn1.h"
#include "common/utils/utils.h"

#include "f1ap_interface_management.h"
#include "f1ap_lib_common.h"
#include "f1ap_lib_includes.h"
#include "f1ap_messages_types.h"
#include "f1ap_lib_extern.h"

const int nrb_lut[29] = {11,  18,  24,  25,  31,  32,  38,  51,  52,  65,  66,  78,  79,  93, 106,
                         107, 121, 132, 133, 135, 160, 162, 189, 216, 217, 245, 264, 270, 273};

static int to_NRNRB(int nrb)
{
  for (int i = 0; i < sizeofArray(nrb_lut); i++)
    if (nrb_lut[i] == nrb)
      return i;
  AssertFatal(1 == 0, "nrb %d is not in the list of possible NRNRB\n", nrb);
}

/**
 * @brief F1AP Setup Request memory management
 */
static void free_f1ap_cell(f1ap_served_cell_info_t *info, f1ap_gnb_du_system_info_t *sys_info)
{
  if (sys_info) {
    free(sys_info->mib);
    free(sys_info->sib1);
    free(sys_info);
  }
  if (info->measurement_timing_config)
    free(info->measurement_timing_config);
  if (info->tac)
    free(info->tac);
}

static F1AP_Served_Cell_Information_t encode_served_cell_info(const f1ap_served_cell_info_t *c)
{
  /* 4.1.1 served cell Information */
  F1AP_Served_Cell_Information_t scell_info = {0};
  addnRCGI(scell_info.nRCGI, c);

  /* - nRPCI */
  scell_info.nRPCI = c->nr_pci; // int 0..1007

  /* - fiveGS_TAC */
  if (c->tac != NULL) {
    uint32_t tac = htonl(*c->tac);
    asn1cCalloc(scell_info.fiveGS_TAC, netOrder);
    OCTET_STRING_fromBuf(netOrder, ((char *)&tac) + 1, 3);
  }

  /* - Configured_EPS_TAC */
  if (0) {
    scell_info.configured_EPS_TAC = (F1AP_Configured_EPS_TAC_t *)calloc(1, sizeof(F1AP_Configured_EPS_TAC_t));
    OCTET_STRING_fromBuf(scell_info.configured_EPS_TAC, "2", 2);
  }

  /* servedPLMN information */
  asn1cSequenceAdd(scell_info.servedPLMNs.list, F1AP_ServedPLMNs_Item_t, servedPLMN_item);
  MCC_MNC_TO_PLMNID(c->plmn.mcc, c->plmn.mnc, c->plmn.mnc_digit_length, &servedPLMN_item->pLMN_Identity);

  F1AP_NR_Mode_Info_t *nR_Mode_Info = &scell_info.nR_Mode_Info;

  if (c->mode == F1AP_MODE_FDD) { // FDD
    const f1ap_fdd_info_t *fdd = &c->fdd;
    nR_Mode_Info->present = F1AP_NR_Mode_Info_PR_fDD;
    asn1cCalloc(nR_Mode_Info->choice.fDD, fDD_Info);
    /* FDD.1.1 UL NRFreqInfo ARFCN */
    fDD_Info->uL_NRFreqInfo.nRARFCN = fdd->ul_freqinfo.arfcn;

    /* FDD.1.3 freqBandListNr */
    int ul_band = 1;
    for (int j = 0; j < ul_band; j++) {
      asn1cSequenceAdd(fDD_Info->uL_NRFreqInfo.freqBandListNr.list, F1AP_FreqBandNrItem_t, nr_freqBandNrItem);
      /* FDD.1.3.1 freqBandIndicatorNr*/
      nr_freqBandNrItem->freqBandIndicatorNr = fdd->ul_freqinfo.band;
    }

    /* FDD.2.1 DL NRFreqInfo ARFCN */
    fDD_Info->dL_NRFreqInfo.nRARFCN = fdd->dl_freqinfo.arfcn;
    /* FDD.2.3 freqBandListNr */
    int dl_bands = 1;
    for (int j = 0; j < dl_bands; j++) {
      asn1cSequenceAdd(fDD_Info->dL_NRFreqInfo.freqBandListNr.list, F1AP_FreqBandNrItem_t, nr_freqBandNrItem);
      /* FDD.2.3.1 freqBandIndicatorNr*/
      nr_freqBandNrItem->freqBandIndicatorNr = fdd->dl_freqinfo.band;
    } // for FDD : DL freq_Bands
    /* FDD.3 UL Transmission Bandwidth */
    fDD_Info->uL_Transmission_Bandwidth.nRSCS = fdd->ul_tbw.scs;
    fDD_Info->uL_Transmission_Bandwidth.nRNRB = to_NRNRB(fdd->ul_tbw.nrb);
    /* FDD.4 DL Transmission Bandwidth */
    fDD_Info->dL_Transmission_Bandwidth.nRSCS = fdd->dl_tbw.scs;
    fDD_Info->dL_Transmission_Bandwidth.nRNRB = to_NRNRB(fdd->dl_tbw.nrb);
  } else if (c->mode == F1AP_MODE_TDD) {
    const f1ap_tdd_info_t *tdd = &c->tdd;
    nR_Mode_Info->present = F1AP_NR_Mode_Info_PR_tDD;
    asn1cCalloc(nR_Mode_Info->choice.tDD, tDD_Info);
    /* TDD.1.1 nRFreqInfo ARFCN */
    tDD_Info->nRFreqInfo.nRARFCN = tdd->freqinfo.arfcn;
    /* TDD.1.3 freqBandListNr */
    int bands = 1;
    for (int j = 0; j < bands; j++) {
      asn1cSequenceAdd(tDD_Info->nRFreqInfo.freqBandListNr.list, F1AP_FreqBandNrItem_t, nr_freqBandNrItem);
      /* TDD.1.3.1 freqBandIndicatorNr*/
      nr_freqBandNrItem->freqBandIndicatorNr = tdd->freqinfo.band;
    }
    /* TDD.2 transmission_Bandwidth */
    tDD_Info->transmission_Bandwidth.nRSCS = tdd->tbw.scs;
    tDD_Info->transmission_Bandwidth.nRNRB = to_NRNRB(tdd->tbw.nrb);
  } else {
    AssertFatal(false, "unknown duplex mode %d\n", c->mode);
  }

  /* - measurementTimingConfiguration */
  OCTET_STRING_fromBuf(&scell_info.measurementTimingConfiguration,
                       (const char *)c->measurement_timing_config,
                       c->measurement_timing_config_len);

  return scell_info;
}

static F1AP_GNB_DU_System_Information_t *encode_system_info(const f1ap_gnb_du_system_info_t *sys_info)
{
  if (sys_info == NULL)
    return NULL; /* optional: can be NULL */

  F1AP_GNB_DU_System_Information_t *enc_sys_info = calloc(1, sizeof(*enc_sys_info));
  AssertFatal(enc_sys_info != NULL, "out of memory\n");

  AssertFatal(sys_info->mib != NULL, "MIB must be present in DU sys info\n");
  OCTET_STRING_fromBuf(&enc_sys_info->mIB_message, (const char *)sys_info->mib, sys_info->mib_length);

  AssertFatal(sys_info->sib1 != NULL, "SIB1 must be present in DU sys info\n");
  OCTET_STRING_fromBuf(&enc_sys_info->sIB1_message, (const char *)sys_info->sib1, sys_info->sib1_length);

  return enc_sys_info;
}

/* ====================================
 *          F1AP Setup Request
 * ==================================== */

/**
 * @brief F1AP Setup Request encoding
 */
F1AP_F1AP_PDU_t *encode_f1ap_setup_request(const f1ap_setup_req_t *msg)
{
  F1AP_F1AP_PDU_t *pdu = calloc(1, sizeof(*pdu));
  AssertFatal(pdu != NULL, "out of memory\n");

  /* Create */
  /* 0. pdu Type */
  pdu->present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu->choice.initiatingMessage, initMsg);
  initMsg->procedureCode = F1AP_ProcedureCode_id_F1Setup;
  initMsg->criticality = F1AP_Criticality_reject;
  initMsg->value.present = F1AP_InitiatingMessage__value_PR_F1SetupRequest;
  F1AP_F1SetupRequest_t *f1Setup = &initMsg->value.choice.F1SetupRequest;
  /* mandatory */
  /* c1. Transaction ID (integer value) */
  asn1cSequenceAdd(f1Setup->protocolIEs.list, F1AP_F1SetupRequestIEs_t, ieC1);
  ieC1->id = F1AP_ProtocolIE_ID_id_TransactionID;
  ieC1->criticality = F1AP_Criticality_reject;
  ieC1->value.present = F1AP_F1SetupRequestIEs__value_PR_TransactionID;
  ieC1->value.choice.TransactionID = msg->transaction_id;
  /* mandatory */
  /* c2. GNB_DU_ID (integer value) */
  asn1cSequenceAdd(f1Setup->protocolIEs.list, F1AP_F1SetupRequestIEs_t, ieC2);
  ieC2->id = F1AP_ProtocolIE_ID_id_gNB_DU_ID;
  ieC2->criticality = F1AP_Criticality_reject;
  ieC2->value.present = F1AP_F1SetupRequestIEs__value_PR_GNB_DU_ID;
  asn_int642INTEGER(&ieC2->value.choice.GNB_DU_ID, msg->gNB_DU_id);
  /* optional */
  /* c3. GNB_DU_Name */
  if (msg->gNB_DU_name) {
    asn1cSequenceAdd(f1Setup->protocolIEs.list, F1AP_F1SetupRequestIEs_t, ieC3);
    ieC3->id = F1AP_ProtocolIE_ID_id_gNB_DU_Name;
    ieC3->criticality = F1AP_Criticality_ignore;
    ieC3->value.present = F1AP_F1SetupRequestIEs__value_PR_GNB_DU_Name;
    char *gNB_DU_name = msg->gNB_DU_name;
    OCTET_STRING_fromBuf(&ieC3->value.choice.GNB_DU_Name, gNB_DU_name, strlen(gNB_DU_name));
  }

  /* mandatory */
  /* c4. served cells list */
  asn1cSequenceAdd(f1Setup->protocolIEs.list, F1AP_F1SetupRequestIEs_t, ieCells);
  ieCells->id = F1AP_ProtocolIE_ID_id_gNB_DU_Served_Cells_List;
  ieCells->criticality = F1AP_Criticality_reject;
  ieCells->value.present = F1AP_F1SetupRequestIEs__value_PR_GNB_DU_Served_Cells_List;
  for (int i = 0; i < msg->num_cells_available; i++) {
    /* mandatory */
    /* 4.1 served cells item */
    const f1ap_served_cell_info_t *cell = &msg->cell[i].info;
    const f1ap_gnb_du_system_info_t *sys_info = msg->cell[i].sys_info;
    asn1cSequenceAdd(ieCells->value.choice.GNB_DU_Served_Cells_List.list, F1AP_GNB_DU_Served_Cells_ItemIEs_t, duServedCell);
    duServedCell->id = F1AP_ProtocolIE_ID_id_GNB_DU_Served_Cells_Item;
    duServedCell->criticality = F1AP_Criticality_reject;
    duServedCell->value.present = F1AP_GNB_DU_Served_Cells_ItemIEs__value_PR_GNB_DU_Served_Cells_Item;
    F1AP_GNB_DU_Served_Cells_Item_t *scell_item = &duServedCell->value.choice.GNB_DU_Served_Cells_Item;
    scell_item->served_Cell_Information = encode_served_cell_info(cell);
    scell_item->gNB_DU_System_Information = encode_system_info(sys_info);
  }

  /* mandatory */
  /* c5. RRC VERSION */
  asn1cSequenceAdd(f1Setup->protocolIEs.list, F1AP_F1SetupRequestIEs_t, ie2);
  ie2->id = F1AP_ProtocolIE_ID_id_GNB_DU_RRC_Version;
  ie2->criticality = F1AP_Criticality_reject;
  ie2->value.present = F1AP_F1SetupRequestIEs__value_PR_RRC_Version;
  // RRC Version: "This IE is not used in this release."
  // we put one bit for each byte in rrc_ver that is != 0
  uint8_t bits = 0;
  for (int i = 0; i < sizeofArray(msg->rrc_ver); ++i)
    bits |= (msg->rrc_ver[i] != 0) << i;
  BIT_STRING_t *bs = &ie2->value.choice.RRC_Version.latest_RRC_Version;
  bs->buf = calloc(1, sizeof(char));
  AssertFatal(bs->buf != NULL, "out of memory\n");
  bs->buf[0] = bits;
  bs->size = 1;
  bs->bits_unused = 5;

  F1AP_ProtocolExtensionContainer_10696P228_t *p = calloc(1, sizeof(F1AP_ProtocolExtensionContainer_10696P228_t));
  asn1cSequenceAdd(p->list, F1AP_RRC_Version_ExtIEs_t, rrcv_ext);
  rrcv_ext->id = F1AP_ProtocolIE_ID_id_latest_RRC_Version_Enhanced;
  rrcv_ext->criticality = F1AP_Criticality_ignore;
  rrcv_ext->extensionValue.present = F1AP_RRC_Version_ExtIEs__extensionValue_PR_OCTET_STRING_SIZE_3_;
  OCTET_STRING_t *os = &rrcv_ext->extensionValue.choice.OCTET_STRING_SIZE_3_;
  os->size = 3;
  os->buf = malloc(sizeofArray(msg->rrc_ver) * sizeof(*os->buf));
  AssertFatal(os->buf != NULL, "out of memory\n");
  for (int i = 0; i < sizeofArray(msg->rrc_ver); ++i)
    os->buf[i] = msg->rrc_ver[i];
  ie2->value.choice.RRC_Version.iE_Extensions = (struct F1AP_ProtocolExtensionContainer *)p;
  return pdu;
}

/**
 * @brief F1AP Setup Request decoding
 */
bool decode_f1ap_setup_request(const F1AP_F1AP_PDU_t *pdu, f1ap_setup_req_t *out)
{
  F1AP_F1SetupRequest_t *container = &pdu->choice.initiatingMessage->value.choice.F1SetupRequest;
  F1AP_F1SetupRequestIEs_t *ie;

  /* Transaction ID*/
  F1AP_LIB_FIND_IE(F1AP_F1SetupRequestIEs_t, ie, container, F1AP_ProtocolIE_ID_id_TransactionID, true);
  out->transaction_id = ie->value.choice.TransactionID;

  /* gNB_DU_id */
  // this function exits if the ie is mandatory
  F1AP_LIB_FIND_IE(F1AP_F1SetupRequestIEs_t, ie, container, F1AP_ProtocolIE_ID_id_gNB_DU_ID, true);
  asn_INTEGER2ulong(&ie->value.choice.GNB_DU_ID, &out->gNB_DU_id);
  /* gNB_DU_name */
  F1AP_LIB_FIND_IE(F1AP_F1SetupRequestIEs_t, ie, container, F1AP_ProtocolIE_ID_id_gNB_DU_Name, false);
  out->gNB_DU_name = NULL;
  if (ie != NULL) {
    out->gNB_DU_name = calloc(ie->value.choice.GNB_DU_Name.size + 1, sizeof(char));
    *out->gNB_DU_name = *ie->value.choice.GNB_DU_Name.buf;
  }
  /* GNB_DU_Served_Cells_List */
  F1AP_LIB_FIND_IE(F1AP_F1SetupRequestIEs_t, ie, container, F1AP_ProtocolIE_ID_id_gNB_DU_Served_Cells_List, true);
  out->num_cells_available = ie->value.choice.GNB_DU_Served_Cells_List.list.count;
  for (int i = 0; i < out->num_cells_available; i++) {
    F1AP_GNB_DU_Served_Cells_Item_t *served_cells_item =
        &(((F1AP_GNB_DU_Served_Cells_ItemIEs_t *)ie->value.choice.GNB_DU_Served_Cells_List.list.array[i])
              ->value.choice.GNB_DU_Served_Cells_Item);
    F1AP_Served_Cell_Information_t *servedCellInformation = &served_cells_item->served_Cell_Information;

    /* tac */
    if (servedCellInformation->fiveGS_TAC) {
      out->cell[i].info.tac = malloc(sizeof(*out->cell[i].info.tac));
      AssertFatal(out->cell[i].info.tac != NULL, "out of memory\n");
      OCTET_STRING_TO_INT24(servedCellInformation->fiveGS_TAC, *out->cell[i].info.tac);
    }

    /* - nRCGI */
    TBCD_TO_MCC_MNC(&(servedCellInformation->nRCGI.pLMN_Identity),
                    out->cell[i].info.plmn.mcc,
                    out->cell[i].info.plmn.mnc,
                    out->cell[i].info.plmn.mnc_digit_length);
    // NR cellID
    BIT_STRING_TO_NR_CELL_IDENTITY(&servedCellInformation->nRCGI.nRCellIdentity, out->cell[i].info.nr_cellid);
    /* - nRPCI */
    out->cell[i].info.nr_pci = servedCellInformation->nRPCI;
    // FDD Cells
    if (servedCellInformation->nR_Mode_Info.present == F1AP_NR_Mode_Info_PR_fDD) {
      out->cell[i].info.mode = F1AP_MODE_FDD;
      f1ap_fdd_info_t *FDDs = &out->cell[i].info.fdd;
      F1AP_FDD_Info_t *fDD_Info = servedCellInformation->nR_Mode_Info.choice.fDD;
      FDDs->ul_freqinfo.arfcn = fDD_Info->uL_NRFreqInfo.nRARFCN;
      AssertFatal(fDD_Info->uL_NRFreqInfo.freqBandListNr.list.count == 1, "cannot handle more than one frequency band\n");
      for (int f = 0; f < fDD_Info->uL_NRFreqInfo.freqBandListNr.list.count; f++) {
        F1AP_FreqBandNrItem_t *FreqItem = fDD_Info->uL_NRFreqInfo.freqBandListNr.list.array[f];
        FDDs->ul_freqinfo.band = FreqItem->freqBandIndicatorNr;
        AssertFatal(FreqItem->supportedSULBandList.list.count == 0, "cannot handle SUL bands!\n");
      }
      FDDs->dl_freqinfo.arfcn = fDD_Info->dL_NRFreqInfo.nRARFCN;
      int dlBands = fDD_Info->dL_NRFreqInfo.freqBandListNr.list.count;
      AssertFatal(dlBands == 1, "cannot handled more than one frequency band\n");
      for (int dlB = 0; dlB < dlBands; dlB++) {
        F1AP_FreqBandNrItem_t *FreqItem = fDD_Info->dL_NRFreqInfo.freqBandListNr.list.array[dlB];
        FDDs->dl_freqinfo.band = FreqItem->freqBandIndicatorNr;
        int num_available_supported_SULBands = FreqItem->supportedSULBandList.list.count;
        AssertFatal(num_available_supported_SULBands == 0, "cannot handle SUL bands!\n");
      }
      FDDs->ul_tbw.scs = fDD_Info->uL_Transmission_Bandwidth.nRSCS;
      FDDs->ul_tbw.nrb = nrb_lut[fDD_Info->uL_Transmission_Bandwidth.nRNRB];
      FDDs->dl_tbw.scs = fDD_Info->dL_Transmission_Bandwidth.nRSCS;
      FDDs->dl_tbw.nrb = nrb_lut[fDD_Info->dL_Transmission_Bandwidth.nRNRB];
    } else if (servedCellInformation->nR_Mode_Info.present == F1AP_NR_Mode_Info_PR_tDD) {
      out->cell[i].info.mode = F1AP_MODE_TDD;
      f1ap_tdd_info_t *TDDs = &out->cell[i].info.tdd;
      F1AP_TDD_Info_t *tDD_Info = servedCellInformation->nR_Mode_Info.choice.tDD;
      TDDs->freqinfo.arfcn = tDD_Info->nRFreqInfo.nRARFCN;
      AssertFatal(tDD_Info->nRFreqInfo.freqBandListNr.list.count == 1, "cannot handle more than one frequency band\n");
      for (int f = 0; f < tDD_Info->nRFreqInfo.freqBandListNr.list.count; f++) {
        struct F1AP_FreqBandNrItem *FreqItem = tDD_Info->nRFreqInfo.freqBandListNr.list.array[f];
        TDDs->freqinfo.band = FreqItem->freqBandIndicatorNr;
        int num_available_supported_SULBands = FreqItem->supportedSULBandList.list.count;
        AssertFatal(num_available_supported_SULBands == 0, "cannot hanlde SUL bands!\n");
      }
      TDDs->tbw.scs = tDD_Info->transmission_Bandwidth.nRSCS;
      TDDs->tbw.nrb = nrb_lut[tDD_Info->transmission_Bandwidth.nRNRB];
    } else {
      AssertFatal(false, "unknown NR Mode info %d\n", servedCellInformation->nR_Mode_Info.present);
    }

    /* MeasurementConfig */
    if (servedCellInformation->measurementTimingConfiguration.size > 0)
      out->cell[i].info.measurement_timing_config =
          cp_octet_string(&servedCellInformation->measurementTimingConfiguration, &out->cell[i].info.measurement_timing_config_len);

    struct F1AP_GNB_DU_System_Information *DUsi = served_cells_item->gNB_DU_System_Information;
    if (DUsi != NULL) {
      // System Information
      out->cell[i].sys_info = calloc(1, sizeof(*out->cell[i].sys_info));
      AssertFatal(out->cell[i].sys_info != NULL, "out of memory\n");
      f1ap_gnb_du_system_info_t *sys_info = out->cell[i].sys_info;
      /* mib */
      sys_info->mib = calloc(DUsi->mIB_message.size, sizeof(char));
      *sys_info->mib = *DUsi->mIB_message.buf;
      sys_info->mib_length = DUsi->mIB_message.size;
      /* sib1 */
      sys_info->sib1 = calloc(DUsi->sIB1_message.size, sizeof(char));
      *sys_info->sib1 = *DUsi->sIB1_message.buf;
      sys_info->sib1_length = DUsi->sIB1_message.size;
    }
  }

  /* Handle RRC Version */
  F1AP_LIB_FIND_IE(F1AP_F1SetupRequestIEs_t, ie, container, F1AP_ProtocolIE_ID_id_GNB_DU_RRC_Version, true);
  // Latest RRC Version: "This IE is not used in this release."
  // BIT_STRING_to_uint8(&ie->value.choice.RRC_Version.latest_RRC_Version);
  if (ie->value.choice.RRC_Version.iE_Extensions) {
    F1AP_ProtocolExtensionContainer_10696P228_t *ext =
        (F1AP_ProtocolExtensionContainer_10696P228_t *)ie->value.choice.RRC_Version.iE_Extensions;
    if (ext->list.count > 0) {
      F1AP_RRC_Version_ExtIEs_t *rrcext = ext->list.array[0];
      OCTET_STRING_t *os = &rrcext->extensionValue.choice.OCTET_STRING_SIZE_3_;
      DevAssert(os->size == 3);
      for (int i = 0; i < 3; ++i)
        out->rrc_ver[i] = os->buf[i];
    }
  }
  return true;
}

/**
 * @brief F1AP Setup Request deep copy
 */
f1ap_setup_req_t cp_f1ap_setup_request(const f1ap_setup_req_t *msg)
{
  f1ap_setup_req_t cp;
  /* gNB_DU_id */
  cp.gNB_DU_id = msg->gNB_DU_id;
  /* gNB_DU_name */
  cp.gNB_DU_name = strdup(msg->gNB_DU_name);
  /* transaction_id */
  cp.transaction_id = msg->transaction_id;
  /* num_cells_available */
  cp.num_cells_available = msg->num_cells_available;
  for (int n = 0; n < msg->num_cells_available; n++) {
    /* cell.info */
    cp.cell[n].info = msg->cell[n].info;
    cp.cell[n].info.mode = msg->cell[n].info.mode;
    cp.cell[n].info.tdd = msg->cell[n].info.tdd;
    cp.cell[n].info.fdd = msg->cell[n].info.fdd;
    if (msg->cell[n].info.tac) {
      cp.cell[n].info.tac = malloc(sizeof(*cp.cell[n].info.tac));
      AssertFatal(cp.cell[n].info.tac != NULL, "out of memory\n");
      *cp.cell[n].info.tac = *msg->cell[n].info.tac;
    }
    if (msg->cell[n].info.measurement_timing_config_len) {
      cp.cell[n].info.measurement_timing_config = calloc(msg->cell[n].info.measurement_timing_config_len, sizeof(uint8_t));
      *cp.cell[n].info.measurement_timing_config = *msg->cell[n].info.measurement_timing_config;
    }
    cp.cell[n].info.plmn = msg->cell[n].info.plmn;
    /* cell.sys_info */
    if (msg->cell[n].sys_info) {
      f1ap_gnb_du_system_info_t *orig_sys_info = msg->cell[n].sys_info;
      f1ap_gnb_du_system_info_t *copy_sys_info = calloc(1, sizeof(*copy_sys_info));
      AssertFatal(copy_sys_info, "out of memory\n");
      cp.cell[n].sys_info = copy_sys_info;
      copy_sys_info->mib = calloc(orig_sys_info->mib_length, sizeof(uint8_t));
      AssertFatal(copy_sys_info->mib, "out of memory\n");
      *copy_sys_info->mib = *orig_sys_info->mib;
      copy_sys_info->mib_length = orig_sys_info->mib_length;
      if (orig_sys_info->sib1_length > 0) {
        copy_sys_info->sib1 = calloc(orig_sys_info->sib1_length, sizeof(uint8_t));
        AssertFatal(copy_sys_info->sib1, "out of memory\n");
        *copy_sys_info->sib1 = *orig_sys_info->sib1;
        copy_sys_info->sib1_length = orig_sys_info->sib1_length;
      }
    }
  }
  for (int i = 0; i < sizeofArray(msg->rrc_ver); i++)
    cp.rrc_ver[i] = msg->rrc_ver[i];
  return cp;
}

/**
 * @brief F1AP Setup Request equality check
 */
bool eq_f1ap_setup_request(const f1ap_setup_req_t *a, const f1ap_setup_req_t *b)
{
  CHECK_AND_RETURN_FALSE(a->gNB_DU_id == b->gNB_DU_id, "a=%ld, b=%ld", a->gNB_DU_id, b->gNB_DU_id);
  CHECK_AND_RETURN_FALSE(strcmp(a->gNB_DU_name, b->gNB_DU_name) == 0, "a=%s, b=%s", a->gNB_DU_name, b->gNB_DU_name);
  CHECK_AND_RETURN_FALSE(a->transaction_id == b->transaction_id, "a=%ld, b=%ld", a->transaction_id, b->transaction_id);
  CHECK_AND_RETURN_FALSE(a->num_cells_available == b->num_cells_available,
                         "a=%d, b=%d",
                         a->num_cells_available,
                         b->num_cells_available);
  for (int i = 0; i < a->num_cells_available; i++) {
    if (!eq_f1ap_cell_info(&a->cell[i].info, &b->cell[i].info))
      return false;
    if (!eq_f1ap_sys_info(a->cell[i].sys_info, b->cell[i].sys_info))
      return false;
  }
  CHECK_AND_RETURN_FALSE(sizeofArray(a->rrc_ver) == sizeofArray(b->rrc_ver),
                         "a=%ld, b=%ld",
                         sizeofArray(a->rrc_ver),
                         sizeofArray(b->rrc_ver));
  for (int i = 0; i < sizeofArray(a->rrc_ver); i++) {
    CHECK_AND_RETURN_FALSE(a->rrc_ver[i] == b->rrc_ver[i], "a=%d, b=%d", a->rrc_ver[i], b->rrc_ver[i]);
  }
  return true;
}

/**
 * @brief F1AP Setup Request memory management
 */
void free_f1ap_setup_request(f1ap_setup_req_t *msg)
{
  DevAssert(msg != NULL);
  free(msg->gNB_DU_name);
  for (int i = 0; i < msg->num_cells_available; i++) {
    free_f1ap_cell(&msg->cell[i].info, msg->cell[i].sys_info);
  }
}