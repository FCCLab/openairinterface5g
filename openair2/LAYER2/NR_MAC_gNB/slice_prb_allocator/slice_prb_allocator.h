/*!
 * \file slice_prb_allocator.h
 * \brief Network Slice PRB Range Allocation Algorithm - Public OOP Interface
 * 
 * This is the public API for the Network Slice PRB Range Allocation Algorithm.
 * Normal OAI users should use the OOP-like scheduler interface provided here.
 * 
 * For internal functional implementation, see slice_prb_allocator_internal.h
 */

#ifndef SLICE_PRB_ALLOCATOR_H
#define SLICE_PRB_ALLOCATOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  /* For NULL */

/*! \brief Slice identifier with SST and SD bit fields (S-NSSAI) */
typedef struct {
  uint32_t sst : 8;   /*!< Slice/Service Type (8 bits, 0-255) */
  uint32_t sd : 24;   /*!< Slice Differentiator (24 bits, 0-0xffffff) */
} slice_id_t;

/*! \brief Helper: Create slice_id_t from SST and SD values
 *  \param sst Slice/Service Type (0-255)
 *  \param sd Slice Differentiator (0-0xffffff)
 *  \return slice_id_t structure
 */
static inline slice_id_t slice_id_create(uint8_t sst, uint32_t sd) {
  slice_id_t id = {.sst = sst, .sd = sd & 0xffffff};
  return id;
}

/*! \brief Helper: Create slice_id_t from integer (treats int as simple ID, sets sst=int, sd=0)
 *  \param id Integer slice identifier
 *  \return slice_id_t structure
 */
static inline slice_id_t slice_id_from_int(int id) {
  slice_id_t sid = {.sst = (uint8_t)(id & 0xff), .sd = 0};
  return sid;
}

/*! \brief Helper: Compare slice_id_t with integer slice_id
 *  \param sid slice_id_t structure
 *  \param id Integer slice identifier
 *  \return true if they match (based on sst field), false otherwise
 */
static inline bool slice_id_eq_int(const slice_id_t *sid, int id) {
  return (sid != NULL && sid->sst == (id & 0xff) && sid->sd == 0);
}

/*! \brief Helper: Compare two slice_id_t structures
 *  \param sid1 First slice_id_t structure
 *  \param sid2 Second slice_id_t structure
 *  \return true if they match, false otherwise
 */
static inline bool slice_id_eq(const slice_id_t *sid1, const slice_id_t *sid2) {
  return (sid1 != NULL && sid2 != NULL && 
          sid1->sst == sid2->sst && sid1->sd == sid2->sd);
}

/*! \brief PRB range allocation result for a slice (used in OOP interface) */
typedef struct {
  slice_id_t slice_id;  /*!< Slice ID with SST and SD */
  int start_prb;  /*!< Inclusive start PRB index */
  int end_prb;    /*!< Exclusive end PRB index (end_prb - start_prb = num_prbs) */
  int num_prbs;   /*!< Number of PRBs allocated */
} slice_prb_range_t;

/* ============================================================================
 * OOP-like Scheduler Interface
 * ============================================================================ */

/*! \brief Statistics for a single slice */
typedef struct {
  slice_id_t slice_id;                    /*!< Slice ID with SST and SD */
  int latest_start_prb;                    /*!< Latest start PRB index */
  int latest_end_prb;                      /*!< Latest end PRB index */
  int latest_num_prbs;                     /*!< Latest number of PRBs */
  float avg_start_prb;                     /*!< Moving average of start PRB */
  float avg_end_prb;                       /*!< Moving average of end PRB */
  float avg_num_prbs;                      /*!< Moving average of number of PRBs */
  int sample_count;                        /*!< Number of samples for moving average */
} slice_statistics_t;

/* Forward declarations for internal structures (opaque to users) */
typedef struct slice_alloc_input slice_alloc_input_t;
typedef struct slice_alloc_result slice_alloc_result_t;

/*! \brief Slice scheduler object that manages slices and allocations */
typedef struct {
  slice_alloc_input_t *input;              /*!< Internal: Input structure with slice configurations */
  slice_alloc_result_t *result;            /*!< Internal: Result structure with PRB allocations */
  slice_statistics_t *statistics;          /*!< Statistics for each slice */
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
 *  \param sst Slice/Service Type (0-255)
 *  \param sd Slice Differentiator (0-0xffffff)
 *  \param dedicated Dedicated PRB ratio (0.0-1.0)
 *  \param min Minimum PRB ratio (0.0-1.0)
 *  \param max Maximum PRB ratio (0.0-1.0)
 *  \param has Whether this slice has active UEs
 *  \param require Current PRB requirement (0 = not used)
 *  \return 0 on success, -1 on error
 */
int slice_sch_add_slice(slice_scheduler_t *obj, uint8_t sst, uint32_t sd, float dedicated,
                        float min, float max, bool has, int require);

/*! \brief Delete a slice from the scheduler
 *  \param obj Scheduler object
 *  \param sst Slice/Service Type (0-255)
 *  \param sd Slice Differentiator (0-0xffffff)
 *  \return 0 on success, -1 on error (slice not found)
 */
int slice_sch_del_slice(slice_scheduler_t *obj, uint8_t sst, uint32_t sd);

/*! \brief Update the PRB requirement for a slice
 *  \param obj Scheduler object
 *  \param sst Slice/Service Type (0-255)
 *  \param sd Slice Differentiator (0-0xffffff)
 *  \param require New PRB requirement (0 = not used)
 *  \return 0 on success, -1 on error (slice not found)
 */
int slice_sch_update_require(slice_scheduler_t *obj, uint8_t sst, uint32_t sd, int require);

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
 *  \param sst Slice/Service Type (0-255)
 *  \param sd Slice Differentiator (0-0xffffff)
 *  \param stats Output: Statistics structure (can be NULL to just check existence)
 *  \return 0 on success, -1 if slice not found
 */
int slice_sch_get_slice_statistics(const slice_scheduler_t *obj, uint8_t sst, uint32_t sd, slice_statistics_t *stats);

/*! \brief Get all slice statistics
 *  \param obj Scheduler object
 *  \param num_stats Output: Number of statistics entries
 *  \return Pointer to statistics array, or NULL on error
 */
const slice_statistics_t* slice_sch_get_all_statistics(const slice_scheduler_t *obj, int *num_stats);

#endif /* SLICE_PRB_ALLOCATOR_H */
