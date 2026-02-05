# Handover Success Verification Checklist

## Summary

All code paths for N2 handover with **multiple QoS flows** have been verified and fixed. The handover should now succeed when the 5GC (free5gc) sends Handover Request with QFI 1 and QFI 2 (or more).

---

## End-to-End Handover Flow (Target gNB)

| Step | Component | Status | Notes |
|------|-----------|--------|-------|
| 1 | **NGAP**: Handover Request received | OK | Decoded in `decode_ng_handover_request` |
| 2 | **NGAP**: `decodePDUSessionResourceSetup` | OK | Decodes all QoS flows from `QosFlowSetupRequestList` (ngap_common.c:259-262) |
| 3 | **RRC**: `rrc_gNB_process_Handover_Request` | FIXED | Calls `cp_pdusession_transfer_to_pdusession` for each PDU session |
| 4 | **RRC**: `cp_pdusession_transfer_to_pdusession` | FIXED | Now adds **all** QoS flows (was: only QFI 1) |
| 5 | **RRC**: `trigger_bearer_setup` | OK | `FOR_EACH_SEQ_ARR` over `session->qos` – sends all to E1 |
| 6 | **RRC**: `fill_e1_drb_to_setup` | OK | Iterates over all QoS flows in session |
| 7 | **CU-UP**: `e1_bearer_context_setup` | FIXED | Removed `AssertFatal(numQosFlow2Setup==1)` |
| 8 | **CU-UP**: `fill_DRB_configList_e1` | OK | Loops over `numQosFlow2Setup`, adds all to `mappedQoS_FlowsToAdd` |
| 9 | **SDAP/PDCP**: `e1_add_bearers` | OK | Receives full config with all QFIs |
| 10 | **F1**: `fill_drb_to_be_setup` | FIXED | Now supports multiple flows per DRB |
| 11 | **F1AP**: `encode_drb_info_nr` | OK | Iterates `drb->flows_len`, encodes all flows |
| 12 | **RRC**: `rrc_gNB_send_NGAP_HANDOVER_REQUEST_ACKNOWLEDGE` | OK | `FOR_EACH_SEQ_ARR` over `session->param.qos` – includes all |
| 13 | **NGAP**: `encode_ng_handover_request_ack` | OK | Encodes all from `ack_transfer.qos_setup_list` |

---

## End-to-End Handover Flow (Source gNB)

| Step | Component | Status | Notes |
|------|-----------|--------|-------|
| 1 | **RRC**: `rrc_gNB_send_NGAP_HANDOVER_REQUIRED` | OK | `FOR_EACH_SEQ_ARR` over `session->qos` – includes all QFIs |
| 2 | **NGAP**: `encode_ng_handover_required` | OK | `pdu_session_resource[i].nb_of_qos_flow` – encodes all |

---

## Fixes Applied

| File | Function | Change |
|------|----------|--------|
| `rrc_gNB_NGAP.c` | `cp_pdusession_transfer_to_pdusession` | Add all QoS flows (removed `if (i==0)` filter) |
| `rrc_gNB_NGAP.c` | `nr_rrc_update_qos` | Relaxed `nb_qos == 1` to `nb_qos > 0 && nb_qos < MAX_QOS_FLOWS` |
| `rrc_gNB_mobility.c` | `fill_drb_to_be_setup` | Support multiple flows: `flows_len = seq_arr_size`, iterate all |
| `cucp_cuup_handler.c` | `e1_bearer_context_setup` | Replaced `AssertFatal(numQosFlow2Setup==1)` with `DevAssert` range check |
| `ran_func_gtp.c` | E2 GTP report | Relaxed `seq_arr_size==1` to `> 0` |

---

## Remaining Assertions (Non-Blocking)

| File | Assertion | Impact |
|------|-----------|--------|
| `rrc_gNB_radio_bearers.c` | `// AssertFatal(seq_arr_size(qos)==1)` | **Commented out** – no effect |
| `cucp_cuup_handler.c` | `AssertFatal(req_pdu->numDRB2Setup==1)` | One DRB per PDU session – design choice, not QoS-related |

---

## Data Flow Verification

```
5GC Handover Request (QFI 1, QFI 2)
    ↓
decode_ng_handover_request → pdusessionTransfer.nb_qos=2, qos[0], qos[1]
    ↓
cp_pdusession_transfer_to_pdusession → dst->qos = [QFI1, QFI2]  ✓ FIXED
    ↓
trigger_bearer_setup → add_pduSession(session with 2 QoS flows)
    ↓
fill_e1_drb_to_setup → numQosFlow2Setup=2, qosFlows[0], qosFlows[1]
    ↓
E1AP Bearer Setup → CU-UP accepts (AssertFatal removed)  ✓ FIXED
    ↓
fill_DRB_configList_e1 → mappedQoS_FlowsToAdd = [QFI1, QFI2]
    ↓
SDAP/PDCP configured with both QFIs
    ↓
F1 UE Context Setup (when DU responds) → fill_drb_to_be_setup
    ↓
drb->nr.flows_len=2, flows[0], flows[1]  ✓ FIXED
    ↓
Handover Request Acknowledge → qos_setup_list = [QFI1, QFI2]  ✓
```

---

## Pre-Flight Checklist

Before testing handover:

1. **Build**: `./build_oai --gNB` (ensure asn1c is installed: `./build_oai -I`)
2. **5GC**: free5gc AMF/SMF configured with multiple QoS flows per PDU session
3. **Target cell**: OAI gNB DU connected, NR Cell ID matches Handover Request
4. **PLMN**: GUAMI PLMN in Handover Request is in gNB's allowed PLMN list
5. **CU-UP**: E1 association established (handover requires CU-CP + CU-UP)

---

## Expected Log Messages (Success)

```
[NR_RRC] Received Handover Request (on NR Cell ID=..., PCI=...)
[NR_RRC] Added QoS flow with qfi=1, total number of QoS flows = 1
[NR_RRC] Added QoS flow with qfi=2, total number of QoS flows = 2
[NR_RRC] UE X: added DRB Y for PDU session ID Z
[E1AP] UE X: add PDU session ID Z (1 bearers)
[NR_RRC] Sending Handover Request Acknowledge
```

---

## If Handover Still Fails

1. **HandoverPreparationFailure**: Check AMF/SMF logs; may be 5GC-side
2. **HandoverFailure from target**: Check gNB logs for cause (cell not found, PLMN, etc.)
3. **QFI 2 still rejected**: Verify you rebuilt OAI after the fixes
4. **UERANSIM as target**: UERANSIM may have its own QoS flow limits; this fix applies to **OAI as target gNB**
