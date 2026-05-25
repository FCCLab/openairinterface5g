/*
 * Optional slot export for resgrid_plotter (channel-based / --data-mode 3).
 * Wire format: resouce-grid/docs/CHANNEL_BASED_INTERFACE.md
 *
 * Data path
 * ---------
 * NFAPI DL/UL TTI PDUs are turned into axis-aligned rectangles (sym_start/count,
 * prb_start/count) tagged with RESGRID_CHAN_* and optional RNTI, then written as
 * one v1 "empty marker" frame or chunked v2 multi-region frames (see below).
 *
 * CSI-RS / SRS
 * -------------
 * CSI-RS (`NFAPI_NR_DL_TTI_CSI_RS_PDU_TYPE`): NFAPI carries `start_rb` / `nr_of_rbs` and row/l0/l1;
 * symbol span follows the same `SL_to_bitmap` patterns as `nr_csirs_meas_scheduling()` in
 * gNB_scheduler_primitives.c (axis-aligned bbox).
 * SRS (`NFAPI_NR_UL_CONFIG_SRS_PDU_TYPE`): BWP-wide PRB rectangle (`bwp_start` / `bwp_size`) and
 * `SL_to_bitmap(time_start_position, 1<<num_symbols)` as in `nr_configure_srs()` (gNB_scheduler_srs.c).
 *
 * Each DCI in a PDCCH PDU (DL_TTI or UL_DCI.request) becomes one rectangle: the
 * axis-aligned bbox of PRBs touched by CceIndex..CceIndex+L-1 using the same
 * CCE->REG->RB geometry as fill_pdcch_vrb_map() / cce_to_reg_interleaving() in
 * gNB_scheduler_primitives.c (not the full CORESET outline). UL DCI is still on
 * the downlink air interface, so those regions are appended to the DL MQ list.
 *
 * Environment
 * -----------
 * Both slot MQ names must be set or the module does nothing:
 *   FAPI_MQ_PATH_DL / FAPI_MQ_PATH_UL
 *
 * TDD routing
 * -----------
 * FDD: both MQs written each schedule tick (v1 empty if that direction has zero
 * regions). TDD DL-only slot: DL MQ only. TDD UL-only slot: UL MQ only.
 * TDD mixed slot: no write to either MQ (ambiguous overlay). slot_id increments
 * only when its MQ is actually written.
 */

#include <errno.h>
#include <limits.h>
#include <mqueue.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "assertions.h"
#include "common/utils/LOG/log.h"
#include "common/utils/nr/nr_common.h"
#include "NR_MAC_COMMON/nr_mac.h"
#include "NR_MAC_COMMON/nr_mac_common.h"
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_MAC_gNB/resgrid_plotter_slot_mq.h"
#include "nfapi_nr_interface_scf.h"

#define RESGRID_SLOT_MAGIC 0x31564153u
#define RESGRID_SLOT_VER1 1u
#define RESGRID_SLOT_VER2 2u
#define RESGRID_SLOT_FLAG_MULTI 1u
#define RESGRID_MQ_CHUNK_MAGIC 0x52474D43u
#define RESGRID_MQ_CHUNK_VERSION 2u

/* Logical channel bytes (paired DL/UL + sounding); must match plotter CUDA legend. */
#define RESGRID_CHAN_SSB 1u
#define RESGRID_CHAN_PRACH 2u
#define RESGRID_CHAN_PDCCH 3u
#define RESGRID_CHAN_PUCCH 4u
#define RESGRID_CHAN_PDSCH 5u
#define RESGRID_CHAN_PUSCH 6u
#define RESGRID_CHAN_CSI_RS 7u
#define RESGRID_CHAN_SRS 8u

/** Keep one slot message well below the visualizer mq message size. */
#define RESGRID_REGIONS_CHUNK 236
#define RESGRID_MAX_REGIONS 1536

#pragma pack(push, 1)
/* v1: single-slot heartbeat - sym/prb counts zero; channel 0.
 * v2 + FLAG_MULTI: sym/prb zero in hdr; next uint16_t = N; then N x SlotRegionDesc. */
typedef struct {
  uint32_t magic;
  uint16_t version;
  uint16_t flags;
  uint32_t slot_id;
  uint16_t sym_start;
  uint16_t sym_count;
  uint16_t prb_start;
  uint16_t prb_count;
  uint32_t rnti;
  uint8_t channel;
  uint8_t reserved0;
  uint16_t reserved1;
  uint32_t n_complex;
  uint32_t reserved2;
  uint32_t reserved3;
} resgrid_SlotMsgHdr;
#pragma pack(pop)

_Static_assert(sizeof(resgrid_SlotMsgHdr) == 40, "resgrid slot header size");

#pragma pack(push, 1)
typedef struct {
  uint32_t magic;
  uint32_t version;
  uint32_t frame_seq;
  uint16_t chunk_idx;
  uint16_t n_chunks;
  uint32_t logical_len;
  uint32_t byte_off;
  uint16_t prb;
  uint16_t symbols;
} resgrid_MqChunkHdr;
#pragma pack(pop)

_Static_assert(sizeof(resgrid_MqChunkHdr) == 28, "resgrid mq chunk header size");

#pragma pack(push, 1)
typedef struct {
  uint16_t sym_start;
  uint16_t sym_count;
  uint16_t prb_start;
  uint16_t prb_count;
  uint32_t rnti;
  uint8_t channel;
  uint8_t reserved[3];
} resgrid_SlotRegionDesc;
#pragma pack(pop)

_Static_assert(sizeof(resgrid_SlotRegionDesc) == 16, "resgrid slot region size");

/* Working copy while collecting; flattened into SlotRegionDesc on write. */
typedef struct {
  uint16_t sym_start;
  uint16_t sym_count;
  uint16_t prb_start;
  uint16_t prb_count;
  uint32_t rnti;
  uint8_t channel;
} resgrid_region_t;

static void resgrid_clamp_region(resgrid_region_t *r, int max_prb, int max_sym);

/** Symbol bitmask for NZP CSI-RS from NFAPI `row` / `symb_l0` / `symb_l1` (matches MAC vrb_map OR patterns). */
static uint16_t resgrid_csirs_symbol_mask(uint8_t row, uint8_t symb_l0, uint8_t symb_l1)
{
  switch (row) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 6:
    case 9:
      return SL_to_bitmap(symb_l0, 1);
    case 5:
    case 7:
    case 8:
    case 10:
    case 11:
    case 12:
      return SL_to_bitmap(symb_l0, 2);
    case 13:
    case 14:
    case 16:
    case 17:
      return (uint16_t)(SL_to_bitmap(symb_l0, 2) | SL_to_bitmap(symb_l1, 2));
    case 15:
    case 18:
      return SL_to_bitmap(symb_l0, 3);
    default:
      return 0;
  }
}

static void resgrid_sym_mask_to_span(uint16_t mask, uint16_t *sym_start, uint16_t *sym_count)
{
  if (mask == 0) {
    *sym_start = 0;
    *sym_count = 0;
    return;
  }
  int lo = -1;
  int hi = -1;
  for (int s = 0; s < NR_NUMBER_OF_SYMBOLS_PER_SLOT; s++) {
    if (mask & (1u << s)) {
      if (lo < 0)
        lo = s;
      hi = s;
    }
  }
  if (lo < 0 || hi < 0) {
    *sym_start = 0;
    *sym_count = 0;
    return;
  }
  *sym_start = (uint16_t)lo;
  *sym_count = (uint16_t)(hi - lo + 1);
}

static int resgrid_prach_symbol_duration(uint8_t prach_format)
{
  /* Same table as get_nr_prach_duration() in openair1/SCHED_NR/nr_prach_procedures.c */
  const int val[14] = {0, 0, 0, 0, 2, 4, 6, 2, 12, 2, 6, 2, 4, 6};
  if (prach_format >= sizeofArray(val))
    return 0;
  return val[prach_format];
}

/* One bbox per DCI; PRB extent = min/max of RB indices marked by MAC/PHY PDCCH mapping. */
static void resgrid_pdcch_pdu_append_dcis(const nfapi_nr_dl_tti_pdcch_pdu_rel15_t *p,
                                          int max_prb,
                                          int max_sym,
                                          resgrid_region_t *out,
                                          int *n_io,
                                          int out_cap)
{
  int n_rb = 0;
  int rb_off_unused = 0;
  get_coreset_rballoc(p->FreqDomainResource, &n_rb, &rb_off_unused);
  const int Lbundle = (int)p->RegBundleSize;
  const int R = (int)p->InterleaverSize;
  const int N_symb = (int)p->DurationSymbols;
  if (n_rb <= 0 || N_symb <= 0 || Lbundle <= 0 || (Lbundle % N_symb) != 0)
    return;

  const int N_regs = n_rb * N_symb;
  const int B_rb = Lbundle / N_symb;
  if (B_rb <= 0)
    return;

  int C = 0;
  if (R > 0) {
    const int denom = Lbundle * R;
    if (denom <= 0 || (N_regs % denom) != 0)
      return;
    C = N_regs / denom;
  }

  const int n_shift = (int)p->ShiftIndex;

  for (uint16_t di = 0; di < p->numDlDci && *n_io < out_cap; di++) {
    const nfapi_nr_dl_dci_pdu_t *dci = &p->dci_pdu[di];
    const int agg = (int)dci->AggregationLevel;
    const int first_cce = (int)dci->CceIndex;
    if (agg <= 0 || first_cce < 0)
      continue;

    int prb_min = INT_MAX;
    int prb_max = INT_MIN;
    for (int j = first_cce; j < first_cce + agg; j++) {
      for (int k = 6 * j / Lbundle; k < (6 * j / Lbundle + 6 / Lbundle); k++) {
        const int f = cce_to_reg_interleaving(R, k, n_shift, C, Lbundle, N_regs);
        for (int rb = 0; rb < B_rb; rb++) {
          const int abs_rb = (int)p->BWPStart + f * B_rb + rb;
          if (abs_rb < prb_min)
            prb_min = abs_rb;
          if (abs_rb > prb_max)
            prb_max = abs_rb;
        }
      }
    }

    if (prb_min > prb_max || prb_min < 0)
      continue;

    resgrid_region_t reg = {.sym_start = p->StartSymbolIndex,
                            .sym_count = (uint16_t)N_symb,
                            .prb_start = (uint16_t)prb_min,
                            .prb_count = (uint16_t)(prb_max - prb_min + 1),
                            .rnti = dci->RNTI,
                            .channel = RESGRID_CHAN_PDCCH};
    resgrid_clamp_region(&reg, max_prb, max_sym);
    if (reg.sym_count > 0 && reg.prb_count > 0)
      out[(*n_io)++] = reg;
  }
}

static void resgrid_clamp_region(resgrid_region_t *r, int max_prb, int max_sym)
{
  if (r->sym_count == 0 || r->prb_count == 0)
    return;
  if (r->sym_start >= max_sym) {
    r->sym_count = 0;
    return;
  }
  if (r->prb_start >= max_prb) {
    r->prb_count = 0;
    return;
  }
  if (r->sym_start + r->sym_count > (uint16_t)max_sym)
    r->sym_count = (uint16_t)max_sym - r->sym_start;
  if (r->prb_start + r->prb_count > (uint16_t)max_prb)
    r->prb_count = (uint16_t)max_prb - r->prb_start;
}

static const char *resgrid_env_nonempty(const char *key)
{
  const char *p = getenv(key);
  return (p && p[0]) ? p : NULL;
}

/** DL slot MQ name (`FAPI_MQ_PATH_DL`). */
static const char *resgrid_slot_mq_path_dl(void)
{
  return resgrid_env_nonempty("FAPI_MQ_PATH_DL");
}

static const char *resgrid_slot_mq_path_ul(void)
{
  return resgrid_env_nonempty("FAPI_MQ_PATH_UL");
}

static void resgrid_log_config_once(const char *path_dl, const char *path_ul)
{
  static bool logged;
  if (logged)
    return;
  LOG_I(NR_MAC, "resgrid slot mq configured DL=%s UL=%s\n", path_dl ? path_dl : "(null)", path_ul ? path_ul : "(null)");
  logged = true;
}

static void resgrid_log_missing_config_once(const char *path_dl, const char *path_ul)
{
  static bool warned;
  if (warned)
    return;
  LOG_W(NR_MAC,
        "resgrid slot mq disabled: DL path=%s UL path=%s (set FAPI_MQ_PATH_DL/UL)\n",
        path_dl ? path_dl : "(null)",
        path_ul ? path_ul : "(null)");
  warned = true;
}

static void resgrid_log_first_send_once(const char *label, const char *path, uint32_t slot_id, int n_regs, bool empty_marker)
{
  static bool logged_dl;
  static bool logged_ul;
  bool *logged = NULL;
  if (label && strcmp(label, "DL") == 0)
    logged = &logged_dl;
  else if (label && strcmp(label, "UL") == 0)
    logged = &logged_ul;
  if (logged == NULL || *logged)
    return;
  LOG_I(NR_MAC,
        "resgrid slot mq %s active path=%s slot_id=%u regions=%d mode=%s\n",
        label,
        path,
        slot_id,
        n_regs,
        empty_marker ? "empty" : "regions");
  *logged = true;
}

static uint32_t resgrid_next_mq_frame_seq(const char *label)
{
  static uint32_t dl_seq;
  static uint32_t ul_seq;
  static uint32_t other_seq;

  if (label && strcmp(label, "DL") == 0)
    return ++dl_seq;
  if (label && strcmp(label, "UL") == 0)
    return ++ul_seq;
  return ++other_seq;
}

static uint64_t resgrid_now_ms(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
}

static void resgrid_log_send_rate(const char *label)
{
  static uint64_t window_start_ms;
  static uint64_t dl_window_msgs;
  static uint64_t ul_window_msgs;
  static uint64_t dl_total_msgs;
  static uint64_t ul_total_msgs;

  if (label && strcmp(label, "DL") == 0) {
    dl_window_msgs++;
    dl_total_msgs++;
  } else if (label && strcmp(label, "UL") == 0) {
    ul_window_msgs++;
    ul_total_msgs++;
  } else {
    return;
  }

  const uint64_t now_ms = resgrid_now_ms();
  if (window_start_ms == 0)
    window_start_ms = now_ms;

  if (now_ms - window_start_ms < 2000u)
    return;

  LOG_I(NR_MAC,
        "resgrid slot mq sent last_2s DL=%llu UL=%llu total=%llu cumulative DL=%llu UL=%llu total=%llu\n",
        (unsigned long long)dl_window_msgs,
        (unsigned long long)ul_window_msgs,
        (unsigned long long)(dl_window_msgs + ul_window_msgs),
        (unsigned long long)dl_total_msgs,
        (unsigned long long)ul_total_msgs,
        (unsigned long long)(dl_total_msgs + ul_total_msgs));

  dl_window_msgs = 0;
  ul_window_msgs = 0;
  window_start_ms = now_ms;
}

static void resgrid_slot_drop_mq(gNB_MAC_INST *gNB, mqd_t *mq_store)
{
  (void)gNB;
  if (*mq_store != (mqd_t)-1) {
    mq_close(*mq_store);
    *mq_store = (mqd_t)-1;
  }
}

static void resgrid_slot_drop_dl(gNB_MAC_INST *gNB)
{
  resgrid_slot_drop_mq(gNB, &gNB->resgrid_slot_mq_dl);
}

static void resgrid_slot_drop_ul(gNB_MAC_INST *gNB)
{
  resgrid_slot_drop_mq(gNB, &gNB->resgrid_slot_mq_ul);
}

static bool resgrid_ensure_slot_mq_open(gNB_MAC_INST *gNB, mqd_t *mq_store, const char *path, const char *label)
{
  if (*mq_store != (mqd_t)-1)
    return true;
  if (!path)
    return false;
  mqd_t mq = mq_open(path, O_WRONLY | O_NONBLOCK);
  if (mq == (mqd_t)-1) {
    static uint32_t warn_ctr;
    if ((warn_ctr++ % 1000u) == 0u)
      LOG_W(NR_MAC, "resgrid slot mq %s open(%s): %s\n", label ? label : "?", path, strerror(errno));
    return false;
  }
  *mq_store = mq;
  LOG_I(NR_MAC, "resgrid slot mq %s writer opened %s (mq=%d)\n", label ? label : "?", path, (int)mq);
  return true;
}

static bool resgrid_slot_mq_send(mqd_t mq, const char *label, const void *buf, size_t len)
{
  struct mq_attr attr = {0};
  long mq_msgsize_cap = 8192;
  if (mq_getattr(mq, &attr) == 0 && attr.mq_msgsize > 0)
    mq_msgsize_cap = attr.mq_msgsize;

  const size_t hdr_sz = sizeof(resgrid_MqChunkHdr);
  if ((size_t)mq_msgsize_cap <= hdr_sz) {
    errno = EMSGSIZE;
    return false;
  }

  const size_t chunk_cap = (size_t)mq_msgsize_cap - hdr_sz;
  const uint8_t *p = (const uint8_t *)buf;
  const uint32_t frame_seq = resgrid_next_mq_frame_seq(label);
  const uint32_t n_chunks_u32 = len == 0 ? 1u : (uint32_t)((len + chunk_cap - 1u) / chunk_cap);
  if (n_chunks_u32 == 0 || n_chunks_u32 > 65535u) {
    errno = EMSGSIZE;
    return false;
  }

  const uint16_t n_chunks = (uint16_t)n_chunks_u32;
  size_t off = 0;
  for (uint16_t ci = 0; ci < n_chunks; ++ci) {
    const size_t remain = len - off;
    const size_t take = remain > chunk_cap ? chunk_cap : remain;
    const size_t msg_len = hdr_sz + take;
    uint8_t *msg = malloc(msg_len);
    if (!msg) {
      errno = ENOMEM;
      return false;
    }

    resgrid_MqChunkHdr h;
    h.magic = RESGRID_MQ_CHUNK_MAGIC;
    h.version = RESGRID_MQ_CHUNK_VERSION;
    h.frame_seq = frame_seq;
    h.chunk_idx = ci;
    h.n_chunks = n_chunks;
    h.logical_len = (uint32_t)len;
    h.byte_off = (uint32_t)off;
    h.prb = 0;
    h.symbols = 0;

    memcpy(msg, &h, hdr_sz);
    if (take > 0)
      memcpy(msg + hdr_sz, p + off, take);

    while (mq_send(mq, (const char *)msg, msg_len, 0) < 0) {
      if (errno == EINTR)
        continue;
      free(msg);
      return false;
    }

    free(msg);
    resgrid_log_send_rate(label);
    off += take;
  }
  return true;
}

/* v1 frame: slot advance marker only (zero-area region). */
static void resgrid_pack_hdr_v1_empty(resgrid_SlotMsgHdr *h, uint32_t slot_id)
{
  memset(h, 0, sizeof(*h));
  h->magic = RESGRID_SLOT_MAGIC;
  h->version = RESGRID_SLOT_VER1;
  h->flags = 0;
  h->slot_id = slot_id;
  h->sym_start = 0;
  h->sym_count = 0;
  h->prb_start = 0;
  h->prb_count = 0;
  h->rnti = 0;
  h->channel = 0;
  h->reserved0 = 0;
  h->reserved1 = 0;
  h->n_complex = 0;
  h->reserved2 = 0;
  h->reserved3 = 0;
}

/* v2 multi header: geometry lives in following uint16 count + SlotRegionDesc[]. */
static void resgrid_pack_hdr_v2_multi(resgrid_SlotMsgHdr *h, uint32_t slot_id, uint32_t n_complex)
{
  memset(h, 0, sizeof(*h));
  h->magic = RESGRID_SLOT_MAGIC;
  h->version = RESGRID_SLOT_VER2;
  h->flags = RESGRID_SLOT_FLAG_MULTI;
  h->slot_id = slot_id;
  h->n_complex = n_complex;
}

/* Split large region lists so each mq_send stays within the queue message size. */
static bool resgrid_emit_v2_chunks(gNB_MAC_INST *gNB,
                                   mqd_t *mq_store,
                                   const char *label,
                                   const char *path,
                                   uint32_t slot_id,
                                   resgrid_region_t *regs,
                                   int n_regs)
{
  if (!resgrid_ensure_slot_mq_open(gNB, mq_store, path, label))
    return false;
  const mqd_t mq = *mq_store;
  int off = 0;
  while (off < n_regs) {
    const int chunk = n_regs - off > RESGRID_REGIONS_CHUNK ? RESGRID_REGIONS_CHUNK : n_regs - off;
    const size_t blob = sizeof(resgrid_SlotMsgHdr) + sizeof(uint16_t) + (size_t)chunk * sizeof(resgrid_SlotRegionDesc);
    uint8_t *buf = malloc(blob);
    if (!buf)
      return false;
    resgrid_SlotMsgHdr *h = (resgrid_SlotMsgHdr *)buf;
    resgrid_pack_hdr_v2_multi(h, slot_id, 0);
    uint16_t *pn = (uint16_t *)(buf + sizeof(resgrid_SlotMsgHdr));
    *pn = (uint16_t)chunk;
    resgrid_SlotRegionDesc *rd = (resgrid_SlotRegionDesc *)(buf + sizeof(resgrid_SlotMsgHdr) + sizeof(uint16_t));
    for (int i = 0; i < chunk; i++) {
      const resgrid_region_t *s = &regs[off + i];
      rd[i].sym_start = s->sym_start;
      rd[i].sym_count = s->sym_count;
      rd[i].prb_start = s->prb_start;
      rd[i].prb_count = s->prb_count;
      rd[i].rnti = s->rnti;
      rd[i].channel = s->channel;
      rd[i].reserved[0] = rd[i].reserved[1] = rd[i].reserved[2] = 0;
    }
    bool ok = resgrid_slot_mq_send(mq, label, buf, blob);
    free(buf);
    if (!ok) {
      static uint32_t w;
      if ((w++ % 500u) == 0u)
        LOG_W(NR_MAC, "resgrid slot mq %s send failed path=%s: %s\n", label ? label : "?", path, strerror(errno));
      if (errno != EAGAIN)
        resgrid_slot_drop_mq(gNB, mq_store);
      return false;
    }
    off += chunk;
  }
  resgrid_log_first_send_once(label, path, slot_id, n_regs, false);
  return true;
}

static bool resgrid_emit_v1_empty(gNB_MAC_INST *gNB, mqd_t *mq_store, const char *label, const char *path, uint32_t slot_id)
{
  if (!resgrid_ensure_slot_mq_open(gNB, mq_store, path, label))
    return false;
  const mqd_t mq = *mq_store;
  resgrid_SlotMsgHdr h;
  resgrid_pack_hdr_v1_empty(&h, slot_id);
  if (!resgrid_slot_mq_send(mq, label, &h, sizeof h)) {
    static uint32_t w;
    if ((w++ % 500u) == 0u)
      LOG_W(NR_MAC, "resgrid slot mq %s send (v1 empty) failed path=%s: %s\n", label ? label : "?", path, strerror(errno));
    if (errno != EAGAIN)
      resgrid_slot_drop_mq(gNB, mq_store);
    return false;
  }
  resgrid_log_first_send_once(label, path, slot_id, 0, true);
  return true;
}

static void resgrid_emit_regions_or_empty(gNB_MAC_INST *gNB,
                                          mqd_t *mq_store,
                                          const char *label,
                                          const char *path,
                                          uint32_t slot_id,
                                          resgrid_region_t *regs,
                                          int n_regs)
{
  /* Reader distinguishes slot tick with no shapes vs payload present. */
  if (n_regs <= 0)
    (void)resgrid_emit_v1_empty(gNB, mq_store, label, path, slot_id);
  else
    (void)resgrid_emit_v2_chunks(gNB, mq_store, label, path, slot_id, regs, n_regs);
}

/* Scan DL_tti PDUs plus UL_DCI.request PDCCH PDUs (UL grants on DL). */
static int resgrid_collect_dl(gNB_MAC_INST *gNB,
                              const nfapi_nr_dl_tti_request_t *dl,
                              const nfapi_nr_ul_dci_request_t *ul_dci,
                              NR_ServingCellConfigCommon_t *scc,
                              int max_prb,
                              int max_sym,
                              resgrid_region_t *out,
                              int out_cap)
{
  int n = 0;
  (void)gNB;
  if (!dl)
    return 0;
  const nfapi_nr_dl_tti_request_body_t *body = &dl->dl_tti_request_body;
  const long band = *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0];
  const NR_SubcarrierSpacing_t ssb_scs = *scc->ssbSubcarrierSpacing;
  const frequency_range_t freq_range = get_freq_range_from_arfcn(scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA);

  for (int i = 0; i < body->nPDUs && n < out_cap; i++) {
    const nfapi_nr_dl_tti_request_pdu_t *pdu = &body->dl_tti_pdu_list[i];
    switch (pdu->PDUType) {
      case NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE: {
        const nfapi_nr_dl_tti_pdcch_pdu_rel15_t *p = &pdu->pdcch_pdu.pdcch_pdu_rel15;
        resgrid_pdcch_pdu_append_dcis(p, max_prb, max_sym, out, &n, out_cap);
      } break;

      /* Scheduled DL data: NFAPI rbStart/rbSize + symbol span are explicit. */
      case NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE: {
        const nfapi_nr_dl_tti_pdsch_pdu_rel15_t *p = &pdu->pdsch_pdu.pdsch_pdu_rel15;
        out[n].sym_start = p->StartSymbolIndex;
        out[n].sym_count = p->NrOfSymbols;
        out[n].prb_start = p->BWPStart + p->rbStart;
        out[n].prb_count = p->rbSize;
        out[n].rnti = p->rnti;
        out[n].channel = RESGRID_CHAN_PDSCH;
        resgrid_clamp_region(&out[n], max_prb, max_sym);
        if (out[n].sym_count > 0 && out[n].prb_count > 0)
          n++;
      } break;

      /* SS/PBCH block: map band/SCS/ssbOffsetPointA into slot-local PRB/symbol box. */
      case NFAPI_NR_DL_TTI_SSB_PDU_TYPE: {
        const nfapi_nr_dl_tti_ssb_pdu_rel15_t *p = &pdu->ssb_pdu.ssb_pdu_rel15;
        const uint16_t ssb_sym = get_ssb_start_symbol(band, ssb_scs, p->SsbBlockIndex) % NR_NUMBER_OF_SYMBOLS_PER_SLOT;
        int prb0;
        if (freq_range == FR2) {
          int sh = (int)ssb_scs - 2;
          if (sh < 0)
            sh = 0;
          prb0 = (int)(p->ssbOffsetPointA >> sh);
        } else
          prb0 = (int)(p->ssbOffsetPointA >> ssb_scs);
        const int extra = (p->SsbSubcarrierOffset > 0) ? 1 : 0;
        out[n].sym_start = ssb_sym;
        out[n].sym_count = 4;
        out[n].prb_start = (uint16_t)prb0;
        out[n].prb_count = (uint16_t)(20 + extra);
        out[n].rnti = 0;
        out[n].channel = RESGRID_CHAN_SSB;
        resgrid_clamp_region(&out[n], max_prb, max_sym);
        if (out[n].sym_count > 0 && out[n].prb_count > 0)
          n++;
      } break;

      case NFAPI_NR_DL_TTI_CSI_RS_PDU_TYPE: {
        const nfapi_nr_dl_tti_csi_rs_pdu_rel15_t *p = &pdu->csi_rs_pdu.csi_rs_pdu_rel15;
        if (p->nr_of_rbs == 0)
          break;
        const uint16_t sm = resgrid_csirs_symbol_mask(p->row, p->symb_l0, p->symb_l1);
        uint16_t ss = 0;
        uint16_t sc = 0;
        resgrid_sym_mask_to_span(sm, &ss, &sc);
        if (sc == 0)
          break;
        out[n].sym_start = ss;
        out[n].sym_count = sc;
        out[n].prb_start = p->start_rb;
        out[n].prb_count = p->nr_of_rbs;
        out[n].rnti = 0;
        out[n].channel = RESGRID_CHAN_CSI_RS;
        resgrid_clamp_region(&out[n], max_prb, max_sym);
        if (out[n].sym_count > 0 && out[n].prb_count > 0)
          n++;
      } break;

      default:
        break;
    }
  }

  if (ul_dci != NULL) {
    for (uint8_t ui = 0; ui < ul_dci->numPdus && n < out_cap; ui++) {
      const nfapi_nr_ul_dci_request_pdus_t *upd = &ul_dci->ul_dci_pdu_list[ui];
      if (upd->PDUType != NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE)
        continue;
      const nfapi_nr_dl_tti_pdcch_pdu_rel15_t *p = &upd->pdcch_pdu.pdcch_pdu_rel15;
      resgrid_pdcch_pdu_append_dcis(p, max_prb, max_sym, out, &n, out_cap);
    }
  }

  return n;
}

/* UL regions from ul_tti only (PUSCH / PUCCH / PRACH). PRACH PRBs derived from RACH config. */
static int resgrid_collect_ul(gNB_MAC_INST *gNB,
                              const nfapi_nr_ul_tti_request_t *ul,
                              NR_ServingCellConfigCommon_t *scc,
                              int max_prb,
                              int max_sym,
                              resgrid_region_t *out,
                              int out_cap,
                              int n0)
{
  int n = n0;
  if (!ul)
    return n;

  NR_BWP_UplinkCommon_t *initialUplinkBWP = scc->uplinkConfigCommon->initialUplinkBWP;
  const int bwp_start = NRRIV2PRBOFFSET(initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  const int mu_pusch = scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
  const uint16_t n_ra_rb = (uint16_t)get_N_RA_RB(gNB->config[0].prach_config.prach_sub_c_spacing.value, mu_pusch);
  uint16_t msg1_freq_start = 0;
  if (initialUplinkBWP->rach_ConfigCommon && initialUplinkBWP->rach_ConfigCommon->present == NR_SetupRelease_RACH_ConfigCommon_PR_setup) {
    NR_RACH_ConfigGeneric_t *rachgen = &initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric;
    msg1_freq_start = (uint16_t)rachgen->msg1_FrequencyStart;
  }

  for (int i = 0; i < ul->n_pdus && n < out_cap; i++) {
    const nfapi_nr_ul_tti_request_number_of_pdus_t *pdu = &ul->pdus_list[i];
    switch (pdu->pdu_type) {
      case NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE: {
        const nfapi_nr_pusch_pdu_t *p = &pdu->pusch_pdu;
        out[n].sym_start = p->start_symbol_index;
        out[n].sym_count = p->nr_of_symbols;
        out[n].prb_start = p->bwp_start + p->rb_start;
        out[n].prb_count = p->rb_size;
        out[n].rnti = p->rnti;
        out[n].channel = RESGRID_CHAN_PUSCH;
        resgrid_clamp_region(&out[n], max_prb, max_sym);
        if (out[n].sym_count > 0 && out[n].prb_count > 0)
          n++;
      } break;

      case NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE: {
        const nfapi_nr_pucch_pdu_t *p = &pdu->pucch_pdu;
        out[n].sym_start = p->start_symbol_index;
        out[n].sym_count = p->nr_of_symbols;
        out[n].prb_start = p->bwp_start + p->prb_start;
        out[n].prb_count = p->prb_size;
        out[n].rnti = p->rnti;
        out[n].channel = RESGRID_CHAN_PUCCH;
        resgrid_clamp_region(&out[n], max_prb, max_sym);
        if (out[n].sym_count > 0 && out[n].prb_count > 0)
          n++;
      } break;

      case NFAPI_NR_UL_CONFIG_PRACH_PDU_TYPE: {
        const nfapi_nr_prach_pdu_t *p = &pdu->prach_pdu;
        const int N_dur = resgrid_prach_symbol_duration(p->prach_format);
        if (N_dur <= 0 || p->num_prach_ocas == 0)
          break;
        const uint32_t span = (uint32_t)p->prach_start_symbol + (uint32_t)p->num_prach_ocas * (uint32_t)N_dur;
        out[n].sym_start = p->prach_start_symbol;
        out[n].sym_count = (uint16_t)((span <= (uint32_t)max_sym) ? (span - p->prach_start_symbol) : (uint32_t)(max_sym - p->prach_start_symbol));
        if (out[n].sym_count == 0)
          break;
        out[n].prb_start = (uint16_t)(bwp_start + msg1_freq_start + (uint16_t)p->num_ra * n_ra_rb);
        out[n].prb_count = n_ra_rb;
        out[n].rnti = 0;
        out[n].channel = RESGRID_CHAN_PRACH;
        resgrid_clamp_region(&out[n], max_prb, max_sym);
        if (out[n].sym_count > 0 && out[n].prb_count > 0)
          n++;
      } break;

      case NFAPI_NR_UL_CONFIG_SRS_PDU_TYPE: {
        const nfapi_nr_srs_pdu_t *p = &pdu->srs_pdu;
        if (p->bwp_size == 0)
          break;
        const unsigned num_sym = 1u << (unsigned)p->num_symbols;
        const uint16_t sm = SL_to_bitmap((int)p->time_start_position, (int)num_sym);
        uint16_t ss = 0;
        uint16_t sc = 0;
        resgrid_sym_mask_to_span(sm, &ss, &sc);
        if (sc == 0)
          break;
        out[n].sym_start = ss;
        out[n].sym_count = sc;
        out[n].prb_start = p->bwp_start;
        out[n].prb_count = p->bwp_size;
        out[n].rnti = p->rnti;
        out[n].channel = RESGRID_CHAN_SRS;
        resgrid_clamp_region(&out[n], max_prb, max_sym);
        if (out[n].sym_count > 0 && out[n].prb_count > 0)
          n++;
      } break;

      default:
        break;
    }
  }
  return n;
}

void nr_mac_resgrid_slot_mq_cleanup(struct gNB_MAC_INST_s *gNB_in)
{
  gNB_MAC_INST *gNB = (gNB_MAC_INST *)gNB_in;
  resgrid_slot_drop_dl(gNB);
  resgrid_slot_drop_ul(gNB);
}

/*
 * Collect rectangles for this scheduling call, then apply TDD policy:
 *   FDD        -> emit DL list to DL MQ and UL list to UL MQ (counts may differ).
 *   TDD mixed  -> emit nothing (both n_*_mq forced to 0); avoids ambiguous overlays.
 *   TDD DL/UL  -> only the matching-direction MQ gets data for that slot.
 * emit_* gates increment slot_id only when that MQ is written (see block comment top).
 * ul_dci regions are appended with DL regions (both are DL-air PDCCH); NULL skips UL-DCI merge.
 */
void nr_mac_resgrid_emit_after_schedule(struct gNB_MAC_INST_s *gNB_in,
                                        const nfapi_nr_dl_tti_request_t *dl,
                                        const nfapi_nr_ul_tti_request_t *ul,
                                        const nfapi_nr_ul_dci_request_t *ul_dci,
                                        frame_t frame,
                                        slot_t slot)
{
  gNB_MAC_INST *gNB = (gNB_MAC_INST *)gNB_in;
  (void)frame;
  const char *path_dl = resgrid_slot_mq_path_dl();
  const char *path_ul = resgrid_slot_mq_path_ul();
  if (!path_dl || !path_ul) {
    resgrid_log_missing_config_once(path_dl, path_ul);
    return;
  }
  resgrid_log_config_once(path_dl, path_ul);

  NR_ServingCellConfigCommon_t *scc = gNB->common_channels[0].ServingCellConfigCommon;
  const int max_sym = NR_NUMBER_OF_SYMBOLS_PER_SLOT;
  const int max_prb = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;

  const frame_structure_t *fs = &gNB->frame_structure;

  resgrid_region_t regs_dl[RESGRID_MAX_REGIONS];
  resgrid_region_t regs_ul[RESGRID_MAX_REGIONS];
  const int ndl = resgrid_collect_dl(gNB, dl, ul_dci, scc, max_prb, max_sym, regs_dl, RESGRID_MAX_REGIONS);
  const int nul = resgrid_collect_ul(gNB, ul, scc, max_prb, max_sym, regs_ul, RESGRID_MAX_REGIONS, 0);

  int n_dl_mq = 0;
  int n_ul_mq = 0;

  const bool tdd_mixed = (fs->frame_type != FDD) && is_mixed_slot(slot, fs);
  if (fs->frame_type == FDD) {
    n_dl_mq = ndl;
    n_ul_mq = nul;
  } else if (tdd_mixed) {
    n_dl_mq = 0;
    n_ul_mq = 0;
  } else {
    if (is_dl_slot(slot, fs))
      n_dl_mq = ndl;
    if (is_ul_slot(slot, fs))
      n_ul_mq = nul;
  }

  const bool emit_dl = (fs->frame_type == FDD) || (is_dl_slot(slot, fs) && !tdd_mixed);
  const bool emit_ul = (fs->frame_type == FDD) || (is_ul_slot(slot, fs) && !tdd_mixed);

  if (emit_dl) {
    gNB->resgrid_slot_seq_dl++;
    resgrid_emit_regions_or_empty(gNB, &gNB->resgrid_slot_mq_dl, "DL", path_dl, gNB->resgrid_slot_seq_dl, regs_dl, n_dl_mq);
  }
  if (emit_ul) {
    gNB->resgrid_slot_seq_ul++;
    resgrid_emit_regions_or_empty(gNB, &gNB->resgrid_slot_mq_ul, "UL", path_ul, gNB->resgrid_slot_seq_ul, regs_ul, n_ul_mq);
  }
}
