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
  int num_slices;                          /*!< Number of configured slices */
  int total_prbs;                          /*!< Total number of PRBs available */
  slice_config_t slices[];                 /*!< Flexible array member: slice configurations */
} slice_alloc_input_t;

/*! \brief Output result of PRB allocation */
typedef struct {
  int num_active_slices;                   /*!< Number of slices with allocated PRBs */
  int total_allocated_prbs;                /*!< Total PRBs allocated (should equal total_prbs) */
  slice_prb_range_t ranges[];              /*!< Flexible array member: PRB ranges for each slice */
} slice_alloc_result_t;

/*! \brief Allocate slice_alloc_input_t with flexible array member
 *  \param num_slices Number of slices
 *  \return Allocated structure or NULL on error
 *  \note Caller must free with free_slice_input()
 */
slice_alloc_input_t* allocate_slice_input(int num_slices);

/*! \brief Allocate slice_alloc_result_t with flexible array member
 *  \param num_slices Number of slices (for array size)
 *  \return Allocated structure or NULL on error
 *  \note Caller must free with free_slice_result()
 */
slice_alloc_result_t* allocate_slice_result(int num_slices);

/*! \brief Free slice_alloc_input_t
 *  \param input Structure to free (can be NULL)
 */
void free_slice_input(slice_alloc_input_t *input);

/*! \brief Free slice_alloc_result_t
 *  \param result Structure to free (can be NULL)
 */
void free_slice_result(slice_alloc_result_t *result);

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
 *  Note: Input and result structures must be allocated with allocate_slice_input()
 *        and allocate_slice_result() respectively.
 */
int calculate_slice_prb_ranges(const slice_alloc_input_t *input, slice_alloc_result_t *result);

/*! \brief Validate slice configuration
 *  \param input Input parameters to validate
 *  \return true if valid, false otherwise
 */
bool validate_slice_config(const slice_alloc_input_t *input);

/*! \brief Print allocation result (for debugging)
 *  \param result Allocation result to print
 *  \param num_slices Number of slices in the input (to know array size)
 */
void print_slice_allocation(const slice_alloc_result_t *result, int num_slices);

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

/* ============================================================================
 * OOP-like Scheduler Interface
 * ============================================================================ */

/*! \brief Statistics for a single slice */
typedef struct {
  int slice_id;                           /*!< Slice ID */
  int latest_start_prb;                    /*!< Latest start PRB index */
  int latest_end_prb;                      /*!< Latest end PRB index */
  int latest_num_prbs;                     /*!< Latest number of PRBs */
  float avg_start_prb;                     /*!< Moving average of start PRB */
  float avg_end_prb;                       /*!< Moving average of end PRB */
  float avg_num_prbs;                      /*!< Moving average of number of PRBs */
  int sample_count;                        /*!< Number of samples for moving average */
} slice_statistics_t;

/*! \brief Slice scheduler object that manages slices and allocations */
typedef struct {
  slice_alloc_input_t *input;              /*!< Input structure with slice configurations */
  slice_alloc_result_t *result;            /*!< Result structure with PRB allocations */
  slice_statistics_t *statistics;           /*!< Statistics for each slice */
  int slices_capacity;                     /*!< Current capacity for reallocation */
  bool result_valid;                       /*!< Whether the result is up-to-date */
} slice_scheduler_t;

/*! \brief Create and initialize a new slice scheduler
 *  \param total_prbs Total number of PRBs available
 *  \return Pointer to initialized scheduler, or NULL on error
 */
slice_scheduler_t* slice_sch_create(int total_prbs);

/*! \brief Destroy a slice scheduler and free resources
 *  \param obj Pointer to scheduler object (can be NULL)
 */
void slice_sch_destroy(slice_scheduler_t *obj);

/*! \brief Add a slice to the scheduler
 *  \param obj Scheduler object
 *  \param slice_id Unique slice identifier
 *  \param dedicated Dedicated PRB ratio (0.0-1.0)
 *  \param min Minimum PRB ratio (0.0-1.0)
 *  \param max Maximum PRB ratio (0.0-1.0)
 *  \param has Whether this slice has active UEs
 *  \param require Current PRB requirement (0 = not used)
 *  \return 0 on success, -1 on error
 */
int slice_sch_add_slice(slice_scheduler_t *obj, int slice_id, float dedicated,
                        float min, float max, bool has, int require);

/*! \brief Delete a slice from the scheduler
 *  \param obj Scheduler object
 *  \param slice_id Slice identifier to remove
 *  \return 0 on success, -1 on error (slice not found)
 */
int slice_sch_del_slice(slice_scheduler_t *obj, int slice_id);

/*! \brief Update the PRB requirement for a slice
 *  \param obj Scheduler object
 *  \param slice_id Slice identifier
 *  \param require New PRB requirement (0 = not used)
 *  \return 0 on success, -1 on error (slice not found)
 */
int slice_sch_update_require(slice_scheduler_t *obj, int slice_id, int require);

/*! \brief Perform scheduling/allocation of PRBs to slices
 *  \param obj Scheduler object
 *  \return 0 on success, -1 on error
 */
int slice_sch_schedule(slice_scheduler_t *obj);

/*! \brief Get the current allocation result (const)
 *  \param obj Scheduler object
 *  \param num_ranges Output: Number of ranges in the result
 *  \return Pointer to const array of PRB ranges, or NULL on error
 */
const slice_prb_range_t* slice_sch_get_allocation(const slice_scheduler_t *obj, int *num_ranges);

/*! \brief Get allocation statistics
 *  \param obj Scheduler object
 *  \param num_active_slices Output: Number of active slices
 *  \param total_allocated_prbs Output: Total allocated PRBs
 *  \return 0 on success, -1 on error
 */
int slice_sch_get_stats(const slice_scheduler_t *obj, int *num_active_slices, int *total_allocated_prbs);

/*! \brief Get statistics for a specific slice
 *  \param obj Scheduler object
 *  \param slice_id Slice ID to get statistics for
 *  \param stats Output: Statistics structure (can be NULL to just check existence)
 *  \return 0 on success, -1 if slice not found
 */
int slice_sch_get_slice_statistics(const slice_scheduler_t *obj, int slice_id, slice_statistics_t *stats);

/*! \brief Get all slice statistics
 *  \param obj Scheduler object
 *  \param num_stats Output: Number of statistics entries
 *  \return Pointer to statistics array, or NULL on error
 */
const slice_statistics_t* slice_sch_get_all_statistics(const slice_scheduler_t *obj, int *num_stats);

#endif /* SLICE_PRB_ALLOCATOR_H */
