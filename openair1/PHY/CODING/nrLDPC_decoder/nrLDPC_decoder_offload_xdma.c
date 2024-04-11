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

#define _BSD_SOURCE
#define _XOPEN_SOURCE 500

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <byteswap.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include "xdma_diag.h"
#include "nrLDPC_decoder_offload_xdma.h"

typedef unsigned long long U64;
static struct option const long_opts[] = {{"device", required_argument, NULL, 'd'},
                                          {"address", required_argument, NULL, 'a'},
                                          {"size", required_argument, NULL, 's'},
                                          {"offset", required_argument, NULL, 'o'},
                                          {"count", required_argument, NULL, 'c'},
                                          {"data infile", required_argument, NULL, 'f'},
                                          {"data outfile", required_argument, NULL, 'w'},
                                          {"help", no_argument, NULL, 'h'},
                                          {"verbose", no_argument, NULL, 'v'},
                                          {0, 0, 0, 0}};
#if 0
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

#define DEVICE_NAME_DEFAULT_READ "/dev/xdma0_c2h_1"
#define DEVICE_NAME_DEFAULT_WRITE "/dev/xdma0_h2c_1"
#define SIZE_DEFAULT (32)
#define COUNT_DEFAULT (1)

#define OFFSET_DEC_IN 0x0000
#define OFFSET_DEC_OUT 0x0004
#define OFFSET_ENC_IN 0x0008
#define OFFSET_ENC_OUT 0x000c
#define OFFSET_RESET 0x0020

#define CB_PROCESS_NUMBER 12 // add by JW
#endif
void* map_base;
int fd;
int fd_enc_write, fd_enc_read;
int fd_dec_write, fd_dec_read;
char *allocated_write, *allocated_read;

// dma_from_device.c
#if 0
int test_dma_end_read();
int test_dma_enc_write();
#endif
static int no_write = 0;

// [Start] #include "dma_utils.c" ===================================

/*
 * man 2 write:
 * On Linux, write() (and similar system calls) will transfer at most
 * 	0x7ffff000 (2,147,479,552) bytes, returning the number of bytes
 *	actually transferred.  (This is true on both 32-bit and 64-bit
 *	systems.)
 */

#define RW_MAX_SIZE 0x7ffff000

int verbose = 0;

uint64_t getopt_integer(char* optarg)
{
  int rc;
  uint64_t value;

  rc = sscanf(optarg, "0x%lx", &value);
  if (rc <= 0)
    rc = sscanf(optarg, "%lu", &value);
  // printf("sscanf() = %d, value = 0x%lx\n", rc, value);

  return value;
}

ssize_t read_to_buffer(char* fname, int fd, char* buffer, uint64_t size, uint64_t base)
{
  ssize_t rc;
  uint64_t count = 0;
  char* buf = buffer;
  off_t offset = base;

  while (count < size) {
    uint64_t bytes = size - count;

    if (bytes > RW_MAX_SIZE)
      bytes = RW_MAX_SIZE;

    if (offset) {
      rc = lseek(fd, offset, SEEK_SET);
      if (rc != offset) {
        fprintf(stderr, "%s, seek off 0x%lx != 0x%lx.\n", fname, rc, offset);
        perror("seek file");
        return -EIO;
      }
    }
    /* read data from file into memory buffer */
    rc = read(fd, buf, bytes);

    if (rc != bytes) {
      fprintf(stderr, "%s, R off 0x%lx, 0x%lx != 0x%lx.\n", fname, count, rc, bytes);
      perror("read file");
      return -EIO;
    }

    count += bytes;
    buf += bytes;
    offset += bytes;
  }

  if (count != size) {
    fprintf(stderr, "%s, R failed 0x%lx != 0x%lx.\n", fname, count, size);
    return -EIO;
  }
  return count;
}

ssize_t write_from_buffer(char* fname, int fd, char* buffer, uint64_t size, uint64_t base)
{
  ssize_t rc;
  uint64_t count = 0;
  char* buf = buffer;
  off_t offset = base;

  while (count < size) {
    uint64_t bytes = size - count;

    if (bytes > RW_MAX_SIZE)
      bytes = RW_MAX_SIZE;

    if (offset) {
      rc = lseek(fd, offset, SEEK_SET);
      if (rc != offset) {
        fprintf(stderr, "%s, seek off 0x%lx != 0x%lx.\n", fname, rc, offset);
        perror("seek file");
        return -EIO;
      }
    }

    /* write data to file from memory buffer */
    rc = write(fd, buf, bytes);
    if (rc != bytes) {
      fprintf(stderr, "%s, W off 0x%lx, 0x%lx != 0x%lx.\n", fname, offset, rc, bytes);
      perror("write file");
      return -EIO;
    }

    count += bytes;
    buf += bytes;
    offset += bytes;
  }

  if (count != size) {
    fprintf(stderr, "%s, R failed 0x%lx != 0x%lx.\n", fname, count, size);
    return -EIO;
  }
  return count;
}

/* Subtract timespec t2 from t1
 *
 * Both t1 and t2 must already be normalized
 * i.e. 0 <= nsec < 1000000000
 */
static int timespec_check(struct timespec* t)
{
  if ((t->tv_nsec < 0) || (t->tv_nsec >= 1000000000))
    return -1;
  return 0;
}

/*
void timespec_sub(struct timespec* t1, struct timespec* t2)
{
  if (timespec_check(t1) < 0) {
    fprintf(stderr, "invalid time #1: %lld.%.9ld.\n", (long long)t1->tv_sec, t1->tv_nsec);
    return;
  }
  if (timespec_check(t2) < 0) {
    fprintf(stderr, "invalid time #2: %lld.%.9ld.\n", (long long)t2->tv_sec, t2->tv_nsec);
    return;
  }
  t1->tv_sec -= t2->tv_sec;
  t1->tv_nsec -= t2->tv_nsec;
  if (t1->tv_nsec >= 1000000000) {
    t1->tv_sec++;
    t1->tv_nsec -= 1000000000;
  } else if (t1->tv_nsec < 0) {
    t1->tv_sec--;
    t1->tv_nsec += 1000000000;
  }
}
*/

// [End] #include "dma_utils.c" ===================================

int test_dma_enc_read(char* EncOut, EncIPConf Confparam)
{
  // U64 tTotal = MLogPhyTick();
  ssize_t rc;
  uint64_t i;

  void* virt_addr;

  uint64_t size;
  uint32_t writeval;

  uint32_t Z_val;

  uint16_t max_schedule, mb, id, bg, z_j, kb, z_a;
  uint16_t z_set;
  uint32_t ctrl_data;
  uint32_t CB_num = CB_PROCESS_NUMBER;

  // this values should be given by Shane
  max_schedule = 0;
  mb = Confparam.mb;
  id = CB_num;
  bg = Confparam.BGSel - 1;
  z_set = Confparam.z_set - 1;
  z_j = Confparam.z_j;

  if (z_set == 0)
    z_a = 2;
  else if (z_set == 1)
    z_a = 3;
  else if (z_set == 2)
    z_a = 5;
  else if (z_set == 3)
    z_a = 7;
  else if (z_set == 4)
    z_a = 9;
  else if (z_set == 5)
    z_a = 11;
  else if (z_set == 6)
    z_a = 13;
  else
    z_a = 15;

  if (bg == 0)
    kb = 22;
  else if (bg == 1)
    kb = 10;
  else if (bg == 2)
    kb = 9;
  else if (bg == 3)
    kb = 8;
  else
    kb = 6;
  mb = Confparam.kb_1 + kb;
  Z_val = (unsigned int)(z_a << z_j);
  ctrl_data = (max_schedule << 30) | ((mb - kb) << 24) | (id << 19) | (bg << 6) | (z_set << 3) | z_j;
  // printf("max_schedule:%d (mb - kb):%d id:%d bg:%d z_set:%d z_j:%d\n",max_schedule,(mb - kb),id,bg,z_set,z_j);
  uint32_t OutDataNUM = Z_val * mb;
  uint32_t Out_dwNumItems_p128;
  uint32_t Out_dwNumItems;

  if ((OutDataNUM & 0x7F) == 0)
    Out_dwNumItems_p128 = OutDataNUM >> 5;
  else
    Out_dwNumItems_p128 = ((OutDataNUM >> 7) + 1) << 2;
  // printf("0x%04x \n",Out_dwNumItems_p128);
  Out_dwNumItems = ((Out_dwNumItems_p128 << 2) * CB_num);
  // printf("0x%04x \n",Out_dwNumItems);
  // MLogPhyTask(PID_DL_FEC_GEN3_R1, tTotal, MLogPhyTick());
  size = Out_dwNumItems;
  writeval = ctrl_data;
  // printf("read : %d byte, ctrl : 0x%08x\n",size,writeval);

  /* calculate the virtual address to be accessed */
  virt_addr = map_base + OFFSET_ENC_OUT;

  /* swap 32-bit endianess if host is not little-endian */
  writeval = htoll(writeval);
  *((uint32_t*)virt_addr) = writeval;
  // MLogPhyTask(PID_DL_FEC_GEN3_R2, tTotal, MLogPhyTick());
  if (fd_enc_read < 0) {
    fprintf(stderr, "unable to open device %s, %d.\n", DEVICE_NAME_DEFAULT_ENC_READ, fd_enc_read);
    perror("open device");
    return -EINVAL;
  }

  /* lseek & read data from AXI MM into buffer using SGDMA */
  rc = read_to_buffer(DEVICE_NAME_DEFAULT_ENC_READ, fd_enc_read, EncOut, size, 0);
  // rc = read_to_buffer(DEVICE_NAME_DEFAULT_ENC_READ, fd_enc_read, allocated_read, size, 0);
  // MLogPhyTask(PID_DL_FEC_GEN3_R3, tTotal, MLogPhyTick());
  if (rc < 0)
    goto out;

  rc = 0;

out:

  return rc;
}

int test_dma_enc_write(char* data, EncIPConf Confparam)
{
  uint64_t i;
  ssize_t rc;
  // U64 tTotal = MLogPhyTick();
  void* virt_addr;

  uint64_t size;
  uint32_t writeval;

  uint32_t Z_val;
  uint16_t max_schedule, mb, id, bg, z_j, kb, z_a;
  uint16_t z_set;
  uint32_t ctrl_data;
  uint32_t CB_num = CB_PROCESS_NUMBER;

  // this values should be given by Shane
  max_schedule = 0;

  mb = Confparam.mb;
  id = CB_num;
  bg = Confparam.BGSel - 1;
  z_set = Confparam.z_set - 1;
  z_j = Confparam.z_j;

  if (z_set == 0)
    z_a = 2;
  else if (z_set == 1)
    z_a = 3;
  else if (z_set == 2)
    z_a = 5;
  else if (z_set == 3)
    z_a = 7;
  else if (z_set == 4)
    z_a = 9;
  else if (z_set == 5)
    z_a = 11;
  else if (z_set == 6)
    z_a = 13;
  else
    z_a = 15;

  if (bg == 0)
    kb = 22;
  else if (bg == 1)
    kb = 10;
  else if (bg == 2)
    kb = 9;
  else if (bg == 3)
    kb = 8;
  else
    kb = 6;
  mb = Confparam.kb_1 + kb;
  Z_val = (unsigned int)(z_a << z_j);
  ctrl_data = (max_schedule << 30) | ((mb - kb) << 24) | (id << 19) | (bg << 6) | (z_set << 3) | z_j;
  // printf("max_schedule:%d (mb - kb):%d id:%d bg:%d z_set:%d z_j:%d\n",max_schedule,(mb - kb),id,bg,z_set,z_j);
  uint32_t InDataNUM = Z_val * kb;
  uint32_t In_dwNumItems_p128;
  uint32_t In_dwNumItems;

  if ((InDataNUM & 0x7F) == 0)
    In_dwNumItems_p128 = InDataNUM >> 5;
  else
    In_dwNumItems_p128 = ((InDataNUM >> 7) + 1) << 2;

  In_dwNumItems = ((In_dwNumItems_p128 << 2) * CB_num);
  // MLogPhyTask(PID_DL_FEC_GEN3_W1, tTotal, MLogPhyTick());
  size = In_dwNumItems;
  writeval = ctrl_data;

  /* calculate the virtual address to be accessed */
  virt_addr = map_base + OFFSET_ENC_IN;

  /* swap 32-bit endianess if host is not little-endian */
  writeval = htoll(writeval);
  *((uint32_t*)virt_addr) = writeval;
  // MLogPhyTask(PID_DL_FEC_GEN3_W2, tTotal, MLogPhyTick());
  if (fd_enc_write < 0) {
    fprintf(stderr, "unable to open device %s, %d.\n", DEVICE_NAME_DEFAULT_ENC_WRITE, fd_enc_write);
    perror("open device");
    return -EINVAL;
  }

  rc = write_from_buffer(DEVICE_NAME_DEFAULT_ENC_WRITE, fd_enc_write, data, size, 0);
  // rc = write_from_buffer(DEVICE_NAME_DEFAULT_ENC_WRITE, fd_enc_write, allocated_write, size, 0);
  if (rc < 0)
    goto out;
  // MLogPhyTask(PID_DL_FEC_GEN3_W3, tTotal, MLogPhyTick());
  rc = 0;

out:

  return rc;
}

// int test_dma_dec_read(unsigned int *DecOut, DecIPConf Confparam)
int test_dma_dec_read(char* DecOut, DecIPConf Confparam)
{
  struct timespec read_start_2, read_end_2;
  ssize_t rc;
  uint64_t i;

  void* virt_addr;

  uint64_t size;
  uint32_t writeval;

  uint32_t Z_val;

  uint16_t max_schedule, mb, id, bg, z_j, kb, z_a, max_iter, sc_idx;
  uint16_t z_set;
  uint32_t ctrl_data;
  uint32_t CB_num = Confparam.CB_num; // CB_PROCESS_NUMBER_Dec;//

  // this values should be given by Shane
  max_schedule = 0;
  mb = Confparam.mb;
  id = CB_num;
  bg = Confparam.BGSel - 1;
  z_set = Confparam.z_set - 1;
  z_j = Confparam.z_j;
  // max_iter = 4;
  max_iter = 8;
  sc_idx = 12;

  if (z_set == 0)
    z_a = 2;
  else if (z_set == 1)
    z_a = 3;
  else if (z_set == 2)
    z_a = 5;
  else if (z_set == 3)
    z_a = 7;
  else if (z_set == 4)
    z_a = 9;
  else if (z_set == 5)
    z_a = 11;
  else if (z_set == 6)
    z_a = 13;
  else
    z_a = 15;

  if (bg == 0)
    kb = 22;
  else if (bg == 1)
    kb = 10;
  else if (bg == 2)
    kb = 9;
  else if (bg == 3)
    kb = 8;
  else
    kb = 6;

  Z_val = (unsigned int)(z_a << z_j);
  ctrl_data =
      (max_schedule << 30) | ((mb - kb) << 24) | (id << 19) | (max_iter << 13) | (sc_idx << 9) | (bg << 6) | (z_set) << 3 | z_j;

  uint32_t OutDataNUM = Z_val * kb;
  uint32_t Out_dwNumItems_p128;
  uint32_t Out_dwNumItems;

  if (CB_num & 0x01) // odd cb number
  {
    if ((OutDataNUM & 0xFF) == 0)
      Out_dwNumItems_p128 = OutDataNUM;
    else
      Out_dwNumItems_p128 = 256 * ((OutDataNUM / 256) + 1);

    Out_dwNumItems = (Out_dwNumItems_p128 * CB_num) >> 3;
    // printf("Z_val%d CB_num%d OutDataNUM%d Out_dwNumItems_p128%d Out_dwNumItems%d\n" , Z_val, CB_num, OutDataNUM,
    // Out_dwNumItems_p128, Out_dwNumItems);
  } else {
    if ((OutDataNUM & 0x7F) == 0)
      Out_dwNumItems_p128 = OutDataNUM;
    else
      Out_dwNumItems_p128 = 128 * ((OutDataNUM / 128) + 1);

    Out_dwNumItems = (Out_dwNumItems_p128 * CB_num) >> 3;
    // printf("Z_val%d CB_num%d OutDataNUM%d Out_dwNumItems_p128%d Out_dwNumItems%d\n" , Z_val, CB_num, OutDataNUM,
    // Out_dwNumItems_p128, Out_dwNumItems);
    if ((Out_dwNumItems & 0x1f) != 0)
      Out_dwNumItems = ((Out_dwNumItems + 31) >> 5) << 5;

    // printf("Z_val%d kb%d OutDataNUM%d Out_dwNumItems_p128%d Out_dwNumItems%d CB_num=%d\n" , Z_val, kb, OutDataNUM,
    // Out_dwNumItems_p128, Out_dwNumItems, CB_num);
  }
  size = Out_dwNumItems;
  writeval = ctrl_data;

  /* calculate the virtual address to be accessed */
  virt_addr = map_base + OFFSET_DEC_OUT;

  /* swap 32-bit endianess if host is not little-endian */
  writeval = htoll(writeval);
  *((uint32_t*)virt_addr) = writeval;

  if (fd_dec_read < 0) {
    fprintf(stderr, "unable to open device %s, %d.\n", DEVICE_NAME_DEFAULT_DEC_READ, fd_dec_read);
    perror("open device");
    return -EINVAL;
  }

  // clock_gettime(CLOCK_MONOTONIC, &read_start_2);
  /* lseek & read data from AXI MM into buffer using SGDMA */
  rc = read_to_buffer(DEVICE_NAME_DEFAULT_DEC_READ, fd_dec_read, DecOut, size, 0);
  if (rc < 0)
    goto out;

  rc = 0;
  // clock_gettime(CLOCK_MONOTONIC, &read_end_2);
  // timespec_sub(&read_end_2, &read_start_2);
  // printf("[2]read_to_buffer() time %.2f µsec\n", (float)(read_end_2.tv_nsec) / 1000);

out:

  return rc;
}

// int test_dma_dec_write(unsigned int *data, DecIPConf Confparam)
int test_dma_dec_write(char* data, DecIPConf Confparam)
{
  uint64_t i;
  ssize_t rc;

  void* virt_addr;

  uint64_t size;
  uint32_t writeval;

  uint32_t Z_val;
  uint16_t max_schedule, mb, id, bg, z_j, kb, z_a, max_iter, sc_idx;
  uint16_t z_set;
  uint32_t ctrl_data;
  uint32_t CB_num = Confparam.CB_num; // CB_PROCESS_NUMBER_Dec;//

  // this values should be given by Shane
  max_schedule = 0;
  mb = Confparam.mb;
  id = CB_num;
  bg = Confparam.BGSel - 1;
  z_set = Confparam.z_set - 1;
  z_j = Confparam.z_j;

  // max_iter = 4;
  max_iter = 8;
  sc_idx = 12;

  if (z_set == 0)
    z_a = 2;
  else if (z_set == 1)
    z_a = 3;
  else if (z_set == 2)
    z_a = 5;
  else if (z_set == 3)
    z_a = 7;
  else if (z_set == 4)
    z_a = 9;
  else if (z_set == 5)
    z_a = 11;
  else if (z_set == 6)
    z_a = 13;
  else
    z_a = 15;

  if (bg == 0)
    kb = 22;
  else if (bg == 1)
    kb = 10;
  else if (bg == 2)
    kb = 9;
  else if (bg == 3)
    kb = 8;
  else
    kb = 6;

  Z_val = (unsigned int)(z_a << z_j);
  ctrl_data =
      (max_schedule << 30) | ((mb - kb) << 24) | (id << 19) | (max_iter << 13) | (sc_idx << 9) | (bg << 6) | (z_set) << 3 | z_j;

  uint32_t InDataNUM = Z_val * mb;
  uint32_t In_dwNumItems_p128;
  uint32_t In_dwNumItems;

  InDataNUM = Z_val * mb * 8;
  if ((InDataNUM & 0x7F) == 0)
    In_dwNumItems_p128 = InDataNUM;
  else
    In_dwNumItems_p128 = 128 * ((InDataNUM / 128) + 1);

  In_dwNumItems = (In_dwNumItems_p128 * CB_num) >> 3;
  if ((In_dwNumItems & 0x1f) != 0)
    In_dwNumItems = ((In_dwNumItems + 31) >> 5) << 5;

  // printf("Z_val[%d] CB_num[%d] mb[%d] InDataNUM[%d] In_dwNumItems_p128[%d] In_dwNumItems[%d]\n" , Z_val, CB_num, mb, InDataNUM,
  // In_dwNumItems_p128, In_dwNumItems);
  size = In_dwNumItems;
  writeval = ctrl_data;

  /* calculate the virtual address to be accessed */
  virt_addr = map_base + OFFSET_DEC_IN;

  /* swap 32-bit endianess if host is not little-endian */
  writeval = htoll(writeval);
  *((uint32_t*)virt_addr) = writeval;

  if (fd_dec_write < 0) {
    fprintf(stderr, "unable to open device %s, %d.\n", DEVICE_NAME_DEFAULT_DEC_WRITE, fd_dec_write);
    perror("open device");
    return -EINVAL;
  }

  rc = write_from_buffer(DEVICE_NAME_DEFAULT_DEC_WRITE, fd_dec_write, data, size, 0);
  if (rc < 0)
    goto out;

  rc = 0;

out:

  return rc;
}
void test_dma_init()
{
  /* access width */
  int access_width = 'w';
  char* device2 = "/dev/xdma0_user"; //
  uint32_t size1 = 24 * 1024;
  uint32_t size2 = 24 * 1024 * 3;

  // printf("\n###################################################\n");
  if ((fd = open(device2, O_RDWR | O_SYNC)) == -1)
    FATAL;
  // printf("#     CHARACTER DEVICE %s OPENED.    #\n", device2);
  fflush(stdout);

  /* map one page */
  map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (map_base == (void*)-1)
    FATAL;
  // printf("#     MEMORY MAPPED AT ADDRESS %p.    #\n", map_base);
  // printf("###################################################\n\n");

  void* virt_addr;
  virt_addr = map_base + OFFSET_RESET;
  *((uint32_t*)virt_addr) = 1;

  fd_enc_write = open(DEVICE_NAME_DEFAULT_ENC_WRITE, O_RDWR);
  fd_enc_read = open(DEVICE_NAME_DEFAULT_ENC_READ, O_RDWR);
  fd_dec_write = open(DEVICE_NAME_DEFAULT_DEC_WRITE, O_RDWR);
  fd_dec_read = open(DEVICE_NAME_DEFAULT_DEC_READ, O_RDWR);

  fflush(stdout);

  allocated_write = NULL;
  posix_memalign((void**)&allocated_write, 4096 /*alignment */, size1 + 4096);
  allocated_read = NULL;
  posix_memalign((void**)&allocated_read, 4096 /*alignment */, size2 + 4096);
}
void dma_reset()
{
  char* device2 = "/dev/xdma0_user"; //

  void* virt_addr;
  virt_addr = map_base + PCIE_OFF;
  *((uint32_t*)virt_addr) = 1;

  if (munmap(map_base, MAP_SIZE) == -1)
    FATAL;
  close(fd_enc_write);
  close(fd_enc_read);
  close(fd_dec_write);
  close(fd_dec_read);
  close(fd);

  // printf("\n###################################################\n");
  if ((fd = open(device2, O_RDWR | O_SYNC)) == -1)
    FATAL;
  // printf("#     CHARACTER DEVICE %s OPENED.    #\n", device2);
  fflush(stdout);

  /* map one page */
  map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (map_base == (void*)-1)
    FATAL;
  // printf("#     MEMORY MAPPED AT ADDRESS %p.    #\n", map_base);
  // printf("###################################################\n\n");

  // void *virt_addr;
  virt_addr = map_base + PCIE_OFF;
  *((uint32_t*)virt_addr) = 1;

  // void *virt_addr;
  virt_addr = map_base + OFFSET_RESET;
  *((uint32_t*)virt_addr) = 1;

  fd_enc_write = open(DEVICE_NAME_DEFAULT_ENC_WRITE, O_RDWR);
  fd_enc_read = open(DEVICE_NAME_DEFAULT_ENC_READ, O_RDWR);
  fd_dec_write = open(DEVICE_NAME_DEFAULT_DEC_WRITE, O_RDWR);
  fd_dec_read = open(DEVICE_NAME_DEFAULT_DEC_READ, O_RDWR);

  fflush(stdout);
}
void test_dma_shutdown()
{
  free(allocated_write);
  free(allocated_read);
}
#if 1
// reg_rx.c
int nrLDPC_decoder_FPGA_PYM(int8_t* buf_in, int8_t* buf_out, DecIFConf dec_conf)
{
  struct timespec ts_start, ts_end; // evaluate core xdma run time
  struct timespec ts_start0, ts_end0; // ealuate time from input setting to output setting include xdma
  struct timespec read_start, read_end;
  struct timespec write_start, write_end;

  int Zc;
  int nRows;
  int baseGraph;
  int CB_num;

  DecIPConf Confparam;
  int z_a, z_tmp, ii, jj, i;
  int z_j = 0;

  int numChannelLlrs; // input soft bits length, Zc x 66 - length of filler bits
  int numFillerBits; // filler bits length
  char in_softbits[26112 * 52 + 1]; // Random by default value, 52 is max CBs in UL 272RBs and 2 layers with 64QAM
  char out_MessageBytes[1056 * 52 + 1]; // 52 = max number of code block
  int iterationAtTermination; // output results
  int parityPassedAtTermination; // output results

  // char buffer_in[26112 * 52 + 1];
  char buffer_out[1056 * 52 + 1];

  int numMsgBits, numMsgBytes, input_CBoffset, output_CBoffset;

  uint8_t i_LS;

  static int init_flag = 0;
  if (init_flag == 0) {
    /*Init*/
    test_dma_init();
    init_flag = 1;
  } else {
    dma_reset();
  }

  clock_gettime(CLOCK_MONOTONIC, &ts_start0); // time start0
  // LDPC input parameter
  Zc = dec_conf.Zc; // shifting size
  nRows = dec_conf.nRows; // number of Rows
  baseGraph = dec_conf.BG; // base graph
  CB_num = dec_conf.numCB; // 31 number of code block
  numChannelLlrs = dec_conf.numChannelLls; // input soft bits length, Zc x 66 - length of filler bits
  numFillerBits = dec_conf.numFillerBits; // filler bits length

  // calc xdma LDPC parameter
  // calc i_LS
  if ((Zc % 15) == 0)
    i_LS = 7;
  else if ((Zc % 13) == 0)
    i_LS = 6;
  else if ((Zc % 11) == 0)
    i_LS = 5;
  else if ((Zc % 9) == 0)
    i_LS = 4;
  else if ((Zc % 7) == 0)
    i_LS = 3;
  else if ((Zc % 5) == 0)
    i_LS = 2;
  else if ((Zc % 3) == 0)
    i_LS = 1;
  else
    i_LS = 0;

  // calc z_a
  if (i_LS == 0)
    z_a = 2;
  else
    z_a = i_LS * 2 + 1;

  // calc z_j
  z_tmp = Zc / z_a;
  while (z_tmp % 2 == 0) {
    z_j = z_j + 1;
    z_tmp = z_tmp / 2;
  }

  // calc CB_num and mb
  Confparam.CB_num = CB_num;
  if (baseGraph == 1)
    Confparam.mb = 22 + nRows;
  else
    Confparam.mb = 10 + nRows;

  // set BGSel, z_set, z_j
  Confparam.BGSel = baseGraph;
  Confparam.z_set = i_LS + 1;
  Confparam.z_j = z_j;

  // calc output numMsgBits
  if (baseGraph == 1)
    numMsgBits = Zc * 22 - numFillerBits;
  else
    numMsgBits = Zc * 10 - numFillerBits;

  // Calc input CB offset
  input_CBoffset = Zc * Confparam.mb * 8;
  if ((input_CBoffset & 0x7F) == 0)
    input_CBoffset = input_CBoffset / 8;
  else
    input_CBoffset = 16 * ((input_CBoffset / 128) + 1);

  // Calc output CB offset
  output_CBoffset = Zc * (Confparam.mb - nRows);
  if ((output_CBoffset & 0x7F) == 0)
    output_CBoffset = output_CBoffset / 8;
  else
    output_CBoffset = 16 * ((output_CBoffset / 128) + 1);

    // memset(buf_in, 0, 26112 * 52 + 1);
    // memset(buf_out, 0, 27500); // memset(buffer_out, 0, 1056 * 52 + 1);

#if 1 // Input Setting FPGA
    // set input buffer_in from the llr output (in_softbits)
    // Arrange data format
    // for (jj = 0; jj < CB_num; jj++) {
    //   for (ii = 0; ii < (numChannelLlrs + numFillerBits + Zc * 2); ii++) {
    //     if (buf_in[ii + input_CBoffset * jj] == -128) {
    //       buf_in[ii + input_CBoffset * jj] = -127;
    //     }
    //       if (ii < Zc * 2)
    //         buf_in[ii + input_CBoffset * jj] = 0x00;
    //       else if (ii < numMsgBits)
    //         buf_in[ii + input_CBoffset * jj] = ((buf_in[ii - Zc * 2 + numChannelLlrs * jj]) ^ (0xFF)) + 1;
    //       else if (ii < (numMsgBits + numFillerBits))
    //         buf_in[ii + input_CBoffset * jj] = 0x80;
    //       else
    //         buf_in[ii + input_CBoffset * jj] = ((buf_in[ii - Zc * 2 - numFillerBits + numChannelLlrs * jj]) ^ (0xFF)) + 1;
    //   }
    //   printf("\nInput_LLR[%d] = ", jj);
    //   for (i = 0; i < 20; i++) {
    //     printf("%d,", buf_in[i + input_CBoffset * jj + 2 * Zc]);
    //   }
    // }

    // printf("input setting done\n");
#endif // Input Setting FPGA

  // LDPC accelerator start
  // printf("[%s] Start DMA write\n", __func__);
  // clock_gettime(CLOCK_MONOTONIC, &ts_start); // time start
  // ===================================================
  // printf("[%s] DMA write 0\n", __func__);
  // write into accelerator
  // clock_gettime(CLOCK_MONOTONIC, &write_start);
  if (test_dma_dec_write(buf_in, Confparam) != 0) {
    exit(1);
    printf("write exit!!\n");
  }
  // clock_gettime(CLOCK_MONOTONIC, &write_end);
  // timespec_sub(&write_end, &write_start);
  // printf("Write time %.2f µsec\n", (float)(write_end.tv_nsec) / 1000);
  // ===================================================
  // printf("[%s] DMA read 0\n", __func__);
  // read output of accelerator
  // clock_gettime(CLOCK_MONOTONIC, &read_start);
  if (test_dma_dec_read(buf_out, Confparam) != 0) {
    exit(1);
    printf("read exit!!\n");
  }
  // clock_gettime(CLOCK_MONOTONIC, &read_end);
  // timespec_sub(&read_end, &read_start);
  // printf("[1]Read time %.2f µsec\n", (float)(read_end.tv_nsec) / 1000);
  // // ===================================================
  // clock_gettime(CLOCK_MONOTONIC, &ts_end); // time end
  // printf("[%s] End DMA read\n", __func__);

  //  LDPC accelerator end
  // timespec_sub(&ts_end, &ts_start);
  // printf("[%s] finish DMA, CB_num[%d], total time %ld nsec\n", __func__, CB_num, ts_end.tv_nsec);

#if 1 // Output Setting FPGA
  // set output out_MessageBytes from the xdma output (buffer_out) , iterationAtTermination , parityPassedAtTermination
  for (jj = 0; jj < CB_num; jj++) {
    if ((numMsgBits & 0x7) == 0)
      numMsgBytes = numMsgBits / 8;
    else {
      numMsgBytes = (numMsgBits / 8) + 1;
    }
    iterationAtTermination = 1; // output
    parityPassedAtTermination = 1; // output
    // memcpy((int8_t*)&buf_out[output_CBoffset * jj], (int8_t*)&buffer_out[output_CBoffset * jj], numMsgBytes);

    // -----------------------------------
    // Compare output information:
    // -----------------------------------
    // printf("buffer_out[%d] = ", jj);
    // for (i = 0; i < 10; i++) {
    //   printf("%d, ", buffer_out[i + output_CBoffset * jj]);
    // }
    // printf("\n");
    // printf("buf_out[%d] = ", jj);
    // for (i = 0; i < 10; i++) {
    //   printf("%d, ", buf_out[i + output_CBoffset * jj]);
    // }
    // printf("\n");
  }
  // printf("[%s] Output setting done\n", __func__);

#endif // Output Setting FPGA
  // clock_gettime(CLOCK_MONOTONIC, &ts_end0); // time end0
  // timespec_sub(&ts_end0, &ts_start0);
  // printf("[%s] finish LDPC, CB_num[%d], total time %ld nsec\n", __func__, CB_num, ts_end0.tv_nsec);
  // printf("Accelerator card is completed!\n");
  return 0;
}
#endif
