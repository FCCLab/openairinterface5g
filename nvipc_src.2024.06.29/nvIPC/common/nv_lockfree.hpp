/*
 * Copyright (c) 2023-2024, NVIDIA CORPORATION.  All rights reserved.
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
#include "nv_ipc_mempool.h"

namespace nv {

#define LOCK_FREE_OPT_APP_INTERNAL (0)
#define LOCK_FREE_OPT_SHM_PRIMARY (1)
#define LOCK_FREE_OPT_SHM_SECONDARY (2)

// NOTE: name length (including '\0') <= 32. Long than 31 characters will be truncated so may cause error.
template <typename T>
class lock_free_mem_pool {
public:
    lock_free_mem_pool(uint32_t length, uint32_t flags = LOCK_FREE_OPT_APP_INTERNAL, const char* name = nullptr) {
        int primary = 0;
        if (flags == LOCK_FREE_OPT_SHM_PRIMARY) {
            primary = 1;
        } else if (flags == LOCK_FREE_OPT_SHM_SECONDARY) {
            primary = 0;
        } else {
            primary = 0xFF;
        }

        mempool = nv_ipc_mempool_open(primary, name, sizeof(T), length, -1);
        if (mempool == nullptr) {
            // throw std::runtime_error(std::string("Create lock-free memory pool failed")); 
        }
    }

    T* alloc() {
        int32_t index = mempool->alloc(mempool);
        return reinterpret_cast<T*>(mempool->get_addr(mempool, index));
    }

    int free(T* buf) {
        int32_t index = mempool->get_index(mempool, buf);
        return  mempool->free(mempool);
    }

    int get_pool_len(T* buf) {
        return  mempool->get_pool_len(mempool);
    }

    int get_free_count(T* buf) {
        return  mempool->get_free_count(mempool);
    }

    ~lock_free_mem_pool() {
        if (mempool != nullptr) {
            mempool->close(mempool);
        }
    }

private:
    nv_ipc_mempool_t* mempool;
};

template <typename T>
class lock_free_ring_pool {
public:
    lock_free_ring_pool(const char* name, uint32_t length, uint32_t flags = LOCK_FREE_OPT_APP_INTERNAL) {
        ring_type_t type;
        if (flags == LOCK_FREE_OPT_SHM_PRIMARY) {
            type = RING_TYPE_SHM_PRIMARY;
        } else if (flags == LOCK_FREE_OPT_SHM_SECONDARY) {
            type = RING_TYPE_SHM_SECONDARY;
        } else {
            type = RING_TYPE_APP_INTERNAL;
        }

        ring = nv_ipc_ring_open(type, name, length, sizeof(T*));
        if (ring == nullptr) {
            throw std::runtime_error(std::string("Create lock-free ring failed")); 
        }
    }

    T* alloc() {
        int32_t index = ring->alloc(ring);
        return index < 0 ? nullptr : reinterpret_cast<T*>(ring->get_addr(ring, index));
    }

    int free(T* buf) {
        int32_t index = ring->get_index(ring, buf);
        return ring->free(ring, index);
    }

    int enqueue(T* buf) {
        int32_t index = ring->get_index(ring, buf);
        return ring->enqueue_by_index(ring, index);
    }

    T* dequeue() {
        int32_t index = ring->dequeue_by_index(ring);
        return reinterpret_cast<T*>(ring->get_addr(ring, index));
    }

    int copy_enqueue(T* obj) {
        return ring->enqueue(ring, obj);
    }

    int copy_dequeue(T* obj) {
        return ring->dequeue(ring, obj);
    }

    unsigned int get_count() {
        int count = ring->get_count(ring);
        return count < 0 ? 0 : count;
    }

    ~lock_free_ring_pool() {
        if (ring != nullptr) {
            ring->close(ring);
        }
    }

private:
    nv_ipc_ring_t* ring;
};


} // namespace nv

#endif /* NV_RING_H_INCLUDED_ */
