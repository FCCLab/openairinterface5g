# Root Cause Analysis: QFI 2 Rejection During Handover in OAI gNB

## Summary

When the 5GC (free5gc SMF) sends a Handover Request with **multiple QoS flows** (e.g., QFI 1 and QFI 2) per PDU session, the **OAI gNB only accepts QFI 1** and rejects QFI 2. This is due to **multiple hard-coded "1 QoS flow only" limitations** across the OAI RAN stack.

---

## Root Cause Chain

### 1. **NGAP/RRC: PDU Session Transfer Decoding** (Primary Entry Point)

**File:** `openair2/RRC/NR/rrc_gNB_NGAP.c`  
**Function:** `cp_pdusession_transfer_to_pdusession()` (lines 193–215)

When decoding the Handover Request's PDU Session Resource Setup Transfer, OAI **explicitly drops all QoS flows except the first**:

```c
// OAI gNB only supports 1 QoS flow per PDU session
// If AMF sends multiple QoS flows, only accept the first one
for (uint8_t i = 0; i < src->nb_qos; ++i) {
  if (i == 0) {
    if (!add_qos(&dst->qos, &src->qos[i])) { ... }
  } else {
    LOG_W(NR_RRC, "Skipping QoS flow %d (qfi=%d): OAI gNB only supports 1 QoS flow per PDU session\n", i, src->qos[i].qfi);
  }
}
```

**Effect:** Only QFI 1 is stored in the UE's PDU session context. QFI 2 is never added.

---

### 2. **RRC Mobility: F1 DRB Setup** (Handover Path)

**File:** `openair2/RRC/NR/rrc_gNB_mobility.c`  
**Function:** `fill_drb_to_be_setup()` (lines 96–137)

When building the F1 UE Context Setup Request for handover, the code **asserts exactly 1 QoS flow** per PDU session:

```c
drb->nr.flows_len = 1;
drb->nr.flows = calloc_or_fail(1, sizeof(*drb->nr.flows));

// Since we don't have QFI mapping in the new structure, we'll use the first QoS flow
AssertFatal(seq_arr_size(&pdu->param.qos) == 1, "only 1 Qos flow supported\n");
nr_rrc_qos_t *qos_param = (nr_rrc_qos_t *)seq_arr_at(&pdu->param.qos, 0);
```

**Effect:** If more than one QoS flow were present, the gNB would crash. Because of (1), only one flow is ever present.

---

### 3. **CU-UP / E1AP Handler** (Bearer Setup)

**File:** `openair2/LAYER2/nr_pdcp/cucp_cuup_handler.c`  
**Function:** E1AP Bearer Context Setup handler (lines 190–205)

The CU-UP **asserts exactly 1 QoS flow per DRB**:

```c
AssertFatal(req_drb->numQosFlow2Setup == 1, "can only handle one QoS Flow per DRB\n");
```

**Effect:** Even if RRC sent multiple QoS flows to E1AP, the CU-UP would abort. The GTP tunnel creation also uses only `req_drb->qosFlows[0].qfi`.

---

### 4. **RRC Radio Bearers** (Legacy Check – Now Commented Out)

**File:** `openair2/RRC/NR/rrc_gNB_radio_bearers.c`  
**Function:** `add_qos()` (line 77)

Previously enforced:

```c
// AssertFatal(seq_arr_size(qos) == 1, "only 1 Qos flow supported\n");
```

This assertion is **commented out** in the current codebase, so it no longer causes a crash. The real limitation is in (1).

---

### 5. **PDU Session Modify** (Related Limitation)

**File:** `openair2/RRC/NR/rrc_gNB_NGAP.c` (around line 893)

```c
DevAssert(nb_qos == 1);
```

---

## Handover Flow (Target gNB)

```
AMF → Handover Request (PDU Session with QFI 1, QFI 2)
        ↓
NGAP Handler (ngap_gNB_handle_handover_request)
        ↓
RRC (rrc_gNB_process_Handover_Request)
        ↓
cp_pdusession_transfer_to_pdusession()  ← DROPS QFI 2 HERE
        ↓
trigger_bearer_setup()  (only QFI 1 in session->qos)
        ↓
E1AP Bearer Setup → CU-UP (cucp_cuup_handler)  ← Asserts 1 QFI/DRB
        ↓
F1 UE Context Setup (fill_drb_to_be_setup)  ← Asserts 1 QFI
        ↓
Handover Request Acknowledge  ← Only QFI 1 in qos_setup_list
```

---

## Applied Fix (Multi-QoS Support)

| File | Change |
|------|--------|
| `openair2/RRC/NR/rrc_gNB_NGAP.c` | **DONE**: `cp_pdusession_transfer_to_pdusession()` – add all QoS flows from the transfer (removed `if (i == 0)` restriction) |
| `openair2/RRC/NR/rrc_gNB_NGAP.c` | **DONE**: `nr_rrc_update_qos()` – relaxed `DevAssert(nb_qos == 1)` to allow multiple flows |
| `openair2/RRC/NR/rrc_gNB_mobility.c` | **DONE**: `fill_drb_to_be_setup()` – support multiple QoS flows per DRB; iterate over all flows in `pdu->param.qos` |
| `openair2/LAYER2/nr_pdcp/cucp_cuup_handler.c` | **DONE**: Replaced `AssertFatal(numQosFlow2Setup == 1)` with `DevAssert` allowing 1–MAX_QOS_FLOWS |
| `openair2/E2AP/RAN_FUNCTION/CUSTOMIZED/ran_func_gtp.c` | **DONE**: Relaxed `DevAssert(seq_arr_size == 1)` to allow multiple flows (E2 report uses first) |

---

## Workaround (No Code Changes)

Configure the 5GC (free5gc SMF) so that each PDU session has **only one QoS flow**. This avoids triggering the OAI limitations. See `LOI_QOS_FLOW_GNB.md` for SMF configuration guidance.

---

---

## Comparison: openair1 vs openair2 vs openair3

### openair1 (PHY layer)

- **Role:** Physical layer (modulation, scheduling, RF)
- **QoS flow handling:** None. QoS is above PHY.
- **Limitation:** N/A – no QoS flow concept at this layer.

---

### openair2 (RRC, L2, F1AP, E1AP)

- **Role:** RRC, MAC, RLC, PDCP, SDAP, F1AP, E1AP – gNB control and user plane logic
- **QoS flow handling:** Processes PDU sessions and QoS flows from NGAP
- **Limitation:** Enforces **1 QoS flow per PDU session** in several places:

| File | Function | Limitation |
|------|----------|------------|
| `rrc_gNB_NGAP.c` | `cp_pdusession_transfer_to_pdusession()` | Skips QFI 2+ when copying from NGAP transfer |
| `rrc_gNB_mobility.c` | `fill_drb_to_be_setup()` | `AssertFatal(seq_arr_size(&pdu->param.qos) == 1)` |
| `cucp_cuup_handler.c` | E1AP Bearer Setup | `AssertFatal(req_drb->numQosFlow2Setup == 1)` |
| `rrc_gNB_radio_bearers.c` | `add_qos()` | Assertion commented out (was 1 flow) |

---

### openair3 (NGAP, S1AP, GTP-U)

- **Role:** NGAP/S1AP protocol stack, GTP-U user plane
- **QoS flow handling:** Encode/decode only – no filtering
- **Limitation:** None – passes through whatever it receives

| Component | Behavior |
|-----------|----------|
| **NGAP (ngap_common.c)** | `decodePDUSessionResourceSetup()` decodes all QoS flows from `QosFlowSetupRequestList` (lines 259–262). No filtering. |
| **NGAP (ngap_gNB_nas_procedures.c)** | `encode_ngap_pdusession_setup_response_transfer()` encodes all flows in `pdusession->nb_of_qos_flow` (line 583). |
| **NGAP (ngap_gNB_mobility_management.c)** | Handover encode/decode uses `nb_of_qos_flow` – encodes whatever RRC provides. |
| **ocp-gtpu (gtp_itf.cpp)** | GTP-U handles QFI per packet (PDU Session Container). Supports multiple QFIs. |

**Conclusion:** openair3 does not limit QoS flows. The restriction is in openair2, which filters before passing data to openair3 for encoding.

---

## References

- **LOI_QOS_FLOW_GNB.md** – Documents the "only 1 Qos flow supported" crash and workarounds
- **3GPP TS 38.413** – NGAP Handover Preparation, Handover Request Acknowledge
- **3GPP TS 38.331** – RRC, QoS flow to DRB mapping
