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

/* ====================================
 *          F1AP Setup Response
 * ==================================== */

/**
 * @brief F1AP Setup Response encoding
 */
F1AP_F1AP_PDU_t *encode_f1ap_setup_response(const f1ap_setup_resp_t *msg)
{
  F1AP_F1AP_PDU_t *pdu = calloc(1, sizeof(*pdu));
  AssertFatal(pdu != NULL, "out of memory\n");

  /* Create */
  /* 0. Message Type */
  pdu->present = F1AP_F1AP_PDU_PR_successfulOutcome;
  asn1cCalloc(pdu->choice.successfulOutcome, tmp);
  tmp->procedureCode = F1AP_ProcedureCode_id_F1Setup;
  tmp->criticality = F1AP_Criticality_reject;
  tmp->value.present = F1AP_SuccessfulOutcome__value_PR_F1SetupResponse;
  F1AP_F1SetupResponse_t *out = &pdu->choice.successfulOutcome->value.choice.F1SetupResponse;
  /* mandatory */
  /* c1. Transaction ID (integer value)*/
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupResponseIEs_t, ie1);
  ie1->id = F1AP_ProtocolIE_ID_id_TransactionID;
  ie1->criticality = F1AP_Criticality_reject;
  ie1->value.present = F1AP_F1SetupResponseIEs__value_PR_TransactionID;
  ie1->value.choice.TransactionID = msg->transaction_id;

  /* optional */
  /* c2. GNB_CU_Name */
  if (msg->gNB_CU_name != NULL) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupResponseIEs_t, ie2);
    ie2->id = F1AP_ProtocolIE_ID_id_gNB_CU_Name;
    ie2->criticality = F1AP_Criticality_ignore;
    ie2->value.present = F1AP_F1SetupResponseIEs__value_PR_GNB_CU_Name;
    OCTET_STRING_fromBuf(&ie2->value.choice.GNB_CU_Name, msg->gNB_CU_name, strlen(msg->gNB_CU_name));
  }

  /* optional */
  /* c3. cells to be Activated list */
  if (msg->num_cells_to_activate > 0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupResponseIEs_t, ie3);
    ie3->id = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List;
    ie3->criticality = F1AP_Criticality_reject;
    ie3->value.present = F1AP_F1SetupResponseIEs__value_PR_Cells_to_be_Activated_List;

    for (int i = 0; i < msg->num_cells_to_activate; i++) {
      asn1cSequenceAdd(ie3->value.choice.Cells_to_be_Activated_List.list,
                       F1AP_Cells_to_be_Activated_List_ItemIEs_t,
                       cells_to_be_activated_ies);
      cells_to_be_activated_ies->id = F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item;
      cells_to_be_activated_ies->criticality = F1AP_Criticality_reject;
      cells_to_be_activated_ies->value.present = F1AP_Cells_to_be_Activated_List_ItemIEs__value_PR_Cells_to_be_Activated_List_Item;
      /* 3.1 cells to be Activated list item */
      F1AP_Cells_to_be_Activated_List_Item_t *cells_to_be_activated_item =
          &cells_to_be_activated_ies->value.choice.Cells_to_be_Activated_List_Item;

      /* mandatory */
      /* - nRCGI */
      addnRCGI(cells_to_be_activated_item->nRCGI, msg->cells_to_activate + i);

      /* optional */
      /* - nRPCI */
      cells_to_be_activated_item->nRPCI = (F1AP_NRPCI_t *)calloc(1, sizeof(F1AP_NRPCI_t));
      *cells_to_be_activated_item->nRPCI = msg->cells_to_activate[i].nrpci;

      /* optional */
      /* - gNB-CU System Information */
      if (msg->cells_to_activate[i].num_SI > 0) {
        /* 3.1.2 gNB-CUSystem Information */
        F1AP_ProtocolExtensionContainer_10696P112_t *p = calloc(1, sizeof(*p));
        cells_to_be_activated_item->iE_Extensions = (struct F1AP_ProtocolExtensionContainer *)p;
        asn1cSequenceAdd(p->list, F1AP_Cells_to_be_Activated_List_ItemExtIEs_t, cells_to_be_activated_itemExtIEs);
        cells_to_be_activated_itemExtIEs->id = F1AP_ProtocolIE_ID_id_gNB_CUSystemInformation;
        cells_to_be_activated_itemExtIEs->criticality = F1AP_Criticality_reject;
        cells_to_be_activated_itemExtIEs->extensionValue.present =
            F1AP_Cells_to_be_Activated_List_ItemExtIEs__extensionValue_PR_GNB_CUSystemInformation;
        F1AP_GNB_CUSystemInformation_t *gNB_CUSystemInformation =
            &cells_to_be_activated_itemExtIEs->extensionValue.choice.GNB_CUSystemInformation;
        /* gNB-CU System Information message */
        for (int sIBtype = 2; sIBtype < 21; sIBtype++) {
          if (msg->cells_to_activate[i].SI_container[sIBtype] != NULL) {
            CHECK_F1AP_CONDITION(msg->cells_to_activate[i].SI_type[sIBtype] > 6 && msg->cells_to_activate[i].SI_type[sIBtype] != 9);
            asn1cSequenceAdd(gNB_CUSystemInformation->sibtypetobeupdatedlist.list, F1AP_SibtypetobeupdatedListItem_t, sib_item);
            sib_item->sIBtype = sIBtype;
            OCTET_STRING_fromBuf(&sib_item->sIBmessage,
                                 (const char *)msg->cells_to_activate[i].SI_container[sIBtype],
                                 msg->cells_to_activate[i].SI_container_length[sIBtype]);
          }
        }
      }
    }
  }

  /* mandatory */
  /* c5. RRC VERSION */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupResponseIEs_t, ie4);
  ie4->id = F1AP_ProtocolIE_ID_id_GNB_CU_RRC_Version;
  ie4->criticality = F1AP_Criticality_reject;
  ie4->value.present = F1AP_F1SetupResponseIEs__value_PR_RRC_Version;
  // RRC Version: "This IE is not used in this release."
  // we put one bit for each byte in rrc_ver that is != 0
  uint8_t bits = 0;
  for (int i = 0; i < sizeofArray(msg->rrc_ver); ++i)
    bits |= (msg->rrc_ver[i] != 0) << i;
  BIT_STRING_t *bs = &ie4->value.choice.RRC_Version.latest_RRC_Version;
  bs->buf = calloc(1, sizeof(char));
  AssertFatal(bs->buf != NULL, "out of memory\n");
  bs->buf[0] = bits;
  bs->size = 1;
  bs->bits_unused = 5;

  F1AP_ProtocolExtensionContainer_10696P228_t *p =
      (F1AP_ProtocolExtensionContainer_10696P228_t *)calloc(1, sizeof(F1AP_ProtocolExtensionContainer_10696P228_t));
  asn1cSequenceAdd(p->list, F1AP_RRC_Version_ExtIEs_t, rrcv_ext);
  rrcv_ext->id = F1AP_ProtocolIE_ID_id_latest_RRC_Version_Enhanced;
  rrcv_ext->criticality = F1AP_Criticality_ignore;
  rrcv_ext->extensionValue.present = F1AP_RRC_Version_ExtIEs__extensionValue_PR_OCTET_STRING_SIZE_3_;
  OCTET_STRING_t *os = &rrcv_ext->extensionValue.choice.OCTET_STRING_SIZE_3_;
  os->size = 3;
  os->buf = malloc(3 * sizeof(*os->buf));
  AssertFatal(os->buf != NULL, "out of memory\n");
  for (int i = 0; i < sizeofArray(msg->rrc_ver); ++i)
    os->buf[i] = msg->rrc_ver[i];
  ie4->value.choice.RRC_Version.iE_Extensions = (struct F1AP_ProtocolExtensionContainer *)p;

  return pdu;
}

/**
 * @brief F1AP Setup Response decoding
 */
bool decode_f1ap_setup_response(const F1AP_F1AP_PDU_t *pdu, f1ap_setup_resp_t *out)
{
  CHECK_IE_CONDITION(pdu->present == F1AP_F1AP_PDU_PR_successfulOutcome);
  CHECK_IE_CONDITION(pdu->choice.successfulOutcome->procedureCode == F1AP_ProcedureCode_id_F1Setup);
  CHECK_IE_CONDITION(pdu->choice.successfulOutcome->value.present == F1AP_SuccessfulOutcome__value_PR_F1SetupResponse);
  F1AP_F1SetupResponse_t *in = &pdu->choice.successfulOutcome->value.choice.F1SetupResponse;
  F1AP_F1SetupResponseIEs_t *ie;
  for (int i = 0; i < in->protocolIEs.list.count; i++) {
    ie = in->protocolIEs.list.array[i];
    switch (ie->id) {
      case F1AP_ProtocolIE_ID_id_TransactionID:
        CHECK_IE_CONDITION(ie->value.present == F1AP_F1SetupResponseIEs__value_PR_TransactionID);
        CHECK_IE_CONDITION(ie->value.choice.TransactionID != -1);
        CHECK_CRITICALITY_REJECT(ie->criticality);
        out->transaction_id = ie->value.choice.TransactionID;
        break;

      case F1AP_ProtocolIE_ID_id_gNB_CU_Name:
        CHECK_IE_CONDITION(ie->criticality == F1AP_Criticality_ignore);
        CHECK_IE_CONDITION(ie->value.present == F1AP_F1SetupResponseIEs__value_PR_GNB_CU_Name);
        out->gNB_CU_name = malloc(ie->value.choice.GNB_CU_Name.size + 1);
        *out->gNB_CU_name = *ie->value.choice.GNB_CU_Name.buf;
        out->gNB_CU_name[ie->value.choice.GNB_CU_Name.size] = '\0';
        break;

      case F1AP_ProtocolIE_ID_id_GNB_CU_RRC_Version:
        CHECK_CRITICALITY_REJECT(ie->criticality);
        CHECK_IE_CONDITION(ie->value.present == F1AP_F1SetupResponseIEs__value_PR_RRC_Version);
        // RRC Version: "This IE is not used in this release."
        if (ie->value.choice.RRC_Version.iE_Extensions) {
          F1AP_ProtocolExtensionContainer_10696P228_t *ext =
              (F1AP_ProtocolExtensionContainer_10696P228_t *)ie->value.choice.RRC_Version.iE_Extensions;
          if (ext->list.count > 0) {
            F1AP_RRC_Version_ExtIEs_t *rrcext = ext->list.array[0];
            OCTET_STRING_t *os = &rrcext->extensionValue.choice.OCTET_STRING_SIZE_3_;
            for (int i = 0; i < 3; i++)
              out->rrc_ver[i] = os->buf[i];
          }
        }
        break;

      case F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List: {
        CHECK_CRITICALITY_REJECT(ie->criticality);
        CHECK_IE_CONDITION(ie->value.present == F1AP_F1SetupResponseIEs__value_PR_Cells_to_be_Activated_List);
        int num_cells_to_activate = ie->value.choice.Cells_to_be_Activated_List.list.count;

        for (int i = 0; i < num_cells_to_activate; i++) {
          F1AP_Cells_to_be_Activated_List_ItemIEs_t *cells_to_be_activated_list_item_ies =
              (F1AP_Cells_to_be_Activated_List_ItemIEs_t *)ie->value.choice.Cells_to_be_Activated_List.list.array[i];
          CHECK_IE_CONDITION(cells_to_be_activated_list_item_ies->id == F1AP_ProtocolIE_ID_id_Cells_to_be_Activated_List_Item);
          CHECK_CRITICALITY_REJECT(cells_to_be_activated_list_item_ies->criticality);
          CHECK_IE_CONDITION(cells_to_be_activated_list_item_ies->value.present
                             == F1AP_Cells_to_be_Activated_List_ItemIEs__value_PR_Cells_to_be_Activated_List_Item);
          F1AP_Cells_to_be_Activated_List_Item_t *cell =
              &cells_to_be_activated_list_item_ies->value.choice.Cells_to_be_Activated_List_Item;
          TBCD_TO_MCC_MNC(&cell->nRCGI.pLMN_Identity,
                          out->cells_to_activate[i].plmn.mcc,
                          out->cells_to_activate[i].plmn.mnc,
                          out->cells_to_activate[i].plmn.mnc_digit_length);
          BIT_STRING_TO_NR_CELL_IDENTITY(&cell->nRCGI.nRCellIdentity, out->cells_to_activate[i].nr_cellid);
          if (cell->nRPCI != NULL)
            out->cells_to_activate[i].nrpci = *cell->nRPCI;
          F1AP_ProtocolExtensionContainer_10696P112_t *ext = (F1AP_ProtocolExtensionContainer_10696P112_t *)cell->iE_Extensions;

          if (ext == NULL)
            continue;

          for (int cnt = 0; cnt < ext->list.count; cnt++) {
            F1AP_Cells_to_be_Activated_List_ItemExtIEs_t *cells_to_be_activated_list_itemExtIEs =
                (F1AP_Cells_to_be_Activated_List_ItemExtIEs_t *)ext->list.array[cnt];

            switch (cells_to_be_activated_list_itemExtIEs->id) {
              case F1AP_ProtocolIE_ID_id_gNB_CUSystemInformation: {
                out->cells_to_activate[i].nrpci = (cell->nRPCI != NULL) ? *cell->nRPCI : 0;
                F1AP_GNB_CUSystemInformation_t *gNB_CUSystemInformation =
                    (F1AP_GNB_CUSystemInformation_t *)&cells_to_be_activated_list_itemExtIEs->extensionValue.choice
                        .GNB_CUSystemInformation;
                out->cells_to_activate[i].num_SI = gNB_CUSystemInformation->sibtypetobeupdatedlist.list.count;
                AssertError(out->cells_to_activate[i].num_SI == 0, return -false, "System Information handling not implemented");
                CHECK_IE_CONDITION(ext->list.count == 1); // At least one SI message should be there, and only 1 for now

                for (int si = 0; si < gNB_CUSystemInformation->sibtypetobeupdatedlist.list.count; si++) {
                  F1AP_SibtypetobeupdatedListItem_t *sib_item = gNB_CUSystemInformation->sibtypetobeupdatedlist.list.array[si];
                  size_t size = sib_item->sIBmessage.size;
                  out->cells_to_activate[i].SI_container_length[si] = size;
                  out->cells_to_activate[i].SI_container[si] = malloc(out->cells_to_activate[i].SI_container_length[si]);
                  memcpy(out->cells_to_activate[i].SI_container[si], sib_item->sIBmessage.buf, size);
                  out->cells_to_activate[i].SI_type[si] = sib_item->sIBtype;
                }

                break;
              }

              case F1AP_ProtocolIE_ID_id_AvailablePLMNList:
                AssertError(1 == 0, return false, "F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              case F1AP_ProtocolIE_ID_id_ExtendedAvailablePLMN_List:
                AssertError(1 == 0, return false, "F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              case F1AP_ProtocolIE_ID_id_IAB_Info_IAB_donor_CU:
                AssertError(1 == 0, return false, "F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              case F1AP_ProtocolIE_ID_id_AvailableSNPN_ID_List:
                AssertError(1 == 0, return false, "F1AP_ProtocolIE_ID_id_AvailablePLMNList not supported yet\n");
                break;

              default:
                AssertError(1 == 0,
                            return false,
                            "F1AP_ProtocolIE_ID_id %d unknown\n",
                            (int)cells_to_be_activated_list_itemExtIEs->id);
                break;
            }
          }
        }
        break;
      }
      default:
        AssertError(1 == 0, return false, "F1AP_ProtocolIE_ID_id %d unknown\n", (int)ie->id);
        break;
    }
  }

  return true;
}

/**
 * @brief F1AP Setup Response equality check
 */
bool eq_f1ap_setup_response(const f1ap_setup_resp_t *a, const f1ap_setup_resp_t *b)
{
  CHECK_AND_RETURN_FALSE(strcmp(a->gNB_CU_name, b->gNB_CU_name) == 0, "a='%s', b='%s'", a->gNB_CU_name, b->gNB_CU_name);
  CHECK_AND_RETURN_FALSE(a->num_cells_to_activate == b->num_cells_to_activate, "a=%d, b=%d", a->num_cells_to_activate, b->num_cells_to_activate);
  CHECK_AND_RETURN_FALSE(a->transaction_id == b->transaction_id, "a=%ld, b=%ld", a->transaction_id, b->transaction_id);
  if (a->num_cells_to_activate) {
    for (int i = 0; i < a->num_cells_to_activate; i++) {
      CHECK_AND_RETURN_FALSE(a->cells_to_activate[i].nr_cellid == b->cells_to_activate[i].nr_cellid,
        "a=%ld, b=%ld", a->cells_to_activate[i].nr_cellid, b->cells_to_activate[i].nr_cellid);
      CHECK_AND_RETURN_FALSE(a->cells_to_activate[i].nrpci == b->cells_to_activate[i].nrpci,
        "a=%d, b=%d", a->cells_to_activate[i].nrpci, b->cells_to_activate[i].nrpci);
      CHECK_AND_RETURN_FALSE(a->cells_to_activate[i].num_SI == b->cells_to_activate[i].num_SI,
        "a=%d, b=%d", a->cells_to_activate[i].num_SI, b->cells_to_activate[i].num_SI);
      CHECK_AND_RETURN_FALSE(a->cells_to_activate[i].nr_cellid == b->cells_to_activate[i].nr_cellid,
        "a=%ld, b=%ld", a->cells_to_activate[i].nr_cellid, b->cells_to_activate[i].nr_cellid);
      if (!eq_f1ap_plmn(&a->cells_to_activate[i].plmn, &b->cells_to_activate[i].plmn))
        return false;
      for (int j = 0; j < b->cells_to_activate[i].num_SI; j++) {
        CHECK_AND_RETURN_FALSE(*a->cells_to_activate[i].SI_container[j] == *b->cells_to_activate[i].SI_container[j],
          "a=%d, b=%d", *a->cells_to_activate[i].SI_container[j], *b->cells_to_activate[i].SI_container[j]);
        CHECK_AND_RETURN_FALSE(a->cells_to_activate[i].SI_container_length[j] == b->cells_to_activate[i].SI_container_length[j],
          "a=%d, b=%d", a->cells_to_activate[i].SI_container_length[j], b->cells_to_activate[i].SI_container_length[j]);
        CHECK_AND_RETURN_FALSE(a->cells_to_activate[i].SI_type[j] ==  b->cells_to_activate[i].SI_type[j],
          "a=%d, b=%d", a->cells_to_activate[i].SI_type[j], b->cells_to_activate[i].SI_type[j]);
      }
    }
  }
  CHECK_AND_RETURN_FALSE(sizeofArray(a->rrc_ver) == sizeofArray(b->rrc_ver),
    "a=%ld, b=%ld", sizeofArray(a->rrc_ver), sizeofArray(b->rrc_ver));
  for (int i = 0; i < sizeofArray(a->rrc_ver); i++) {
    CHECK_AND_RETURN_FALSE(a->rrc_ver[i] == b->rrc_ver[i], "a=%d, b=%d", a->rrc_ver[i], b->rrc_ver[i]);
  }
  return true;
}

/**
 * @brief F1AP Setup Response deep copy
 */
f1ap_setup_resp_t cp_f1ap_setup_response(const f1ap_setup_resp_t *msg)
{
  f1ap_setup_resp_t cp;
  /* gNB_CU_name */
  cp.gNB_CU_name = strdup(msg->gNB_CU_name);
  /* transaction_id */
  cp.transaction_id = msg->transaction_id;
  /* num_cells_available */
  cp.num_cells_to_activate = msg->num_cells_to_activate;
  for (int n = 0; n < msg->num_cells_to_activate; n++) {
    /* cell.info */
    cp.cells_to_activate[n].nr_cellid = msg->cells_to_activate[n].nr_cellid;
    cp.cells_to_activate[n].nrpci = msg->cells_to_activate[n].nrpci;
    cp.cells_to_activate[n].num_SI = msg->cells_to_activate[n].num_SI;
    cp.cells_to_activate[n].plmn = msg->cells_to_activate[n].plmn;
    for (int j = 0; j < msg->cells_to_activate[n].num_SI; j++) {
      cp.cells_to_activate[n].SI_container[j] = msg->cells_to_activate[n].SI_container[j];
      cp.cells_to_activate[n].SI_container_length[j]  = msg->cells_to_activate[n].SI_container_length[j];
      cp.cells_to_activate[n].SI_type[j] = msg->cells_to_activate[n].SI_type[j];
    }
  }
  for (int i = 0; i < sizeofArray(msg->rrc_ver); i++)
    cp.rrc_ver[i] = msg->rrc_ver[i];
  return cp;
}

/**
 * @brief F1AP Setup Response memory management
 */
void free_f1ap_setup_response(f1ap_setup_resp_t *msg)
{
  DevAssert(msg != NULL);
  if (msg->gNB_CU_name != NULL)
    free(msg->gNB_CU_name);
  for (int i = 0; i < msg->num_cells_to_activate; i++)
    for (int j = 0; j < msg->cells_to_activate[i].num_SI; j++)
      if (msg->cells_to_activate[i].SI_container_length[j] > 0)
        free(msg->cells_to_activate[i].SI_container[j]);
}

/* ====================================
 *          F1AP Setup Failure
 * ==================================== */

/**
 * @brief F1AP Setup Failure encoding
 */
F1AP_F1AP_PDU_t *encode_f1ap_setup_failure(const f1ap_setup_failure_t *msg)
{
  F1AP_F1AP_PDU_t *pdu = calloc(1, sizeof(*pdu));
  AssertFatal(pdu != NULL, "out of memory\n");
  /* Create */
  /* 0. Message Type */
  asn1cCalloc(pdu->choice.unsuccessfulOutcome, UnsuccessfulOutcome);
  pdu->present = F1AP_F1AP_PDU_PR_unsuccessfulOutcome;
  UnsuccessfulOutcome->procedureCode = F1AP_ProcedureCode_id_F1Setup;
  UnsuccessfulOutcome->criticality = F1AP_Criticality_reject;
  UnsuccessfulOutcome->value.present = F1AP_UnsuccessfulOutcome__value_PR_F1SetupFailure;
  F1AP_F1SetupFailure_t *out = &pdu->choice.unsuccessfulOutcome->value.choice.F1SetupFailure;
  /* mandatory */
  /* c1. Transaction ID (integer value)*/
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupFailureIEs_t, ie1);
  ie1->id = F1AP_ProtocolIE_ID_id_TransactionID;
  ie1->criticality = F1AP_Criticality_reject;
  ie1->value.present = F1AP_F1SetupFailureIEs__value_PR_TransactionID;
  ie1->value.choice.TransactionID = msg->transaction_id;
  /* mandatory */
  /* c2. Cause */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupFailureIEs_t, ie2);
  ie2->id = F1AP_ProtocolIE_ID_id_Cause;
  ie2->criticality = F1AP_Criticality_ignore;
  ie2->value.present = F1AP_F1SetupFailureIEs__value_PR_Cause;
  ie2->value.choice.Cause.present = F1AP_Cause_PR_radioNetwork;
  ie2->value.choice.Cause.choice.radioNetwork = msg->cause;
  /* optional */
  /* c3. TimeToWait */
  if (msg->time_to_wait > 0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupFailureIEs_t, ie3);
    ie3->id = F1AP_ProtocolIE_ID_id_TimeToWait;
    ie3->criticality = F1AP_Criticality_ignore;
    ie3->value.present = F1AP_F1SetupFailureIEs__value_PR_TimeToWait;
    ie3->value.choice.TimeToWait = F1AP_TimeToWait_v10s;
  }
  /* optional */
  /* c4. CriticalityDiagnostics*/
  if (msg->criticality_diagnostics) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_F1SetupFailureIEs_t, ie4);
    ie4->id = F1AP_ProtocolIE_ID_id_CriticalityDiagnostics;
    ie4->criticality = F1AP_Criticality_ignore;
    ie4->value.present = F1AP_F1SetupFailureIEs__value_PR_CriticalityDiagnostics;
    asn1cCallocOne(ie4->value.choice.CriticalityDiagnostics.procedureCode, F1AP_ProcedureCode_id_UEContextSetup);
    asn1cCallocOne(ie4->value.choice.CriticalityDiagnostics.triggeringMessage, F1AP_TriggeringMessage_initiating_message);
    asn1cCallocOne(ie4->value.choice.CriticalityDiagnostics.procedureCriticality, F1AP_Criticality_reject);
    asn1cCallocOne(ie4->value.choice.CriticalityDiagnostics.transactionID, 0);
  }
  return pdu;
}

/**
 * @brief F1AP Setup Failure decoding
 */
bool decode_f1ap_setup_failure(const F1AP_F1AP_PDU_t *pdu, f1ap_setup_failure_t *out)
{
  F1AP_F1SetupFailureIEs_t *ie;
  F1AP_F1SetupFailure_t *choice = &pdu->choice.unsuccessfulOutcome->value.choice.F1SetupFailure;
  /* Transaction ID */
  F1AP_LIB_FIND_IE(F1AP_F1SetupFailureIEs_t, ie, choice, F1AP_ProtocolIE_ID_id_TransactionID, true);
  out->transaction_id = ie->value.choice.TransactionID;
  /* Cause */
  F1AP_LIB_FIND_IE(F1AP_F1SetupFailureIEs_t, ie, choice, F1AP_ProtocolIE_ID_id_Cause, true);
  out->cause = ie->value.choice.Cause.choice.radioNetwork;
  /* Time to wait */
  F1AP_LIB_FIND_IE(F1AP_F1SetupFailureIEs_t, ie, choice, F1AP_ProtocolIE_ID_id_TimeToWait, false);
  if (ie != NULL)
    out->time_to_wait = ie->value.choice.TimeToWait;
  return true;
}

/**
 * @brief F1AP Setup Failure equality check
 */
bool eq_f1ap_setup_failure(const f1ap_setup_failure_t *a, const f1ap_setup_failure_t *b)
{
  if (a->transaction_id != b->transaction_id)
    return false;
  return true;
}

/**
 * @brief F1AP Setup Failure deep copy
 */
f1ap_setup_failure_t cp_f1ap_setup_failure(const f1ap_setup_failure_t *msg)
{
  f1ap_setup_failure_t cp;
  /* transaction_id */
  cp.transaction_id = msg->transaction_id;
  return cp;
}

/* ====================================
 *   F1AP gNB-DU Configuration Update
 * ==================================== */

static int read_slice_info(const F1AP_ServedPLMNs_Item_t *plmn, nssai_t *nssai, int max_nssai)
{
  if (plmn->iE_Extensions == NULL)
    return 0;

  const F1AP_ProtocolExtensionContainer_10696P34_t *p = (F1AP_ProtocolExtensionContainer_10696P34_t *)plmn->iE_Extensions;
  if (p->list.count == 0)
    return 0;

  const F1AP_ServedPLMNs_ItemExtIEs_t *splmn = p->list.array[0];
  DevAssert(splmn->id == F1AP_ProtocolIE_ID_id_TAISliceSupportList);
  DevAssert(splmn->extensionValue.present == F1AP_ServedPLMNs_ItemExtIEs__extensionValue_PR_SliceSupportList);
  const F1AP_SliceSupportList_t *ssl = &splmn->extensionValue.choice.SliceSupportList;
  AssertFatal(ssl->list.count <= max_nssai, "cannot handle more than 16 slices\n");
  for (int s = 0; s < ssl->list.count; ++s) {
    const F1AP_SliceSupportItem_t *sl = ssl->list.array[s];
    nssai_t *n = &nssai[s];
    OCTET_STRING_TO_INT8(&sl->sNSSAI.sST, n->sst);
    n->sd = 0xffffff;
    if (sl->sNSSAI.sD != NULL)
      OCTET_STRING_TO_INT24(sl->sNSSAI.sD, n->sd);
  }

  return ssl->list.count;
}

/**
 * @brief F1 gNB-DU Configuration Update encoding (9.2.1.7 of 3GPP TS 38.473)
 */
F1AP_F1AP_PDU_t *encode_f1ap_du_configuration_update(const f1ap_gnb_du_configuration_update_t *msg)
{
  F1AP_F1AP_PDU_t *pdu = calloc(1, sizeof(*pdu));
  AssertFatal(pdu != NULL, "out of memory\n");
  /* Create */
  /* 0. Message Type */
  pdu->present = F1AP_F1AP_PDU_PR_initiatingMessage;
  asn1cCalloc(pdu->choice.initiatingMessage, initMsg);
  initMsg->procedureCode = F1AP_ProcedureCode_id_gNBDUConfigurationUpdate;
  initMsg->criticality   = F1AP_Criticality_reject;
  initMsg->value.present = F1AP_InitiatingMessage__value_PR_GNBDUConfigurationUpdate;
  F1AP_GNBDUConfigurationUpdate_t *out = &initMsg->value.choice.GNBDUConfigurationUpdate;

  /* mandatory */
  /* c1. Transaction ID (integer value) */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_GNBDUConfigurationUpdateIEs_t, ie1);
  ie1->id                        = F1AP_ProtocolIE_ID_id_TransactionID;
  ie1->criticality               = F1AP_Criticality_reject;
  ie1->value.present             = F1AP_GNBDUConfigurationUpdateIEs__value_PR_TransactionID;
  ie1->value.choice.TransactionID = msg->transaction_id;

  /* mandatory */
  /* c2. Served_Cells_To_Add */
  if (msg->num_cells_to_add > 0) {
    AssertFatal(false, "code for adding cells not tested\n");
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_GNBDUConfigurationUpdateIEs_t, ie2);
    ie2->id = F1AP_ProtocolIE_ID_id_Served_Cells_To_Add_List;
    ie2->criticality = F1AP_Criticality_reject;
    ie2->value.present = F1AP_GNBDUConfigurationUpdateIEs__value_PR_Served_Cells_To_Add_List;

    for (int j = 0; j < msg->num_cells_to_add; j++) {
      const f1ap_served_cell_info_t *cell = &msg->cell_to_add[j].info;
      const f1ap_gnb_du_system_info_t *sys_info = msg->cell_to_add[j].sys_info;
      asn1cSequenceAdd(ie2->value.choice.Served_Cells_To_Add_List.list,
                       F1AP_Served_Cells_To_Add_ItemIEs_t,
                       served_cells_to_add_item_ies);
      served_cells_to_add_item_ies->id = F1AP_ProtocolIE_ID_id_Served_Cells_To_Add_Item;
      served_cells_to_add_item_ies->criticality = F1AP_Criticality_reject;
      served_cells_to_add_item_ies->value.present = F1AP_Served_Cells_To_Add_ItemIEs__value_PR_Served_Cells_To_Add_Item;
      F1AP_Served_Cells_To_Add_Item_t *served_cells_to_add_item =
          &served_cells_to_add_item_ies->value.choice.Served_Cells_To_Add_Item;
      served_cells_to_add_item->served_Cell_Information = encode_served_cell_info(cell);
      served_cells_to_add_item->gNB_DU_System_Information = encode_system_info(sys_info);
    }
  }

  /* mandatory */
  /* c3. Served_Cells_To_Modify */
  if (msg->num_cells_to_modify > 0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_GNBDUConfigurationUpdateIEs_t, ie3);
    ie3->id = F1AP_ProtocolIE_ID_id_Served_Cells_To_Modify_List;
    ie3->criticality = F1AP_Criticality_reject;
    ie3->value.present = F1AP_GNBDUConfigurationUpdateIEs__value_PR_Served_Cells_To_Modify_List;
    for (int i = 0; i < msg->num_cells_to_modify; i++) {
      const f1ap_served_cell_info_t *cell = &msg->cell_to_modify[i].info;
      const f1ap_gnb_du_system_info_t *sys_info = msg->cell_to_modify[i].sys_info;
      asn1cSequenceAdd(ie3->value.choice.Served_Cells_To_Modify_List.list,
                       F1AP_Served_Cells_To_Modify_ItemIEs_t,
                       served_cells_to_modify_item_ies);
      served_cells_to_modify_item_ies->id = F1AP_ProtocolIE_ID_id_Served_Cells_To_Modify_Item;
      served_cells_to_modify_item_ies->criticality = F1AP_Criticality_reject;
      served_cells_to_modify_item_ies->value.present = F1AP_Served_Cells_To_Modify_ItemIEs__value_PR_Served_Cells_To_Modify_Item;
      F1AP_Served_Cells_To_Modify_Item_t *served_cells_to_modify_item =
          &served_cells_to_modify_item_ies->value.choice.Served_Cells_To_Modify_Item;

      F1AP_NRCGI_t *oldNRCGI = &served_cells_to_modify_item->oldNRCGI;
      const f1ap_plmn_t *old_plmn = &msg->cell_to_modify[i].old_plmn;
      MCC_MNC_TO_PLMNID(old_plmn->mcc, old_plmn->mnc, old_plmn->mnc_digit_length, &oldNRCGI->pLMN_Identity);
      NR_CELL_ID_TO_BIT_STRING(msg->cell_to_modify[i].old_nr_cellid, &oldNRCGI->nRCellIdentity);

      served_cells_to_modify_item->served_Cell_Information = encode_served_cell_info(cell);
      served_cells_to_modify_item->gNB_DU_System_Information = encode_system_info(sys_info);
    }
  }

  /* mandatory */
  /* c4. Served_Cells_To_Delete */
  if (msg->num_cells_to_delete > 0) {
    asn1cSequenceAdd(out->protocolIEs.list, F1AP_GNBDUConfigurationUpdateIEs_t, ie4);
    ie4->id = F1AP_ProtocolIE_ID_id_Served_Cells_To_Delete_List;
    ie4->criticality = F1AP_Criticality_reject;
    ie4->value.present = F1AP_GNBDUConfigurationUpdateIEs__value_PR_Served_Cells_To_Delete_List;
    AssertFatal(msg->num_cells_to_delete == 0, "code for deleting cells not tested\n");
    for (int i = 0; i < msg->num_cells_to_delete; i++) {
      asn1cSequenceAdd(ie4->value.choice.Served_Cells_To_Delete_List.list,
                       F1AP_Served_Cells_To_Delete_ItemIEs_t,
                       served_cells_to_delete_item_ies);
      served_cells_to_delete_item_ies->id = F1AP_ProtocolIE_ID_id_Served_Cells_To_Delete_Item;
      served_cells_to_delete_item_ies->criticality = F1AP_Criticality_reject;
      served_cells_to_delete_item_ies->value.present = F1AP_Served_Cells_To_Delete_ItemIEs__value_PR_Served_Cells_To_Delete_Item;
      F1AP_Served_Cells_To_Delete_Item_t *served_cells_to_delete_item =
          &served_cells_to_delete_item_ies->value.choice.Served_Cells_To_Delete_Item;
      addnRCGI(served_cells_to_delete_item->oldNRCGI, &msg->cell_to_delete[i]);
    }
  }

  /* optional */
  /* c5. GNB_DU_ID (integer value) */
  asn1cSequenceAdd(out->protocolIEs.list, F1AP_GNBDUConfigurationUpdateIEs_t, ie5);
  ie5->id = F1AP_ProtocolIE_ID_id_gNB_DU_ID;
  ie5->criticality = F1AP_Criticality_reject;
  ie5->value.present = F1AP_GNBDUConfigurationUpdateIEs__value_PR_GNB_DU_ID;
  asn_int642INTEGER(&ie5->value.choice.GNB_DU_ID, msg->gNB_DU_ID);

  return pdu;
}

/**
 * @brief F1 gNB-DU Configuration Update decoding (9.2.1.7 of 3GPP TS 38.473)
 */
bool decode_f1ap_du_configuration_update(const F1AP_F1AP_PDU_t *pdu, f1ap_gnb_du_configuration_update_t *out)
{
  F1AP_GNBDUConfigurationUpdate_t *in = &pdu->choice.initiatingMessage->value.choice.GNBDUConfigurationUpdate;
  F1AP_GNBDUConfigurationUpdateIEs_t *ie;

  /* 3GPP TS 38.473 Transaction ID*/
  F1AP_LIB_FIND_IE(F1AP_GNBDUConfigurationUpdateIEs_t, ie, in, F1AP_ProtocolIE_ID_id_TransactionID, true);
  out->transaction_id = ie->value.choice.TransactionID;

  /* 3GPP TS 38.473 Served Cells To Add List */
  F1AP_LIB_FIND_IE(F1AP_GNBDUConfigurationUpdateIEs_t, ie, in, F1AP_ProtocolIE_ID_id_Served_Cells_To_Add_List, false);

  if (ie != NULL) {
    out->num_cells_to_add = ie->value.choice.Served_Cells_To_Add_List.list.count;

    for (int i = 0; i < out->num_cells_to_add; i++) {
      F1AP_Served_Cells_To_Add_Item_t *served_cells_item =
          &((F1AP_Served_Cells_To_Add_ItemIEs_t *)ie->value.choice.Served_Cells_To_Add_List.list.array[i])
               ->value.choice.Served_Cells_To_Add_Item;
      F1AP_Served_Cell_Information_t *servedCellInformation = &served_cells_item->served_Cell_Information;
      /* tac */
      if (servedCellInformation->fiveGS_TAC) {
        out->cell_to_add[i].info.tac = malloc(sizeof(*out->cell_to_add[i].info.tac));
        AssertFatal(out->cell_to_add[i].info.tac != NULL, "out of memory\n");
        OCTET_STRING_TO_INT24(servedCellInformation->fiveGS_TAC, *out->cell_to_add[i].info.tac);
      }

      /* - nRCGI */
      TBCD_TO_MCC_MNC(&(servedCellInformation->nRCGI.pLMN_Identity),
                      out->cell_to_add[i].info.plmn.mcc,
                      out->cell_to_add[i].info.plmn.mnc,
                      out->cell_to_add[i].info.plmn.mnc_digit_length);
      // NR cellID
      BIT_STRING_TO_NR_CELL_IDENTITY(&servedCellInformation->nRCGI.nRCellIdentity, out->cell_to_add[i].info.nr_cellid);

      /* - nRPCI */
      out->cell_to_add[i].info.nr_pci = servedCellInformation->nRPCI;

      AssertFatal(servedCellInformation->servedPLMNs.list.count == 1, "only one PLMN handled\n");
      out->cell_to_add[i].info.num_ssi =
          read_slice_info(servedCellInformation->servedPLMNs.list.array[0], out->cell_to_add[i].info.nssai, 16);

      // FDD Cells
      if (servedCellInformation->nR_Mode_Info.present == F1AP_NR_Mode_Info_PR_fDD) {
        out->cell_to_add[i].info.mode = F1AP_MODE_FDD;
        f1ap_fdd_info_t *FDDs = &out->cell_to_add[i].info.fdd;
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
        AssertFatal(dlBands == 0, "cannot handled more than one frequency band\n");
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
        out->cell_to_add[i].info.mode = F1AP_MODE_TDD;
        f1ap_tdd_info_t *TDDs = &out->cell_to_add[i].info.tdd;
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
        out->cell_to_add[i].info.measurement_timing_config =
            cp_octet_string(&servedCellInformation->measurementTimingConfiguration,
                            &out->cell_to_add[i].info.measurement_timing_config_len);

      struct F1AP_GNB_DU_System_Information *DUsi = served_cells_item->gNB_DU_System_Information;
      // System Information
      out->cell_to_add[i].sys_info = calloc(1, sizeof(*out->cell_to_add[i].sys_info));
      AssertFatal(out->cell_to_add[i].sys_info != NULL, "out of memory\n");
      f1ap_gnb_du_system_info_t *sys_info = out->cell_to_add[i].sys_info;
      /* mib */
      sys_info->mib = calloc(DUsi->mIB_message.size, sizeof(char));
      memcpy(sys_info->mib, DUsi->mIB_message.buf, DUsi->mIB_message.size);
      sys_info->mib_length = DUsi->mIB_message.size;
      /* sib1 */
      sys_info->sib1 = calloc(DUsi->sIB1_message.size, sizeof(char));
      memcpy(sys_info->sib1, DUsi->sIB1_message.buf, DUsi->sIB1_message.size);
      sys_info->sib1_length = DUsi->sIB1_message.size;
    }
  } else {
    out->num_cells_to_add = 0;
  }

  /* 3GPP TS 38.473 Served Cells To Modify List */
  F1AP_LIB_FIND_IE(F1AP_GNBDUConfigurationUpdateIEs_t, ie, in, F1AP_ProtocolIE_ID_id_Served_Cells_To_Modify_List, false);
  if (ie) {
    out->num_cells_to_modify = ie->value.choice.Served_Cells_To_Modify_List.list.count;

    for (int i = 0; i < out->num_cells_to_modify; i++) {
      F1AP_Served_Cells_To_Modify_Item_t *served_cells_item =
          &((F1AP_Served_Cells_To_Modify_ItemIEs_t *)ie->value.choice.Served_Cells_To_Modify_List.list.array[i])
               ->value.choice.Served_Cells_To_Modify_Item;

      /* OLD NRCGI */
      TBCD_TO_MCC_MNC(&(served_cells_item->oldNRCGI.pLMN_Identity),
                      out->cell_to_modify[i].old_plmn.mcc,
                      out->cell_to_modify[i].old_plmn.mnc,
                      out->cell_to_modify[i].old_plmn.mnc_digit_length);

      BIT_STRING_TO_NR_CELL_IDENTITY(&served_cells_item->oldNRCGI.nRCellIdentity, out->cell_to_modify[i].old_nr_cellid);

      F1AP_Served_Cell_Information_t *servedCellInformation = &served_cells_item->served_Cell_Information;
      /* SERVED CELL INFORMATION*/
      /* tac */
      if (servedCellInformation->fiveGS_TAC) {
        out->cell_to_modify[i].info.tac = malloc(sizeof(*out->cell_to_modify[i].info.tac));
        AssertFatal(out->cell_to_modify[i].info.tac != NULL, "out of memory\n");
        OCTET_STRING_TO_INT24(servedCellInformation->fiveGS_TAC, *out->cell_to_modify[i].info.tac);
      }

      /* - nRCGI */
      TBCD_TO_MCC_MNC(&(servedCellInformation->nRCGI.pLMN_Identity),
                      out->cell_to_modify[i].info.plmn.mcc,
                      out->cell_to_modify[i].info.plmn.mnc,
                      out->cell_to_modify[i].info.plmn.mnc_digit_length);
      // NR cellID
      BIT_STRING_TO_NR_CELL_IDENTITY(&servedCellInformation->nRCGI.nRCellIdentity, out->cell_to_modify[i].info.nr_cellid);

      /* - nRPCI */
      out->cell_to_modify[i].info.nr_pci = servedCellInformation->nRPCI;

      // FDD Cells
      if (servedCellInformation->nR_Mode_Info.present == F1AP_NR_Mode_Info_PR_fDD) {
        out->cell_to_modify[i].info.mode = F1AP_MODE_FDD;
        f1ap_fdd_info_t *FDDs = &out->cell_to_modify[i].info.fdd;
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
        AssertFatal(dlBands == 0, "cannot handled more than one frequency band\n");
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
        out->cell_to_modify[i].info.mode = F1AP_MODE_TDD;
        f1ap_tdd_info_t *TDDs = &out->cell_to_modify[i].info.tdd;
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
        out->cell_to_modify[i].info.measurement_timing_config =
            cp_octet_string(&servedCellInformation->measurementTimingConfiguration,
                            &out->cell_to_modify[i].info.measurement_timing_config_len);

      /*gNB DU SYSTEM INFORMATION */
      struct F1AP_GNB_DU_System_Information *DUsi = served_cells_item->gNB_DU_System_Information;
      if (DUsi != NULL) {
        // System Information
        out->cell_to_modify[i].sys_info = calloc(1, sizeof(*out->cell_to_modify[i].sys_info));
        AssertFatal(out->cell_to_modify[i].sys_info != NULL, "out of memory\n");
        f1ap_gnb_du_system_info_t *sys_info = out->cell_to_modify[i].sys_info;
        /* mib */
        sys_info->mib = calloc(DUsi->mIB_message.size, sizeof(char));
        AssertFatal(out->cell_to_modify[i].sys_info->mib != NULL, "out of memory\n");
        memcpy(sys_info->mib, DUsi->mIB_message.buf, DUsi->mIB_message.size);
        sys_info->mib_length = DUsi->mIB_message.size;

        /* sib1 */
        sys_info->sib1 = calloc(DUsi->sIB1_message.size, sizeof(char));
        AssertFatal(out->cell_to_modify[i].sys_info->sib1 != NULL, "out of memory\n");
        memcpy(sys_info->sib1, DUsi->sIB1_message.buf, DUsi->sIB1_message.size);
        sys_info->sib1_length = DUsi->sIB1_message.size;
      }
    }
  } else {
    out->num_cells_to_modify = 0;
  }

  /* 3GPP TS 38.473 Served Cells To Delete List */
  F1AP_LIB_FIND_IE(F1AP_GNBDUConfigurationUpdateIEs_t, ie, in, F1AP_ProtocolIE_ID_id_Served_Cells_To_Delete_List, false);
  if (ie) {
    out->num_cells_to_delete = ie->value.choice.Served_Cells_To_Delete_List.list.count;
    for (int i = 0; i < out->num_cells_to_delete; i++) {
      F1AP_Served_Cells_To_Delete_Item_t *served_cells_item =
          &((F1AP_Served_Cells_To_Delete_ItemIEs_t *)ie->value.choice.Served_Cells_To_Delete_List.list.array[i])
               ->value.choice.Served_Cells_To_Delete_Item;
      /* - Old nRCGI */
      TBCD_TO_MCC_MNC(&(served_cells_item->oldNRCGI.pLMN_Identity),
                      out->cell_to_delete[i].plmn.mcc,
                      out->cell_to_delete[i].plmn.mnc,
                      out->cell_to_delete[i].plmn.mnc_digit_length);
      // NR cellID
      BIT_STRING_TO_NR_CELL_IDENTITY(&served_cells_item->oldNRCGI.nRCellIdentity, out->cell_to_delete[i].nr_cellid);
    }
  } else {
    out->num_cells_to_delete = 0;
  }

  /* 3GPP TS 38.473 Cells Status List */
  F1AP_LIB_FIND_IE(F1AP_GNBDUConfigurationUpdateIEs_t, ie, in, F1AP_ProtocolIE_ID_id_Cells_Status_List, false);

  /* 3GPP TS 38.473 Dedicated SI Delivery Needed UE List */
  F1AP_LIB_FIND_IE(F1AP_GNBDUConfigurationUpdateIEs_t, ie, in, F1AP_ProtocolIE_ID_id_Dedicated_SIDelivery_NeededUE_List, false);

  /* 3GPP TS 38.473 gNB-DU ID */
  F1AP_LIB_FIND_IE(F1AP_GNBDUConfigurationUpdateIEs_t, ie, in, F1AP_ProtocolIE_ID_id_gNB_DU_ID, false);
  if (ie != NULL)
    asn_INTEGER2ulong(&ie->value.choice.GNB_DU_ID, &out->gNB_DU_ID);

  /* 3GPP TS 38.473 gNB-DU TNL Association To Remove List */
  F1AP_LIB_FIND_IE(F1AP_GNBDUConfigurationUpdateIEs_t, ie, in, F1AP_ProtocolIE_ID_id_GNB_DU_TNL_Association_To_Remove_List, false);
  return pdu;
}

void free_f1ap_du_configuration_update(f1ap_gnb_du_configuration_update_t *msg)
{
  for (int i = 0; i < msg->num_cells_to_add; i++)
    free_f1ap_cell(&msg->cell_to_add[i].info, msg->cell_to_add[i].sys_info);
  for (int i = 0; i < msg->num_cells_to_modify; i++)
    free_f1ap_cell(&msg->cell_to_modify[i].info, msg->cell_to_modify[i].sys_info);
}

/**
 * @brief F1 gNB-DU Configuration Update check
 */
bool eq_f1ap_du_configuration_update(const f1ap_gnb_du_configuration_update_t *a, const f1ap_gnb_du_configuration_update_t *b)
{
  CHECK_AND_RETURN_FALSE(a->gNB_DU_ID == b->gNB_DU_ID, "a='%ld', b='%ld'", a->gNB_DU_ID, b->gNB_DU_ID);
  CHECK_AND_RETURN_FALSE(a->transaction_id == b->transaction_id, "a='%ld', b='%ld'", a->transaction_id, b->transaction_id);
  /* to add */
  CHECK_AND_RETURN_FALSE(a->num_cells_to_add == b->num_cells_to_add, "a='%d', b='%d'", a->num_cells_to_add, b->num_cells_to_add);
  for (int i = 0; i < a->num_cells_to_add; i++) {
    if (!eq_f1ap_cell_info(&a->cell_to_add[i].info, &b->cell_to_add[i].info))
      return false;
    if (a->cell_to_add[i].sys_info && b->cell_to_add[i].sys_info) {
      if (!eq_f1ap_sys_info(a->cell_to_add[i].sys_info, b->cell_to_add[i].sys_info))
        return false;
    }
  }
  /* to delete */
  CHECK_AND_RETURN_FALSE(a->num_cells_to_delete == b->num_cells_to_delete,
                         "a='%d', b='%d'",
                         a->num_cells_to_delete,
                         b->num_cells_to_delete);
  for (int i = 0; i < a->num_cells_to_delete; i++) {
    CHECK_AND_RETURN_FALSE(a->cell_to_delete[i].nr_cellid == b->cell_to_delete[i].nr_cellid,
                           "a='%ld', b='%ld'",
                           a->cell_to_delete[i].nr_cellid,
                           b->cell_to_delete[i].nr_cellid);
    if (!eq_f1ap_plmn(&a->cell_to_delete[i].plmn, &b->cell_to_delete[i].plmn))
      return false;
  }
  /* to modify */
  CHECK_AND_RETURN_FALSE(a->num_cells_to_modify == b->num_cells_to_modify,
                         "a='%d', b='%d'",
                         a->num_cells_to_modify,
                         b->num_cells_to_modify);
  for (int i = 0; i < a->num_cells_to_modify; i++) {
    if (!eq_f1ap_cell_info(&a->cell_to_modify[i].info, &b->cell_to_modify[i].info))
      return false;
    if (a->cell_to_modify[i].sys_info && b->cell_to_modify[i].sys_info) {
      if (!eq_f1ap_sys_info(a->cell_to_modify[i].sys_info, b->cell_to_modify[i].sys_info))
        return false;
    }
  }
  return true;
}

/**
 * @brief F1 gNB-DU Configuration Update deep copy
 */
f1ap_gnb_du_configuration_update_t cp_f1ap_du_configuration_update(const f1ap_gnb_du_configuration_update_t *msg)
{
  f1ap_gnb_du_configuration_update_t cp;
  /* gNB_DU_ID */
  cp.gNB_DU_ID = msg->gNB_DU_ID;
  /* transaction_id */
  cp.transaction_id = msg->transaction_id;
  /* to add */
  cp.num_cells_to_add = msg->num_cells_to_add;
  /* to delete */
  cp.num_cells_to_delete = msg->num_cells_to_delete;
  for (int i = 0; i < cp.num_cells_to_delete; i++) {
    cp.cell_to_delete[i].nr_cellid = msg->cell_to_delete[i].nr_cellid;
    cp.cell_to_delete[i].plmn = msg->cell_to_delete[i].plmn;
  }
  /* to modify */
  cp.num_cells_to_modify = msg->num_cells_to_modify;
  for (int i = 0; i < cp.num_cells_to_modify; i++) {
    cp.cell_to_modify[i].info = msg->cell_to_modify[i].info;
    if (cp.cell_to_modify[i].info.measurement_timing_config_len > 0) {
      cp.cell_to_modify[i].info.measurement_timing_config_len = msg->cell_to_modify[i].info.measurement_timing_config_len;
      cp.cell_to_modify[i].info.measurement_timing_config = malloc(sizeof(*cp.cell_to_modify[i].info.measurement_timing_config));
      *cp.cell_to_modify[i].info.measurement_timing_config = *msg->cell_to_modify[i].info.measurement_timing_config;
    }
    /* TAC */
    cp.cell_to_modify[i].info.tac = calloc(1, sizeof(uint32_t));
    AssertFatal(cp.cell_to_modify[i].info.tac != NULL, "out of memory\n");
    *cp.cell_to_modify[i].info.tac = *msg->cell_to_modify[i].info.tac;
    /* System information */
    cp.cell_to_modify[i].sys_info = malloc(sizeof(*cp.cell_to_modify[i].sys_info));
    AssertFatal(cp.cell_to_modify[i].sys_info != NULL, "out of memory\n");
    if (msg->cell_to_modify[i].sys_info->mib_length > 0) {
      cp.cell_to_modify[i].sys_info->mib_length = msg->cell_to_modify[i].sys_info->mib_length;
      cp.cell_to_modify[i].sys_info->mib = malloc(sizeof(*cp.cell_to_modify[i].sys_info->mib));
      *cp.cell_to_modify[i].sys_info->mib = *msg->cell_to_modify[i].sys_info->mib;
    }
    if (msg->cell_to_modify[i].sys_info->sib1_length > 0) {
      cp.cell_to_modify[i].sys_info->sib1_length = msg->cell_to_modify[i].sys_info->sib1_length;
      cp.cell_to_modify[i].sys_info->sib1 = malloc(sizeof(*cp.cell_to_modify[i].sys_info->sib1));
      *cp.cell_to_modify[i].sys_info->sib1 = *msg->cell_to_modify[i].sys_info->sib1;
    }
  }
  return cp;
}