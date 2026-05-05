/*
 * Optional slot FIFO export for resgrid_plotter (channel-based / --data-mode 3).
 * Wire format: resouce-grid/docs/CHANNEL_BASED_INTERFACE.md
 *
 * Data path
 * ---------
 * NFAPI DL/UL TTI PDUs are turned into axis-aligned rectangles (sym_start/count,
 * prb_start/count) tagged with RESGRID_CHAN_* and optional RNTI, then written as
 * one v1 “empty marker” frame or chunked v2 multi-region frames (see below).
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
 * CCE→REG→RB geometry as fill_pdcch_vrb_map() / cce_to_reg_interleaving() in
 * gNB_scheduler_primitives.c (not the full CORESET outline). UL DCI is still on
 * the downlink air interface, so those regions are appended to the DL FIFO list.
 *
 * Environment
 * -----------
 * Both FIFO paths must be set or the module does nothing:
 *   RESGRID_SLOT_FIFO_PATH_DL / RESGRID_SLOT_FIFO_PATH_UL
 * or CUDA_SLOT_FIFO_PATH_DL / CUDA_SLOT_FIFO_PATH_UL (aliases).
 *
 * TDD routing
 * -----------
 * FDD: both FIFOs written each schedule tick (v1 empty if that direction has zero
 * regions). TDD DL-only slot: DL FIFO only. TDD UL-only slot: UL FIFO only.
 * TDD mixed slot: no write to either FIFO (ambiguous overlay). slot_id increments
 * only when its FIFO is actually written.
 */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "assertions.h"
#include "common/utils/LOG/log.h"
#include "common/utils/nr/nr_common.h"
#include "NR_MAC_COMMON/nr_mac.h"
#include "NR_MAC_COMMON/nr_mac_common.h"
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_MAC_gNB/resgrid_plotter_slot_fifo.h"
#include "nfapi_nr_interface_scf.h"

#define RESGRID_SLOT_MAGIC 0x31564153u
#define RESGRID_SLOT_VER1 1u
#define RESGRID_SLOT_VER2 2u
#define RESGRID_SLOT_FLAG_MULTI 1u

/* Logical channel bytes (paired DL/UL + sounding); must match plotter CUDA legend. */
#define RESGRID_CHAN_SSB 1u
#define RESGRID_CHAN_PRACH 2u
#define RESGRID_CHAN_PDCCH 3u
#define RESGRID_CHAN_PUCCH 4u
#define RESGRID_CHAN_PDSCH 5u
#define RESGRID_CHAN_PUSCH 6u
#define RESGRID_CHAN_CSI_RS 7u
#define RESGRID_CHAN_SRS 8u

/** Keep one Linux pipe write under PIPE_BUF (4096) for atomicity on common kernels. */
#define RESGRID_REGIONS_CHUNK 236
#define RESGRID_MAX_REGIONS 1536

#pragma pack(push, 1)
/* v1: single-slot heartbeat — sym/prb counts zero; channel 0.
 * v2 + FLAG_MULTI: sym/prb zero in hdr; next uint16_t = N; then N × SlotRegionDesc. */
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

static bool resgrid_write_all(int fd, const void *buf, size_t len)
{
  const uint8_t *p = (const uint8_t *)buf;
  size_t off = 0;
  while (off < len) {
    ssize_t w = write(fd, p + off, len - off);
    if (w < 0) {
      if (errno == EINTR)
        continue;
      return false;
    }
    if (w == 0)
      return false;
    off += (size_t)w;
  }
  return true;
}

static const char *resgrid_env_nonempty(const char *key)
{
  const char *p = getenv(key);
  return (p && p[0]) ? p : NULL;
}

/** DL slot FIFO path (`*_DL` or CUDA equivalent). */
static const char *resgrid_fifo_path_dl_dual(void)
{
  const char *p = resgrid_env_nonempty("RESGRID_SLOT_FIFO_PATH_DL");
  if (p)
    return p;
  return resgrid_env_nonempty("CUDA_SLOT_FIFO_PATH_DL");
}

static const char *resgrid_fifo_path_ul_dual(void)
{
  const char *p = resgrid_env_nonempty("RESGRID_SLOT_FIFO_PATH_UL");
  if (p)
    return p;
  return resgrid_env_nonempty("CUDA_SLOT_FIFO_PATH_UL");
}

static void resgrid_fifo_drop_fd(gNB_MAC_INST *gNB, int *fd_store)
{
  if (*fd_store >= 0) {
    close(*fd_store);
    *fd_store = -1;
  }
}

static void resgrid_fifo_drop_dl(gNB_MAC_INST *gNB)
{
  resgrid_fifo_drop_fd(gNB, &gNB->resgrid_slot_fifo_fd_dl);
}

static void resgrid_fifo_drop_ul(gNB_MAC_INST *gNB)
{
  resgrid_fifo_drop_fd(gNB, &gNB->resgrid_slot_fifo_fd_ul);
}

static bool resgrid_ensure_fifo_open(gNB_MAC_INST *gNB, int *fd_store, const char *path)
{
  if (*fd_store >= 0)
    return true;
  if (!path)
    return false;
  int fd = open(path, O_WRONLY | O_NONBLOCK);
  if (fd < 0) {
    static uint32_t warn_ctr;
    if ((warn_ctr++ % 1000u) == 0u)
      LOG_W(NR_MAC, "resgrid slot FIFO open(%s): %s\n", path, strerror(errno));
    return false;
  }
  *fd_store = fd;
  LOG_I(NR_MAC, "resgrid slot FIFO writer opened %s (fd=%d)\n", path, fd);
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

/* Split large region lists so each write stays <= PIPE_BUF for pipe atomicity. */
static bool resgrid_emit_v2_chunks(gNB_MAC_INST *gNB,
                                   int *fd_store,
                                   const char *path,
                                   uint32_t slot_id,
                                   resgrid_region_t *regs,
                                   int n_regs)
{
  if (!resgrid_ensure_fifo_open(gNB, fd_store, path))
    return false;
  const int fd = *fd_store;
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
    bool ok = resgrid_write_all(fd, buf, blob);
    free(buf);
    if (!ok) {
      static uint32_t w;
      if ((w++ % 500u) == 0u)
        LOG_W(NR_MAC, "resgrid slot FIFO write failed: %s\n", strerror(errno));
      resgrid_fifo_drop_fd(gNB, fd_store);
      return false;
    }
    off += chunk;
  }
  return true;
}

static bool resgrid_emit_v1_empty(gNB_MAC_INST *gNB, int *fd_store, const char *path, uint32_t slot_id)
{
  if (!resgrid_ensure_fifo_open(gNB, fd_store, path))
    return false;
  const int fd = *fd_store;
  resgrid_SlotMsgHdr h;
  resgrid_pack_hdr_v1_empty(&h, slot_id);
  if (!resgrid_write_all(fd, &h, sizeof h)) {
    static uint32_t w;
    if ((w++ % 500u) == 0u)
      LOG_W(NR_MAC, "resgrid slot FIFO write (v1 empty) failed: %s\n", strerror(errno));
    resgrid_fifo_drop_fd(gNB, fd_store);
    return false;
  }
  return true;
}

static void resgrid_emit_regions_or_empty(gNB_MAC_INST *gNB,
                                          int *fd_store,
                                          const char *path,
                                          uint32_t slot_id,
                                          resgrid_region_t *regs,
                                          int n_regs)
{
  /* Reader distinguishes slot tick with no shapes vs payload present. */
  if (n_regs <= 0)
    (void)resgrid_emit_v1_empty(gNB, fd_store, path, slot_id);
  else
    (void)resgrid_emit_v2_chunks(gNB, fd_store, path, slot_id, regs, n_regs);
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

void nr_mac_resgrid_slot_fifo_cleanup(struct gNB_MAC_INST_s *gNB_in)
{
  gNB_MAC_INST *gNB = (gNB_MAC_INST *)gNB_in;
  resgrid_fifo_drop_dl(gNB);
  resgrid_fifo_drop_ul(gNB);
}

/*
 * Collect rectangles for this scheduling call, then apply TDD policy:
 *   FDD        -> emit DL list to DL FIFO and UL list to UL FIFO (counts may differ).
 *   TDD mixed  -> emit nothing (both n_*_fifo forced to 0); avoids ambiguous overlays.
 *   TDD DL/UL  -> only the matching-direction FIFO gets data for that slot.
 * emit_* gates increment slot_id only when that FIFO is written (see block comment top).
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
  const char *path_dl = resgrid_fifo_path_dl_dual();
  const char *path_ul = resgrid_fifo_path_ul_dual();
  if (!path_dl || !path_ul)
    return;

  NR_ServingCellConfigCommon_t *scc = gNB->common_channels[0].ServingCellConfigCommon;
  const int max_sym = NR_NUMBER_OF_SYMBOLS_PER_SLOT;
  const int max_prb = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->carrierBandwidth;

  const frame_structure_t *fs = &gNB->frame_structure;

  resgrid_region_t regs_dl[RESGRID_MAX_REGIONS];
  resgrid_region_t regs_ul[RESGRID_MAX_REGIONS];
  const int ndl = resgrid_collect_dl(gNB, dl, ul_dci, scc, max_prb, max_sym, regs_dl, RESGRID_MAX_REGIONS);
  const int nul = resgrid_collect_ul(gNB, ul, scc, max_prb, max_sym, regs_ul, RESGRID_MAX_REGIONS, 0);

  int n_dl_fifo = 0;
  int n_ul_fifo = 0;

  const bool tdd_mixed = (fs->frame_type != FDD) && is_mixed_slot(slot, fs);
  if (fs->frame_type == FDD) {
    n_dl_fifo = ndl;
    n_ul_fifo = nul;
  } else if (tdd_mixed) {
    n_dl_fifo = 0;
    n_ul_fifo = 0;
  } else {
    if (is_dl_slot(slot, fs))
      n_dl_fifo = ndl;
    if (is_ul_slot(slot, fs))
      n_ul_fifo = nul;
  }

  const bool emit_dl = (fs->frame_type == FDD) || (is_dl_slot(slot, fs) && !tdd_mixed);
  const bool emit_ul = (fs->frame_type == FDD) || (is_ul_slot(slot, fs) && !tdd_mixed);

  if (emit_dl) {
    gNB->resgrid_slot_seq_dl++;
    resgrid_emit_regions_or_empty(gNB, &gNB->resgrid_slot_fifo_fd_dl, path_dl, gNB->resgrid_slot_seq_dl, regs_dl, n_dl_fifo);
  }
  if (emit_ul) {
    gNB->resgrid_slot_seq_ul++;
    resgrid_emit_regions_or_empty(gNB, &gNB->resgrid_slot_fifo_fd_ul, path_ul, gNB->resgrid_slot_seq_ul, regs_ul, n_ul_fifo);
  }
}
