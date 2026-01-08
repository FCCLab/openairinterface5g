/*!
 * \file slice_prb_allocator.c
 * \brief Implementation of Network Slice PRB Range Allocation Algorithm
 */

#include "slice_prb_allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*! \brief Helper: Get minimum of two integers */
static inline int min_int(int a, int b) {
  return (a < b) ? a : b;
}

/*! \brief Helper: Get maximum of two integers */
static inline int max_int(int a, int b) {
  return (a > b) ? a : b;
}

bool validate_slice_config(const slice_alloc_input_t *input) {
  if (input == NULL) {
    return false;
  }
  
  if (input->num_slices < 0 || input->num_slices > MAX_NUM_SLICES) {
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
  
  // Initialize result
  memset(result, 0, sizeof(slice_alloc_result_t));
  
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

void print_slice_allocation(const slice_alloc_result_t *result) {
  if (result == NULL) {
    return;
  }
  
  printf("=== Slice PRB Allocation Result ===\n");
  printf("Total Allocated PRBs: %d\n", result->total_allocated_prbs);
  printf("Active Slices: %d\n\n", result->num_active_slices);
  
  for (int s = 0; s < MAX_NUM_SLICES; ++s) {
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
