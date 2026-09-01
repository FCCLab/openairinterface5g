#ifndef RAN_FUNC_RC_HELPER_H
#define RAN_FUNC_RC_HELPER_H

#include "openair2/E2AP/flexric/src/agent/../sm/sm_io.h"
#include "NR_MAC_gNB/gNB_scheduler_types.h"
#include "common/ran_context.h"
#include "openair2/LAYER2/NR_MAC_gNB/slice_prb_allocator/slice_prb_allocator_internal.h"
slice_config_t extract_slice_config_from_ctrl_req(e2sm_rc_ctrl_msg_frmt_1_t const* frmt_1);

#endif