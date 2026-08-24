#include <stdio.h>
#include <math.h>
#include "openair2/E2AP/flexric/src/sm/rc_sm/enc/rc_enc_asn_xapp_lib.h"
#include "openair2/LAYER2/NR_MAC_gNB/slice_prb_allocator/slice_prb_allocator_internal.h"
#include "openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "openair2/E2AP/RAN_FUNCTION/O-RAN/ran_func_rc_helper.h"
void test_extract_slice_config()
{       
    const float epsilon = 0.00001f;

    int64_t dedicated_prb_input = 50;
    int64_t min_prb_input = 80;
    int64_t max_prb_input = 90;
    uint8_t sst_input = 0x1;
    uint32_t sd_input = 0xffffff;

    e2sm_rc_ctrl_msg_frmt_1_t msg = get_ctrl_msg_frmt_1(dedicated_prb_input, min_prb_input, max_prb_input, sst_input, sd_input);
    slice_config_t extracted_slice_config = extract_slice_config_from_ctrl_req(&msg);

    assert(fabs(extracted_slice_config.dedicated_prb_ratio - 0.5) < epsilon);
    assert(fabs(extracted_slice_config.min_prb_ratio - 0.8) < epsilon);
    assert(fabs(extracted_slice_config.max_prb_ratio - 0.9) < epsilon);
    assert(extracted_slice_config.slice_id.sd == sd_input);
    assert(extracted_slice_config.slice_id.sst == sst_input);
}

int main()
{
    test_extract_slice_config();
    printf("\nRC extract_slice_config launched\n");
    return 0;
}
