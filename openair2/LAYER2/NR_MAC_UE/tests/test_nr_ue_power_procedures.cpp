#include "gtest/gtest.h"
extern "C" {
#include "openair2/LAYER2/NR_MAC_UE/mac_proto.h"
}
#include <cstdio>
#include "common/utils/LOG/log.h"

TEST(power_procedures_fr1, test_prach_max_tx_power_mpr)
{
  // Inner PRB, MPR = 1.5
  int prb_start = 4;
  int N_RB_UL = 51; // 10Mhz
  EXPECT_EQ(22, nr_get_Pcmax(23, 20, FR1, 2, false, 1, N_RB_UL, false, 6, prb_start));

  // Outer PRB, MPR = 3
  prb_start = 0;
  EXPECT_EQ(21, nr_get_Pcmax(23, 20, FR1, 2, false, 1, N_RB_UL, false, 6, prb_start));

  // Channel bandwidth conditon not met, no MPR
  N_RB_UL = 106;
  EXPECT_EQ(23, nr_get_Pcmax(23, 20, FR1, 2, false, 1, N_RB_UL, false, 6, prb_start));
}

TEST(power_procedures_fr1, test_not_implemented)
{
  int N_RB_UL = 51;
  EXPECT_DEATH(nr_get_Pcmax(23, 20, FR1, 1, false, 1, N_RB_UL, false, 6, 0), "MPR for Pi/2 BPSK not implemented yet");
}

int main(int argc, char** argv)
{
  logInit();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
