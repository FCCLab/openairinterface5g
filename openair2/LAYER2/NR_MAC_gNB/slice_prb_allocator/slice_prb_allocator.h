/*!
 * \file slice_prb_allocator.h
 * \brief Network Slice PRB Range Allocation Algorithm
 * 
 * This module implements a three-pass algorithm for allocating Physical Resource Blocks (PRBs)
 * to network slices based on dedicated, minimum, and maximum PRB ratios.
 * 
 * Algorithm Overview:
 * 1. First Pass: Allocate dedicated PRBs (non-shareable) to each slice
 * 2. Second Pass: Distribute remaining PRBs to meet minimum guarantees
 * 3. Third Pass: Distribute any remaining PRBs proportionally up to maximum limits
 * 
 * The algorithm ensures:
 * - dedicated_prb_ratio <= min_prb_ratio <= max_prb_ratio
 * - All allocations respect max_prb_ratio as a hard limit
 * - PRBs are allocated contiguously (for frequency-domain slicing)
 */

#ifndef SLICE_PRB_ALLOCATOR_H
#define SLICE_PRB_ALLOCATOR_H

#include <stdint.h>
#include <stdbool.h>

/*! \brief Maximum number of slices supported
 *  When used in OAI, MAX_NUM_SLICES is already defined in platform_constants.h
 *  For standalone compilation, defaults to 32
 */
#ifndef MAX_NUM_SLICES
#define MAX_NUM_SLICES 32
#endif

/*! \brief Network slice configuration */
typedef struct {
  int slice_id;              /*!< Unique slice identifier */
  float dedicated_prb_ratio; /*!< Dedicated PRB ratio (0.0-1.0), non-shareable */
  float min_prb_ratio;       /*!< Minimum PRB ratio (0.0-1.0), guaranteed */
  float max_prb_ratio;       /*!< Maximum PRB ratio (0.0-1.0), hard limit */
  bool has_active_ues;       /*!< Whether this slice has active UEs with data */
  int required_prbs;         /*!< Current PRB requirement for this slice (0 = not used, all symbols allocated) */
} slice_config_t;

/*! \brief PRB range allocation result for a slice */
typedef struct {
  int slice_id;
  int start_prb;  /*!< Inclusive start PRB index */
  int end_prb;    /*!< Exclusive end PRB index (end_prb - start_prb = num_prbs) */
  int num_prbs;   /*!< Number of PRBs allocated */
} slice_prb_range_t;

/*! \brief Input parameters for PRB allocation */
typedef struct {
  slice_config_t slices[MAX_NUM_SLICES];  /*!< Array of slice configurations */
  int num_slices;                          /*!< Number of configured slices */
  int total_prbs;                          /*!< Total number of PRBs available */
} slice_alloc_input_t;

/*! \brief Output result of PRB allocation */
typedef struct {
  slice_prb_range_t ranges[MAX_NUM_SLICES];  /*!< Array of PRB ranges for each slice */
  int num_active_slices;                      /*!< Number of slices with allocated PRBs */
  int total_allocated_prbs;                   /*!< Total PRBs allocated (should equal total_prbs) */
} slice_alloc_result_t;

/*! \brief Calculate PRB ranges for each slice
 *  \param input Input parameters (slice configs, total PRBs, PRB requirements)
 *  \param result Output result (PRB ranges for each slice)
 *  \return Number of slices with allocated PRBs, or -1 on error
 * 
 *  Algorithm:
 *  1. Allocate dedicated PRBs to slices with active UEs
 *  2. If dedicated allocations exceed total, scale them down proportionally
 *  3. Distribute remaining PRBs to meet minimum guarantees
 *  4. Distribute any remaining PRBs considering PRB requirements (slices with higher
 *     PRB needs get priority, up to their maximum limits)
 *  5. Assign contiguous PRB ranges
 * 
 *  Note: If required_prbs is 0 for a slice, requirement-based allocation is not used
 *        for that slice and algorithm falls back to proportional distribution.
 */
int calculate_slice_prb_ranges(const slice_alloc_input_t *input, slice_alloc_result_t *result);

/*! \brief Validate slice configuration
 *  \param input Input parameters to validate
 *  \return true if valid, false otherwise
 */
bool validate_slice_config(const slice_alloc_input_t *input);

/*! \brief Print allocation result (for debugging)
 *  \param result Allocation result to print
 */
void print_slice_allocation(const slice_alloc_result_t *result);

/*! \brief Pass 1: Allocate dedicated PRBs (non-shareable)
 *  \param input Input parameters
 *  \param result Result structure (will be updated with dedicated allocations)
 *  \param allocated_prbs Output: Total PRBs allocated after this pass
 *  \param num_active_slices Output: Number of slices with active UEs
 *  \return 0 on success, -1 on error
 */
int pass1_allocate_dedicated(const slice_alloc_input_t *input, slice_alloc_result_t *result,
                              int *allocated_prbs, int *num_active_slices);

/*! \brief Pass 2: Allocate prioritized resources (min - dedicated) based on required_prbs
 *  \param input Input parameters
 *  \param result Result structure (will be updated with prioritized allocations)
 *  \param allocated_prbs Input/Output: Current allocated PRBs, updated after this pass
 *  \param remaining_prbs Input/Output: Remaining PRBs, updated after this pass
 *  \return 0 on success, -1 on error
 */
int pass2_allocate_prioritized(const slice_alloc_input_t *input, slice_alloc_result_t *result,
                                int *allocated_prbs, int *remaining_prbs);

/*! \brief Pass 3: Allocate shared resources (max - min) proportionally
 *  \param input Input parameters
 *  \param result Result structure (will be updated with shared allocations)
 *  \param allocated_prbs Input/Output: Current allocated PRBs, updated after this pass
 *  \param remaining_prbs Input/Output: Remaining PRBs, updated after this pass
 *  \return 0 on success, -1 on error
 */
int pass3_allocate_shared(const slice_alloc_input_t *input, slice_alloc_result_t *result,
                          int *allocated_prbs, int *remaining_prbs);

/*! \brief Pass 4: Assign contiguous PRB ranges
 *  \param input Input parameters
 *  \param result Result structure (will be updated with start_prb and end_prb)
 *  \return 0 on success, -1 on error
 */
int pass4_assign_ranges(const slice_alloc_input_t *input, slice_alloc_result_t *result);

#endif /* SLICE_PRB_ALLOCATOR_H */
