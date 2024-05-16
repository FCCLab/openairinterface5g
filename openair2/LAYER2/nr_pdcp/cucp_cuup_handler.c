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

#include "cucp_cuup_handler.h"

#include "NR_DRB-ToAddModList.h"

#include "common/platform_types.h"
#include "intertask_interface.h"
#include "openair2/COMMON/e1ap_messages_types.h"
#include "openair3/ocp-gtpu/gtp_itf.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "nr_pdcp_oai_api.h"
#include "cuup_cucp_if.h"
#include "common/utils/oai_asn1.h"
#include "openair2/SDAP/nr_sdap/nr_sdap.h"
#include "openair2/E1AP/e1ap_common.h"
#include "openair2/F1AP/f1ap_common.h"

static void fill_DRB_configList_e1(NR_DRB_ToAddModList_t *DRB_configList, const pdu_session_to_setup_t *pdu)
{
  for (int i = 0; i < pdu->numDRB2Setup; i++) {
    const DRB_nGRAN_to_setup_t *drb = pdu->DRBnGRanList + i;
    asn1cSequenceAdd(DRB_configList->list, struct NR_DRB_ToAddMod, ie);
    ie->drb_Identity = drb->id;
    ie->cnAssociation = CALLOC(1, sizeof(*ie->cnAssociation));
    ie->cnAssociation->present = NR_DRB_ToAddMod__cnAssociation_PR_sdap_Config;

    // sdap_Config
    asn1cCalloc(ie->cnAssociation->choice.sdap_Config, sdap_config);
    sdap_config->pdu_Session = pdu->sessionId;
    /* SDAP */
    sdap_config->sdap_HeaderDL = drb->sdap_config.sDAP_Header_DL;
    sdap_config->sdap_HeaderUL = drb->sdap_config.sDAP_Header_UL;
    sdap_config->defaultDRB    = drb->sdap_config.defaultDRB;
    asn1cCalloc(sdap_config->mappedQoS_FlowsToAdd, FlowsToAdd);
    for (int j = 0; j < drb->numQosFlow2Setup; j++) {
      asn1cSequenceAdd(FlowsToAdd->list, NR_QFI_t, qfi);
      *qfi = drb->qosFlows[j].qfi;
    }
    sdap_config->mappedQoS_FlowsToRelease = NULL;

    // pdcp_Config
    ie->reestablishPDCP = NULL;
    ie->recoverPDCP = NULL;
    asn1cCalloc(ie->pdcp_Config, pdcp_config);
    asn1cCalloc(pdcp_config->drb, drbCfg);
    asn1cCallocOne(drbCfg->discardTimer, drb->pdcp_config.discardTimer);
    asn1cCallocOne(drbCfg->pdcp_SN_SizeUL, drb->pdcp_config.pDCP_SN_Size_UL);
    asn1cCallocOne(drbCfg->pdcp_SN_SizeDL, drb->pdcp_config.pDCP_SN_Size_DL);
    drbCfg->headerCompression.present = NR_PDCP_Config__drb__headerCompression_PR_notUsed;
    drbCfg->headerCompression.choice.notUsed = 0;
    drbCfg->integrityProtection = NULL;
    drbCfg->statusReportRequired = NULL;
    drbCfg->outOfOrderDelivery = NULL;
    pdcp_config->moreThanOneRLC = NULL;
    pdcp_config->t_Reordering = calloc(1, sizeof(*pdcp_config->t_Reordering));
    *pdcp_config->t_Reordering = drb->pdcp_config.reorderingTimer;
    pdcp_config->ext1 = NULL;
    if (pdu->integrityProtectionIndication == 0 || // Required
        pdu->integrityProtectionIndication == 1) { // Preferred
        asn1cCallocOne(drbCfg->integrityProtection, NR_PDCP_Config__drb__integrityProtection_enabled);
    }
    if (pdu->confidentialityProtectionIndication == 2) { // Not Needed
      asn1cCalloc(pdcp_config->ext1, ext1);
      asn1cCallocOne(ext1->cipheringDisabled, NR_PDCP_Config__ext1__cipheringDisabled_true);
    }
  }
}


/**
 * @brief Create a GTP-U tunnel on the F1 interface
 */
static int drb_gtpu_create(instance_t instance,
                           uint32_t ue_id,
                           const DRB_nGRAN_to_setup_t *drb,
                           gtpv1u_gnb_create_tunnel_resp_t *resp_f1)
{
  teid_t dummy_teid = 0xffff; // we will update later with answer from DU
  in_addr_t dummy_address = {0}; // IPv4, updated later with answer from DU
  gtpCallback callBack = cu_f1u_data_req;
  const int bearer_id = 0; // 1 DRB
  int qfi = -1; // don't put PDU session marker in GTP for F1-U
  gtpv1u_gnb_create_tunnel_req_t create_tunnel_req = {
      .ue_id = ue_id,
      .outgoing_teid = dummy_teid,
      .outgoing_qfi[bearer_id].qfi[0] = qfi,
      .pdusession_id = drb->id,
      .incoming_rb_id = drb->id,
      .dst_addr.length = 32,
  };
  memcpy(&create_tunnel_req.dst_addr.buffer, &dummy_address, sizeof(uint8_t) * 4);
  LOG_D(E1AP, "In %s: Incoming RB ID %ld Outgoing RB ID %ld - UL TEID (dummy) %d QFI %d\n", __func__, drb->id, drb->id, dummy_teid, qfi);
  // we use gtpv1u_create_ngu_tunnel because it returns the interface
  // address and port of the interface; apart from that, we also might call
  // newGtpuCreateTunnel() directly
  return gtpv1u_create_ngu_tunnel(instance, &create_tunnel_req, resp_f1, callBack, NULL);
}

/**
 * @brief   Fill GTP-U Create Tunnel Request for a given N3 Tunnel
 * @param   req_n3      Pointer to GTP-U Create Tunnel Request
 * @param   req_pdu     Pointer to PDU Session to Setup
 * @param   tunnel_id   Tunnel ID
 */
static void fill_gtpu_n3_req(gtpv1u_gnb_create_tunnel_req_t *req_n3, const pdu_session_to_setup_t *req_pdu, int tunnel_id)
{
  req_n3->incoming_rb_id = req_pdu->DRBnGRanList[0].id; // NOTE assuming the first DRB is default
  req_n3->pdusession_id = req_pdu->sessionId;
  memcpy(&req_n3->dst_addr.buffer, &req_pdu->tlAddress, sizeof(uint8_t) * 4);
  req_n3->dst_addr.length = 32;
  req_n3->outgoing_teid = req_pdu->teId;
}

/**
 * @brief Loop though the number of QoS Flows to setup per DRB and return DRB-to-QoS mapping
 */
static gtpv1u_qos_t process_qosflowSetup(DRB_nGRAN_setup_t *resp_drb, const DRB_nGRAN_to_setup_t *req_drb)
{
  gtpv1u_qos_t outgoing_qfi;
  int k;
  resp_drb->numQosFlowSetup = req_drb->numQosFlow2Setup;
  /* Loop though the number of QoS Flows to setup for current DRB */
  outgoing_qfi.rb_id = resp_drb->id;
  for (k = 0; k < resp_drb->numQosFlowSetup; k++) {
    const qos_flow_to_setup_t *qosflow2Setup = &req_drb->qosFlows[k];
    qos_flow_setup_t *qosflowSetup = &resp_drb->qosFlows[k];
    qosflowSetup->qfi = qosflow2Setup->qfi;
    outgoing_qfi.qfi[k] = qosflowSetup->qfi;
    LOG_D(E1AP, "DRB %d with QFI %ld to setup\n", outgoing_qfi.rb_id, qosflow2Setup->qfi);
  }
  /* Set remaining QFIs to -1 */
  for (int r = k + 1; r < SDAP_MAX_QFI; r++) {
    outgoing_qfi.qfi[r] = -1;
  }
  return outgoing_qfi;
}

static instance_t get_n3_gtp_instance(void)
{
  const e1ap_upcp_inst_t *inst = getCxtE1(0);
  AssertFatal(inst, "need to have E1 instance\n");
  return inst->gtpInstN3;
}

static instance_t get_f1_gtp_instance(void)
{
  const f1ap_cudu_inst_t *inst = getCxt(0);
  if (!inst)
    return -1; // means no F1
  return inst->gtpInst;
}

void e1_bearer_context_setup(const e1ap_bearer_setup_req_t *req)
{
  bool need_ue_id_mgmt = e1_used();

  /* mirror the CU-CP UE ID for CU-UP */
  uint32_t cu_up_ue_id = req->gNB_cu_cp_ue_id;
  f1_ue_data_t ued = {.secondary_ue = req->gNB_cu_cp_ue_id};
  if (need_ue_id_mgmt) {
    cu_add_f1_ue_data(cu_up_ue_id, &ued);
    LOG_I(E1AP, "adding UE with CU-CP UE ID %d and CU-UP UE ID %d\n", req->gNB_cu_cp_ue_id, cu_up_ue_id);
  }

  instance_t n3inst = get_n3_gtp_instance();
  instance_t f1inst = get_f1_gtp_instance();

  e1ap_bearer_setup_resp_t resp = {
    .gNB_cu_cp_ue_id = req->gNB_cu_cp_ue_id,
    .gNB_cu_up_ue_id = cu_up_ue_id,
  };
  resp.numPDUSessions = req->numPDUSessions;
  /* Init GTP-U tunnel request/response for N3 */
  gtpv1u_gnb_create_tunnel_req_t req_n3 = {
      .ue_id = cu_up_ue_id,
  };
  gtpv1u_gnb_create_tunnel_resp_t resp_n3 = {0};
  /**
   * Loop over the number of PDU Sessions to setup
   * for each PDU session, a GTP-U N3 Tunnel Create Request
   * and fill the relevant item in E1 Bearer Setup Response */
  for (int i = 0; i < resp.numPDUSessions; ++i) {
    pdu_session_setup_t *resp_pdu = resp.pduSession + i;
    const pdu_session_to_setup_t *req_pdu = req->pduSession + i;
    resp_pdu->id = req_pdu->sessionId;

    AssertFatal(req_pdu->numDRB2Modify == 0, "DRB modification not implemented\n");
    resp_pdu->numDRBSetup = req_pdu->numDRB2Setup;
    fill_gtpu_n3_req(&req_n3, req_pdu, i);
    /* Loop though the number of DRB to setup
     * if required, for each DRB a F1-U GTP-U Tunnel Create Request
     * and fill the relevant item in E1 Bearer Setup Response */
    for (int d = 0; d < resp_pdu->numDRBSetup; d++) {
      const DRB_nGRAN_to_setup_t *req_drb = &req_pdu->DRBnGRanList[d];
      DRB_nGRAN_setup_t *resp_drb = &resp_pdu->DRBnGRanList[d];
      resp_drb->id = req_drb->id;
      /* Fill PDU session markers */
      req_n3.outgoing_qfi[d] = process_qosflowSetup(resp_drb, req_drb);
      // create PDCP bearers. This will also create SDAP bearers
      NR_DRB_ToAddModList_t DRB_configList = {0};
      fill_DRB_configList_e1(&DRB_configList, req_pdu);
      add_drb(true, // set this to notify PDCP that his not UE
              cu_up_ue_id,
              DRB_configList.list.array[d],
              ((req->integrityProtectionAlgorithm << 4) | req->cipheringAlgorithm) & 0x0f,
              (((req->integrityProtectionAlgorithm << 4) | req->cipheringAlgorithm) >> 4) & 0x0f,
              (uint8_t *)req->encryptionKey,
              (uint8_t *)req->integrityProtectionKey);
      /* Handle F1-U tunnels setup */
      if (f1inst >= 0) { /* we have F1(-U) */
        /* Set up the F1-U GTP tunnel for current DRB */
        gtpv1u_gnb_create_tunnel_resp_t resp_f1 = { .num_tunnels = 1 };
        int ret = drb_gtpu_create(f1inst, cu_up_ue_id, req_drb, &resp_f1);
        AssertFatal(ret >= 0, "Unable to create GTP Tunnel for F1-U\n");
        resp_drb->numUpParam = 1;
        memcpy(&resp_drb->UpParamList[d].tlAddress, &resp_f1.gnb_addr.buffer, 4);
        resp_drb->UpParamList[d].teId = resp_f1.gnb_NGu_teid;
        LOG_D(E1AP, "Created F1-U tunnel for DRB %ld (Teid %ld tlAddress %x)\n",
              req_drb->id,
              resp_drb->UpParamList[d].teId,
              resp_drb->UpParamList[d].tlAddress);
      }
    }

    // We assume all DRBs to setup have been setup successfully, so we always
    // send successful outcome in response and no failed DRBs
    resp_pdu->numDRBFailed = 0;
    /**
    * GTP tunnel for N3/to core
    * there is one GTP-U tunnel per PDU session
    * and it can containes multiple QoS Flows
    * @note Mapping only the default QFI (by implementation, assuming is the first in the list)
    * @note we use gtpv1u_create_ngu_tunnel because it returns the interface
    *       address and port of the interface; apart from that, we also might call
    *       newGtpuCreateTunnel() directly
    */
    LOG_D(E1AP, "In %s: GTP tunnel for N3/to core for PDU Session %ld\n", __func__, req_pdu->sessionId);
    int ret = gtpv1u_create_ngu_tunnel(n3inst, &req_n3, &resp_n3, nr_pdcp_data_req_drb, sdap_data_req);
    AssertFatal(ret >= 0, "Unable to create GTP Tunnel for NG-U\n");
    resp_n3.num_tunnels += 1;
    // NOTE: we should have 1 GTP tunnel per PDU session on the N3 interface
    resp_pdu->teId = resp_n3.gnb_NGu_teid;
    memcpy(&resp_pdu->tlAddress, &resp_n3.gnb_addr.buffer, 4);
  }
  AssertFatal(resp_n3.num_tunnels == resp.numPDUSessions, "could not create all tunnels\n");
  get_e1_if()->bearer_setup_response(&resp);
}

/**
 * @brief Fill Bearer Context Modification Response and send to callback
 */
void e1_bearer_context_modif(const e1ap_bearer_mod_req_t *req)
{
  AssertFatal(req->numPDUSessionsMod > 0, "need at least one PDU session to modify\n");

  e1ap_bearer_modif_resp_t modif = {
      .gNB_cu_cp_ue_id = req->gNB_cu_cp_ue_id,
      .gNB_cu_up_ue_id = req->gNB_cu_up_ue_id,
      .numPDUSessionsMod = req->numPDUSessionsMod,
  };

  instance_t f1inst = get_f1_gtp_instance();

  /* PDU Session Resource To Modify List (see 9.3.3.11 of TS 38.463) */
  for (int i = 0; i < req->numPDUSessionsMod; i++) {
    DevAssert(req->pduSessionMod[i].sessionId > 0);
    LOG_I(E1AP,
          "UE %d: updating PDU session ID %ld (%ld bearers)\n",
          req->gNB_cu_up_ue_id,
          req->pduSessionMod[i].sessionId,
          req->pduSessionMod[i].numDRB2Modify);
    modif.pduSessionMod[i].id = req->pduSessionMod[i].sessionId;
    modif.pduSessionMod[i].numDRBModified = req->pduSessionMod[i].numDRB2Modify;
    /* DRBs to modify */
    for (int j = 0; j < req->pduSessionMod[i].numDRB2Modify; j++) {
      const DRB_nGRAN_to_mod_t *to_modif = &req->pduSessionMod[i].DRBnGRanModList[j];
      DRB_nGRAN_modified_t *modified = &modif.pduSessionMod[i].DRBnGRanModList[j];
      modified->id = to_modif->id;

      if (to_modif->pdcp_config.pDCP_Reestablishment) {
        nr_pdcp_reestablishment(req->gNB_cu_up_ue_id, to_modif->id, false);
      }

      if (f1inst < 0) // no F1-U?
        continue; // nothing to do

      /* Loop through DL UP Transport Layer params list
       * and update GTP tunnel outgoing addr and TEID */
      for (int k = 0; k < to_modif->numDlUpParam; k++) {
        in_addr_t addr = to_modif->DlUpParamList[k].tlAddress;
        GtpuUpdateTunnelOutgoingAddressAndTeid(f1inst, req->gNB_cu_cp_ue_id, to_modif->id, addr, to_modif->DlUpParamList[k].teId);
      }
    }
  }

  get_e1_if()->bearer_modif_response(&modif);
}

void e1_bearer_release_cmd(const e1ap_bearer_release_cmd_t *cmd)
{
  bool need_ue_id_mgmt = e1_used();

  instance_t n3inst = get_n3_gtp_instance();
  instance_t f1inst = get_f1_gtp_instance();

  LOG_I(E1AP, "releasing UE %d\n", cmd->gNB_cu_up_ue_id);

  newGtpuDeleteAllTunnels(n3inst, cmd->gNB_cu_up_ue_id);
  if (f1inst >= 0)  // is there F1-U?
    newGtpuDeleteAllTunnels(f1inst, cmd->gNB_cu_up_ue_id);
  if (need_ue_id_mgmt) {
    // see issue #706: in monolithic, gNB will free PDCP of UE
    nr_pdcp_remove_UE(cmd->gNB_cu_up_ue_id);
    cu_remove_f1_ue_data(cmd->gNB_cu_up_ue_id);
  }
  nr_sdap_delete_ue_entities(cmd->gNB_cu_up_ue_id);

  e1ap_bearer_release_cplt_t cplt = {
    .gNB_cu_cp_ue_id = cmd->gNB_cu_cp_ue_id,
    .gNB_cu_up_ue_id = cmd->gNB_cu_up_ue_id,
  };

  get_e1_if()->bearer_release_complete(&cplt);
}
