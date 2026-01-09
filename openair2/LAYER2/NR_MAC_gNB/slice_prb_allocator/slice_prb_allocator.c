/*!
 * \file slice_prb_allocator.c
 * \brief Implementation of Network Slice PRB Range Allocation Algorithm
 */

#include "slice_prb_allocator.h"
#include "slice_prb_allocator_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*! \brief Minimum capacity for dynamic slice arrays */
#define MIN_CAPACITY 8

/*! \brief Exponential moving average alpha factor (0.1 = 10% weight for new value) */
#define STATS_EMA_ALPHA 0.1f

/*! \brief Helper: Get minimum of two integers */
static inline int min_int(int a, int b) {
  return (a < b) ? a : b;
}

/*! \brief Helper: Get maximum of two integers */
static inline int max_int(int a, int b) {
  return (a > b) ? a : b;
}

slice_alloc_input_t* allocate_slice_input(int num_slices) {
  if (num_slices < 0) {
    return NULL;
  }
  size_t size = sizeof(slice_alloc_input_t) + num_slices * sizeof(slice_config_t);
  slice_alloc_input_t *input = (slice_alloc_input_t*)calloc(1, size);
  if (input != NULL) {
    input->num_slices = num_slices;
  }
  return input;
}

slice_alloc_result_t* allocate_slice_result(int num_slices) {
  if (num_slices < 0) {
    return NULL;
  }
  size_t size = sizeof(slice_alloc_result_t) + num_slices * sizeof(slice_prb_range_t);
  slice_alloc_result_t *result = (slice_alloc_result_t*)calloc(1, size);
  return result;
}

void free_slice_input(slice_alloc_input_t *input) {
  if (input != NULL) {
    free(input);
  }
}

void free_slice_result(slice_alloc_result_t *result) {
  if (result != NULL) {
    free(result);
  }
}

/*! \brief Helper: Find slice index by slice_id */
static int find_slice_index(const slice_scheduler_t *obj, int slice_id) {
  if (obj == NULL || obj->input == NULL) {
    return -1;
  }
  for (int i = 0; i < obj->input->num_slices; ++i) {
    if (obj->input->slices[i].slice_id == slice_id) {
      return i;
    }
  }
  return -1;
}


/* ============================================================================
 * Legacy Functions (kept for backward compatibility)
 * ============================================================================ */

bool validate_slice_config(const slice_alloc_input_t *input) {
  if (input == NULL) {
    return false;
  }
  
  if (input->num_slices < 0) {
    return false;
  }
  
  if (input->total_prbs <= 0) {
    return false;
  }
  
  // Validate each slice configuration
  for (int s = 0; s < input->num_slices; ++s) {
    const slice_config_t *slice = &input->slices[s];
    
    // Validate ratios are in [0.0, 1.0]
    if (slice->dedicated_prb_ratio < 0.0 || slice->dedicated_prb_ratio > 1.0) {
      return false;
    }
    if (slice->min_prb_ratio < 0.0 || slice->min_prb_ratio > 1.0) {
      return false;
    }
    if (slice->max_prb_ratio < 0.0 || slice->max_prb_ratio > 1.0) {
      return false;
    }
    
    // Validate ratio relationships: dedicated <= min <= max
    // Note: We allow max < min as an edge case (max takes precedence)
    // In this case, we only require dedicated <= max (the effective limit)
    if (slice->min_prb_ratio <= slice->max_prb_ratio) {
      // Normal case: min <= max, so dedicated <= min <= max
      if (slice->dedicated_prb_ratio > slice->min_prb_ratio) {
        return false;
      }
    } else {
      // Edge case: max < min, so we only require dedicated <= max
      if (slice->dedicated_prb_ratio > slice->max_prb_ratio) {
        return false;
      }
    }
  }
  
  return true;
}

int pass1_allocate_dedicated(const slice_alloc_input_t *input, slice_alloc_result_t *result,
                              int *allocated_prbs, int *num_active_slices) {
  if (input == NULL || result == NULL || allocated_prbs == NULL || num_active_slices == NULL) {
    return -1;
  }
  
  *allocated_prbs = 0;
  *num_active_slices = 0;
  int total_prbs = input->total_prbs;
  
  // Allocate dedicated PRBs (non-shareable) to slices with active UEs
  for (int s = 0; s < input->num_slices; ++s) {
    const slice_config_t *slice = &input->slices[s];
    
    if (!slice->has_active_ues) {
      continue;
    }
    
    int dedicated_prbs = (int)(total_prbs * slice->dedicated_prb_ratio + 0.5);
    int min_prbs = (int)(total_prbs * slice->min_prb_ratio + 0.5);
    int max_prbs = (int)(total_prbs * slice->max_prb_ratio + 0.5);
    
    // Ensure min >= dedicated
    if (min_prbs < dedicated_prbs) {
      min_prbs = dedicated_prbs;
    }
    
    // Ensure dedicated doesn't exceed max
    if (dedicated_prbs > max_prbs) {
      dedicated_prbs = max_prbs;
    }
    
    result->ranges[s].num_prbs = dedicated_prbs;
    *allocated_prbs += dedicated_prbs;
    (*num_active_slices)++;
  }
  
  // If dedicated allocations exceed total, scale them down proportionally
  if (*allocated_prbs > total_prbs) {
    float scale = (float)total_prbs / *allocated_prbs;
    *allocated_prbs = 0;
    for (int s = 0; s < input->num_slices; ++s) {
      if (result->ranges[s].num_prbs > 0) {
        result->ranges[s].num_prbs = (int)(result->ranges[s].num_prbs * scale + 0.5);
        *allocated_prbs += result->ranges[s].num_prbs;
      }
    }
  }
  
  return 0;
}

int pass2_allocate_prioritized(const slice_alloc_input_t *input, slice_alloc_result_t *result,
                                int *allocated_prbs, int *remaining_prbs) {
  if (input == NULL || result == NULL || allocated_prbs == NULL || remaining_prbs == NULL) {
    return -1;
  }
  
  if (*remaining_prbs <= 0) {
    return 0; // Nothing to allocate
  }
  
  int total_prbs = input->total_prbs;
  int total_prioritized_needed = 0;
  
  // Calculate total prioritized resources needed (based on required_prbs)
  for (int s = 0; s < input->num_slices; ++s) {
    const slice_config_t *slice = &input->slices[s];
    if (!slice->has_active_ues) {
      continue;
    }
    int min_prbs = (int)(total_prbs * slice->min_prb_ratio + 0.5);
    int dedicated_prbs = (int)(total_prbs * slice->dedicated_prb_ratio + 0.5);
    int prioritized_prbs = min_prbs - dedicated_prbs; // Prioritized but shareable portion
    
    // Check if slice needs prioritized resources based on required_prbs
    if (prioritized_prbs > 0 && slice->required_prbs > 0) {
      // Slice needs more PRBs than currently allocated
      if (slice->required_prbs > result->ranges[s].num_prbs) {
        int prioritized_needed = slice->required_prbs - result->ranges[s].num_prbs;
        // Don't exceed the prioritized portion (min - dedicated)
        if (prioritized_needed > prioritized_prbs) {
          prioritized_needed = prioritized_prbs;
        }
        if (prioritized_needed > 0) {
          total_prioritized_needed += prioritized_needed;
        }
      }
      // If required_prbs <= current_allocation, slice doesn't need prioritized resources
      // (they remain shareable for other slices)
    } else if (prioritized_prbs > 0 && slice->required_prbs == 0) {
      // No required_prbs specified, use traditional min guarantee logic
      int min_needed = min_prbs - result->ranges[s].num_prbs;
      if (min_needed > 0) {
        total_prioritized_needed += min_needed;
      }
    }
  }
  
  // Allocate prioritized resources to slices that need them
  if (total_prioritized_needed > 0) {
    float scale = (float)(*remaining_prbs) / total_prioritized_needed;
    if (scale > 1.0) {
      scale = 1.0; // Can't allocate more than needed
    }
    
    for (int s = 0; s < input->num_slices; ++s) {
      const slice_config_t *slice = &input->slices[s];
      if (!slice->has_active_ues) {
        continue;
      }
      int min_prbs = (int)(total_prbs * slice->min_prb_ratio + 0.5);
      int dedicated_prbs = (int)(total_prbs * slice->dedicated_prb_ratio + 0.5);
      int prioritized_prbs = min_prbs - dedicated_prbs;
      int max_prbs = (int)(total_prbs * slice->max_prb_ratio + 0.5);
      
      int prioritized_needed = 0;
      
      // Determine how much prioritized resources this slice needs
      if (prioritized_prbs > 0 && slice->required_prbs > 0) {
        // Slice needs more PRBs based on required_prbs
        if (slice->required_prbs > result->ranges[s].num_prbs) {
          prioritized_needed = slice->required_prbs - result->ranges[s].num_prbs;
          // Don't exceed the prioritized portion
          if (prioritized_needed > prioritized_prbs) {
            prioritized_needed = prioritized_prbs;
          }
        }
        // If required_prbs <= current_allocation, prioritized_needed = 0 (slice doesn't need it)
      } else if (prioritized_prbs > 0 && slice->required_prbs == 0) {
        // No required_prbs specified, use traditional min guarantee
        prioritized_needed = min_prbs - result->ranges[s].num_prbs;
      }
      
      if (prioritized_needed > 0) {
        int additional = (int)(prioritized_needed * scale + 0.5);
        // Ensure we don't exceed max_prb_ratio
        if (result->ranges[s].num_prbs + additional > max_prbs) {
          additional = max_prbs - result->ranges[s].num_prbs;
        }
        if (additional > 0) {
          result->ranges[s].num_prbs += additional;
          *allocated_prbs += additional;
          *remaining_prbs -= additional;
        }
      }
    }
  }
  
  return 0;
}

int pass3_allocate_shared(const slice_alloc_input_t *input, slice_alloc_result_t *result,
                          int *allocated_prbs, int *remaining_prbs) {
  if (input == NULL || result == NULL || allocated_prbs == NULL || remaining_prbs == NULL) {
    return -1;
  }
  
  if (*remaining_prbs <= 0) {
    return 0; // Nothing to allocate
  }
  
  int total_prbs = input->total_prbs;
  int total_capacity = 0;
  int total_prb_deficit = 0;
  
  // Calculate how much each slice can still take (up to max)
  for (int s = 0; s < input->num_slices; ++s) {
    const slice_config_t *slice = &input->slices[s];
    if (!slice->has_active_ues) {
      continue;
    }
    int max_prbs = (int)(total_prbs * slice->max_prb_ratio + 0.5);
    int can_add = max_prbs - result->ranges[s].num_prbs;
    if (can_add > 0) {
      total_capacity += can_add;
    }
    
    // Calculate PRB deficit (how many PRBs are still needed)
    if (slice->required_prbs > 0) {
      int prb_deficit = slice->required_prbs - result->ranges[s].num_prbs;
      if (prb_deficit > 0) {
        total_prb_deficit += prb_deficit;
      }
    }
  }
  
  // Distribute remaining PRBs
  if (total_capacity > 0) {
    int remaining_to_allocate = *remaining_prbs;
    
    // If PRB requirement-based allocation is enabled and there are PRB deficits
    if (total_prb_deficit > 0) {
      // Allocate based on PRB requirements (weighted by PRB deficit)
      for (int s = 0; s < input->num_slices && remaining_to_allocate > 0; ++s) {
        const slice_config_t *slice = &input->slices[s];
        if (!slice->has_active_ues) {
          continue;
        }
        
        int max_prbs = (int)(total_prbs * slice->max_prb_ratio + 0.5);
        int can_add = max_prbs - result->ranges[s].num_prbs;
        
        if (can_add > 0 && remaining_to_allocate > 0 && slice->required_prbs > 0) {
          // Calculate PRB deficit for this slice
          int prb_deficit = slice->required_prbs - result->ranges[s].num_prbs;
          
          if (prb_deficit > 0) {
            // Allocate proportionally based on PRB deficit, but respect capacity
            int add = (int)((float)prb_deficit / total_prb_deficit * remaining_to_allocate + 0.5);
            
            // Don't exceed what's needed for PRB requirement
            if (add > prb_deficit) {
              add = prb_deficit;
            }
            
            // Don't exceed slice capacity
            if (add > can_add) {
              add = can_add;
            }
            
            // Don't exceed remaining PRBs
            if (add > remaining_to_allocate) {
              add = remaining_to_allocate;
            }
            
            if (add > 0) {
              result->ranges[s].num_prbs += add;
              *allocated_prbs += add;
              remaining_to_allocate -= add;
              
              // Update PRB deficit tracking
              int new_deficit = slice->required_prbs - result->ranges[s].num_prbs;
              if (new_deficit > 0) {
                total_prb_deficit -= (prb_deficit - new_deficit);
              } else {
                total_prb_deficit -= prb_deficit;
              }
            }
          }
        }
      }
      
      // If there are still remaining PRBs after requirement-based allocation,
      // distribute them proportionally based on capacity
      if (remaining_to_allocate > 0) {
        // Recalculate total capacity
        total_capacity = 0;
        for (int s = 0; s < input->num_slices; ++s) {
          const slice_config_t *slice = &input->slices[s];
          if (!slice->has_active_ues) {
            continue;
          }
          int max_prbs = (int)(total_prbs * slice->max_prb_ratio + 0.5);
          int can_add = max_prbs - result->ranges[s].num_prbs;
          if (can_add > 0) {
            total_capacity += can_add;
          }
        }
        
        // Distribute remaining PRBs proportionally
        if (total_capacity > 0) {
          for (int s = 0; s < input->num_slices && remaining_to_allocate > 0; ++s) {
            const slice_config_t *slice = &input->slices[s];
            if (!slice->has_active_ues) {
              continue;
            }
            int max_prbs = (int)(total_prbs * slice->max_prb_ratio + 0.5);
            int can_add = max_prbs - result->ranges[s].num_prbs;
            
            if (can_add > 0 && remaining_to_allocate > 0) {
              int add = (int)((float)can_add / total_capacity * remaining_to_allocate + 0.5);
              if (add > can_add) {
                add = can_add;
              }
              if (add > remaining_to_allocate) {
                add = remaining_to_allocate;
              }
              if (add > 0) {
                result->ranges[s].num_prbs += add;
                *allocated_prbs += add;
                remaining_to_allocate -= add;
                total_capacity -= can_add;
              }
            }
          }
        }
      }
      *remaining_prbs = remaining_to_allocate;
    } else {
      // No PRB requirements or requirement-based allocation disabled
      // Fall back to proportional distribution based on capacity
      for (int s = 0; s < input->num_slices && remaining_to_allocate > 0; ++s) {
        const slice_config_t *slice = &input->slices[s];
        if (!slice->has_active_ues) {
          continue;
        }
        int max_prbs = (int)(total_prbs * slice->max_prb_ratio + 0.5);
        int can_add = max_prbs - result->ranges[s].num_prbs;
        
        if (can_add > 0 && remaining_to_allocate > 0) {
          int add = (int)((float)can_add / total_capacity * remaining_to_allocate + 0.5);
          if (add > can_add) {
            add = can_add;
          }
          if (add > remaining_to_allocate) {
            add = remaining_to_allocate;
          }
          if (add > 0) {
            result->ranges[s].num_prbs += add;
            *allocated_prbs += add;
            remaining_to_allocate -= add;
            total_capacity -= can_add;
          }
        }
      }
      *remaining_prbs = remaining_to_allocate;
    }
  }
  
  return 0;
}

int pass4_assign_ranges(const slice_alloc_input_t *input, slice_alloc_result_t *result) {
  if (input == NULL || result == NULL) {
    return -1;
  }
  
  // Assign PRB ranges (contiguous allocation)
  int current_prb = 0;
  for (int s = 0; s < input->num_slices; ++s) {
    if (result->ranges[s].num_prbs > 0) {
      result->ranges[s].start_prb = current_prb;
      result->ranges[s].end_prb = current_prb + result->ranges[s].num_prbs;
      current_prb = result->ranges[s].end_prb;
    }
  }
  
  return 0;
}

int calculate_slice_prb_ranges(const slice_alloc_input_t *input, slice_alloc_result_t *result) {
  if (input == NULL || result == NULL) {
    return -1;
  }
  
  if (!validate_slice_config(input)) {
    return -1;
  }
  
  // Initialize result fields (but not the flexible array - it's already zeroed by caller)
  result->num_active_slices = 0;
  result->total_allocated_prbs = 0;
  
  if (input->num_slices == 0) {
    return 0;
  }
  
  int num_active_slices = 0;
  int allocated_prbs = 0;
  int total_prbs = input->total_prbs;
  
  // Initialize ranges
  for (int s = 0; s < input->num_slices; ++s) {
    result->ranges[s].slice_id = input->slices[s].slice_id;
    result->ranges[s].start_prb = 0;
    result->ranges[s].end_prb = 0;
    result->ranges[s].num_prbs = 0;
  }
  
  // Pass 1: Allocate dedicated PRBs
  if (pass1_allocate_dedicated(input, result, &allocated_prbs, &num_active_slices) != 0) {
    return -1;
  }
  
  // Pass 2: Allocate prioritized resources
  int remaining_prbs = total_prbs - allocated_prbs;
  if (remaining_prbs > 0) {
    if (pass2_allocate_prioritized(input, result, &allocated_prbs, &remaining_prbs) != 0) {
      return -1;
    }
  }
  
  // Pass 3: Allocate shared resources
  if (remaining_prbs > 0) {
    if (pass3_allocate_shared(input, result, &allocated_prbs, &remaining_prbs) != 0) {
      return -1;
    }
  }
  
  // Pass 4: Assign contiguous ranges
  if (pass4_assign_ranges(input, result) != 0) {
    return -1;
  }
  
  result->num_active_slices = num_active_slices;
  result->total_allocated_prbs = allocated_prbs;
  
  return num_active_slices;
}

void print_slice_allocation(const slice_alloc_result_t *result, int num_slices) {
  if (result == NULL) {
    return;
  }
  
  if (num_slices < 0) {
    return;
  }
  
  printf("=== Slice PRB Allocation Result ===\n");
  printf("Total Allocated PRBs: %d\n", result->total_allocated_prbs);
  printf("Active Slices: %d\n\n", result->num_active_slices);
  
  // Iterate through all slices and print those with allocated PRBs
  for (int s = 0; s < num_slices; ++s) {
    if (result->ranges[s].num_prbs > 0) {
      printf("Slice %d: PRBs [%d, %d) (%d PRBs)\n",
             result->ranges[s].slice_id,
             result->ranges[s].start_prb,
             result->ranges[s].end_prb,
             result->ranges[s].num_prbs);
    }
  }
  printf("\n");
}

/* ============================================================================
 * OOP-like Scheduler Interface Implementation
 * ============================================================================ */

slice_scheduler_t* slice_sch_create(int total_prbs) {
  if (total_prbs <= 0) {
    return NULL;
  }
  
  slice_scheduler_t *obj = (slice_scheduler_t*)calloc(1, sizeof(slice_scheduler_t));
  if (obj == NULL) {
    return NULL;
  }
  
  obj->slices_capacity = MIN_CAPACITY; // Initial capacity
  obj->result_valid = false;
  
  // Allocate input structure
  obj->input = allocate_slice_input(obj->slices_capacity);
  if (obj->input == NULL) {
    free(obj);
    return NULL;
  }
  obj->input->num_slices = 0;
  obj->input->total_prbs = total_prbs;
  
  // Allocate result structure
  obj->result = allocate_slice_result(obj->slices_capacity);
  if (obj->result == NULL) {
    free_slice_input(obj->input);
    free(obj);
    return NULL;
  }
  
  // Allocate statistics array
  obj->statistics = (slice_statistics_t*)calloc(obj->slices_capacity, sizeof(slice_statistics_t));
  if (obj->statistics == NULL) {
    free_slice_result(obj->result);
    free_slice_input(obj->input);
    free(obj);
    return NULL;
  }
  
  return obj;
}

void slice_sch_destroy(slice_scheduler_t *obj) {
  if (obj != NULL) {
    if (obj->input != NULL) {
      free_slice_input(obj->input);
    }
    if (obj->result != NULL) {
      free_slice_result(obj->result);
    }
    if (obj->statistics != NULL) {
      free(obj->statistics);
    }
    free(obj);
  }
}

int slice_sch_add_slice(slice_scheduler_t *obj, int slice_id, float dedicated,
                        float min, float max, bool has, int require) {
  if (obj == NULL || obj->input == NULL) {
    return -1;
  }
  
  // Check if slice already exists
  if (find_slice_index(obj, slice_id) >= 0) {
    return -1; // Slice already exists
  }
  
  // Reallocate if needed
  if (obj->input->num_slices >= obj->slices_capacity) {
    int new_capacity = obj->slices_capacity * 2;
    
    // Reallocate input structure (preserves num_slices, total_prbs, and existing slices)
    size_t input_size = sizeof(slice_alloc_input_t) + new_capacity * sizeof(slice_config_t);
    slice_alloc_input_t *new_input = (slice_alloc_input_t*)realloc(obj->input, input_size);
    if (new_input == NULL) {
      return -1; // Allocation failed
    }
    
    // Reallocate result structure
    size_t result_size = sizeof(slice_alloc_result_t) + new_capacity * sizeof(slice_prb_range_t);
    slice_alloc_result_t *new_result = (slice_alloc_result_t*)realloc(obj->result, result_size);
    if (new_result == NULL) {
      // If result realloc fails, try to restore input to original size
      // (though this might also fail, but we try)
      size_t old_input_size = sizeof(slice_alloc_input_t) + obj->slices_capacity * sizeof(slice_config_t);
      slice_alloc_input_t *old_input = (slice_alloc_input_t*)realloc(new_input, old_input_size);
      if (old_input != NULL) {
        obj->input = old_input;
      }
      return -1; // Allocation failed
    }
    
    // Reallocate statistics array
    slice_statistics_t *new_statistics = (slice_statistics_t*)realloc(obj->statistics, 
                                                                       new_capacity * sizeof(slice_statistics_t));
    if (new_statistics == NULL) {
      // If statistics realloc fails, try to restore input and result
      size_t old_input_size = sizeof(slice_alloc_input_t) + obj->slices_capacity * sizeof(slice_config_t);
      size_t old_result_size = sizeof(slice_alloc_result_t) + obj->slices_capacity * sizeof(slice_prb_range_t);
      slice_alloc_input_t *old_input = (slice_alloc_input_t*)realloc(new_input, old_input_size);
      slice_alloc_result_t *old_result = (slice_alloc_result_t*)realloc(new_result, old_result_size);
      if (old_input != NULL) {
        obj->input = old_input;
      }
      if (old_result != NULL) {
        obj->result = old_result;
      }
      return -1; // Allocation failed
    }
    
    // All reallocs succeeded, update pointers and capacity
    obj->input = new_input;
    obj->result = new_result;
    obj->statistics = new_statistics;
    obj->slices_capacity = new_capacity;
  }
  
  // Validate ratios
  if (dedicated < 0.0 || dedicated > 1.0 ||
      min < 0.0 || min > 1.0 ||
      max < 0.0 || max > 1.0) {
    return -1;
  }
  
  // Validate ratio relationships
  if (min <= max) {
    if (dedicated > min) {
      return -1;
    }
  } else {
    if (dedicated > max) {
      return -1;
    }
  }
  
  // Validate require
  if (require < 0) {
    return -1;
  }
  
  // Add the slice
  slice_config_t *slice = &obj->input->slices[obj->input->num_slices];
  slice->slice_id = slice_id;
  slice->dedicated_prb_ratio = dedicated;
  slice->min_prb_ratio = min;
  slice->max_prb_ratio = max;
  slice->has_active_ues = has;
  slice->required_prbs = require;
  
  // Initialize statistics for the new slice
  slice_statistics_t *stats = &obj->statistics[obj->input->num_slices];
  stats->slice_id = slice_id;
  stats->latest_start_prb = 0;
  stats->latest_end_prb = 0;
  stats->latest_num_prbs = 0;
  stats->avg_start_prb = 0.0f;
  stats->avg_end_prb = 0.0f;
  stats->avg_num_prbs = 0.0f;
  stats->sample_count = 0;
  
  obj->input->num_slices++;
  obj->result_valid = false; // Invalidate result
  
  return 0;
}

int slice_sch_del_slice(slice_scheduler_t *obj, int slice_id) {
  if (obj == NULL || obj->input == NULL) {
    return -1;
  }
  
  int idx = find_slice_index(obj, slice_id);
  if (idx < 0) {
    return -1; // Slice not found
  }
  
  // Shift remaining slices to fill the gap
  for (int i = idx; i < obj->input->num_slices - 1; ++i) {
    obj->input->slices[i] = obj->input->slices[i + 1];
    // Also shift statistics
    obj->statistics[i] = obj->statistics[i + 1];
  }
  
  obj->input->num_slices--;
  obj->result_valid = false; // Invalidate result
  
  // Shrink array if capacity is much larger than needed
  // Shrink when using less than 25% of capacity, but keep minimum capacity
  if (obj->slices_capacity > MIN_CAPACITY && obj->input->num_slices < obj->slices_capacity / 4) {
    int new_capacity = obj->slices_capacity / 2;
    // Ensure new capacity is at least MIN_CAPACITY and at least num_slices
    if (new_capacity < MIN_CAPACITY) {
      new_capacity = MIN_CAPACITY;
    }
    if (new_capacity < obj->input->num_slices) {
      new_capacity = obj->input->num_slices;
    }
    
    // Reallocate input structure (preserves num_slices, total_prbs, and existing slices)
    size_t input_size = sizeof(slice_alloc_input_t) + new_capacity * sizeof(slice_config_t);
    slice_alloc_input_t *new_input = (slice_alloc_input_t*)realloc(obj->input, input_size);
    if (new_input != NULL) {
      // Reallocate result structure
      size_t result_size = sizeof(slice_alloc_result_t) + new_capacity * sizeof(slice_prb_range_t);
      slice_alloc_result_t *new_result = (slice_alloc_result_t*)realloc(obj->result, result_size);
      if (new_result != NULL) {
        // Reallocate statistics array
        slice_statistics_t *new_statistics = (slice_statistics_t*)realloc(obj->statistics,
                                                                           new_capacity * sizeof(slice_statistics_t));
        if (new_statistics != NULL) {
          // All reallocs succeeded, update pointers and capacity
          obj->input = new_input;
          obj->result = new_result;
          obj->statistics = new_statistics;
          obj->slices_capacity = new_capacity;
        }
        // If statistics realloc fails, keep input and result at new size
        // This is acceptable since we're shrinking
      }
      // If result realloc fails, keep input at new size (input realloc already succeeded)
      // This is acceptable since we're shrinking and the result will be recalculated on next schedule
    }
    // If realloc fails, continue with existing capacity (not a critical error)
  }
  
  return 0;
}

int slice_sch_update_require(slice_scheduler_t *obj, int slice_id, int require) {
  if (obj == NULL || obj->input == NULL) {
    return -1;
  }
  
  if (require < 0) {
    return -1;
  }
  
  int idx = find_slice_index(obj, slice_id);
  if (idx < 0) {
    return -1; // Slice not found
  }
  
  obj->input->slices[idx].required_prbs = require;
  obj->result_valid = false; // Invalidate result
  
  return 0;
}

int slice_sch_schedule(slice_scheduler_t *obj) {
  if (obj == NULL || obj->input == NULL || obj->result == NULL) {
    return -1;
  }
  
  if (obj->input->num_slices == 0) {
    obj->result->num_active_slices = 0;
    obj->result->total_allocated_prbs = 0;
    obj->result_valid = true;
    return 0;
  }
  
  // Validate configuration
  if (!validate_slice_config(obj->input)) {
    return -1;
  }
  
  // Use functional implementation directly on obj->input and obj->result
  int ret = calculate_slice_prb_ranges(obj->input, obj->result);
  if (ret < 0) {
    return -1;
  }
  
  // Update statistics for each slice
  // Note: result->ranges is indexed by slice index (same as input->slices)
  for (int s = 0; s < obj->input->num_slices; ++s) {
    slice_statistics_t *stats = &obj->statistics[s];
    
    // Check if this slice got an allocation (ranges array is indexed by slice index)
    if (obj->result->ranges[s].num_prbs > 0) {
      // Update latest values
      stats->latest_start_prb = obj->result->ranges[s].start_prb;
      stats->latest_end_prb = obj->result->ranges[s].end_prb;
      stats->latest_num_prbs = obj->result->ranges[s].num_prbs;
      
      // Update moving averages using exponential moving average (EMA)
      if (stats->sample_count == 0) {
        // First sample: initialize averages
        stats->avg_start_prb = (float)stats->latest_start_prb;
        stats->avg_end_prb = (float)stats->latest_end_prb;
        stats->avg_num_prbs = (float)stats->latest_num_prbs;
      } else {
        // Update EMA: new_avg = alpha * new_value + (1 - alpha) * old_avg
        stats->avg_start_prb = STATS_EMA_ALPHA * (float)stats->latest_start_prb + 
                               (1.0f - STATS_EMA_ALPHA) * stats->avg_start_prb;
        stats->avg_end_prb = STATS_EMA_ALPHA * (float)stats->latest_end_prb + 
                             (1.0f - STATS_EMA_ALPHA) * stats->avg_end_prb;
        stats->avg_num_prbs = STATS_EMA_ALPHA * (float)stats->latest_num_prbs + 
                              (1.0f - STATS_EMA_ALPHA) * stats->avg_num_prbs;
      }
      stats->sample_count++;
    } else {
      // Slice didn't get allocation - update latest to 0
      // Don't increment sample_count for slices without allocation
      stats->latest_start_prb = 0;
      stats->latest_end_prb = 0;
      stats->latest_num_prbs = 0;
      // Don't update averages for slices without allocation
    }
  }
  
  obj->result_valid = true;
  
  return 0;
}

const slice_prb_range_t* slice_sch_get_allocation(const slice_scheduler_t *obj, int *num_ranges) {
  if (obj == NULL || num_ranges == NULL || obj->result == NULL) {
    return NULL;
  }
  
  if (!obj->result_valid) {
    return NULL; // Result not valid, need to call schedule first
  }
  
  *num_ranges = obj->result->num_active_slices;
  return obj->result->ranges;
}

int slice_sch_get_stats(const slice_scheduler_t *obj, int *num_active_slices, int *total_allocated_prbs) {
  if (obj == NULL || num_active_slices == NULL || total_allocated_prbs == NULL || obj->result == NULL) {
    return -1;
  }
  
  if (!obj->result_valid) {
    return -1; // Result not valid, need to call schedule first
  }
  
  *num_active_slices = obj->result->num_active_slices;
  *total_allocated_prbs = obj->result->total_allocated_prbs;
  
  return 0;
}

int slice_sch_get_slice_statistics(const slice_scheduler_t *obj, int slice_id, slice_statistics_t *stats) {
  if (obj == NULL || obj->statistics == NULL || stats == NULL) {
    return -1;
  }
  
  // Find the slice index
  int idx = find_slice_index(obj, slice_id);
  if (idx < 0) {
    return -1; // Slice not found
  }
  
  // Copy statistics
  *stats = obj->statistics[idx];
  
  return 0;
}

const slice_statistics_t* slice_sch_get_all_statistics(const slice_scheduler_t *obj, int *num_stats) {
  if (obj == NULL || num_stats == NULL || obj->statistics == NULL || obj->input == NULL) {
    return NULL;
  }
  
  *num_stats = obj->input->num_slices;
  return obj->statistics;
}
