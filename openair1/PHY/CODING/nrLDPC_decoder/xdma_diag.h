/*
 * This file is part of the Xilinx DMA IP Core driver tools for Linux
 *
 * Copyright (c) 2016-present,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */
#ifndef MODULES_TXCTRL_INC_XDMA_DIAG_H_
#define MODULES_TXCTRL_INC_XDMA_DIAG_H_

#ifdef __cplusplus
extern "C" {
#endif

// #define _BSD_SOURCE
// #define _XOPEN_SOURCE 500
// #include "../../LDPC/LDPC_api.h"

// #include "dma_utils.c"

/*static struct option const long_opts[] = {
  {"device", required_argument, NULL, 'd'},
  {"address", required_argument, NULL, 'a'},
  {"size", required_argument, NULL, 's'},
  {"offset", required_argument, NULL, 'o'},
  {"count", required_argument, NULL, 'c'},
  {"data infile", required_argument, NULL, 'f'},
  {"data outfile", required_argument, NULL, 'w'},
  {"help", no_argument, NULL, 'h'},
  {"verbose", no_argument, NULL, 'v'},
  {0, 0, 0, 0}
};*/

typedef struct {
  unsigned char max_schedule; // max_schedule = 0;
  unsigned char mb; // mb = 32;
  unsigned char CB_num; // id = CB_num;
  unsigned char BGSel; // bg = 1;
  unsigned char z_set; // z_set = 0;
  unsigned char z_j; // z_j = 6;
  unsigned char max_iter; // max_iter = 8;
  unsigned char SetIdx; // sc_idx = 12;
} DecIPConf;

typedef struct {
  int SetIdx;
  int NumCBSegm;
  int PayloadLen;
  int Z;
  int z_set;
  int z_j;
  int Kbmax;
  int BGSel;
  unsigned mb;
  unsigned char CB_num;
  unsigned char kb_1;
} EncIPConf;

/* ltoh: little to host */
/* htol: little to host */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ltohl(x) (x)
#define ltohs(x) (x)
#define htoll(x) (x)
#define htols(x) (x)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define ltohl(x) __bswap_32(x)
#define ltohs(x) __bswap_16(x)
#define htoll(x) __bswap_32(x)
#define htols(x) __bswap_16(x)
#endif

#define FATAL                                                                                             \
  do {                                                                                                    \
    fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", __LINE__, __FILE__, errno, strerror(errno)); \
    exit(1);                                                                                              \
  } while (0)

#define MAP_SIZE (32 * 1024UL)
#define MAP_MASK (MAP_SIZE - 1)

#define DEVICE_NAME_DEFAULT_ENC_READ "/dev/xdma0_c2h_1"
#define DEVICE_NAME_DEFAULT_ENC_WRITE "/dev/xdma0_h2c_1"
#define DEVICE_NAME_DEFAULT_DEC_READ "/dev/xdma0_c2h_0"
#define DEVICE_NAME_DEFAULT_DEC_WRITE "/dev/xdma0_h2c_0"
#define SIZE_DEFAULT (32)
#define COUNT_DEFAULT (1)

#define OFFSET_DEC_IN 0x0000
#define OFFSET_DEC_OUT 0x0004
#define OFFSET_ENC_IN 0x0008
#define OFFSET_ENC_OUT 0x000c
#define OFFSET_RESET 0x0020
#define PCIE_OFF 0x0030

#define CB_PROCESS_NUMBER 24 // add by JW
#define CB_PROCESS_NUMBER_Dec 24

// dma_from_device.c

int test_dma_enc_read(char *EncOut, EncIPConf Confparam);
int test_dma_enc_write(char *data, EncIPConf Confparam);
int test_dma_dec_read(char *DecOut, DecIPConf Confparam);
int test_dma_dec_write(char *data, DecIPConf Confparam);
void test_dma_init();
void test_dma_shutdown();
void dma_reset();

#ifdef __cplusplus
}
#endif
#endif
