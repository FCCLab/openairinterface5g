/*
 * Optional slot FIFO export for resgrid_plotter channel-based mode.
 * Wire format: resouce-grid/docs/CHANNEL_BASED_INTERFACE.md
 *
 * DL/UL FIFO env vars and TDD routing: see file comment in resgrid_plotter_slot_fifo.c.
 */
#ifndef RESGRID_PLOTTER_SLOT_FIFO_H
#define RESGRID_PLOTTER_SLOT_FIFO_H

#include "common/ngran_types.h"
#include "nfapi_nr_interface_scf.h"

struct gNB_MAC_INST_s;

/**
 * After the MAC scheduler builds the NFAPI slot picture, emit a serialized
 * description of physical channels (rectangles in PRB × symbol) to named FIFOs
 * so an external plotter can render the resource grid.
 *
 * ul_dci — UL grants on PDCCH (merged into DL FIFO regions as RESGRID_CHAN_PDCCH).
 * frame / slot — only used indirectly via gNB frame_structure for TDD direction;
 *                frame is currently unused (kept for API stability).
 *
 * No-op unless both DL and UL FIFO paths are set in the environment (see .c).
 */
void nr_mac_resgrid_emit_after_schedule(struct gNB_MAC_INST_s *gNB,
                                        const nfapi_nr_dl_tti_request_t *dl,
                                        const nfapi_nr_ul_tti_request_t *ul,
                                        const nfapi_nr_ul_dci_request_t *ul_dci,
                                        frame_t frame,
                                        slot_t slot);

/** Close FIFO fds on gNB (e.g. shutdown). Safe to call more than once. */
void nr_mac_resgrid_slot_fifo_cleanup(struct gNB_MAC_INST_s *gNB);

#endif /* RESGRID_PLOTTER_SLOT_FIFO_H */
