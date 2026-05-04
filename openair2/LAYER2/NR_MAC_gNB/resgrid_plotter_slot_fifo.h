/*
 * Optional slot FIFO export for resgrid_plotter channel-based mode.
 * Wire format: resouce-grid/docs/CHANNEL_BASED_INTERFACE.md
 */
#ifndef RESGRID_PLOTTER_SLOT_FIFO_H
#define RESGRID_PLOTTER_SLOT_FIFO_H

#include "common/ngran_types.h"
#include "nfapi_nr_interface_scf.h"

struct gNB_MAC_INST_s;

void nr_mac_resgrid_emit_after_schedule(struct gNB_MAC_INST_s *gNB,
                                        const nfapi_nr_dl_tti_request_t *dl,
                                        const nfapi_nr_ul_tti_request_t *ul,
                                        frame_t frame,
                                        slot_t slot);

void nr_mac_resgrid_slot_fifo_cleanup(struct gNB_MAC_INST_s *gNB);

#endif /* RESGRID_PLOTTER_SLOT_FIFO_H */
