/*!
 * \file test_slice_prb_allocator.c
 * \brief Unit tests for Network Slice PRB Range Allocation Algorithm
 * 
 * Compile with:
 *   gcc -Wall -Wextra -std=c11 -O2 -g test_slice_prb_allocator.c slice_prb_allocator.c -o test_slice_prb_allocator -lm
 * 
 * Run with:
 *   ./test_slice_prb_allocator
 */

#include "slice_prb_allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#define ASSERT_EQ(a, b, msg) do { \
  if ((a) != (b)) { \
    fprintf(stderr, "FAIL: %s: expected %d, got %d\n", (msg), (b), (a)); \
    exit(1); \
  } \
} while(0)

#define ASSERT_GE(a, b, msg) do { \
  if ((a) < (b)) { \
    fprintf(stderr, "FAIL: %s: expected >= %d, got %d\n", (msg), (b), (a)); \
    exit(1); \
  } \
} while(0)

#define ASSERT_LE(a, b, msg) do { \
  if ((a) > (b)) { \
    fprintf(stderr, "FAIL: %s: expected <= %d, got %d\n", (msg), (b), (a)); \
    exit(1); \
  } \
} while(0)

#define ASSERT_GT(a, b, msg) do { \
  if ((a) <= (b)) { \
    fprintf(stderr, "FAIL: %s: expected > %d, got %d\n", (msg), (b), (a)); \
    exit(1); \
  } \
} while(0)

#define ASSERT_TRUE(cond, msg) do { \
  if (!(cond)) { \
    fprintf(stderr, "FAIL: %s\n", (msg)); \
    exit(1); \
  } \
} while(0)

static int tests_run = 0;
static int tests_passed = 0;

/* Helper function to print slice configuration */
static void print_slice_config(const slice_config_t *slice, int idx) {
  printf("    Slice %d (ID %d):\n", idx, slice->slice_id);
  printf("      Dedicated: %.1f%%, Min: %.1f%%, Max: %.1f%%, Active UEs: %s, Required PRBs: %d",
         slice->dedicated_prb_ratio * 100.0,
         slice->min_prb_ratio * 100.0,
         slice->max_prb_ratio * 100.0,
         slice->has_active_ues ? "Yes" : "No",
         slice->required_prbs);
  printf("\n");
}

/* Helper function to print allocation result with required PRBs */
static void print_allocation_result_with_required(const slice_alloc_result_t *result, 
                                                   const slice_alloc_input_t *input, 
                                                   int total_prbs) {
  printf("  Allocation Result:\n");
  printf("    Active Slices: %d\n", result->num_active_slices);
  printf("    Total Allocated: %d / %d PRBs\n", result->total_allocated_prbs, total_prbs);
  printf("    Per-Slice Allocation:\n");
  for (int s = 0; s < MAX_NUM_SLICES; ++s) {
    if (result->ranges[s].num_prbs > 0) {
      float percentage = (float)result->ranges[s].num_prbs / total_prbs * 100.0;
      // Find the corresponding slice config to get required_prbs
      int required_prbs = 0;
      for (int i = 0; i < input->num_slices; ++i) {
        if (input->slices[i].slice_id == result->ranges[s].slice_id) {
          required_prbs = input->slices[i].required_prbs;
          break;
        }
      }
      printf("      Slice %d: PRBs [%d, %d) = %d PRBs (%.1f%%)", 
             result->ranges[s].slice_id,
             result->ranges[s].start_prb,
             result->ranges[s].end_prb,
             result->ranges[s].num_prbs,
             percentage);
      if (required_prbs > 0) {
        printf(", Required: %d PRBs", required_prbs);
      }
      printf("\n");
    }
  }
}

#define TEST(name) \
  do { \
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"); \
    printf("Test %d: %s\n", tests_run + 1, #name); \
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"); \
    tests_run++; \
    test_##name(); \
    tests_passed++; \
    printf("  ✓ PASS\n\n"); \
  } while(0)

/* Test 1: Basic allocation with two slices */
static void test_basic_two_slices(void) {
  printf("  Purpose: Test basic two-slice allocation with different ratios\n");
  printf("  Expected: Both slices get their dedicated PRBs, then remaining PRBs are\n");
  printf("            distributed proportionally up to their maximum limits\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  // Slice 1: 30% dedicated, 30% min, 50% max
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.30f;
  input.slices[0].min_prb_ratio = 0.30f;
  input.slices[0].max_prb_ratio = 0.50f;
  input.slices[0].has_active_ues = true;
  
  // Slice 2: 20% dedicated, 20% min, 50% max
  input.slices[1].slice_id = 2;
  input.slices[1].dedicated_prb_ratio = 0.20f;
  input.slices[1].min_prb_ratio = 0.20f;
  input.slices[1].max_prb_ratio = 0.50f;
  input.slices[1].has_active_ues = true;
  
  input.num_slices = 2;
  input.total_prbs = 100;
  
  printf("  Input Configuration:\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Number of Slices: %d\n", input.num_slices);
  print_slice_config(&input.slices[0], 0);
  print_slice_config(&input.slices[1], 1);
  printf("\n");
  
  int ret = calculate_slice_prb_ranges(&input, &result);
  
  print_allocation_result_with_required(&result, &input, input.total_prbs);
  printf("\n");
  
  printf("  Verification:\n");
  ASSERT_EQ(ret, 2, "Should allocate to 2 slices");
  ASSERT_EQ(result.num_active_slices, 2, "Should have 2 active slices");
  ASSERT_EQ(result.total_allocated_prbs, 100, "Should allocate all 100 PRBs");
  
  // Check slice 1: should get at least 30 PRBs (dedicated), up to 50 PRBs (max)
  printf("    ✓ Slice 1: %d PRBs (expected: ≥30, ≤50)\n", result.ranges[0].num_prbs);
  ASSERT_GE(result.ranges[0].num_prbs, 30, "Slice 1 should get at least 30 PRBs");
  ASSERT_LE(result.ranges[0].num_prbs, 50, "Slice 1 should not exceed 50 PRBs");
  ASSERT_EQ(result.ranges[0].slice_id, 1, "Slice 1 ID should be 1");
  ASSERT_EQ(result.ranges[0].start_prb, 0, "Slice 1 should start at PRB 0");
  ASSERT_EQ(result.ranges[0].end_prb, result.ranges[0].start_prb + result.ranges[0].num_prbs,
            "Slice 1 end_prb should be start_prb + num_prbs");
  
  // Check slice 2: should get at least 20 PRBs (dedicated), up to 50 PRBs (max)
  printf("    ✓ Slice 2: %d PRBs (expected: ≥20, ≤50)\n", result.ranges[1].num_prbs);
  ASSERT_GE(result.ranges[1].num_prbs, 20, "Slice 2 should get at least 20 PRBs");
  ASSERT_LE(result.ranges[1].num_prbs, 50, "Slice 2 should not exceed 50 PRBs");
  ASSERT_EQ(result.ranges[1].slice_id, 2, "Slice 2 ID should be 2");
  ASSERT_EQ(result.ranges[1].start_prb, result.ranges[0].end_prb,
            "Slice 2 should start where slice 1 ends");
  ASSERT_EQ(result.ranges[1].end_prb, result.ranges[1].start_prb + result.ranges[1].num_prbs,
            "Slice 2 end_prb should be start_prb + num_prbs");
  
  // Check that ranges don't overlap
  ASSERT_EQ(result.ranges[0].end_prb, result.ranges[1].start_prb,
            "Slice ranges should be contiguous");
  
  // Total should equal 100
  ASSERT_EQ(result.ranges[0].num_prbs + result.ranges[1].num_prbs, 100,
            "Total PRBs should equal 100");
  printf("    ✓ Ranges are contiguous: [%d, %d) and [%d, %d)\n",
         result.ranges[0].start_prb, result.ranges[0].end_prb,
         result.ranges[1].start_prb, result.ranges[1].end_prb);
}

/* Test 2: Single slice with all PRBs */
static void test_single_slice_all_prbs(void) {
  printf("  Purpose: Test single slice that gets all available PRBs\n");
  printf("  Expected: Single slice gets 100%% of PRBs (106 PRBs)\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 1.0f;
  input.slices[0].min_prb_ratio = 1.0f;
  input.slices[0].max_prb_ratio = 1.0f;
  input.slices[0].has_active_ues = true;
  
  input.num_slices = 1;
  input.total_prbs = 106;
  
  printf("  Input Configuration:\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Number of Slices: %d\n", input.num_slices);
  print_slice_config(&input.slices[0], 0);
  printf("\n");
  
  int ret = calculate_slice_prb_ranges(&input, &result);
  
  print_allocation_result_with_required(&result, &input, input.total_prbs);
  printf("\n");
  
  printf("  Verification:\n");
  ASSERT_EQ(ret, 1, "Should allocate to 1 slice");
  ASSERT_EQ(result.ranges[0].num_prbs, 106, "Slice should get all 106 PRBs");
  ASSERT_EQ(result.ranges[0].start_prb, 0, "Should start at PRB 0");
  ASSERT_EQ(result.ranges[0].end_prb, 106, "Should end at PRB 106");
  ASSERT_EQ(result.total_allocated_prbs, 106, "Should allocate all PRBs");
  printf("    ✓ Slice gets all %d PRBs: [%d, %d)\n",
         result.ranges[0].num_prbs, result.ranges[0].start_prb, result.ranges[0].end_prb);
}

/* Test 3: Slice with no active UEs should get no PRBs */
static void test_no_active_ues(void) {
  printf("  Purpose: Test that slices without active UEs get no PRBs\n");
  printf("  Expected: Slice with has_active_ues=false gets 0 PRBs\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.5f;
  input.slices[0].min_prb_ratio = 0.5f;
  input.slices[0].max_prb_ratio = 0.5f;
  input.slices[0].has_active_ues = false;  // No active UEs
  
  input.num_slices = 1;
  input.total_prbs = 100;
  
  printf("  Input Configuration:\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Number of Slices: %d\n", input.num_slices);
  print_slice_config(&input.slices[0], 0);
  printf("    Note: Slice has no active UEs, so it should get 0 PRBs\n\n");
  
  int ret = calculate_slice_prb_ranges(&input, &result);
  
  print_allocation_result_with_required(&result, &input, input.total_prbs);
  printf("\n");
  
  printf("  Verification:\n");
  ASSERT_EQ(ret, 0, "Should allocate to 0 slices");
  ASSERT_EQ(result.ranges[0].num_prbs, 0, "Slice with no UEs should get 0 PRBs");
  ASSERT_EQ(result.total_allocated_prbs, 0, "Should allocate 0 PRBs");
  printf("    ✓ Slice with no active UEs correctly gets 0 PRBs\n");
}

/* Test 4: Multiple slices with different ratios */
static void test_multiple_slices_different_ratios(void) {
  printf("  Purpose: Test allocation with 3 slices having different dedicated/min/max ratios\n");
  printf("  Expected: Each slice gets at least its minimum, respects its maximum,\n");
  printf("            and all PRBs are allocated contiguously\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  // Slice 1: 10% dedicated, 20% min, 40% max
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.10f;
  input.slices[0].min_prb_ratio = 0.20f;
  input.slices[0].max_prb_ratio = 0.40f;
  input.slices[0].has_active_ues = true;
  
  // Slice 2: 15% dedicated, 25% min, 50% max
  input.slices[1].slice_id = 2;
  input.slices[1].dedicated_prb_ratio = 0.15f;
  input.slices[1].min_prb_ratio = 0.25f;
  input.slices[1].max_prb_ratio = 0.50f;
  input.slices[1].has_active_ues = true;
  
  // Slice 3: 5% dedicated, 10% min, 30% max
  input.slices[2].slice_id = 3;
  input.slices[2].dedicated_prb_ratio = 0.05f;
  input.slices[2].min_prb_ratio = 0.10f;
  input.slices[2].max_prb_ratio = 0.30f;
  input.slices[2].has_active_ues = true;
  
  input.num_slices = 3;
  input.total_prbs = 100;
  
  printf("  Input Configuration:\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Number of Slices: %d\n", input.num_slices);
  print_slice_config(&input.slices[0], 0);
  print_slice_config(&input.slices[1], 1);
  print_slice_config(&input.slices[2], 2);
  printf("\n");
  
  int ret = calculate_slice_prb_ranges(&input, &result);
  
  print_allocation_result_with_required(&result, &input, input.total_prbs);
  printf("\n");
  
  printf("  Verification:\n");
  ASSERT_EQ(ret, 3, "Should allocate to 3 slices");
  ASSERT_EQ(result.total_allocated_prbs, 100, "Should allocate all 100 PRBs");
  
  // Check each slice respects its constraints
  printf("    ✓ Slice 1: %d PRBs (expected: ≥20, ≤40)\n", result.ranges[0].num_prbs);
  ASSERT_GE(result.ranges[0].num_prbs, 20, "Slice 1 should get at least min (20 PRBs)");
  ASSERT_LE(result.ranges[0].num_prbs, 40, "Slice 1 should not exceed max (40 PRBs)");
  
  printf("    ✓ Slice 2: %d PRBs (expected: ≥25, ≤50)\n", result.ranges[1].num_prbs);
  ASSERT_GE(result.ranges[1].num_prbs, 25, "Slice 2 should get at least min (25 PRBs)");
  ASSERT_LE(result.ranges[1].num_prbs, 50, "Slice 2 should not exceed max (50 PRBs)");
  
  printf("    ✓ Slice 3: %d PRBs (expected: ≥10, ≤30)\n", result.ranges[2].num_prbs);
  ASSERT_GE(result.ranges[2].num_prbs, 10, "Slice 3 should get at least min (10 PRBs)");
  ASSERT_LE(result.ranges[2].num_prbs, 30, "Slice 3 should not exceed max (30 PRBs)");
  
  // Check contiguous allocation
  ASSERT_EQ(result.ranges[0].end_prb, result.ranges[1].start_prb,
            "Slice 1 and 2 should be contiguous");
  ASSERT_EQ(result.ranges[1].end_prb, result.ranges[2].start_prb,
            "Slice 2 and 3 should be contiguous");
  
  // Total should equal 100
  int total = result.ranges[0].num_prbs + result.ranges[1].num_prbs + result.ranges[2].num_prbs;
  ASSERT_EQ(total, 100, "Total PRBs should equal 100");
  printf("    ✓ Ranges are contiguous: [%d, %d), [%d, %d), [%d, %d)\n",
         result.ranges[0].start_prb, result.ranges[0].end_prb,
         result.ranges[1].start_prb, result.ranges[1].end_prb,
         result.ranges[2].start_prb, result.ranges[2].end_prb);
}

/* Test 5: Dedicated allocations exceeding total PRBs */
static void test_dedicated_exceeds_total(void) {
  printf("  Purpose: Test proportional scaling when dedicated allocations exceed 100%%\n");
  printf("  Expected: When total dedicated > 100%%, allocations are scaled down\n");
  printf("            proportionally (each slice gets 50%% in this case)\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  // Two slices, each wants 60% dedicated (total 120%)
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.60f;
  input.slices[0].min_prb_ratio = 0.60f;
  input.slices[0].max_prb_ratio = 0.60f;
  input.slices[0].has_active_ues = true;
  
  input.slices[1].slice_id = 2;
  input.slices[1].dedicated_prb_ratio = 0.60f;
  input.slices[1].min_prb_ratio = 0.60f;
  input.slices[1].max_prb_ratio = 0.60f;
  input.slices[1].has_active_ues = true;
  
  input.num_slices = 2;
  input.total_prbs = 100;
  
  printf("  Input Configuration:\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Number of Slices: %d\n", input.num_slices);
  print_slice_config(&input.slices[0], 0);
  print_slice_config(&input.slices[1], 1);
  printf("    Note: Total dedicated = 120%% > 100%%, so scaling is needed\n");
  printf("          Expected scale factor: 100/120 = 0.833\n");
  printf("          Each slice: 60%% × 0.833 = 50%% = 50 PRBs\n\n");
  
  int ret = calculate_slice_prb_ranges(&input, &result);
  
  print_allocation_result_with_required(&result, &input, input.total_prbs);
  printf("\n");
  
  printf("  Verification:\n");
  ASSERT_EQ(ret, 2, "Should allocate to 2 slices");
  ASSERT_EQ(result.total_allocated_prbs, 100, "Should allocate all 100 PRBs");
  
  // Should be scaled down proportionally (each gets 50 PRBs)
  printf("    ✓ Slice 1: %d PRBs (expected: 50 after scaling)\n", result.ranges[0].num_prbs);
  ASSERT_EQ(result.ranges[0].num_prbs, 50, "Slice 1 should get 50 PRBs (scaled)");
  printf("    ✓ Slice 2: %d PRBs (expected: 50 after scaling)\n", result.ranges[1].num_prbs);
  ASSERT_EQ(result.ranges[1].num_prbs, 50, "Slice 2 should get 50 PRBs (scaled)");
  printf("    ✓ Proportional scaling works correctly\n");
}

/* Test 6: Max ratio enforcement */
static void test_max_ratio_enforcement(void) {
  printf("  Purpose: Test that max_prb_ratio is enforced as a hard limit\n");
  printf("  Expected: Even with 90 PRBs remaining, slice cannot exceed 30%% (30 PRBs)\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  // Slice with 30% max, but plenty of remaining PRBs
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.10f;
  input.slices[0].min_prb_ratio = 0.10f;
  input.slices[0].max_prb_ratio = 0.30f;  // Hard limit at 30%
  input.slices[0].has_active_ues = true;
  
  input.num_slices = 1;
  input.total_prbs = 100;
  
  printf("  Input Configuration:\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Number of Slices: %d\n", input.num_slices);
  print_slice_config(&input.slices[0], 0);
  printf("    Note: After dedicated (10 PRBs), 90 PRBs remain, but max is 30%%\n");
  printf("          Slice should get exactly 30 PRBs (max limit)\n\n");
  
  int ret = calculate_slice_prb_ranges(&input, &result);
  
  print_allocation_result_with_required(&result, &input, input.total_prbs);
  printf("\n");
  
  printf("  Verification:\n");
  ASSERT_EQ(ret, 1, "Should allocate to 1 slice");
  printf("    ✓ Slice 1: %d PRBs (expected: ≤30, hard limit)\n", result.ranges[0].num_prbs);
  ASSERT_LE(result.ranges[0].num_prbs, 30, "Should not exceed max (30 PRBs)");
  // Even though there are 90 PRBs remaining, slice should only get up to 30
  printf("    ✓ Max ratio (30%%) is correctly enforced as hard limit\n");
}

/* Test 7: Validation tests */
static void test_validation(void) {
  printf("  Purpose: Test input validation and error handling\n");
  printf("  Expected: Invalid inputs return -1, valid inputs succeed\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  printf("  Testing NULL input...\n");
  int ret = calculate_slice_prb_ranges(NULL, &result);
  ASSERT_EQ(ret, -1, "NULL input should return -1");
  printf("    ✓ NULL input correctly rejected\n");
  
  printf("  Testing NULL result...\n");
  ret = calculate_slice_prb_ranges(&input, NULL);
  ASSERT_EQ(ret, -1, "NULL result should return -1");
  printf("    ✓ NULL result correctly rejected\n");
  
  printf("  Testing invalid ratio (> 1.0)...\n");
  // Test invalid ratios
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 1.5f;  // Invalid: > 1.0
  input.slices[0].min_prb_ratio = 0.5f;
  input.slices[0].max_prb_ratio = 0.5f;
  input.slices[0].has_active_ues = true;
  input.num_slices = 1;
  input.total_prbs = 100;
  
  ret = calculate_slice_prb_ranges(&input, &result);
  ASSERT_EQ(ret, -1, "Invalid ratio should return -1");
  printf("    ✓ Ratio > 1.0 correctly rejected\n");
  
  printf("  Testing invalid relationship (dedicated > min)...\n");
  // Test invalid relationship (dedicated > min)
  input.slices[0].dedicated_prb_ratio = 0.5f;
  input.slices[0].min_prb_ratio = 0.3f;  // Invalid: min < dedicated
  input.slices[0].max_prb_ratio = 0.5f;
  
  ret = calculate_slice_prb_ranges(&input, &result);
  ASSERT_EQ(ret, -1, "Invalid ratio relationship should return -1");
  printf("    ✓ Invalid ratio relationship (dedicated > min) correctly rejected\n");
}

/* Test 8: Edge case - zero total PRBs */
static void test_zero_total_prbs(void) {
  printf("  Purpose: Test edge case with zero total PRBs\n");
  printf("  Expected: Zero total PRBs should be rejected (return -1)\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.5f;
  input.slices[0].min_prb_ratio = 0.5f;
  input.slices[0].max_prb_ratio = 0.5f;
  input.slices[0].has_active_ues = true;
  input.num_slices = 1;
  input.total_prbs = 0;  // Invalid
  
  printf("  Input Configuration:\n");
  printf("    Total PRBs: %d (INVALID)\n", input.total_prbs);
  printf("    Number of Slices: %d\n", input.num_slices);
  print_slice_config(&input.slices[0], 0);
  printf("\n");
  
  int ret = calculate_slice_prb_ranges(&input, &result);
  
  printf("  Verification:\n");
  ASSERT_EQ(ret, -1, "Zero total PRBs should return -1");
  printf("    ✓ Zero total PRBs correctly rejected\n");
}

/* Test 9: Real-world scenario - 106 PRBs with 2 slices */
static void test_real_world_106_prbs(void) {
  printf("  Purpose: Test real-world 5G NR scenario with 106 PRBs\n");
  printf("  Expected: eMBB and URLLC slices get appropriate allocations\n");
  printf("            respecting their dedicated, min, and max ratios\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  // Slice 1: 33% dedicated, 33% min, 50% max (typical eMBB slice)
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.33f;
  input.slices[0].min_prb_ratio = 0.33f;
  input.slices[0].max_prb_ratio = 0.50f;
  input.slices[0].has_active_ues = true;
  
  // Slice 2: 20% dedicated, 20% min, 50% max (typical URLLC slice)
  input.slices[1].slice_id = 2;
  input.slices[1].dedicated_prb_ratio = 0.20f;
  input.slices[1].min_prb_ratio = 0.20f;
  input.slices[1].max_prb_ratio = 0.50f;
  input.slices[1].has_active_ues = true;
  
  input.num_slices = 2;
  input.total_prbs = 106;  // Typical 5G NR bandwidth
  
  printf("  Input Configuration:\n");
  printf("    Total PRBs: %d (typical 5G NR bandwidth)\n", input.total_prbs);
  printf("    Number of Slices: %d\n", input.num_slices);
  printf("    Slice 1 (eMBB - Enhanced Mobile Broadband):\n");
  print_slice_config(&input.slices[0], 0);
  printf("    Slice 2 (URLLC - Ultra-Reliable Low-Latency Communication):\n");
  print_slice_config(&input.slices[1], 1);
  printf("\n");
  
  int ret = calculate_slice_prb_ranges(&input, &result);
  
  print_allocation_result_with_required(&result, &input, input.total_prbs);
  printf("\n");
  
  printf("  Verification:\n");
  ASSERT_EQ(ret, 2, "Should allocate to 2 slices");
  ASSERT_EQ(result.total_allocated_prbs, 106, "Should allocate all 106 PRBs");
  
  // Slice 1: at least 35 PRBs (33% of 106), up to 53 PRBs (50% of 106)
  int slice1_min = (int)(106 * 0.33f + 0.5);
  int slice1_max = (int)(106 * 0.50f + 0.5);
  printf("    ✓ Slice 1 (eMBB): %d PRBs (expected: ≥%d, ≤%d)\n",
         result.ranges[0].num_prbs, slice1_min, slice1_max);
  ASSERT_GE(result.ranges[0].num_prbs, slice1_min, "Slice 1 should get at least min");
  ASSERT_LE(result.ranges[0].num_prbs, slice1_max, "Slice 1 should not exceed max");
  
  // Slice 2: at least 21 PRBs (20% of 106), up to 53 PRBs (50% of 106)
  int slice2_min = (int)(106 * 0.20f + 0.5);
  int slice2_max = (int)(106 * 0.50f + 0.5);
  printf("    ✓ Slice 2 (URLLC): %d PRBs (expected: ≥%d, ≤%d)\n",
         result.ranges[1].num_prbs, slice2_min, slice2_max);
  ASSERT_GE(result.ranges[1].num_prbs, slice2_min, "Slice 2 should get at least min");
  ASSERT_LE(result.ranges[1].num_prbs, slice2_max, "Slice 2 should not exceed max");
  
  // Check contiguous allocation
  ASSERT_EQ(result.ranges[0].end_prb, result.ranges[1].start_prb,
            "Slices should be contiguous");
  ASSERT_EQ(result.ranges[1].end_prb, 106, "Last slice should end at total PRBs");
  printf("    ✓ Ranges are contiguous: [%d, %d) and [%d, %d)\n",
         result.ranges[0].start_prb, result.ranges[0].end_prb,
         result.ranges[1].start_prb, result.ranges[1].end_prb);
}

/* Test Pass 1: Dedicated PRB Allocation */
static void test_pass1_dedicated(void) {
  printf("  Purpose: Test Pass 1 (dedicated allocation) in isolation\n");
  printf("  Expected: Allocates dedicated PRBs, handles scaling when exceeds total\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  // Test case 1: Normal dedicated allocation
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.30f;
  input.slices[0].min_prb_ratio = 0.30f;
  input.slices[0].max_prb_ratio = 0.50f;
  input.slices[0].has_active_ues = true;
  
  input.slices[1].slice_id = 2;
  input.slices[1].dedicated_prb_ratio = 0.20f;
  input.slices[1].min_prb_ratio = 0.20f;
  input.slices[1].max_prb_ratio = 0.50f;
  input.slices[1].has_active_ues = true;
  
  input.num_slices = 2;
  input.total_prbs = 100;
  
  // Initialize result
  memset(&result, 0, sizeof(result));
  for (int s = 0; s < input.num_slices; ++s) {
    result.ranges[s].slice_id = input.slices[s].slice_id;
  }
  
  int allocated_prbs = 0;
  int num_active_slices = 0;
  
  printf("  Input Configuration:\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  print_slice_config(&input.slices[0], 0);
  print_slice_config(&input.slices[1], 1);
  printf("\n");
  
  int ret = pass1_allocate_dedicated(&input, &result, &allocated_prbs, &num_active_slices);
  
  printf("  Pass 1 Result:\n");
  printf("    Allocated PRBs: %d\n", allocated_prbs);
  printf("    Active Slices: %d\n", num_active_slices);
  printf("    Slice 1: %d PRBs (expected: 30)\n", result.ranges[0].num_prbs);
  printf("    Slice 2: %d PRBs (expected: 20)\n", result.ranges[1].num_prbs);
  printf("\n");
  
  printf("  Verification:\n");
  ASSERT_EQ(ret, 0, "Pass 1 should succeed");
  ASSERT_EQ(num_active_slices, 2, "Should have 2 active slices");
  ASSERT_EQ(result.ranges[0].num_prbs, 30, "Slice 1 should get 30 PRBs");
  ASSERT_EQ(result.ranges[1].num_prbs, 20, "Slice 2 should get 20 PRBs");
  ASSERT_EQ(allocated_prbs, 50, "Total allocated should be 50 PRBs");
  
  // Test case 2: Dedicated exceeds total (scaling)
  input.slices[0].dedicated_prb_ratio = 0.60f;
  input.slices[1].dedicated_prb_ratio = 0.60f;
  
  memset(&result, 0, sizeof(result));
  for (int s = 0; s < input.num_slices; ++s) {
    result.ranges[s].slice_id = input.slices[s].slice_id;
  }
  allocated_prbs = 0;
  num_active_slices = 0;
  
  printf("  Test Case 2: Dedicated exceeds total (60%% + 60%% = 120%%)\n");
  ret = pass1_allocate_dedicated(&input, &result, &allocated_prbs, &num_active_slices);
  
  printf("  Pass 1 Result (with scaling):\n");
  printf("    Allocated PRBs: %d (expected: 100 after scaling)\n", allocated_prbs);
  printf("    Slice 1: %d PRBs (expected: 50 after scaling)\n", result.ranges[0].num_prbs);
  printf("    Slice 2: %d PRBs (expected: 50 after scaling)\n", result.ranges[1].num_prbs);
  printf("\n");
  
  ASSERT_EQ(ret, 0, "Pass 1 should succeed");
  ASSERT_EQ(allocated_prbs, 100, "After scaling, should allocate exactly 100 PRBs");
  ASSERT_EQ(result.ranges[0].num_prbs, 50, "Slice 1 should get 50 PRBs after scaling");
  ASSERT_EQ(result.ranges[1].num_prbs, 50, "Slice 2 should get 50 PRBs after scaling");
}

/* Test Pass 2: Prioritized Resource Allocation */
static void test_pass2_prioritized(void) {
  printf("  Purpose: Test Pass 2 (prioritized allocation) in isolation\n");
  printf("  Expected: Allocates prioritized resources based on required_prbs\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  // Setup: Pass 1 already allocated dedicated PRBs
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.20f;
  input.slices[0].min_prb_ratio = 0.30f;
  input.slices[0].max_prb_ratio = 0.50f;
  input.slices[0].has_active_ues = true;
  input.slices[0].required_prbs = 35; // Needs more than dedicated
  
  input.slices[1].slice_id = 2;
  input.slices[1].dedicated_prb_ratio = 0.10f;
  input.slices[1].min_prb_ratio = 0.25f;
  input.slices[1].max_prb_ratio = 0.50f;
  input.slices[1].has_active_ues = true;
  input.slices[1].required_prbs = 30; // Needs more than dedicated
  
  input.num_slices = 2;
  input.total_prbs = 100;
  
  // Initialize result with Pass 1 allocations
  memset(&result, 0, sizeof(result));
  for (int s = 0; s < input.num_slices; ++s) {
    result.ranges[s].slice_id = input.slices[s].slice_id;
  }
  result.ranges[0].num_prbs = 20; // Dedicated from Pass 1
  result.ranges[1].num_prbs = 10; // Dedicated from Pass 1
  
  int allocated_prbs = 30; // From Pass 1
  int remaining_prbs = 70; // 100 - 30
  
  printf("  Input Configuration (after Pass 1):\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Already allocated: %d PRBs (dedicated)\n", allocated_prbs);
  printf("    Remaining: %d PRBs\n", remaining_prbs);
  print_slice_config(&input.slices[0], 0);
  print_slice_config(&input.slices[1], 1);
  printf("\n");
  
  int ret = pass2_allocate_prioritized(&input, &result, &allocated_prbs, &remaining_prbs);
  
  printf("  Pass 2 Result:\n");
  printf("    Allocated PRBs: %d (was %d, added %d)\n", allocated_prbs, 30, allocated_prbs - 30);
  printf("    Remaining PRBs: %d\n", remaining_prbs);
  printf("    Slice 1: %d PRBs (was 20, added %d, prioritized=10)\n",
         result.ranges[0].num_prbs, result.ranges[0].num_prbs - 20);
  printf("    Slice 2: %d PRBs (was 10, added %d, prioritized=15)\n",
         result.ranges[1].num_prbs, result.ranges[1].num_prbs - 10);
  printf("\n");
  
  printf("  Verification:\n");
  ASSERT_EQ(ret, 0, "Pass 2 should succeed");
  ASSERT_EQ(result.ranges[0].num_prbs, 30, "Slice 1 should get 30 PRBs (20 dedicated + 10 prioritized)");
  ASSERT_EQ(result.ranges[1].num_prbs, 25, "Slice 2 should get 25 PRBs (10 dedicated + 15 prioritized)");
  ASSERT_EQ(allocated_prbs, 55, "Total allocated should be 55 PRBs");
  ASSERT_EQ(remaining_prbs, 45, "Remaining should be 45 PRBs");
}

/* Test Pass 3: Shared Resource Allocation */
static void test_pass3_shared(void) {
  printf("  Purpose: Test Pass 3 (shared allocation) in isolation\n");
  printf("  Expected: Distributes shared resources proportionally\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  // Setup: Pass 1 and 2 already allocated
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.20f;
  input.slices[0].min_prb_ratio = 0.30f;
  input.slices[0].max_prb_ratio = 0.50f;
  input.slices[0].has_active_ues = true;
  input.slices[0].required_prbs = 45;
  
  input.slices[1].slice_id = 2;
  input.slices[1].dedicated_prb_ratio = 0.10f;
  input.slices[1].min_prb_ratio = 0.25f;
  input.slices[1].max_prb_ratio = 0.50f;
  input.slices[1].has_active_ues = true;
  input.slices[1].required_prbs = 35;
  
  input.num_slices = 2;
  input.total_prbs = 100;
  
  // Initialize result with Pass 1+2 allocations
  memset(&result, 0, sizeof(result));
  for (int s = 0; s < input.num_slices; ++s) {
    result.ranges[s].slice_id = input.slices[s].slice_id;
  }
  result.ranges[0].num_prbs = 30; // After Pass 1+2
  result.ranges[1].num_prbs = 25; // After Pass 1+2
  
  int allocated_prbs = 55; // From Pass 1+2
  int remaining_prbs = 45; // Shared resources
  
  printf("  Input Configuration (after Pass 1+2):\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Already allocated: %d PRBs\n", allocated_prbs);
  printf("    Remaining (shared): %d PRBs\n", remaining_prbs);
  printf("    Slice 1: %d PRBs, max=50, can_add=%d, required=%d\n",
         result.ranges[0].num_prbs, 50 - result.ranges[0].num_prbs, input.slices[0].required_prbs);
  printf("    Slice 2: %d PRBs, max=50, can_add=%d, required=%d\n",
         result.ranges[1].num_prbs, 50 - result.ranges[1].num_prbs, input.slices[1].required_prbs);
  printf("\n");
  
  int ret = pass3_allocate_shared(&input, &result, &allocated_prbs, &remaining_prbs);
  
  printf("  Pass 3 Result:\n");
  printf("    Allocated PRBs: %d (was %d, added %d)\n", allocated_prbs, 55, allocated_prbs - 55);
  printf("    Remaining PRBs: %d\n", remaining_prbs);
  printf("    Slice 1: %d PRBs (was 30, added %d)\n",
         result.ranges[0].num_prbs, result.ranges[0].num_prbs - 30);
  printf("    Slice 2: %d PRBs (was 25, added %d)\n",
         result.ranges[1].num_prbs, result.ranges[1].num_prbs - 25);
  printf("\n");
  
  printf("  Verification:\n");
  ASSERT_EQ(ret, 0, "Pass 3 should succeed");
  ASSERT_GE(result.ranges[0].num_prbs, 30, "Slice 1 should get at least 30 PRBs");
  ASSERT_LE(result.ranges[0].num_prbs, 50, "Slice 1 should not exceed max (50)");
  ASSERT_GE(result.ranges[1].num_prbs, 25, "Slice 2 should get at least 25 PRBs");
  ASSERT_LE(result.ranges[1].num_prbs, 50, "Slice 2 should not exceed max (50)");
  ASSERT_EQ(allocated_prbs + remaining_prbs, 100, "Allocated + remaining should equal total");
}

/* Test Pass 4: Range Assignment */
static void test_pass4_ranges(void) {
  printf("  Purpose: Test Pass 4 (range assignment) in isolation\n");
  printf("  Expected: Assigns contiguous, non-overlapping ranges\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.slices[1].slice_id = 2;
  input.slices[2].slice_id = 3;
  input.num_slices = 3;
  
  // Setup: PRBs already allocated (from previous passes)
  result.ranges[0].num_prbs = 30;
  result.ranges[1].num_prbs = 20;
  result.ranges[2].num_prbs = 50;
  // Slice 0 has no PRBs (should be skipped)
  
  printf("  Input Configuration:\n");
  printf("    Slice 1: %d PRBs\n", result.ranges[0].num_prbs);
  printf("    Slice 2: %d PRBs\n", result.ranges[1].num_prbs);
  printf("    Slice 3: %d PRBs\n", result.ranges[2].num_prbs);
  printf("    Total: %d PRBs\n", 30 + 20 + 50);
  printf("\n");
  
  int ret = pass4_assign_ranges(&input, &result);
  
  printf("  Pass 4 Result:\n");
  printf("    Slice 1: [%d, %d) = %d PRBs\n",
         result.ranges[0].start_prb, result.ranges[0].end_prb, result.ranges[0].num_prbs);
  printf("    Slice 2: [%d, %d) = %d PRBs\n",
         result.ranges[1].start_prb, result.ranges[1].end_prb, result.ranges[1].num_prbs);
  printf("    Slice 3: [%d, %d) = %d PRBs\n",
         result.ranges[2].start_prb, result.ranges[2].end_prb, result.ranges[2].num_prbs);
  printf("\n");
  
  printf("  Verification:\n");
  ASSERT_EQ(ret, 0, "Pass 4 should succeed");
  ASSERT_EQ(result.ranges[0].start_prb, 0, "Slice 1 should start at 0");
  ASSERT_EQ(result.ranges[0].end_prb, 30, "Slice 1 should end at 30");
  ASSERT_EQ(result.ranges[1].start_prb, 30, "Slice 2 should start at 30 (contiguous)");
  ASSERT_EQ(result.ranges[1].end_prb, 50, "Slice 2 should end at 50");
  ASSERT_EQ(result.ranges[2].start_prb, 50, "Slice 3 should start at 50 (contiguous)");
  ASSERT_EQ(result.ranges[2].end_prb, 100, "Slice 3 should end at 100");
  ASSERT_EQ(result.ranges[0].end_prb, result.ranges[1].start_prb,
            "Slices should be contiguous");
  ASSERT_EQ(result.ranges[1].end_prb, result.ranges[2].start_prb,
            "Slices should be contiguous");
}

/* Test Pass 1 Edge Cases */
static void test_pass1_dedicated_static_allocation(void) {
  printf("  Purpose: Test Pass 1 with dedicated = min = max (static allocation)\n");
  printf("  Expected: All PRBs allocated in Pass 1, no scaling needed\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.50f;
  input.slices[0].min_prb_ratio = 0.50f;
  input.slices[0].max_prb_ratio = 0.50f;
  input.slices[0].has_active_ues = true;
  
  input.slices[1].slice_id = 2;
  input.slices[1].dedicated_prb_ratio = 0.50f;
  input.slices[1].min_prb_ratio = 0.50f;
  input.slices[1].max_prb_ratio = 0.50f;
  input.slices[1].has_active_ues = true;
  
  input.num_slices = 2;
  input.total_prbs = 100;
  
  printf("  Input Configuration:\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Number of Slices: %d\n", input.num_slices);
  print_slice_config(&input.slices[0], 0);
  print_slice_config(&input.slices[1], 1);
  printf("\n");
  
  memset(&result, 0, sizeof(result));
  for (int s = 0; s < input.num_slices; ++s) {
    result.ranges[s].slice_id = input.slices[s].slice_id;
  }
  
  int allocated_prbs = 0;
  int num_active_slices = 0;
  
  int ret = pass1_allocate_dedicated(&input, &result, &allocated_prbs, &num_active_slices);
  
  printf("  Pass 1 Result:\n");
  printf("    Allocated PRBs: %d\n", allocated_prbs);
  printf("    Active Slices: %d\n", num_active_slices);
  printf("    Slice 1: %d PRBs\n", result.ranges[0].num_prbs);
  printf("    Slice 2: %d PRBs\n", result.ranges[1].num_prbs);
  printf("\n");
  
  ASSERT_EQ(ret, 0, "Should succeed");
  ASSERT_EQ(allocated_prbs, 100, "Should allocate all PRBs");
  ASSERT_EQ(result.ranges[0].num_prbs, 50, "Slice 1 should get 50 PRBs");
  ASSERT_EQ(result.ranges[1].num_prbs, 50, "Slice 2 should get 50 PRBs");
}

static void test_pass1_max_less_than_dedicated(void) {
  printf("  Purpose: Test Pass 1 when max < dedicated (should cap at max)\n");
  printf("  Expected: Dedicated is capped at max_prb_ratio\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.50f;
  input.slices[0].min_prb_ratio = 0.50f;
  input.slices[0].max_prb_ratio = 0.30f; // Max < dedicated
  input.slices[0].has_active_ues = true;
  
  input.num_slices = 1;
  input.total_prbs = 100;
  
  printf("  Input Configuration:\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Number of Slices: %d\n", input.num_slices);
  print_slice_config(&input.slices[0], 0);
  printf("    Note: Max (30%%) < Dedicated (50%%), so dedicated will be capped at max\n\n");
  
  memset(&result, 0, sizeof(result));
  result.ranges[0].slice_id = input.slices[0].slice_id;
  
  int allocated_prbs = 0;
  int num_active_slices = 0;
  
  int ret = pass1_allocate_dedicated(&input, &result, &allocated_prbs, &num_active_slices);
  
  printf("  Pass 1 Result:\n");
  printf("    Allocated PRBs: %d (capped at max=30)\n", allocated_prbs);
  printf("    Active Slices: %d\n", num_active_slices);
  printf("    Slice 1: %d PRBs (capped at max, dedicated was 50)\n", result.ranges[0].num_prbs);
  printf("\n");
  
  ASSERT_EQ(ret, 0, "Should succeed");
  ASSERT_EQ(result.ranges[0].num_prbs, 30, "Should be capped at max (30)");
  ASSERT_EQ(allocated_prbs, 30, "Total should be 30");
}

static void test_pass1_zero_dedicated(void) {
  printf("  Purpose: Test Pass 1 with zero dedicated (slice still active)\n");
  printf("  Expected: Slice gets 0 PRBs but is counted as active\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.0f;
  input.slices[0].min_prb_ratio = 0.30f;
  input.slices[0].max_prb_ratio = 0.50f;
  input.slices[0].has_active_ues = true;
  
  input.num_slices = 1;
  input.total_prbs = 100;
  
  printf("  Input Configuration:\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Number of Slices: %d\n", input.num_slices);
  print_slice_config(&input.slices[0], 0);
  printf("    Note: Dedicated is 0%%, but slice has active UEs, so it's counted as active\n\n");
  
  memset(&result, 0, sizeof(result));
  result.ranges[0].slice_id = input.slices[0].slice_id;
  
  int allocated_prbs = 0;
  int num_active_slices = 0;
  
  int ret = pass1_allocate_dedicated(&input, &result, &allocated_prbs, &num_active_slices);
  
  printf("  Pass 1 Result:\n");
  printf("    Allocated PRBs: %d\n", allocated_prbs);
  printf("    Active Slices: %d\n", num_active_slices);
  printf("    Slice 1: %d PRBs (zero dedicated)\n", result.ranges[0].num_prbs);
  printf("\n");
  
  ASSERT_EQ(ret, 0, "Should succeed");
  ASSERT_EQ(result.ranges[0].num_prbs, 0, "Should get 0 PRBs (zero dedicated)");
  ASSERT_EQ(allocated_prbs, 0, "Total should be 0");
  ASSERT_EQ(num_active_slices, 1, "Should be counted as active");
}

/* Test Pass 2 Edge Cases */
static void test_pass2_slice_doesnt_need_prioritized(void) {
  printf("  Purpose: Test Pass 2 when slice doesn't need prioritized resources\n");
  printf("  Expected: Prioritized resources remain available for Pass 3\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.20f;
  input.slices[0].min_prb_ratio = 0.30f;
  input.slices[0].max_prb_ratio = 0.50f;
  input.slices[0].has_active_ues = true;
  input.slices[0].required_prbs = 15; // Less than current (20)
  
  input.slices[1].slice_id = 2;
  input.slices[1].dedicated_prb_ratio = 0.10f;
  input.slices[1].min_prb_ratio = 0.25f;
  input.slices[1].max_prb_ratio = 0.50f;
  input.slices[1].has_active_ues = true;
  input.slices[1].required_prbs = 30; // Needs more
  
  input.num_slices = 2;
  input.total_prbs = 100;
  
  printf("  Input Configuration (after Pass 1):\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Already allocated (dedicated): 30 PRBs\n");
  printf("    Remaining: 70 PRBs\n");
  print_slice_config(&input.slices[0], 0);
  print_slice_config(&input.slices[1], 1);
  printf("    Note: Slice 1 required=15 < current=20, so doesn't need prioritized\n");
  printf("          Slice 2 required=30 > current=10, so needs prioritized\n\n");
  
  memset(&result, 0, sizeof(result));
  for (int s = 0; s < input.num_slices; ++s) {
    result.ranges[s].slice_id = input.slices[s].slice_id;
  }
  result.ranges[0].num_prbs = 20; // From Pass 1
  result.ranges[1].num_prbs = 10; // From Pass 1
  
  int allocated_prbs = 30;
  int remaining_prbs = 70;
  
  int ret = pass2_allocate_prioritized(&input, &result, &allocated_prbs, &remaining_prbs);
  
  printf("  Pass 2 Result:\n");
  printf("    Allocated PRBs: %d (was 30, added %d)\n", allocated_prbs, allocated_prbs - 30);
  printf("    Remaining PRBs: %d\n", remaining_prbs);
  printf("    Slice 1: %d PRBs (no change, doesn't need prioritized)\n", result.ranges[0].num_prbs);
  printf("    Slice 2: %d PRBs (was 10, added %d prioritized)\n",
         result.ranges[1].num_prbs, result.ranges[1].num_prbs - 10);
  printf("    Note: Slice 1's prioritized resources (10 PRBs) remain available for Pass 3\n");
  printf("\n");
  
  ASSERT_EQ(ret, 0, "Should succeed");
  ASSERT_EQ(result.ranges[0].num_prbs, 20, "Slice 1 should not get prioritized (doesn't need)");
  ASSERT_EQ(result.ranges[1].num_prbs, 25, "Slice 2 should get prioritized (15 PRBs)");
  // Slice 1's prioritized resources (10 PRBs) remain available, so remaining = 70 - 15 = 55
  // But actually, Slice 1's prioritized (10) + remaining after Pass 2 = 10 + 45 = 55
  // The prioritized resources that weren't claimed become available for Pass 3
  ASSERT_GE(remaining_prbs, 55, "At least 55 PRBs should remain (Slice 1's prioritized not claimed)");
}

static void test_pass2_insufficient_prioritized(void) {
  printf("  Purpose: Test Pass 2 with insufficient prioritized resources\n");
  printf("  Expected: Proportional scaling of prioritized allocations\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.20f;
  input.slices[0].min_prb_ratio = 0.50f; // Needs 30 prioritized
  input.slices[0].max_prb_ratio = 0.60f;
  input.slices[0].has_active_ues = true;
  input.slices[0].required_prbs = 50;
  
  input.slices[1].slice_id = 2;
  input.slices[1].dedicated_prb_ratio = 0.10f;
  input.slices[1].min_prb_ratio = 0.40f; // Needs 30 prioritized
  input.slices[1].max_prb_ratio = 0.60f;
  input.slices[1].has_active_ues = true;
  input.slices[1].required_prbs = 40;
  
  input.num_slices = 2;
  input.total_prbs = 100;
  
  printf("  Input Configuration (after Pass 1):\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Already allocated (dedicated): 30 PRBs\n");
  printf("    Remaining: 40 PRBs (insufficient for both slices' prioritized needs)\n");
  print_slice_config(&input.slices[0], 0);
  print_slice_config(&input.slices[1], 1);
  printf("    Note: Slice 1 needs 30 prioritized, Slice 2 needs 30 prioritized\n");
  printf("          Total need: 60 PRBs, but only 40 available (proportional scaling)\n\n");
  
  memset(&result, 0, sizeof(result));
  for (int s = 0; s < input.num_slices; ++s) {
    result.ranges[s].slice_id = input.slices[s].slice_id;
  }
  result.ranges[0].num_prbs = 20; // From Pass 1
  result.ranges[1].num_prbs = 10; // From Pass 1
  
  int allocated_prbs = 30;
  int remaining_prbs = 40; // Not enough for both (need 60 total)
  
  int ret = pass2_allocate_prioritized(&input, &result, &allocated_prbs, &remaining_prbs);
  
  printf("  Pass 2 Result (with proportional scaling):\n");
  printf("    Allocated PRBs: %d (was 30, added %d)\n", allocated_prbs, allocated_prbs - 30);
  printf("    Remaining PRBs: %d\n", remaining_prbs);
  printf("    Slice 1: %d PRBs (was 20, added %d)\n",
         result.ranges[0].num_prbs, result.ranges[0].num_prbs - 20);
  printf("    Slice 2: %d PRBs (was 10, added %d)\n",
         result.ranges[1].num_prbs, result.ranges[1].num_prbs - 10);
  printf("    Scale factor: 40/60 = 0.667\n");
  printf("\n");
  
  ASSERT_EQ(ret, 0, "Should succeed");
  ASSERT_EQ(allocated_prbs, 70, "Should allocate all remaining (40)");
  ASSERT_EQ(remaining_prbs, 0, "No PRBs should remain");
  // Proportional: 30/60 * 40 = 20 for slice 1, 30/60 * 40 = 20 for slice 2
  ASSERT_EQ(result.ranges[0].num_prbs, 40, "Slice 1 should get 40 (20 + 20)");
  ASSERT_EQ(result.ranges[1].num_prbs, 30, "Slice 2 should get 30 (10 + 20)");
}

static void test_pass2_max_less_than_min(void) {
  printf("  Purpose: Test Pass 2 when max < min (max takes precedence)\n");
  printf("  Expected: Allocation capped at max, min not fully met\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.20f;
  input.slices[0].min_prb_ratio = 0.50f; // Min = 50
  input.slices[0].max_prb_ratio = 0.30f; // Max = 30 (< min!)
  input.slices[0].has_active_ues = true;
  input.slices[0].required_prbs = 50;
  
  input.num_slices = 1;
  input.total_prbs = 100;
  
  printf("  Input Configuration (after Pass 1):\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Already allocated (dedicated): 20 PRBs\n");
  printf("    Remaining: 80 PRBs\n");
  print_slice_config(&input.slices[0], 0);
  printf("    Note: Max (30%%) < Min (50%%), so max takes precedence\n");
  printf("          Slice needs prioritized to reach min=50, but max=30 caps allocation\n\n");
  
  memset(&result, 0, sizeof(result));
  result.ranges[0].slice_id = input.slices[0].slice_id;
  result.ranges[0].num_prbs = 20; // From Pass 1
  
  int allocated_prbs = 20;
  int remaining_prbs = 80;
  
  int ret = pass2_allocate_prioritized(&input, &result, &allocated_prbs, &remaining_prbs);
  
  printf("  Pass 2 Result:\n");
  printf("    Allocated PRBs: %d (was 20, added %d)\n", allocated_prbs, allocated_prbs - 20);
  printf("    Remaining PRBs: %d\n", remaining_prbs);
  printf("    Slice 1: %d PRBs (capped at max=30, min=50 not met)\n", result.ranges[0].num_prbs);
  printf("\n");
  
  ASSERT_EQ(ret, 0, "Should succeed");
  ASSERT_EQ(result.ranges[0].num_prbs, 30, "Should be capped at max (30)");
  ASSERT_EQ(allocated_prbs, 30, "Total should be 30");
}

/* Test Pass 3 Edge Cases */
static void test_pass3_no_prb_requirements(void) {
  printf("  Purpose: Test Pass 3 without PRB requirements (capacity-based)\n");
  printf("  Expected: Proportional distribution based on remaining capacity\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.30f;
  input.slices[0].min_prb_ratio = 0.30f;
  input.slices[0].max_prb_ratio = 0.50f;
  input.slices[0].has_active_ues = true;
  input.slices[0].required_prbs = 0; // No requirement
  
  input.slices[1].slice_id = 2;
  input.slices[1].dedicated_prb_ratio = 0.20f;
  input.slices[1].min_prb_ratio = 0.20f;
  input.slices[1].max_prb_ratio = 0.50f;
  input.slices[1].has_active_ues = true;
  input.slices[1].required_prbs = 0; // No requirement
  
  input.num_slices = 2;
  input.total_prbs = 100;
  
  printf("  Input Configuration (after Pass 1+2):\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Already allocated: 50 PRBs\n");
  printf("    Remaining (shared): 50 PRBs\n");
  print_slice_config(&input.slices[0], 0);
  print_slice_config(&input.slices[1], 1);
  printf("    Note: No PRB requirements (required=0), so capacity-based distribution\n");
  printf("          Slice 1: can_add=20 (max=50 - current=30)\n");
  printf("          Slice 2: can_add=30 (max=50 - current=20)\n\n");
  
  memset(&result, 0, sizeof(result));
  for (int s = 0; s < input.num_slices; ++s) {
    result.ranges[s].slice_id = input.slices[s].slice_id;
  }
  result.ranges[0].num_prbs = 30; // After Pass 1+2
  result.ranges[1].num_prbs = 20; // After Pass 1+2
  
  int allocated_prbs = 50;
  int remaining_prbs = 50; // Shared resources
  
  int ret = pass3_allocate_shared(&input, &result, &allocated_prbs, &remaining_prbs);
  
  printf("  Pass 3 Result (capacity-based):\n");
  printf("    Allocated PRBs: %d (was 50, added %d)\n", allocated_prbs, allocated_prbs - 50);
  printf("    Remaining PRBs: %d\n", remaining_prbs);
  printf("    Slice 1: %d PRBs (was 30, added 20)\n", result.ranges[0].num_prbs);
  printf("    Slice 2: %d PRBs (was 20, added 30)\n", result.ranges[1].num_prbs);
  printf("    Distribution: (20/50) * 50 = 20 for slice 1, (30/50) * 50 = 30 for slice 2\n");
  printf("\n");
  
  ASSERT_EQ(ret, 0, "Should succeed");
  ASSERT_EQ(result.ranges[0].num_prbs, 50, "Slice 1 should get 50 (30 + 20)");
  ASSERT_EQ(result.ranges[1].num_prbs, 50, "Slice 2 should get 50 (20 + 30)");
  ASSERT_EQ(allocated_prbs, 100, "All PRBs should be allocated");
}

static void test_pass3_all_slices_at_max(void) {
  printf("  Purpose: Test Pass 3 when all slices are at max limit\n");
  printf("  Expected: No allocation, remaining PRBs stay unallocated\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.50f;
  input.slices[0].min_prb_ratio = 0.50f;
  input.slices[0].max_prb_ratio = 0.50f;
  input.slices[0].has_active_ues = true;
  
  input.slices[1].slice_id = 2;
  input.slices[1].dedicated_prb_ratio = 0.30f;
  input.slices[1].min_prb_ratio = 0.30f;
  input.slices[1].max_prb_ratio = 0.30f;
  input.slices[1].has_active_ues = true;
  
  input.num_slices = 2;
  input.total_prbs = 100;
  
  printf("  Input Configuration (after Pass 1+2):\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Already allocated: 80 PRBs\n");
  printf("    Remaining (shared): 20 PRBs\n");
  print_slice_config(&input.slices[0], 0);
  print_slice_config(&input.slices[1], 1);
  printf("    Note: Both slices are already at their max limits\n");
  printf("          Slice 1: 50 PRBs (at max=50%%)\n");
  printf("          Slice 2: 30 PRBs (at max=30%%)\n\n");
  
  memset(&result, 0, sizeof(result));
  for (int s = 0; s < input.num_slices; ++s) {
    result.ranges[s].slice_id = input.slices[s].slice_id;
  }
  result.ranges[0].num_prbs = 50; // At max
  result.ranges[1].num_prbs = 30; // At max
  
  int allocated_prbs = 80;
  int remaining_prbs = 20; // But can't allocate (all at max)
  
  int ret = pass3_allocate_shared(&input, &result, &allocated_prbs, &remaining_prbs);
  
  printf("  Pass 3 Result:\n");
  printf("    Allocated PRBs: %d (no change, all at max)\n", allocated_prbs);
  printf("    Remaining PRBs: %d (cannot allocate, all slices at max)\n", remaining_prbs);
  printf("    Slice 1: %d PRBs (at max, no change)\n", result.ranges[0].num_prbs);
  printf("    Slice 2: %d PRBs (at max, no change)\n", result.ranges[1].num_prbs);
  printf("\n");
  
  ASSERT_EQ(ret, 0, "Should succeed");
  ASSERT_EQ(result.ranges[0].num_prbs, 50, "Slice 1 should stay at 50");
  ASSERT_EQ(result.ranges[1].num_prbs, 30, "Slice 2 should stay at 30");
  ASSERT_EQ(remaining_prbs, 20, "20 PRBs should remain (all slices at max)");
}

/* Test Pass 4 Edge Cases */
static void test_pass4_empty_result(void) {
  printf("  Purpose: Test Pass 4 with no PRBs allocated\n");
  printf("  Expected: No ranges assigned, no errors\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.num_slices = 1;
  
  printf("  Input Configuration:\n");
  printf("    Number of Slices: %d\n", input.num_slices);
  printf("    Slice 1: 0 PRBs allocated\n");
  printf("    Note: No PRBs allocated, so no ranges will be assigned\n\n");
  
  // No PRBs allocated (all slices have num_prbs = 0)
  memset(&result, 0, sizeof(result));
  result.ranges[0].slice_id = 1;
  result.ranges[0].num_prbs = 0;
  
  int ret = pass4_assign_ranges(&input, &result);
  
  printf("  Pass 4 Result:\n");
  printf("    Slice 1: [%d, %d) = %d PRBs (no range assigned, 0 PRBs)\n",
         result.ranges[0].start_prb, result.ranges[0].end_prb, result.ranges[0].num_prbs);
  printf("\n");
  
  ASSERT_EQ(ret, 0, "Should succeed");
  ASSERT_EQ(result.ranges[0].start_prb, 0, "Start should be 0");
  ASSERT_EQ(result.ranges[0].end_prb, 0, "End should be 0");
}

static void test_pass4_single_slice(void) {
  printf("  Purpose: Test Pass 4 with single slice\n");
  printf("  Expected: Range [0, num_prbs)\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.num_slices = 1;
  
  printf("  Input Configuration:\n");
  printf("    Number of Slices: %d\n", input.num_slices);
  printf("    Slice 1: 100 PRBs allocated\n");
  printf("    Note: Single slice gets all PRBs, range should be [0, 100)\n\n");
  
  result.ranges[0].slice_id = 1;
  result.ranges[0].num_prbs = 100;
  
  int ret = pass4_assign_ranges(&input, &result);
  
  printf("  Pass 4 Result:\n");
  printf("    Slice 1: [%d, %d) = %d PRBs\n",
         result.ranges[0].start_prb, result.ranges[0].end_prb, result.ranges[0].num_prbs);
  printf("\n");
  
  ASSERT_EQ(ret, 0, "Should succeed");
  ASSERT_EQ(result.ranges[0].start_prb, 0, "Should start at 0");
  ASSERT_EQ(result.ranges[0].end_prb, 100, "Should end at 100");
}

/* Integration Edge Cases */
static void test_integration_max_less_than_min(void) {
  printf("  Purpose: Test full algorithm with max < min (edge case)\n");
  printf("  Expected: Max takes precedence, min not fully met\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.20f;
  input.slices[0].min_prb_ratio = 0.50f; // Min = 50
  input.slices[0].max_prb_ratio = 0.30f; // Max = 30 (< min!)
  input.slices[0].has_active_ues = true;
  input.slices[0].required_prbs = 50;
  
  input.num_slices = 1;
  input.total_prbs = 100;
  
  printf("  Input Configuration:\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Number of Slices: %d\n", input.num_slices);
  print_slice_config(&input.slices[0], 0);
  printf("    Note: Max (30%%) < Min (50%%), so max takes precedence\n\n");
  
  int ret = calculate_slice_prb_ranges(&input, &result);
  
  print_allocation_result_with_required(&result, &input, input.total_prbs);
  printf("\n");
  
  ASSERT_EQ(ret, 1, "Should have 1 active slice");
  ASSERT_EQ(result.ranges[0].num_prbs, 30, "Should be capped at max (30)");
  ASSERT_EQ(result.total_allocated_prbs, 30, "Total should be 30");
  ASSERT_EQ(result.ranges[0].start_prb, 0, "Should start at 0");
  ASSERT_EQ(result.ranges[0].end_prb, 30, "Should end at 30");
}

static void test_integration_all_slices_no_prioritized_need(void) {
  printf("  Purpose: Test when all slices don't need prioritized resources\n");
  printf("  Expected: Prioritized resources go to Pass 3 as shared\n\n");
  
  slice_alloc_input_t input = {0};
  slice_alloc_result_t result = {0};
  
  input.slices[0].slice_id = 1;
  input.slices[0].dedicated_prb_ratio = 0.20f;
  input.slices[0].min_prb_ratio = 0.30f;
  input.slices[0].max_prb_ratio = 0.50f;
  input.slices[0].has_active_ues = true;
  input.slices[0].required_prbs = 15; // Less than dedicated (20)
  
  input.slices[1].slice_id = 2;
  input.slices[1].dedicated_prb_ratio = 0.10f;
  input.slices[1].min_prb_ratio = 0.25f;
  input.slices[1].max_prb_ratio = 0.50f;
  input.slices[1].has_active_ues = true;
  input.slices[1].required_prbs = 5; // Less than dedicated (10)
  
  input.num_slices = 2;
  input.total_prbs = 100;
  
  printf("  Input Configuration:\n");
  printf("    Total PRBs: %d\n", input.total_prbs);
  printf("    Number of Slices: %d\n", input.num_slices);
  print_slice_config(&input.slices[0], 0);
  print_slice_config(&input.slices[1], 1);
  printf("    Note: Both slices don't need prioritized resources\n");
  printf("          Slice 1: required=15 < dedicated=20\n");
  printf("          Slice 2: required=5 < dedicated=10\n");
  printf("          Prioritized resources will go to Pass 3 as shared\n\n");
  
  int ret = calculate_slice_prb_ranges(&input, &result);
  
  print_allocation_result_with_required(&result, &input, input.total_prbs);
  printf("\n");
  
  printf("  Verification:\n");
  ASSERT_EQ(ret, 2, "Should have 2 active slices");
  // Pass 1: 20 + 10 = 30
  // Pass 2: Both don't need prioritized, so 0 added
  // Pass 3: 70 PRBs shared, distributed proportionally
  ASSERT_GE(result.ranges[0].num_prbs, 20, "Slice 1 should get at least 20");
  ASSERT_GE(result.ranges[1].num_prbs, 10, "Slice 2 should get at least 10");
  ASSERT_EQ(result.total_allocated_prbs, 100, "All PRBs should be allocated");
}

int main(void) {
  printf("╔════════════════════════════════════════════════════════════════════════════════╗\n");
  printf("║     Network Slice PRB Allocation Algorithm - Unit Tests                     ║\n");
  printf("╚════════════════════════════════════════════════════════════════════════════════╝\n\n");
  printf("This test suite validates the four-pass PRB allocation algorithm:\n");
  printf("  1. Pass 1: Dedicated PRB allocation (non-shareable)\n");
  printf("  2. Pass 2: Prioritized resource distribution (min - dedicated)\n");
  printf("  3. Pass 3: Shared resource distribution (max - min)\n");
  printf("  4. Pass 4: Contiguous range assignment\n\n");
  
  printf("=== Individual Pass Tests ===\n\n");
  TEST(pass1_dedicated);
  TEST(pass2_prioritized);
  TEST(pass3_shared);
  TEST(pass4_ranges);
  
  printf("\n=== Pass 1 Edge Case Tests ===\n\n");
  TEST(pass1_dedicated_static_allocation);
  TEST(pass1_max_less_than_dedicated);
  TEST(pass1_zero_dedicated);
  
  printf("\n=== Pass 2 Edge Case Tests ===\n\n");
  TEST(pass2_slice_doesnt_need_prioritized);
  TEST(pass2_insufficient_prioritized);
  TEST(pass2_max_less_than_min);
  
  printf("\n=== Pass 3 Edge Case Tests ===\n\n");
  TEST(pass3_no_prb_requirements);
  TEST(pass3_all_slices_at_max);
  
  printf("\n=== Pass 4 Edge Case Tests ===\n\n");
  TEST(pass4_empty_result);
  TEST(pass4_single_slice);
  
  printf("\n=== Integrated Tests (Full Algorithm) ===\n\n");
  TEST(basic_two_slices);
  TEST(single_slice_all_prbs);
  TEST(no_active_ues);
  TEST(multiple_slices_different_ratios);
  TEST(dedicated_exceeds_total);
  TEST(max_ratio_enforcement);
  TEST(validation);
  TEST(zero_total_prbs);
  TEST(real_world_106_prbs);
  
  printf("\n=== Integration Edge Case Tests ===\n\n");
  TEST(integration_max_less_than_min);
  TEST(integration_all_slices_no_prioritized_need);
  
  printf("╔════════════════════════════════════════════════════════════════════════════════╗\n");
  printf("║                            Test Summary                                       ║\n");
  printf("╚════════════════════════════════════════════════════════════════════════════════╝\n");
  printf("  Tests run:    %d\n", tests_run);
  printf("  Tests passed: %d\n", tests_passed);
  printf("  Tests failed: %d\n", tests_run - tests_passed);
  
  if (tests_passed == tests_run) {
    printf("\n  ✓ All tests passed! The algorithm is working correctly.\n\n");
    return 0;
  } else {
    printf("\n  ✗ Some tests failed! Please review the output above.\n\n");
    return 1;
  }
}
