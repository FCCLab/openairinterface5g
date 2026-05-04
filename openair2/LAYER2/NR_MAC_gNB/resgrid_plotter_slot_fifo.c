/*
 * Optional slot FIFO export for resgrid_plotter (channel-based / --data-mode 3).
 * Wire format: resouce-grid/docs/CHANNEL_BASED_INTERFACE.md
 *
 * Env (both required): RESGRID_SLOT_FIFO_PATH_DL and RESGRID_SLOT_FIFO_PATH_UL,
 * or CUDA_SLOT_FIFO_PATH_DL and CUDA_SLOT_FIFO_PATH_UL. If either side is unset,
 * slot export is disabled (no open). Routing: DL NFAPI regions go to the DL FIFO only
 * FDD: both FIFOs each tick (v1 empty if that direction has no NFAPI regions). TDD pure
 * DL: DL FIFO only (same-dir v1 empty if no regions); no UL FIFO write. TDD pure UL: UL
 * FIFO only; no DL FIFO write. TDD mixed: no write to either FIFO. slot_id increments
 * only on writes to that FIFO.
 */

#include <errno.h>
#include <fcntl.h>
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

#define RESGRID_CHAN_PBCH 1u
#define RESGRID_CHAN_PDCCH 2u
#define RESGRID_CHAN_PDSCH 3u
#define RESGRID_CHAN_PUSCH 4u
#define RESGRID_CHAN_PUCCH 5u
#define RESGRID_CHAN_PRACH 6u

/** Keep one Linux pipe write under PIPE_BUF (4096) for atomicity on common kernels. */
#define RESGRID_REGIONS_CHUNK 236
#define RESGRID_MAX_REGIONS 1536

#pragma pack(push, 1)
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

typedef struct {
  uint16_t sym_start;
  uint16_t sym_count;
  uint16_t prb_start;
  uint16_t prb_count;
  uint32_t rnti;
  uint8_t channel;
} resgrid_region_t;

static int resgrid_prach_symbol_duration(uint8_t prach_format)
{
  /* Same table as get_nr_prach_duration() in openair1/SCHED_NR/nr_prach_procedures.c */
  const int val[14] = {0, 0, 0, 0, 2, 4, 6, 2, 12, 2, 6, 2, 4, 6};
  if (prach_format >= sizeofArray(val))
    return 0;
  return val[prach_format];
}

static uint64_t resgrid_coreset_bitmap(const uint8_t freq[6])
{
  return (((uint64_t)freq[0]) << 37) | (((uint64_t)freq[1]) << 29) | (((uint64_t)freq[2]) << 21) | (((uint64_t)freq[3]) << 13)
         | (((uint64_t)freq[4]) << 5) | (((uint64_t)freq[5]) >> 3);
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

static void resgrid_pack_hdr_v2_multi(resgrid_SlotMsgHdr *h, uint32_t slot_id, uint32_t n_complex)
{
  memset(h, 0, sizeof(*h));
  h->magic = RESGRID_SLOT_MAGIC;
  h->version = RESGRID_SLOT_VER2;
  h->flags = RESGRID_SLOT_FLAG_MULTI;
  h->slot_id = slot_id;
  h->n_complex = n_complex;
}

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
  if (n_regs <= 0)
    (void)resgrid_emit_v1_empty(gNB, fd_store, path, slot_id);
  else
    (void)resgrid_emit_v2_chunks(gNB, fd_store, path, slot_id, regs, n_regs);
}

static int resgrid_collect_dl(gNB_MAC_INST *gNB,
                              const nfapi_nr_dl_tti_request_t *dl,
                              NR_ServingCellConfigCommon_t *scc,
                              int max_prb,
                              int max_sym,
                              resgrid_region_t *out,
                              int out_cap)
{
  int n = 0;
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
        uint32_t rnti = 0;
        if (p->numDlDci > 0)
          rnti = p->dci_pdu[0].RNTI;
        const uint64_t bitmap = resgrid_coreset_bitmap(p->FreqDomainResource);
        int j = 0;
        while (j < 45 && n < out_cap) {
          if (((bitmap >> (44 - j)) & 1u) == 0u) {
            j++;
            continue;
          }
          const int run0 = j;
          while (j < 45 && ((bitmap >> (44 - j)) & 1u) != 0u)
            j++;
          out[n].sym_start = p->StartSymbolIndex;
          out[n].sym_count = p->DurationSymbols;
          out[n].prb_start = p->BWPStart + (uint16_t)(6 * run0);
          out[n].prb_count = (uint16_t)(6 * (j - run0));
          out[n].rnti = rnti;
          out[n].channel = RESGRID_CHAN_PDCCH;
          resgrid_clamp_region(&out[n], max_prb, max_sym);
          if (out[n].sym_count > 0 && out[n].prb_count > 0)
            n++;
        }
      } break;

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
        out[n].channel = RESGRID_CHAN_PBCH;
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

void nr_mac_resgrid_emit_after_schedule(struct gNB_MAC_INST_s *gNB_in,
                                        const nfapi_nr_dl_tti_request_t *dl,
                                        const nfapi_nr_ul_tti_request_t *ul,
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
  const int ndl = resgrid_collect_dl(gNB, dl, scc, max_prb, max_sym, regs_dl, RESGRID_MAX_REGIONS);
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
