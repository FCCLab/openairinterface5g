/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright(c) 2017 Intel Corporation
 */

/*! \file PHY/CODING/nr_ulsch_decoding_t2.c
 * \brief Defines the LDPC decoder
 * \author openairinterface
 * \date 28-02-2024
 * \version 1.0
 * \note: based on testbbdev test_bbdev_perf.c functions. Harq buffer offset added.
 * \mbuf and mempool allocated at the init step, LDPC parameters updated from OAI.
 * \warning
 */


// [from gNB coding]
#include "PHY/defs_gNB.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "PHY/CODING/nrLDPC_extern.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "SCHED_NR/sched_nr.h"
#include "SCHED_NR/fapi_nr_l1.h"
#include "defs.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/LOG/log.h"
#include <syscall.h>
#include <openair2/UTIL/OPT/opt.h>
//#define DEBUG_ULSCH_DECODING
//#define gNB_DEBUG_TRACE

#define OAI_UL_LDPC_MAX_NUM_LLR 27000//26112 // NR_LDPC_NCOL_BG1*NR_LDPC_ZMAX = 68*384
//#define DEBUG_CRC
#ifdef DEBUG_CRC
#define PRINT_CRC_CHECK(a) a
#else
#define PRINT_CRC_CHECK(a)
#endif

//extern double cpuf;

#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_interface.h"
#include "nfapi/open-nFAPI/nfapi/public_inc/nfapi_nr_interface.h"

#include <stdint.h>
#include "PHY/sse_intrin.h"
#include "nrLDPCdecoder_defs.h"
#include "nrLDPC_types.h"
#include "nrLDPC_init.h"
#include "nrLDPC_mPass.h"
#include "nrLDPC_cnProc.h"
#include "nrLDPC_bnProc.h"
#include <common/utils/LOG/log.h>
#define NR_LDPC_ENABLE_PARITY_CHECK

#ifdef NR_LDPC_DEBUG_MODE
#include "nrLDPC_tools/nrLDPC_debug.h"
#endif

#include "openair1/PHY/CODING/nrLDPC_extern.h"

#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <rte_eal.h>
#include <rte_common.h>
#include <rte_string_fns.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_pdump.h>
#include "nrLDPC_offload.h"

#include <math.h>

#include <rte_dev.h>
#include <rte_launch.h>
#include <rte_bbdev.h>
#include <rte_malloc.h>
#include <rte_random.h>
#include <rte_hexdump.h>
#include <rte_interrupts.h>

// this socket is the NUMA socket, so the hardware CPU id (numa is complex)
#define GET_SOCKET(socket_id) (((socket_id) == SOCKET_ID_ANY) ? 0 : (socket_id))

#define MAX_QUEUES 32

#define OPS_CACHE_SIZE 256U
#define OPS_POOL_SIZE_MIN 511U /* 0.5K per queue */

#define SYNC_WAIT 0
#define SYNC_START 1
#define INVALID_OPAQUE -1
#define TIME_OUT_POLL 1e8
#define INVALID_QUEUE_ID -1
/* Increment for next code block in external HARQ memory */
#define HARQ_INCR 32768
/* Headroom for filler LLRs insertion in HARQ buffer */
#define FILLER_HEADROOM 1024

pthread_mutex_t encode_mutex;
pthread_mutex_t decode_mutex;

const char *typeStr[] = {
    "RTE_BBDEV_OP_NONE", /**< Dummy operation that does nothing */
    "RTE_BBDEV_OP_TURBO_DEC", /**< Turbo decode */
    "RTE_BBDEV_OP_TURBO_ENC", /**< Turbo encode */
    "RTE_BBDEV_OP_LDPC_DEC", /**< LDPC decode */
    "RTE_BBDEV_OP_LDPC_ENC", /**< LDPC encode */
    "RTE_BBDEV_OP_TYPE_COUNT", /**< Count of different op types */
};

/* Represents tested active devices */
struct active_device {
  const char *driver_name;
  uint8_t dev_id;
  int dec_queue;
  int enc_queue;
  uint16_t queue_ids[MAX_QUEUES];
  uint16_t nb_queues;
  struct rte_mempool *bbdev_dec_op_pool;
  struct rte_mempool *bbdev_enc_op_pool;
  struct rte_mempool *in_mbuf_pool;
  struct rte_mempool *hard_out_mbuf_pool;
  struct rte_mempool *soft_out_mbuf_pool;
  struct rte_mempool *harq_in_mbuf_pool;
  struct rte_mempool *harq_out_mbuf_pool;
} active_devs[RTE_BBDEV_MAX_DEVS];
static int nb_active_devs;

/* Data buffers used by BBDEV ops */
struct test_buffers {
  struct rte_bbdev_op_data *inputs;
  struct rte_bbdev_op_data *hard_outputs;
  struct rte_bbdev_op_data *soft_outputs;
  struct rte_bbdev_op_data *harq_inputs;
  struct rte_bbdev_op_data *harq_outputs;
};

/* Operation parameters specific for given test case */
struct test_op_params {
  struct rte_mempool *mp_dec;
  struct rte_mempool *mp_enc;
  struct rte_bbdev_dec_op *ref_dec_op;
  struct rte_bbdev_enc_op *ref_enc_op;
  uint16_t burst_sz;
  uint16_t num_to_process;
  uint16_t num_lcores;
  int vector_mask;
  rte_atomic16_t sync;
  struct test_buffers q_bufs[RTE_MAX_NUMA_NODES][MAX_QUEUES];
};

/* Contains per lcore params */
struct thread_params {
  uint8_t dev_id;
  uint16_t queue_id;
  uint32_t lcore_id;
  uint64_t start_time;
  double ops_per_sec;
  double mbps;
  uint8_t iter_count;
  double iter_average;
  double bler;
  struct nrLDPCoffload_params *p_offloadParams;
  unsigned int size_offloadParams;
  uint8_t *p_out;
  //uint8_t r;
  uint8_t *harq_pid;
  uint8_t *ulsch_id;
  rte_atomic16_t nb_dequeued;
  rte_atomic16_t processing_status;
  rte_atomic16_t burst_sz;
  struct test_op_params *op_params;
  struct rte_bbdev_dec_op *dec_ops[MAX_BURST];
  struct rte_bbdev_enc_op *enc_ops[MAX_BURST];
};

// DPDK BBDEV copy
static inline void
mbuf_reset(struct rte_mbuf *m)
{
  m->pkt_len = 0;

  do {
    m->data_len = 0;
    m = m->next;
  } while (m != NULL);
}

/* Read flag value 0/1 from bitmap */
// DPDK BBDEV copy
static inline bool
check_bit(uint32_t bitmap, uint32_t bitmask)
{
  return bitmap & bitmask;
}

/* calculates optimal mempool size not smaller than the val */
// DPDK BBDEV copy
static unsigned int
optimal_mempool_size(unsigned int val)
{
  return rte_align32pow2(val + 1) - 1;
}

// DPDK BBDEV modified - sizes passed to the function, use of data_len and nb_segments, remove code related to Soft outputs, HARQ
// inputs, HARQ outputs
static int create_mempools(struct active_device *ad, int socket_id, uint16_t num_ops, int out_buff_sz, int in_max_sz)
{
  unsigned int ops_pool_size, mbuf_pool_size, data_room_size = 0;
  num_ops = 1;
  uint8_t nb_segments = 1;
  ops_pool_size = optimal_mempool_size(RTE_MAX(
      /* Ops used plus 1 reference op */
      RTE_MAX((unsigned int)(ad->nb_queues * num_ops + 1),
              /* Minimal cache size plus 1 reference op */
              (unsigned int)(1.5 * rte_lcore_count() * OPS_CACHE_SIZE + 1)),
      OPS_POOL_SIZE_MIN));

  /* Decoder ops mempool */
  ad->bbdev_dec_op_pool = rte_bbdev_op_pool_create("bbdev_op_pool_dec", RTE_BBDEV_OP_LDPC_DEC,
  /* Encoder ops mempool */                         ops_pool_size, OPS_CACHE_SIZE, socket_id);
  ad->bbdev_enc_op_pool = rte_bbdev_op_pool_create("bbdev_op_pool_enc", RTE_BBDEV_OP_LDPC_ENC,
                                                    ops_pool_size, OPS_CACHE_SIZE, socket_id);

  if ((ad->bbdev_dec_op_pool == NULL) || (ad->bbdev_enc_op_pool == NULL))
    TEST_ASSERT_NOT_NULL(NULL, "ERROR Failed to create %u items ops pool for dev %u on socket %d.",
                         ops_pool_size, ad->dev_id, socket_id);

  /* Inputs */
  mbuf_pool_size = optimal_mempool_size(ops_pool_size * nb_segments);
  data_room_size = RTE_MAX(in_max_sz + RTE_PKTMBUF_HEADROOM + FILLER_HEADROOM, (unsigned int)RTE_MBUF_DEFAULT_BUF_SIZE);
  ad->in_mbuf_pool = rte_pktmbuf_pool_create("in_mbuf_pool", mbuf_pool_size, 0, 0, data_room_size, socket_id);
  TEST_ASSERT_NOT_NULL(ad->in_mbuf_pool,
                       "ERROR Failed to create %u items input pktmbuf pool for dev %u on socket %d.",
                       mbuf_pool_size, ad->dev_id, socket_id);

  /* Hard outputs */
  data_room_size = RTE_MAX(out_buff_sz + RTE_PKTMBUF_HEADROOM + FILLER_HEADROOM, (unsigned int)RTE_MBUF_DEFAULT_BUF_SIZE);
  ad->hard_out_mbuf_pool = rte_pktmbuf_pool_create("hard_out_mbuf_pool", mbuf_pool_size, 0, 0, data_room_size, socket_id);
  TEST_ASSERT_NOT_NULL(ad->hard_out_mbuf_pool,
                       "ERROR Failed to create %u items hard output pktmbuf pool for dev %u on socket %d.",
                       mbuf_pool_size, ad->dev_id, socket_id);
  return 0;
}

const char *ldpcenc_flag_bitmask[] = {
    /** Set for bit-level interleaver bypass on output stream. */
    "RTE_BBDEV_LDPC_INTERLEAVER_BYPASS",
    /** If rate matching is to be performed */
    "RTE_BBDEV_LDPC_RATE_MATCH",
    /** Set for transport block CRC-24A attach */
    "RTE_BBDEV_LDPC_CRC_24A_ATTACH",
    /** Set for code block CRC-24B attach */
    "RTE_BBDEV_LDPC_CRC_24B_ATTACH",
    /** Set for code block CRC-16 attach */
    "RTE_BBDEV_LDPC_CRC_16_ATTACH",
    /** Set if a device supports encoder dequeue interrupts. */
    "RTE_BBDEV_LDPC_ENC_INTERRUPTS",
    /** Set if a device supports scatter-gather functionality. */
    "RTE_BBDEV_LDPC_ENC_SCATTER_GATHER",
    /** Set if a device supports concatenation of non byte aligned output */
    "RTE_BBDEV_LDPC_ENC_CONCATENATION",
};

const char *ldpcdec_flag_bitmask[] = {
    /** Set for transport block CRC-24A checking */
    "RTE_BBDEV_LDPC_CRC_TYPE_24A_CHECK",
    /** Set for code block CRC-24B checking */
    "RTE_BBDEV_LDPC_CRC_TYPE_24B_CHECK",
    /** Set to drop the last CRC bits decoding output */
    "RTE_BBDEV_LDPC_CRC_TYPE_24B_DROP"
    /** Set for bit-level de-interleaver bypass on Rx stream. */
    "RTE_BBDEV_LDPC_DEINTERLEAVER_BYPASS",
    /** Set for HARQ combined input stream enable. */
    "RTE_BBDEV_LDPC_HQ_COMBINE_IN_ENABLE",
    /** Set for HARQ combined output stream enable. */
    "RTE_BBDEV_LDPC_HQ_COMBINE_OUT_ENABLE",
    /** Set for LDPC decoder bypass.
     *  RTE_BBDEV_LDPC_HQ_COMBINE_OUT_ENABLE must be set.
     */
    "RTE_BBDEV_LDPC_DECODE_BYPASS",
    /** Set for soft-output stream enable */
    "RTE_BBDEV_LDPC_SOFT_OUT_ENABLE",
    /** Set for Rate-Matching bypass on soft-out stream. */
    "RTE_BBDEV_LDPC_SOFT_OUT_RM_BYPASS",
    /** Set for bit-level de-interleaver bypass on soft-output stream. */
    "RTE_BBDEV_LDPC_SOFT_OUT_DEINTERLEAVER_BYPASS",
    /** Set for iteration stopping on successful decode condition
     *  i.e. a successful syndrome check.
     */
    "RTE_BBDEV_LDPC_ITERATION_STOP_ENABLE",
    /** Set if a device supports decoder dequeue interrupts. */
    "RTE_BBDEV_LDPC_DEC_INTERRUPTS",
    /** Set if a device supports scatter-gather functionality. */
    "RTE_BBDEV_LDPC_DEC_SCATTER_GATHER",
    /** Set if a device supports input/output HARQ compression. */
    "RTE_BBDEV_LDPC_HARQ_6BIT_COMPRESSION",
    /** Set if a device supports input LLR compression. */
    "RTE_BBDEV_LDPC_LLR_COMPRESSION",
    /** Set if a device supports HARQ input from
     *  device's internal memory.
     */
    "RTE_BBDEV_LDPC_INTERNAL_HARQ_MEMORY_IN_ENABLE",
    /** Set if a device supports HARQ output to
     *  device's internal memory.
     */
    "RTE_BBDEV_LDPC_INTERNAL_HARQ_MEMORY_OUT_ENABLE",
    /** Set if a device supports loop-back access to
     *  HARQ internal memory. Intended for troubleshooting.
     */
    "RTE_BBDEV_LDPC_INTERNAL_HARQ_MEMORY_LOOPBACK",
    /** Set if a device includes LLR filler bits in the circular buffer
     *  for HARQ memory. If not set, it is assumed the filler bits are not
     *  in HARQ memory and handled directly by the LDPC decoder.
     */
    "RTE_BBDEV_LDPC_INTERNAL_HARQ_MEMORY_FILLERS",
};

// DPDK BBDEV modified - in DPDK this function is named add_bbdev_dev, removed code for RTE_BASEBAND_ACC100, IMO we can also remove
// RTE_LIBRTE_PMD_BBDEV_FPGA_LTE_FEC and RTE_LIBRTE_PMD_BBDEV_FPGA_5GNR_FEC - to be checked
static int add_dev(uint8_t dev_id, struct rte_bbdev_info *info)
{
  int ret;
  struct active_device *ad = &active_devs[nb_active_devs];
  unsigned int nb_queues;
  nb_queues = RTE_MIN(rte_lcore_count(), info->drv.max_num_queues);
  nb_queues = RTE_MIN(nb_queues, (unsigned int)MAX_QUEUES);

  /* Display for debug the capabilities of the card */
  for (int i = 0; info->drv.capabilities[i].type != RTE_BBDEV_OP_NONE; i++) {
    printf("device: %d, capability[%d]=%s\n", dev_id, i, typeStr[info->drv.capabilities[i].type]);
    if (info->drv.capabilities[i].type == RTE_BBDEV_OP_LDPC_ENC) {
      const struct rte_bbdev_op_cap_ldpc_enc cap = info->drv.capabilities[i].cap.ldpc_enc;
      printf("    buffers: src = %d, dst = %d\n   capabilites: ", cap.num_buffers_src, cap.num_buffers_dst);
      for (int j = 0; j < sizeof(cap.capability_flags) * 8; j++)
        if (cap.capability_flags & (1ULL << j))
          printf("%s ", ldpcenc_flag_bitmask[j]);
      printf("\n");
    }
    if (info->drv.capabilities[i].type == RTE_BBDEV_OP_LDPC_DEC) {
      const struct rte_bbdev_op_cap_ldpc_dec cap = info->drv.capabilities[i].cap.ldpc_dec;
      printf("    buffers: src = %d, hard out = %d, soft_out %d, llr size %d, llr decimals %d \n   capabilities: ",
             cap.num_buffers_src,
             cap.num_buffers_hard_out,
             cap.num_buffers_soft_out,
             cap.llr_size,
             cap.llr_decimals);
      for (int j = 0; j < sizeof(cap.capability_flags) * 8; j++)
        if (cap.capability_flags & (1ULL << j))
          printf("%s ", ldpcdec_flag_bitmask[j]);
      printf("\n");
    }
  }

  /* setup device */
  ret = rte_bbdev_setup_queues(dev_id, nb_queues, info->socket_id);
  if (ret < 0) {
    printf("rte_bbdev_setup_queues(%u, %u, %d) ret %i\n", dev_id, nb_queues, info->socket_id, ret);
    return TEST_FAILED;
  }

  /* setup device queues */
  struct rte_bbdev_queue_conf qconf = {
      .socket = info->socket_id,
      .queue_size = info->drv.default_queue_conf.queue_size,
  };

  // Search a queue linked to HW capability ldpc decoding
  qconf.op_type = RTE_BBDEV_OP_LDPC_ENC;
  int queue_id;
  for (queue_id = 0; queue_id < nb_queues; ++queue_id) {
    ret = rte_bbdev_queue_configure(dev_id, queue_id, &qconf);
    if (ret == 0) {
      printf("Found LDCP encoding queue (id=%u) at prio%u on dev%u\n", queue_id, qconf.priority, dev_id);
      qconf.priority++;
      //ret = rte_bbdev_queue_configure(ad->dev_id, queue_id, &qconf);
      ad->enc_queue = queue_id;
      ad->queue_ids[queue_id] = queue_id;
      break;
    }
  }
  TEST_ASSERT(queue_id != nb_queues, "ERROR Failed to configure encoding queues on dev %u", dev_id);

  // Search a queue linked to HW capability ldpc encoding
  qconf.op_type = RTE_BBDEV_OP_LDPC_DEC;
  for (queue_id++; queue_id < nb_queues; ++queue_id) {
    ret = rte_bbdev_queue_configure(dev_id, queue_id, &qconf);
    if (ret == 0) {
      printf("Found LDCP decoding queue (id=%u) at prio%u on dev%u\n", queue_id, qconf.priority, dev_id);
      qconf.priority++;
      //ret = rte_bbdev_queue_configure(ad->dev_id, queue_id, &qconf);
      ad->dec_queue = queue_id;
      ad->queue_ids[queue_id] = queue_id;
      break;
    }
  }
  TEST_ASSERT(queue_id != nb_queues, "ERROR Failed to configure encoding queues on dev %u", dev_id);
  ad->nb_queues = 2;
  return TEST_SUCCESS;
}

// DPDK BBDEV modified - nb_segments used, we are not using struct op_data_entries *ref_entries, but struct rte_mbuf *m_head,
// rte_pktmbuf_reset(m_head) added?  if ((op_type == DATA_INPUT) || (op_type == DATA_HARQ_INPUT)) -> no code in else?
static int init_op_data_objs_dec(struct rte_bbdev_op_data *bufs,
                                 uint8_t *input,
                                 t_nrLDPCoffload_params *offloadParams,
                                 struct rte_mempool *mbuf_pool,
                                 const uint16_t n,
                                 enum op_data_type op_type,
                                 uint16_t min_alignment)
{
  uint32_t data_len;
  bool large_input = false;
  uint16_t i = 0;
  for (uint16_t h = 0; h < n; ++h) {
    for (uint16_t j = 0; j < offloadParams[h].C; ++j) {
      data_len = offloadParams[h].E_cb[j];
      char *data;
      struct rte_mbuf *m_head = rte_pktmbuf_alloc(mbuf_pool);
      TEST_ASSERT_NOT_NULL(m_head, "Not enough mbufs in %d data type mbuf pool (needed %u, available %u)",
                           op_type, offloadParams[h].C, mbuf_pool->size);
  
      if (data_len > RTE_BBDEV_LDPC_E_MAX_MBUF) {
        printf("Warning: Larger input size than DPDK mbuf %u\n", data_len);
        large_input = true;
      }
      bufs[i].data = m_head;
      bufs[i].offset = 0;
      bufs[i].length = 0;
  
      if ((op_type == DATA_INPUT) || (op_type == DATA_HARQ_INPUT)) {
        if ((op_type == DATA_INPUT) && large_input) {
          /* Allocate a fake overused mbuf */
          data = rte_malloc(NULL, data_len, 0);
          TEST_ASSERT_NOT_NULL(data, "rte malloc failed with %u bytes", data_len);
          memcpy(data, &input[i * LDPC_MAX_CB_SIZE], data_len);
          m_head->buf_addr = data;
          m_head->buf_iova = rte_malloc_virt2iova(data);
          m_head->data_off = 0;
          m_head->data_len = data_len;
        } else {
          rte_pktmbuf_reset(m_head);
          data = rte_pktmbuf_append(m_head, data_len);
          TEST_ASSERT_NOT_NULL(data, "Couldn't append %u bytes to mbuf from %d data type mbuf pool", data_len, op_type);
          TEST_ASSERT(data == RTE_PTR_ALIGN(data, min_alignment),
                      "Data addr in mbuf (%p) is not aligned to device min alignment (%u)",
                      data,
                      min_alignment);
          rte_memcpy(data, &input[i * LDPC_MAX_CB_SIZE], data_len);
        }
        bufs[i].length += data_len;
      }
      ++i;
    }
  }
  return 0;
}

static int init_op_data_objs_enc(struct rte_bbdev_op_data *bufs,
                                 uint8_t **input_enc,
                                 t_nrLDPCoffload_params *offloadParams,
                                 struct rte_mbuf *m_head,
                                 struct rte_mempool *mbuf_pool,
                                 const uint16_t n,
                                 enum op_data_type op_type,
                                 uint16_t min_alignment)
{
  uint32_t data_len;
  bool large_input = false;
  for (int i = 0; i < n; ++i) {
    data_len = offloadParams->Kr;
    char *data;
    struct rte_mbuf *m_head = rte_pktmbuf_alloc(mbuf_pool);
    TEST_ASSERT_NOT_NULL(m_head, "Not enough mbufs in %d data type mbuf pool (needed %u, available %u)",
                         op_type, n, mbuf_pool->size);

    if (data_len > RTE_BBDEV_LDPC_E_MAX_MBUF) {
      printf("Warning: Larger input size than DPDK mbuf %u\n", data_len);
      large_input = true;
    }
    bufs[i].data = m_head;
    bufs[i].offset = 0;
    bufs[i].length = 0;

    if ((op_type == DATA_INPUT) || (op_type == DATA_HARQ_INPUT)) {
      if ((op_type == DATA_INPUT) && large_input) {
        /* Allocate a fake overused mbuf */
        data = rte_malloc(NULL, data_len, 0);
        TEST_ASSERT_NOT_NULL(data, "rte malloc failed with %u bytes", data_len);
        memcpy(data, &input_enc[0], data_len);
        m_head->buf_addr = data;
        m_head->buf_iova = rte_malloc_virt2iova(data);
        m_head->data_off = 0;
        m_head->data_len = data_len;
      } else {
        rte_pktmbuf_reset(m_head);
        data = rte_pktmbuf_append(m_head, data_len);
        TEST_ASSERT_NOT_NULL(data, "Couldn't append %u bytes to mbuf from %d data type mbuf pool", data_len, op_type);
        TEST_ASSERT(data == RTE_PTR_ALIGN(data, min_alignment),
                    "Data addr in mbuf (%p) is not aligned to device min alignment (%u)",
                    data,
                    min_alignment);
        rte_memcpy(data, input_enc[i], data_len);
      }
      bufs[i].length += data_len;
    }
  }
  return 0;
}


// DPDK BBEV copy
static int allocate_buffers_on_socket(struct rte_bbdev_op_data **buffers, const int len, const int socket)
{
  int i;

  *buffers = rte_zmalloc_socket(NULL, len, 0, socket);
  if (*buffers == NULL) {
    printf("WARNING: Failed to allocate op_data on socket %d\n", socket);
    /* try to allocate memory on other detected sockets */
    for (i = 0; i < socket; i++) {
      *buffers = rte_zmalloc_socket(NULL, len, 0, i);
      if (*buffers != NULL)
        break;
    }
  }

  return (*buffers == NULL) ? TEST_FAILED : TEST_SUCCESS;
}

// DPDK BBDEV copy
static void
free_buffers(struct active_device *ad, struct test_op_params *op_params)
{
  rte_mempool_free(ad->bbdev_dec_op_pool);
  rte_mempool_free(ad->bbdev_enc_op_pool);
  rte_mempool_free(ad->in_mbuf_pool);
  rte_mempool_free(ad->hard_out_mbuf_pool);
  rte_mempool_free(ad->soft_out_mbuf_pool);
  rte_mempool_free(ad->harq_in_mbuf_pool);
  rte_mempool_free(ad->harq_out_mbuf_pool);

  for (int i = 2; i < rte_lcore_count(); ++i) {
    for (int j = 0; j < RTE_MAX_NUMA_NODES; ++j) {
      rte_free(op_params->q_bufs[j][i].inputs);
      rte_free(op_params->q_bufs[j][i].hard_outputs);
      rte_free(op_params->q_bufs[j][i].soft_outputs);
      rte_free(op_params->q_bufs[j][i].harq_inputs);
      rte_free(op_params->q_bufs[j][i].harq_outputs);
    }
  }
}

// OAI / DPDK BBDEV modified - in DPDK named copy_reference_dec_op, here we are passing t_nrLDPCoffload_params *p_offloadParams to
// the function, check what is value of n, commented code for code block mode
static void
set_ldpc_dec_op(struct rte_bbdev_dec_op **ops, unsigned int n,
		unsigned int start_idx,
		struct test_buffers *bufs,
		struct rte_bbdev_dec_op *ref_op,
		//uint8_t r,
		uint8_t *harq_pid,
		uint8_t *ulsch_id,
		t_nrLDPCoffload_params *p_offloadParams,
		unsigned int size_offloadParams)
{
  unsigned int h;
  unsigned int i;
  unsigned int j = 0;
  for (h = 0; h < size_offloadParams; ++h){
    for (i = 0; i < p_offloadParams[h].C; ++i) {
      ops[j]->ldpc_dec.cb_params.e = p_offloadParams[h].E_cb[i];
      ops[j]->ldpc_dec.basegraph = p_offloadParams[h].BG;
      ops[j]->ldpc_dec.z_c = p_offloadParams[h].Z;
      ops[j]->ldpc_dec.q_m = p_offloadParams[h].Qm;
      ops[j]->ldpc_dec.n_filler = p_offloadParams[h].F;
      ops[j]->ldpc_dec.n_cb = p_offloadParams[h].n_cb;
      ops[j]->ldpc_dec.iter_max = p_offloadParams[h].numMaxIter;
      ops[j]->ldpc_dec.rv_index = p_offloadParams[h].rv;
      ops[j]->ldpc_dec.op_flags = RTE_BBDEV_LDPC_ITERATION_STOP_ENABLE |
                                  RTE_BBDEV_LDPC_INTERNAL_HARQ_MEMORY_IN_ENABLE |
                                  RTE_BBDEV_LDPC_INTERNAL_HARQ_MEMORY_OUT_ENABLE |
                                  RTE_BBDEV_LDPC_HQ_COMBINE_OUT_ENABLE;
      if (p_offloadParams[h].setCombIn) {
        ops[j]->ldpc_dec.op_flags |= RTE_BBDEV_LDPC_HQ_COMBINE_IN_ENABLE;
      }
      if (p_offloadParams[h].C > 1) {
        ops[j]->ldpc_dec.op_flags |= RTE_BBDEV_LDPC_CRC_TYPE_24B_DROP;
        ops[j]->ldpc_dec.op_flags |= RTE_BBDEV_LDPC_CRC_TYPE_24B_CHECK;
      }
      //LOG_W(PHY,"ULSCH %02d HARQPID %02d R %02d COMBIN %d RV %d NCB %05d NUM OPS %d E %05d\n", ulsch_id, harq_pid, r, p_offloadParams->setCombIn, p_offloadParams->rv, p_offloadParams->n_cb, n, p_offloadParams->E_cb[i]);
      ops[j]->ldpc_dec.code_block_mode = 1; // ldpc_dec->code_block_mode;
      ops[j]->ldpc_dec.harq_combined_input.offset = ulsch_id[h] * NR_LDPC_MAX_NUM_CB * LDPC_MAX_CB_SIZE + i * LDPC_MAX_CB_SIZE;
      ops[j]->ldpc_dec.harq_combined_output.offset = ulsch_id[h] * NR_LDPC_MAX_NUM_CB * LDPC_MAX_CB_SIZE + i * LDPC_MAX_CB_SIZE;
      if (bufs->hard_outputs != NULL)
        ops[j]->ldpc_dec.hard_output = bufs->hard_outputs[start_idx + j];
      if (bufs->inputs != NULL)
        ops[j]->ldpc_dec.input = bufs->inputs[start_idx + j];
      if (bufs->soft_outputs != NULL)
        ops[j]->ldpc_dec.soft_output = bufs->soft_outputs[start_idx + j];
      if (bufs->harq_inputs != NULL)
        ops[j]->ldpc_dec.harq_combined_input = bufs->harq_inputs[start_idx + j];
      if (bufs->harq_outputs != NULL)
        ops[j]->ldpc_dec.harq_combined_output = bufs->harq_outputs[start_idx + j];
      ++j;
    }
  }
}

static void set_ldpc_enc_op(struct rte_bbdev_enc_op **ops,
                            unsigned int n,
                            unsigned int start_idx,
                            struct rte_bbdev_op_data *inputs,
                            struct rte_bbdev_op_data *outputs,
                            struct rte_bbdev_enc_op *ref_op,
                            t_nrLDPCoffload_params *p_offloadParams)
{
  // struct rte_bbdev_op_ldpc_enc *ldpc_enc = &ref_op->ldpc_enc;
  for (int i = 0; i < n; ++i) {
    ops[i]->ldpc_enc.cb_params.e = p_offloadParams->E_cb[i];
    ops[i]->ldpc_enc.basegraph = p_offloadParams->BG;
    ops[i]->ldpc_enc.z_c = p_offloadParams->Z;
    ops[i]->ldpc_enc.q_m = p_offloadParams->Qm;
    ops[i]->ldpc_enc.n_filler = p_offloadParams->F;
    ops[i]->ldpc_enc.n_cb = p_offloadParams->n_cb;
    ops[i]->ldpc_enc.rv_index = p_offloadParams->rv;
    ops[i]->ldpc_enc.op_flags = RTE_BBDEV_LDPC_RATE_MATCH;
    ops[i]->ldpc_enc.code_block_mode = 1;
    ops[i]->ldpc_enc.output = outputs[start_idx + i];
    ops[i]->ldpc_enc.input = inputs[start_idx + i];
  }
}

// DPDK BBDEV modified - in DPDK called validate_dec_op, int8_t* p_out added, remove code related to op_data_entries, turbo_dec
// replaced by ldpc_dec, removed coderelated to soft_output, memcpy(p_out, data+m->data_off, data_len)
static int retrieve_ldpc_dec_op(struct rte_bbdev_dec_op **ops,
                                const uint16_t n,
                                struct rte_bbdev_dec_op *ref_op,
                                const int vector_mask,
                                uint8_t *p_out)
{
  struct rte_bbdev_op_data *hard_output;
  uint16_t data_len = 0;
  struct rte_mbuf *m;
  unsigned int i;
  char *data;
  int offset = 0;
  for (i = 0; i < n; ++i) {
    hard_output = &ops[i]->ldpc_dec.hard_output;
    m = hard_output->data;
    data_len = rte_pktmbuf_data_len(m) - hard_output->offset;
    data = m->buf_addr;
    memcpy(&p_out[offset], data + m->data_off, data_len);
    offset += data_len;
    rte_pktmbuf_free(ops[i]->ldpc_dec.hard_output.data);
    rte_pktmbuf_free(ops[i]->ldpc_dec.input.data);
  }
  return 0;
}

static int retrieve_ldpc_enc_op(struct rte_bbdev_enc_op **ops, const uint16_t n, struct rte_bbdev_enc_op *ref_op, uint8_t *p_out, uint32_t *E)
{
  struct rte_bbdev_op_data *output;
  struct rte_mbuf *m;
  unsigned int i;
  char *data;
  uint8_t *out;
  int offset = 0;
  for (i = 0; i < n; ++i) {
    output = &ops[i]->ldpc_enc.output;
    m = output->data;
    uint16_t data_len = rte_pktmbuf_data_len(m) - output->offset;
    out = &p_out[offset];
    data = m->buf_addr;
    for (int byte = 0; byte < data_len; byte++)
      for (int bit = 0; bit < 8; bit++)
        out[byte * 8 + bit] = (data[m->data_off + byte] >> (7 - bit)) & 1;
    offset += E[i];
    rte_pktmbuf_free(ops[i]->ldpc_enc.output.data);
    rte_pktmbuf_free(ops[i]->ldpc_enc.input.data);
  }
  return 0;
}

// DPDK BBDEV copy
static int init_test_op_params(struct test_op_params *op_params,
                               enum rte_bbdev_op_type op_type,
                               struct rte_mempool *ops_mp,
                               uint16_t burst_sz,
                               uint16_t num_to_process,
                               uint16_t num_lcores)
{
  int ret = 0;
  if (op_type == RTE_BBDEV_OP_LDPC_DEC) {
    ret = rte_bbdev_dec_op_alloc_bulk(ops_mp, &op_params->ref_dec_op, num_to_process);
    op_params->mp_dec = ops_mp;
  } else {
    ret = rte_bbdev_enc_op_alloc_bulk(ops_mp, &op_params->ref_enc_op, 1);
    op_params->mp_enc = ops_mp;
  }

  TEST_ASSERT_SUCCESS(ret, "rte_bbdev_op_alloc_bulk() failed");

  op_params->burst_sz = burst_sz;
  op_params->num_to_process = num_to_process;
  op_params->num_lcores = num_lcores;
  return 0;
}

// DPDK BBDEV modified - in DPDK called throughput_pmd_lcore_ldpc_dec, code related to extDdr removed
static int
pmd_lcore_ldpc_dec(void *arg)
{
  struct thread_params *tp = arg;
  uint16_t enq, deq;
  const uint16_t queue_id = tp->queue_id;
  //const uint16_t burst_sz = tp->p_offloadParams->C; //tp->op_params->burst_sz;
  //const uint16_t num_ops = tp->p_offloadParams->C;
  unsigned int num_ops_tmp = 0;
  for (unsigned int h = 0; h < tp->size_offloadParams; ++h)
  {
    num_ops_tmp+=tp->p_offloadParams[h].C;
  }
  const unsigned int num_ops = num_ops_tmp;
  const unsigned int burst_sz = num_ops_tmp; //TODO check if this is accurate
  struct rte_bbdev_dec_op **ops_enq;
  struct rte_bbdev_dec_op **ops_deq;
  ops_enq = (struct rte_bbdev_dec_op **)rte_calloc("struct rte_bbdev_dec_op **ops_enq", num_ops, sizeof(struct rte_bbdev_dec_op *), RTE_CACHE_LINE_SIZE);
  ops_deq = (struct rte_bbdev_dec_op **)rte_calloc("struct rte_bbdev_dec_op **ops_dec", num_ops, sizeof(struct rte_bbdev_dec_op *), RTE_CACHE_LINE_SIZE);
  struct rte_bbdev_dec_op *ref_op = tp->op_params->ref_dec_op;
  //uint8_t r = tp->r;
  uint8_t *harq_pid = tp->harq_pid;
  uint8_t *ulsch_id = tp->ulsch_id;
  struct test_buffers *bufs = NULL;
  unsigned int h, i, j;
  int ret;
  struct rte_bbdev_info info;
  uint16_t num_to_enq;
  uint8_t *p_out = tp->p_out;
  t_nrLDPCoffload_params *p_offloadParams = tp->p_offloadParams;
  uint32_t size_offloadParams = tp->size_offloadParams;

  TEST_ASSERT_SUCCESS((burst_sz > MAX_BURST), "BURST_SIZE should be <= %u", MAX_BURST);
  rte_bbdev_info_get(tp->dev_id, &info);

  bufs = &tp->op_params->q_bufs[GET_SOCKET(info.socket_id)][queue_id];
  while (rte_atomic16_read(&tp->op_params->sync) == SYNC_WAIT)
    rte_pause();

  ret = rte_mempool_get_bulk(tp->op_params->mp_dec, (void **)ops_enq, num_ops);
  /*
  for (j = 0; j < num_ops; ++j) {
    ops_enq[j]->mempool = tp->op_params->mp_dec;
  }
  */
  set_ldpc_dec_op(ops_enq, num_ops, 0, bufs, ref_op, harq_pid, ulsch_id, p_offloadParams, size_offloadParams);
  /* Set counter to validate the ordering */
  /*
  for (j = 0; j < num_ops; ++j)
    ops_enq[j]->opaque_data = (void *)(uintptr_t)j;
  */

  for (j = 0; j < num_ops; ++j) {
    mbuf_reset(ops_enq[j]->ldpc_dec.hard_output.data);
  }

  for (enq = 0, deq = 0; enq < num_ops;) {
    num_to_enq = burst_sz;
    if (unlikely(num_ops - enq < num_to_enq))
      num_to_enq = num_ops - enq;

    enq += rte_bbdev_enqueue_ldpc_dec_ops(tp->dev_id, queue_id, &ops_enq[enq], num_to_enq);
    deq += rte_bbdev_dequeue_ldpc_dec_ops(tp->dev_id, queue_id, &ops_deq[deq], enq - deq);
  }

  /* dequeue the remaining */
  int time_out = 0;
  while (deq < enq) {
    deq += rte_bbdev_dequeue_ldpc_dec_ops(tp->dev_id, queue_id, &ops_deq[deq], enq - deq);
    time_out++;
    DevAssert(time_out <= TIME_OUT_POLL);
  }
  // This if statement is not in DPDK
  if (deq == enq) {
    tp->iter_count = 0;
    /* get the max of iter_count for all dequeued ops */
    for (h = 0; h < size_offloadParams; ++h){
      for (i = 0; i < p_offloadParams[h].C; ++i) {
        uint8_t *status = p_offloadParams[h].status_cb[i];
        tp->iter_count = RTE_MAX(ops_enq[i]->ldpc_dec.iter_count, tp->iter_count);
        *status = ops_enq[i]->status;
      }
    }
    ret = retrieve_ldpc_dec_op(ops_deq, num_ops, ref_op, tp->op_params->vector_mask, p_out);
    TEST_ASSERT_SUCCESS(ret, "Validation failed!");
  }
  /*
  for (int k = 0; k < num_ops; k++) {
    rte_mempool_put_bulk(ops_enq[k]->mempool, (void **)ops_enq, num_ops);
  }
  */
  for (j = 0; j < num_ops; ++j) {
    mbuf_reset(ops_enq[j]->ldpc_dec.input.data);
  }

  rte_bbdev_dec_op_free_bulk(ops_enq, num_ops);
  rte_free(ops_enq);
  rte_free(ops_deq);
  // Return the worst decoding number of iterations for all segments
  return tp->iter_count;
}

// DPDK BBDEV copy - in DPDK called throughput_pmd_lcore_ldpc_enc
static int pmd_lcore_ldpc_enc(void *arg)
{
  struct thread_params *tp = arg;
  uint16_t enq, deq;
  const uint16_t queue_id = tp->queue_id;
  const uint16_t burst_sz = tp->p_offloadParams->C;
  tp->op_params->num_to_process = tp->p_offloadParams->C;
  const uint16_t num_ops = tp->op_params->num_to_process;
  struct rte_bbdev_enc_op *ops_enq[num_ops];
  struct rte_bbdev_enc_op *ops_deq[num_ops];
  struct rte_bbdev_enc_op *ref_op = tp->op_params->ref_enc_op;
  int j, ret;
  struct test_buffers *bufs = NULL;
  uint16_t num_to_enq;
  uint8_t *p_out = tp->p_out;
  t_nrLDPCoffload_params *p_offloadParams = tp->p_offloadParams;

  TEST_ASSERT_SUCCESS((burst_sz > MAX_BURST), "BURST_SIZE should be <= %u", MAX_BURST);

  struct rte_bbdev_info info;
  rte_bbdev_info_get(tp->dev_id, &info);

  TEST_ASSERT_SUCCESS((num_ops > info.drv.queue_size_lim), "NUM_OPS cannot exceed %u for this device", info.drv.queue_size_lim);

  bufs = &tp->op_params->q_bufs[GET_SOCKET(info.socket_id)][queue_id];

  while (rte_atomic16_read(&tp->op_params->sync) == SYNC_WAIT)
    rte_pause();

  ret = rte_mempool_get_bulk(tp->op_params->mp_enc, (void **)ops_enq, num_ops);
  ops_enq[0]->mempool = tp->op_params->mp_enc;

  set_ldpc_enc_op(ops_enq, num_ops, 0, bufs->inputs, bufs->hard_outputs, ref_op, p_offloadParams);

  /* Set counter to validate the ordering */
  for (j = 0; j < num_ops; ++j)
    ops_enq[j]->opaque_data = (void *)(uintptr_t)j;

  for (j = 0; j < num_ops; ++j)
    mbuf_reset(ops_enq[j]->ldpc_enc.output.data);

  for (enq = 0, deq = 0; enq < num_ops;) {
    num_to_enq = burst_sz;
    if (unlikely(num_ops - enq < num_to_enq))
      num_to_enq = num_ops - enq;

    enq += rte_bbdev_enqueue_ldpc_enc_ops(tp->dev_id, queue_id, &ops_enq[enq], num_to_enq);
    deq += rte_bbdev_dequeue_ldpc_enc_ops(tp->dev_id, queue_id, &ops_deq[deq], enq - deq);
  }
  /* dequeue the remaining */
  int time_out = 0;
  while (deq < enq) {
    deq += rte_bbdev_dequeue_ldpc_enc_ops(tp->dev_id, queue_id, &ops_deq[deq], enq - deq);
    time_out++;
    DevAssert(time_out <= TIME_OUT_POLL);
  }

  ret = retrieve_ldpc_enc_op(ops_deq, num_ops, ref_op, p_out, tp->p_offloadParams->E_cb);
  TEST_ASSERT_SUCCESS(ret, "Validation failed!");
  //rte_bbdev_enc_op_free_bulk(ops_enq, num_ops);

  if (num_ops > 0)
    rte_mempool_put_bulk(ops_enq[0]->mempool, (void **)ops_enq, num_ops);
  return ret;
}


/*
 * Test function that determines how long an enqueue + dequeue of a burst
 * takes on available lcores.
 */
// OAI / DPDK BBDEV modified - in DPDK called throughput_test, here we pass more parameters to the function (t_nrLDPCoffload_params
// *p_offloadParams, uint8_t r, ...), many commented lines Removed code which specified which function to use based on the op_type,
// now we are using only pmd_lcore_ldpc_dec for RTE_BBDEV_OP_LDPC_DEC op type. Encoder is RTE_BBDEV_OP_LDPC_ENC op type,
// pmd_lcore_ldpc_enc to be implemented.
int start_pmd_dec(struct active_device *ad,
                  struct test_op_params *op_params,
                  t_nrLDPCoffload_params *p_offloadParams,
                  unsigned int size_offloadParams,
                  //uint8_t r,
                  uint8_t *harq_pid,
                  uint8_t *ulsch_id,
                  uint8_t *p_out)
{
  int ret;
  unsigned int lcore_id, used_cores = 0;
  // struct rte_bbdev_info info;
  uint16_t num_lcores;
  // rte_bbdev_info_get(ad->dev_id, &info);
  /* Set number of lcores */
  num_lcores = (ad->nb_queues < (op_params->num_lcores)) ? ad->nb_queues : op_params->num_lcores;
  /* Allocate memory for thread parameters structure */
  struct thread_params *t_params = rte_zmalloc(NULL, num_lcores * sizeof(struct thread_params), RTE_CACHE_LINE_SIZE);
  TEST_ASSERT_NOT_NULL(t_params,
                       "Failed to alloc %zuB for t_params",
                       RTE_ALIGN(sizeof(struct thread_params) * num_lcores, RTE_CACHE_LINE_SIZE));
  rte_atomic16_set(&op_params->sync, SYNC_WAIT);

  /* Master core is set at first entry */
  t_params[0].dev_id = ad->dev_id;
  t_params[0].lcore_id = 15;
  t_params[0].op_params = op_params;
  t_params[0].queue_id = ad->dec_queue;
  used_cores++;
  t_params[0].iter_count = 0;
  t_params[0].p_out = p_out;
  t_params[0].p_offloadParams = p_offloadParams;
  t_params[0].size_offloadParams = size_offloadParams;
  //t_params[0].r = r;
  t_params[0].harq_pid = harq_pid;
  t_params[0].ulsch_id = ulsch_id;
  // For now, we never enter here, we don't use the DPDK thread pool
  RTE_LCORE_FOREACH_WORKER(lcore_id) {
    if (used_cores >= num_lcores)
      break;
    t_params[used_cores].dev_id = ad->dev_id;
    t_params[used_cores].lcore_id = lcore_id;
    t_params[used_cores].op_params = op_params;
    t_params[used_cores].queue_id = ad->queue_ids[used_cores];
    t_params[used_cores].iter_count = 0;
    t_params[used_cores].p_out = p_out;
    t_params[used_cores].p_offloadParams = p_offloadParams;
    t_params[used_cores].size_offloadParams = size_offloadParams;
    //t_params[used_cores].r = r;
    //t_params[used_cores].harq_pid = harq_pid;
    //t_params[used_cores].ulsch_id = ulsch_id;
    rte_eal_remote_launch(pmd_lcore_ldpc_dec, &t_params[used_cores++], lcore_id);
  }

  rte_atomic16_set(&op_params->sync, SYNC_START);

  ret = pmd_lcore_ldpc_dec(&t_params[0]);

  /* Master core is always used */
  // for (used_cores = 1; used_cores < num_lcores; used_cores++)
  //	ret |= rte_eal_wait_lcore(t_params[used_cores].lcore_id);

  rte_free(t_params);
  return ret;
}

int32_t start_pmd_enc(struct active_device *ad,
                      struct test_op_params *op_params,
                      t_nrLDPCoffload_params *p_offloadParams,
                      uint8_t *p_out)
{
  int ret;
  unsigned int lcore_id, used_cores = 0;

  uint16_t num_lcores;
  num_lcores = (ad->nb_queues < (op_params->num_lcores)) ? ad->nb_queues : op_params->num_lcores;
  /* Allocate memory for thread parameters structure */
  struct thread_params *t_params = rte_zmalloc(NULL, num_lcores * sizeof(struct thread_params), RTE_CACHE_LINE_SIZE);
  TEST_ASSERT_NOT_NULL(t_params,
                       "Failed to alloc %zuB for t_params",
                       RTE_ALIGN(sizeof(struct thread_params) * num_lcores, RTE_CACHE_LINE_SIZE));
  rte_atomic16_set(&op_params->sync, SYNC_WAIT);

  /* Master core is set at first entry */
  t_params[0].dev_id = ad->dev_id;
  t_params[0].lcore_id = 14;
  t_params[0].op_params = op_params;
  // t_params[0].queue_id = ad->queue_ids[used_cores++];
  used_cores++;
  t_params[0].queue_id = ad->enc_queue;
  t_params[0].iter_count = 0;
  t_params[0].p_out = p_out;
  t_params[0].p_offloadParams = p_offloadParams;

  // For now, we never enter here, we don't use the DPDK thread pool
  RTE_LCORE_FOREACH_WORKER(lcore_id) {
    if (used_cores >= num_lcores)
      break;
    t_params[used_cores].dev_id = ad->dev_id;
    t_params[used_cores].lcore_id = lcore_id;
    t_params[used_cores].op_params = op_params;
    t_params[used_cores].queue_id = ad->queue_ids[1];
    t_params[used_cores].iter_count = 0;
    rte_eal_remote_launch(pmd_lcore_ldpc_enc, &t_params[used_cores++], lcore_id);
  }

  rte_atomic16_set(&op_params->sync, SYNC_START);
  ret = pmd_lcore_ldpc_enc(&t_params[0]);
  if (ret) {
    rte_free(t_params);
    return ret;
  }

  rte_free(t_params);
  return ret;
}

struct test_op_params *op_params = NULL;

struct rte_mbuf *m_head[DATA_NUM_TYPES];

static int32_t LDPCdecoder_t2(struct nrLDPC_dec_params *p_decParams,
                    uint8_t *harq_pid,
                    uint8_t *ulsch_id,
                    uint8_t *C,
                    uint16_t n,
                    int8_t *p_llr,
                    int8_t *p_out,
                    t_nrLDPC_time_stats *p_profiler,
                    decode_abort_t *ab)
{
  pthread_mutex_lock(&decode_mutex);
  // hardcoded we use first device
  LOG_D(PHY,"Offload\n");
  struct active_device *ad = active_devs;
  t_nrLDPCoffload_params *p_offloadParams;
  p_offloadParams = (t_nrLDPCoffload_params *)rte_calloc("t_nrLDPCoffload_params *p_offloadParams", n, sizeof(t_nrLDPCoffload_params), RTE_CACHE_LINE_SIZE);
  for(uint16_t h = 0; h < n; ++h){
    p_offloadParams[h].E = p_decParams[h].E;
    p_offloadParams[h].n_cb = (p_decParams[h].BG == 1) ? (66 * p_decParams[h].Z) : (50 * p_decParams[h].Z);
    p_offloadParams[h].BG = p_decParams[h].BG;
    p_offloadParams[h].Z = p_decParams[h].Z;
    p_offloadParams[h].rv = p_decParams[h].rv;
    p_offloadParams[h].F = p_decParams[h].F;
    p_offloadParams[h].Qm = p_decParams[h].Qm;
    p_offloadParams[h].numMaxIter = p_decParams[h].numMaxIter;
    p_offloadParams[h].C = C[h];
    p_offloadParams[h].setCombIn = p_decParams[h].setCombIn;
    for (int r = 0; r < C[h]; r++) {
      p_offloadParams[h].E_cb[r] = p_decParams[h].E_cb[r];
      p_offloadParams[h].status_cb[r] = &p_decParams[h].status_cb[r];
    }
  }
  struct rte_bbdev_info info;
  int ret;
  rte_bbdev_info_get(ad->dev_id, &info);
  int socket_id = GET_SOCKET(info.socket_id);
  // fill_queue_buffers -> init_op_data_objs
  struct rte_mempool *in_mp = ad->in_mbuf_pool;
  struct rte_mempool *hard_out_mp = ad->hard_out_mbuf_pool;
  struct rte_mempool *soft_out_mp = ad->soft_out_mbuf_pool;
  struct rte_mempool *harq_in_mp = ad->harq_in_mbuf_pool;
  struct rte_mempool *harq_out_mp = ad->harq_out_mbuf_pool;
  struct rte_mempool *mbuf_pools[DATA_NUM_TYPES] = {in_mp, soft_out_mp, hard_out_mp, harq_in_mp, harq_out_mp};
  uint8_t queue_id = ad->dec_queue;
  struct rte_bbdev_op_data **queue_ops[DATA_NUM_TYPES] = {&op_params->q_bufs[socket_id][queue_id].inputs,
                                                          &op_params->q_bufs[socket_id][queue_id].soft_outputs,
                                                          &op_params->q_bufs[socket_id][queue_id].hard_outputs,
                                                          &op_params->q_bufs[socket_id][queue_id].harq_inputs,
                                                          &op_params->q_bufs[socket_id][queue_id].harq_outputs};
  uint16_t num_blocks = 0;
  for(uint16_t h = 0; h < n; ++h){
    num_blocks += C[h];
  }
  // this should be modified
  // enum rte_bbdev_op_type op_type = RTE_BBDEV_OP_LDPC_DEC;
  for (enum op_data_type type = DATA_INPUT; type < 3; type += 2) {
    ret = allocate_buffers_on_socket(queue_ops[type], num_blocks * sizeof(struct rte_bbdev_op_data), socket_id);
    TEST_ASSERT_SUCCESS(ret, "Couldn't allocate memory for rte_bbdev_op_data structs");
    //rte_pktmbuf_free(m_head[type]);
    ret = init_op_data_objs_dec(*queue_ops[type],
                                (uint8_t *)p_llr,
                                p_offloadParams,
                                mbuf_pools[type],
                                n,
                                type,
                                info.drv.min_alignment);
    TEST_ASSERT_SUCCESS(ret, "Couldn't init rte_bbdev_op_data structs");
  }
  ret = start_pmd_dec(ad, op_params, p_offloadParams, n, harq_pid, ulsch_id, (uint8_t *)p_out);
  if (ret < 0) {
    printf("Couldn't start pmd dec\n");
    pthread_mutex_unlock(&decode_mutex);
    return (p_decParams->numMaxIter); // Fix me: we should propoagate max_iterations properly in the call (impp struct)
  }
  rte_free(p_offloadParams);
  pthread_mutex_unlock(&decode_mutex);
  return ret;
}

static int32_t LDPCencoder_t2(unsigned char **input, unsigned char **output, encoder_implemparams_t *impp)
{
  pthread_mutex_lock(&encode_mutex);
  // hardcoded to use the first found board
  struct active_device *ad = active_devs;
  int ret;
  uint32_t Nref = 0;
  t_nrLDPCoffload_params offloadParams = {.E = impp->E,
                                          .n_cb = (impp->BG == 1) ? (66 * impp->Zc) : (50 * impp->Zc),
                                          .BG = impp->BG,
                                          .Z = impp->Zc,
                                          .rv = impp->rv,
                                          .F = impp->F,
                                          .Qm = impp->Qm,
                                          .C = impp->n_segments,
                                          .Kr = (impp->K - impp->F + 7) / 8};
  for (int r = 0; r < impp->n_segments; r++) {
    offloadParams.E_cb[r] = impp->E_cb[r];
  }
  if (impp->Tbslbrm != 0){
    Nref = 3*impp->Tbslbrm/(2*impp->n_segments);
    offloadParams.n_cb = min(offloadParams.n_cb, Nref);
  }
  struct rte_bbdev_info info;
  rte_bbdev_info_get(ad->dev_id, &info);
  int socket_id = GET_SOCKET(info.socket_id);
  // fill_queue_buffers -> init_op_data_objs
  struct rte_mempool *in_mp = ad->in_mbuf_pool;
  struct rte_mempool *hard_out_mp = ad->hard_out_mbuf_pool;
  struct rte_mempool *soft_out_mp = ad->soft_out_mbuf_pool;
  struct rte_mempool *harq_in_mp = ad->harq_in_mbuf_pool;
  struct rte_mempool *harq_out_mp = ad->harq_out_mbuf_pool;
  struct rte_mempool *mbuf_pools[DATA_NUM_TYPES] = {in_mp, soft_out_mp, hard_out_mp, harq_in_mp, harq_out_mp};
  uint8_t queue_id = ad->enc_queue;
  struct rte_bbdev_op_data **queue_ops[DATA_NUM_TYPES] = {&op_params->q_bufs[socket_id][queue_id].inputs,
                                                          &op_params->q_bufs[socket_id][queue_id].soft_outputs,
                                                          &op_params->q_bufs[socket_id][queue_id].hard_outputs,
                                                          &op_params->q_bufs[socket_id][queue_id].harq_inputs,
                                                          &op_params->q_bufs[socket_id][queue_id].harq_outputs};
  for (enum op_data_type type = DATA_INPUT; type < 3; type += 2) {
    ret = allocate_buffers_on_socket(queue_ops[type], impp->n_segments * sizeof(struct rte_bbdev_op_data), socket_id);
    TEST_ASSERT_SUCCESS(ret, "Couldn't allocate memory for rte_bbdev_op_data structs");
    //m_head[type] = rte_pktmbuf_alloc(mbuf_pools[type]);
    ret = init_op_data_objs_enc(*queue_ops[type],
                                input,
                                &offloadParams,
                                m_head[type],
                                mbuf_pools[type],
                                impp->n_segments,
                                type,
                                info.drv.min_alignment);
    TEST_ASSERT_SUCCESS(ret, "Couldn't init rte_bbdev_op_data structs");
  }
  ret = start_pmd_enc(ad, op_params, &offloadParams, *output);
  pthread_mutex_unlock(&encode_mutex);
  return ret;
}

static int decode_offload_t2(PHY_VARS_gNB *phy_vars_gNB,
                   uint8_t *ULSCH_id,
                   uint16_t nb_ULSCH_id,
                   short **ulsch_llr,
                   nfapi_nr_pusch_pdu_t **pusch_pdu,
                   t_nrLDPC_dec_params *decParams,
                   uint8_t *harq_pid,
                   uint32_t *G)
{
  NR_gNB_ULSCH_t *ulsch;
  NR_UL_gNB_HARQ_t *harq_process;
  int16_t z_ol[NR_LDPC_MAX_NUM_CB * LDPC_MAX_CB_SIZE] __attribute__((aligned(16)));
  int8_t l_ol[NR_LDPC_MAX_NUM_CB * LDPC_MAX_CB_SIZE] __attribute__((aligned(16)));
  uint32_t A;
  int Kr;
  int Kr_bytes;
  int8_t decodeIterations = 0;
  int r_offset = 0;
  int offset = 0;

  uint8_t *C;
  C = (uint8_t *)calloc(nb_ULSCH_id, sizeof(uint8_t));
  unsigned int size_outDec_tmp = 0;

  for (uint16_t h = 0; h < nb_ULSCH_id; h++) {
    r_offset = 0;
    ulsch = &phy_vars_gNB->ulsch[ULSCH_id[h]];
    harq_process = ulsch->harq_process;
    A = (harq_process->TBS) << 3;
    Kr = harq_process->K;
    Kr_bytes = Kr >> 3;
    // new data received, set processedSegments to 0
    if (!decParams[h].setCombIn)
      harq_process->processedSegments = 0;

    for (int r = 0; r < harq_process->C; r++) {
      decParams[h].E_cb[r] = nr_get_E(G[ULSCH_id[h]], harq_process->C, decParams[h].Qm, pusch_pdu[h]->nrOfLayers, r);
      memcpy(&z_ol[offset], ulsch_llr[h] + r_offset, decParams[h].E_cb[r] * sizeof(short));
      simde__m128i *pv_ol128 = (simde__m128i *)&z_ol[offset];
      simde__m128i *pl_ol128 = (simde__m128i *)&l_ol[offset];
      int kc = decParams[h].BG == 2 ? 52 : 68;
      for (int i = 0, j = 0; j < ((kc * harq_process->Z) >> 4) + 1; i += 2, j++) {
        pl_ol128[j] = simde_mm_packs_epi16(pv_ol128[i], pv_ol128[i + 1]);
      }
      decParams[h].F = harq_process->F;
      r_offset += decParams[h].E_cb[r];
      offset += LDPC_MAX_CB_SIZE;
    }
    size_outDec_tmp += harq_process->C * Kr_bytes;
    C[h] = harq_process->C;
  }

  const unsigned int size_outDec = size_outDec_tmp;

  int8_t *p_outDec = calloc(size_outDec, sizeof(int8_t));
  decodeIterations = LDPCdecoder_t2(decParams, harq_pid, ULSCH_id, C, nb_ULSCH_id, (int8_t *)l_ol, p_outDec, NULL, NULL);

  if (decodeIterations < 0) {
    LOG_E(PHY, "ulsch_decoding.c: Problem in LDPC decoder offload\n");
    return -1;
  }

  int offset_p_out = 0;
  for (uint16_t h = 0; h < nb_ULSCH_id; h++) {
    ulsch = &phy_vars_gNB->ulsch[ULSCH_id[h]];
    harq_process = ulsch->harq_process;
    A = (harq_process->TBS) << 3;
    Kr = harq_process->K;
    Kr_bytes = Kr >> 3;
    int offset_b = 0;
    for (int r = 0; r < harq_process->C; r++) {
      if (decParams[h].status_cb[r] == 0 || harq_process->C == 1) {
        memcpy(harq_process->b + offset_b, &p_outDec[offset_p_out], Kr_bytes - (harq_process->F >> 3) - ((harq_process->C > 1) ? 3 : 0));
        harq_process->processedSegments++;
      }
      offset_p_out += (Kr_bytes - (harq_process->F >> 3) - ((harq_process->C > 1) ? 3 : 0));
      offset_b += (Kr_bytes - (harq_process->F >> 3) - ((harq_process->C > 1) ? 3 : 0));
    }
  
    bool crc_valid = false;
    //CRC check made by the T2, no need to perform CRC check for a single code block twice
    if (harq_process->processedSegments == harq_process->C) {
      crc_valid = check_crc(harq_process->b, lenWithCrc(1, A), crcType(1, A));
      if (harq_process->C == 1 && !crc_valid) {
        harq_process->processedSegments--;
      }
    }
  
    if (crc_valid) {
      LOG_D(PHY, "ULSCH: Setting ACK for slot %d TBS %d\n", ulsch->slot, harq_process->TBS);
      ulsch->active = false;
      harq_process->round = 0;
      LOG_D(PHY, "ULSCH received ok \n");
      nr_fill_indication(phy_vars_gNB, ulsch->frame, ulsch->slot, ULSCH_id[h], harq_pid[h], 0, 0);
    } else {
      LOG_D(PHY,
          "[gNB %d] ULSCH: Setting NAK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d)\n",
          phy_vars_gNB->Mod_id,
          ulsch->frame,
          ulsch->slot,
          harq_pid[h],
          ulsch->active,
          harq_process->round,
          harq_process->TBS);
      ulsch->handled = 1;
      decodeIterations = ulsch->max_ldpc_iterations + 1;
      LOG_D(PHY, "ULSCH %d in error\n", ULSCH_id[h]);
      nr_fill_indication(phy_vars_gNB, ulsch->frame, ulsch->slot, ULSCH_id[h], harq_pid[h], 1, 0);
    }
  
    ulsch->last_iteration_cnt = decodeIterations;
  }
  free(p_outDec);
  free(C);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_gNB_ULSCH_DECODING,0);
  return 0;
}

static int nr_ulsch_decoding_t2(PHY_VARS_gNB *phy_vars_gNB,
                      uint8_t *ULSCH_id,
                      uint16_t nb_ULSCH_id,
                      short **ulsch_llr,
                      NR_DL_FRAME_PARMS *frame_parms,
                      nfapi_nr_pusch_pdu_t **pusch_pdu,
                      uint32_t frame,
                      uint8_t nr_tti_rx,
                      uint8_t *harq_pid,
                      uint32_t *G)
{
  if (!ulsch_llr) {
    LOG_E(PHY, "ulsch_decoding.c: NULL ulsch_llr pointer\n");
    return -1;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_gNB_ULSCH_DECODING, 1);

  t_nrLDPC_dec_params *decParams = calloc(nb_ULSCH_id, sizeof(t_nrLDPC_dec_params));

  for(uint16_t h = 0; h < nb_ULSCH_id; h++){

    NR_gNB_ULSCH_t *ulsch = &phy_vars_gNB->ulsch[ULSCH_id[h]];
    NR_gNB_PUSCH *pusch = &phy_vars_gNB->pusch_vars[ULSCH_id[h]];
    NR_UL_gNB_HARQ_t *harq_process = ulsch->harq_process;

    if (!harq_process) {
      LOG_E(PHY, "ulsch_decoding.c: NULL harq_process pointer\n");
      return -1;
    }

    // ------------------------------------------------------------------
    const uint16_t nb_rb = pusch_pdu[h]->rb_size;
    const uint8_t Qm = pusch_pdu[h]->qam_mod_order;
    const uint8_t mcs = pusch_pdu[h]->mcs_index;
    const uint8_t n_layers = pusch_pdu[h]->nrOfLayers;
    // ------------------------------------------------------------------

    harq_process->TBS = pusch_pdu[h]->pusch_data.tb_size;

    decParams[h].check_crc = check_crc;
    decParams[h].BG = pusch_pdu[h]->maintenance_parms_v3.ldpcBaseGraph;
    const uint32_t A = (harq_process->TBS) << 3;
    NR_gNB_PHY_STATS_t *stats = get_phy_stats(phy_vars_gNB, ulsch->rnti);
    if (stats) {
      stats->frame = frame;
      stats->ulsch_stats.round_trials[harq_process->round]++;
      for (int aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
        stats->ulsch_stats.power[aarx] = dB_fixed_x10(pusch->ulsch_power[aarx]);
        stats->ulsch_stats.noise_power[aarx] = dB_fixed_x10(pusch->ulsch_noise_power[aarx]);
      }
      if (!harq_process->harq_to_be_cleared) {
        stats->ulsch_stats.current_Qm = Qm;
        stats->ulsch_stats.current_RI = n_layers;
        stats->ulsch_stats.total_bytes_tx += harq_process->TBS;
      }
    }

    LOG_D(PHY,
          "ULSCH Decoding, harq_pid %d rnti %x TBS %d G %d mcs %d Nl %d nb_rb %d, Qm %d, Coderate %f RV %d round %d new RX %d\n",
          harq_pid[h],
          ulsch->rnti,
          A,
          G[ULSCH_id[h]],
          mcs,
          n_layers,
          nb_rb,
          Qm,
          pusch_pdu[h]->target_code_rate / 10240.0f,
          pusch_pdu[h]->pusch_data.rv_index,
          harq_process->round,
          harq_process->harq_to_be_cleared);

    // [hna] Perform nr_segmenation with input and output set to NULL to calculate only (C, K, Z, F)
    nr_segmentation(NULL,
                    NULL,
                    lenWithCrc(1, A), // size in case of 1 segment
                    &harq_process->C,
                    &harq_process->K,
                    &harq_process->Z, // [hna] Z is Zc
                    &harq_process->F,
                    decParams[h].BG);

    uint16_t a_segments = MAX_NUM_NR_ULSCH_SEGMENTS_PER_LAYER * n_layers; // number of segments to be allocated
    if (harq_process->C > a_segments) {
      LOG_E(PHY, "nr_segmentation.c: too many segments %d, A %d\n", harq_process->C, A);
      return(-1);
    }
    if (nb_rb != 273) {
      a_segments = a_segments*nb_rb;
      a_segments = a_segments/273 +1;
    }
    if (harq_process->C > a_segments) {
      LOG_E(PHY,"Illegal harq_process->C %d > %d\n",harq_process->C,a_segments);
      return -1;
    }

#ifdef DEBUG_ULSCH_DECODING
    printf("ulsch decoding nr segmentation Z %d\n", harq_process->Z);
    if (!frame % 100)
      printf("K %d C %d Z %d \n", harq_process->K, harq_process->C, harq_process->Z);
    printf("Segmentation: C %d, K %d\n",harq_process->C,harq_process->K);
#endif

    decParams[h].Z = harq_process->Z;
    decParams[h].numMaxIter = ulsch->max_ldpc_iterations;
    decParams[h].Qm = Qm;
    decParams[h].rv = pusch_pdu[h]->pusch_data.rv_index;
    decParams[h].outMode = 0;
    decParams[h].setCombIn = !harq_process->harq_to_be_cleared;
    if (harq_process->harq_to_be_cleared) {
      for (int r = 0; r < harq_process->C; r++)
        harq_process->d_to_be_cleared[r] = true;
      harq_process->harq_to_be_cleared = false;
    }
  }

  int ret = decode_offload_t2(phy_vars_gNB, ULSCH_id, nb_ULSCH_id, ulsch_llr, pusch_pdu, decParams, harq_pid, G);
  
  free(decParams);

  return ret;
}

static int nr_dlsch_encoding_t2(PHY_VARS_gNB *gNB,
                      int frame,
                      uint8_t slot,
                      NR_DL_gNB_HARQ_t *harq,
                      NR_DL_FRAME_PARMS *frame_parms,
                      unsigned char *output,
                      time_stats_t *tinput,
                      time_stats_t *tprep,
                      time_stats_t *tparity,
                      time_stats_t *toutput,
                      time_stats_t *dlsch_rate_matching_stats,
                      time_stats_t *dlsch_interleaving_stats,
                      time_stats_t *dlsch_segmentation_stats)
{
  encoder_implemparams_t impp;
  impp.output=output;
  unsigned int crc=1;
  nfapi_nr_dl_tti_pdsch_pdu_rel15_t *rel15 = &harq->pdsch_pdu.pdsch_pdu_rel15;
  impp.Zc = harq->Z;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_DLSCH_ENCODING, VCD_FUNCTION_IN);
  uint32_t A = rel15->TBSize[0]<<3;
  unsigned char *a=harq->pdu;
  if (rel15->rnti != SI_RNTI)
    trace_NRpdu(DIRECTION_DOWNLINK, a, rel15->TBSize[0], WS_C_RNTI, rel15->rnti, frame, slot,0, 0);

  NR_gNB_PHY_STATS_t *phy_stats = NULL;
  if (rel15->rnti != 0xFFFF)
    phy_stats = get_phy_stats(gNB, rel15->rnti);

  if (phy_stats) {
    phy_stats->frame = frame;
    phy_stats->dlsch_stats.total_bytes_tx += rel15->TBSize[0];
    phy_stats->dlsch_stats.current_RI = rel15->nrOfLayers;
    phy_stats->dlsch_stats.current_Qm = rel15->qamModOrder[0];
  }

  int max_bytes = MAX_NUM_NR_DLSCH_SEGMENTS_PER_LAYER*rel15->nrOfLayers*1056;
  int B;
  if (A > NR_MAX_PDSCH_TBS) {
    // Add 24-bit crc (polynomial A) to payload
    crc = crc24a(a,A)>>8;
    a[A>>3] = ((uint8_t *)&crc)[2];
    a[1+(A>>3)] = ((uint8_t *)&crc)[1];
    a[2+(A>>3)] = ((uint8_t *)&crc)[0];
    //printf("CRC %x (A %d)\n",crc,A);
    //printf("a0 %d a1 %d a2 %d\n", a[A>>3], a[1+(A>>3)], a[2+(A>>3)]);
    B = A + 24;
    //    harq->b = a;
    AssertFatal((A / 8) + 4 <= max_bytes,
                "A %d is too big (A/8+4 = %d > %d)\n",
                A,
                (A / 8) + 4,
                max_bytes);
    memcpy(harq->b, a, (A / 8) + 4); // why is this +4 if the CRC is only 3 bytes?
  } else {
    // Add 16-bit crc (polynomial A) to payload
    crc = crc16(a,A)>>16;
    a[A>>3] = ((uint8_t *)&crc)[1];
    a[1+(A>>3)] = ((uint8_t *)&crc)[0];
    //printf("CRC %x (A %d)\n",crc,A);
    //printf("a0 %d a1 %d \n", a[A>>3], a[1+(A>>3)]);
    B = A + 16;
    //    harq->b = a;
    AssertFatal((A / 8) + 3 <= max_bytes,
                "A %d is too big (A/8+3 = %d > %d)\n",
                A,
                (A / 8) + 3,
                max_bytes);
    memcpy(harq->b, a, (A / 8) + 3); // using 3 bytes to mimic the case of 24 bit crc
  }

  impp.BG = rel15->maintenance_parms_v3.ldpcBaseGraph;

  start_meas(dlsch_segmentation_stats);
  impp.Kb = nr_segmentation(harq->b, harq->c, B, &impp.n_segments, &impp.K, &impp.Zc, &impp.F, impp.BG);
  stop_meas(dlsch_segmentation_stats);

  if (impp.n_segments>MAX_NUM_NR_DLSCH_SEGMENTS_PER_LAYER*rel15->nrOfLayers) {
    LOG_E(PHY, "nr_segmentation.c: too many segments %d, B %d\n", impp.n_segments, B);
    return(-1);
  }
  for (int r=0; r<impp.n_segments; r++) {
    //d_tmp[r] = &harq->d[r][0];
    //channel_input[r] = &harq->d[r][0];
#ifdef DEBUG_DLSCH_CODING
    LOG_D(PHY,"Encoder: B %d F %d \n",harq->B, impp.F);
    LOG_D(PHY,"start ldpc encoder segment %d/%d\n",r,impp.n_segments);
    LOG_D(PHY,"input %d %d %d %d %d \n", harq->c[r][0], harq->c[r][1], harq->c[r][2],harq->c[r][3], harq->c[r][4]);

    for (int cnt =0 ; cnt < 22*(*impp.Zc)/8; cnt ++) {
      LOG_D(PHY,"%d ", harq->c[r][cnt]);
    }

    LOG_D(PHY,"\n");
#endif
    //ldpc_encoder_orig((unsigned char*)harq->c[r],harq->d[r],*Zc,Kb,Kr,BG,0);
    //ldpc_encoder_optim((unsigned char*)harq->c[r],(unsigned char*)&harq->d[r][0],*Zc,Kb,Kr,BG,NULL,NULL,NULL,NULL);
  }

  impp.tprep = tprep;
  impp.tinput = tinput;
  impp.tparity = tparity;
  impp.toutput = toutput;
  impp.harq = harq;
  impp.Qm = rel15->qamModOrder[0];
  impp.Tbslbrm = rel15->maintenance_parms_v3.tbSizeLbrmBytes;
  impp.rv = rel15->rvIndex[0];
  int nb_re_dmrs =
      (rel15->dmrsConfigType == NFAPI_NR_DMRS_TYPE1) ? (6 * rel15->numDmrsCdmGrpsNoData) : (4 * rel15->numDmrsCdmGrpsNoData);
  impp.G = nr_get_G(rel15->rbSize,
                    rel15->NrOfSymbols,
                    nb_re_dmrs,
                    get_num_dmrs(rel15->dlDmrsSymbPos),
                    harq->unav_res,
                    rel15->qamModOrder[0],
                    rel15->nrOfLayers);
  for (int r = 0; r < impp.n_segments; r++) {
    impp.E_cb[r] = nr_get_E(impp.G, impp.n_segments, impp.Qm, rel15->nrOfLayers, r);
  }
  LDPCencoder_t2(harq->c, &impp.output, &impp);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_DLSCH_ENCODING, VCD_FUNCTION_OUT);
  return 0;
}

// OAI CODE
int32_t nr_ulsch_decoding_init()
{
  pthread_mutex_init(&encode_mutex, NULL);
  pthread_mutex_init(&decode_mutex, NULL);
  int ret;
  int dev_id = 0;
  struct rte_bbdev_info info;
  struct active_device *ad = active_devs;
  char *dpdk_dev = "41:00.0"; //PCI address of the card
  char *argv_re[] = {"bbdev", "-a", dpdk_dev, "-l", "8-9", "--file-prefix=b6", "--"};
  // EAL initialization, if already initialized (init in xran lib) try to probe DPDK device
  ret = rte_eal_init(7, argv_re);
  if (ret < 0) {
    printf("EAL initialization failed, probing DPDK device %s\n", dpdk_dev);
    if (rte_dev_probe(dpdk_dev) != 0) {
      LOG_E(PHY, "T2 card %s not found\n", dpdk_dev);
      return (-1);
    }
  }
  // Use only device 0 - first detected device
  rte_bbdev_info_get(0, &info);
  // Set number of queues based on number of initialized cores (-l option) and driver
  // capabilities
  TEST_ASSERT_SUCCESS(add_dev(dev_id, &info), "Failed to setup bbdev");
  TEST_ASSERT_SUCCESS(rte_bbdev_stats_reset(dev_id), "Failed to reset stats of bbdev %u", dev_id);
  TEST_ASSERT_SUCCESS(rte_bbdev_start(dev_id), "Failed to start bbdev %u", dev_id);

  //the previous calls have populated this global variable (beurk)
  // One more global to remove, not thread safe global op_params
  op_params = rte_zmalloc(NULL, sizeof(struct test_op_params), RTE_CACHE_LINE_SIZE);
  TEST_ASSERT_NOT_NULL(op_params, "Failed to alloc %zuB for op_params",
                       RTE_ALIGN(sizeof(struct test_op_params), RTE_CACHE_LINE_SIZE));

  int socket_id = GET_SOCKET(info.socket_id);
  int out_max_sz = 8448; // max code block size (for BG1), 22 * 384
  int in_max_sz = LDPC_MAX_CB_SIZE; // max number of encoded bits (for BG2 and MCS0)
  int num_queues = 1;
  int f_ret = create_mempools(ad, socket_id, num_queues, out_max_sz, in_max_sz);
  if (f_ret != TEST_SUCCESS) {
    printf("Couldn't create mempools");
    return -1;
  }
  // get_num_lcores() hardcoded to 1: we use one core for decode, and another for encode
  // this code from bbdev test example is not considering encode and decode test
  // get_num_ops() replaced by 1: LDPC decode and ldpc encode (7th param)
  f_ret = init_test_op_params(op_params, RTE_BBDEV_OP_LDPC_DEC, ad->bbdev_dec_op_pool, num_queues, num_queues, 1);
  f_ret = init_test_op_params(op_params, RTE_BBDEV_OP_LDPC_ENC, ad->bbdev_enc_op_pool, num_queues, num_queues, 1);
  if (f_ret != TEST_SUCCESS) {
    printf("Couldn't init test op params");
    return -1;
  }
  return 0;
}

int32_t nr_ulsch_decoding_shutdown()
{
  struct active_device *ad = active_devs;
  int dev_id = 0;
  struct rte_bbdev_stats stats;
  free_buffers(ad, op_params);
  rte_free(op_params);
  // Stop and close bbdev
  rte_bbdev_stats_get(dev_id, &stats);
  rte_bbdev_stop(dev_id);
  rte_bbdev_close(dev_id);
  memset(active_devs, 0, sizeof(active_devs));
  nb_active_devs = 0;
  return 0;
}

int32_t nr_ulsch_decoding_decoder(PHY_VARS_gNB *gNB, NR_DL_FRAME_PARMS *frame_parms, int frame_rx, int slot_rx, uint32_t *G){

  uint16_t nb_ulsch = 0;
  for (unsigned int ulsch_index = 0; ulsch_index < gNB->max_nb_pusch; ulsch_index++) {
    NR_gNB_ULSCH_t *ulsch = &gNB->ulsch[ulsch_index];
    if ((ulsch->active == true) && (ulsch->frame == frame_rx) && (ulsch->slot == slot_rx) && (ulsch->handled == 0)) {
      nb_ulsch++;
    }
  }

  if(nb_ulsch==0)
    return 0;

  uint8_t *ULSCH_id = calloc(nb_ulsch, sizeof(int));
  uint8_t *harq_pid = calloc(nb_ulsch, sizeof(harq_pid));
  nfapi_nr_pusch_pdu_t **pusch_pdu = calloc(nb_ulsch, sizeof(nfapi_nr_pusch_pdu_t *));
  short **llr = calloc(nb_ulsch, sizeof(short *));

  uint16_t h = 0;
  for (unsigned int ulsch_index = 0; ulsch_index < gNB->max_nb_pusch; ulsch_index++) {
    NR_gNB_ULSCH_t *ulsch = &gNB->ulsch[ulsch_index];
    if ((ulsch->active == true) && (ulsch->frame == frame_rx) && (ulsch->slot == slot_rx) && (ulsch->handled == 0)) {
      ULSCH_id[h] = ulsch_index;
      harq_pid[h] = ulsch->harq_pid; 
      pusch_pdu[h] = &gNB->ulsch[h].harq_process->ulsch_pdu;
      llr[h] = gNB->pusch_vars[h].llr;
      h++;
    }
  }

  int nbDecode = nr_ulsch_decoding_t2(gNB, ULSCH_id, nb_ulsch, llr, frame_parms, pusch_pdu, frame_rx, slot_rx, harq_pid, G);

  free(ULSCH_id);
  free(harq_pid);
  free(pusch_pdu);

  return nbDecode;

}

int nr_ulsch_decoding_encoder(PHY_VARS_gNB *gNB,
                              processingData_L1tx_t *msgTx,
                              int frame,
                              uint8_t slot,
                              NR_DL_FRAME_PARMS *frame_parms,
                              unsigned char **output,
                              time_stats_t *tinput,
                              time_stats_t *tprep,
                              time_stats_t *tparity,
                              time_stats_t *toutput,
                              time_stats_t *dlsch_rate_matching_stats,
                              time_stats_t *dlsch_interleaving_stats,
                              time_stats_t *dlsch_segmentation_stats)
{

  int nbEncode = 0;
  for (int dlsch_id = 0; dlsch_id < msgTx->num_pdsch_slot; dlsch_id++) {
    NR_gNB_DLSCH_t *dlsch = msgTx->dlsch[dlsch_id];

    NR_DL_gNB_HARQ_t *harq = &dlsch->harq_process;
    nbEncode += nr_dlsch_encoding_t2(gNB,
                                  frame,
                                  slot,
                                  harq,
                                  frame_parms,
                                  output[dlsch_id],
                                  tinput,
                                  tprep,
                                  tparity,
                                  toutput,
                                  dlsch_rate_matching_stats,
                                  dlsch_interleaving_stats,
                                  dlsch_segmentation_stats);
  }

  return nbEncode;

}

