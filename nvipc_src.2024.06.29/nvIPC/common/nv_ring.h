/*
 * Copyright (c) 2023, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NV_RING_H_INCLUDED_
#define NV_RING_H_INCLUDED_

#include <stdint.h>
#include "nv_ipc_ring.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef nv_ipc_ring_t nv_ring;

#define NV_RING_OPT_SHM_APP_INTERNAL (0)
#define NV_RING_OPT_SHM_PRIMARY (1)
#define NV_RING_OPT_SHM_SECONDARY (2)

// NOTE: name length (including '\0') <= 32. Long than 31 characters doesn't cause error but will be truncated.
static inline nv_ring* nv_ring_create(const char *name, unsigned int count, unsigned int flags) {
    ring_type_t type;
    if (flags == NV_RING_OPT_SHM_PRIMARY) {
        type = RING_TYPE_SHM_PRIMARY;
    } else if (flags == NV_RING_OPT_SHM_SECONDARY) {
        type = RING_TYPE_SHM_SECONDARY;
    } else {
        type = RING_TYPE_APP_INTERNAL;
    }
    return nv_ipc_ring_open(type, name, count, sizeof(void*));
}

static inline int nv_ring_enqueue(nv_ring *ring, void *obj) {
    return ring->enqueue(ring, &obj);
}

static inline int nv_ring_dequeue(nv_ring *ring, void **obj_p) {
    return ring->dequeue(ring, obj_p);
}

static inline unsigned int nv_ring_count(nv_ring *ring) {
    int count = ring->get_count(ring);
    return count < 0 ? 0 : count;
}

static inline int nv_ring_free(nv_ring *ring) {
    return ring->close(ring);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NV_RING_H_INCLUDED_ */
