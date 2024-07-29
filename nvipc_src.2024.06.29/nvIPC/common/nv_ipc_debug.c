/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#define _GNU_SOURCE /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <pcap/pcap.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#include "nv_ipc.h"
#include "nv_ipc_config.h"
#include "nv_ipc_debug.h"
#include "nv_ipc_efd.h"
#include "nv_ipc_sem.h"
#include "nv_ipc_epoll.h"
#include "nv_ipc_mempool.h"
#include "nv_ipc_utils.h"
#include "array_queue.h"
#include "nv_ipc_forward.h"
#include "nv_utils.h"

#define LOG_PATH_LEN 64

#define TAG (NVLOG_TAG_BASE_NVIPC + 10) //"NVIPC.DEBUG"
#define SHM_DEBUG_SUFFIX "_debug"

#define CONFIG_LOG_ALLOCATE_TIME 1
#define CONFIG_LOG_SEND_TIME 1

#define CONFIG_FORWARD_TEST_THREAD_CORE -1

// Enable nvipc pcap capture or not.
#define ENV_NVIPC_DEBUG_PCAP "NVIPC_DEBUG_EN"
#define CONFIG_DEBUG_PCAP 0

#define ENV_NVIPC_DEBUG_TIMING "NVIPC_DEBUG_TIMING"
#define CONFIG_DEBUG_TIMING 0

#define U64_1E9 (1000 * 1000 * 1000LL)
#define TIMING_STAT_INTERVAL (U64_1E9 * 1) // Print timing statistic log every 1 second

#define LOG_SHM_NAME_LEN 32

#define ENABLE_PCAP_CAPTURE 1

#define SCF_PCAP_PROTOCOL_PORT 9000

// Data link type (LINKTYPE_*): 1 - Ethernet; 113 - Linux cooked capture.
#define PCAP_DATA_LINK_TYPE 113

// Sync with scf_5g_fapi.h
#define SCF_FAPI_ERROR_INDICATION 0x07
#define SCF_FAPI_RESV_1_START 0x08
#define SCF_FAPI_RESV_1_END 0x7F
#define SCF_FAPI_RESV_2_START 0x8A
#define SCF_FAPI_RESV_2_END 0xFF

// INVALID SFN/SLOT for non slot messages like CONFIG.req
#define SFN_SLOT_INVALID 0xFFFFFFFF

static const sfn_slot_t sfn_slot_invalid = {.u32 = SFN_SLOT_INVALID};

static int id_counter = 0x1000;

static FILE*           pcapfile;
static pthread_mutex_t pcapmutex;

#define IPC_DOWNLINK (0)
#define IPC_UPLINK (1)

#ifdef DEBUG_HIGH_RESOLUTION_TIME
#define debug_get_timestamp(ts) clock_gettime(CLOCK_REALTIME, (ts))
#define debug_get_ts_interval(ts1, ts2) nvlog_timespec_interval((ts1), (ts2))
#else
#define debug_get_timestamp(ts) gettimeofday((ts), NULL)
#define debug_get_ts_interval(ts1, ts2) nvlog_timeval_interval((ts1), (ts2))
#endif

typedef enum
{
    ALTRAN_FAPI = 0,
    SCF_FAPI    = 1,
} fapi_type_t;

typedef struct
{
    int32_t config_items[NV_IPC_CFG_ITEM_NUM];
} config_items_t;

static int32_t fapi_type   = -1;
static int32_t fapi_tb_loc = -1;

static int32_t pcap_max_msg_size = -1;
static int32_t pcap_max_data_size = -1;

typedef struct
{
    uint8_t  mac[6];
    char     ip[16];
    uint16_t port;
} net_addr_t;

// NVIDIA MAC prefix: 00:04:4B
static net_addr_t mac_addr = {
    {0x00, 0x04, 0x4B, 0x34, 0x35, 0x36},
    "192.168.1.8",
    38555};

static net_addr_t phy_addr = {
    {0x00, 0x04, 0x4B, 0x44, 0x45, 0x46},
    "192.168.1.9",
    38556};

// Record header for each packet. See https://wiki.wireshark.org/Development/LibpcapFileFormat
typedef struct
{
    uint32_t ts_sec;   /* timestamp seconds */
    uint32_t ts_usec;  /* timestamp microseconds */
    uint32_t incl_len; /* number of octets of packet saved in file */
    uint32_t orig_len; /* actual length of packet */
} record_header_t;

// SLL header for "Linux cooked capture" 16B
typedef struct
{
    uint16_t packet_type;
    uint16_t arphrd_type;
    uint16_t link_layer_addr_len;
    uint8_t  link_layer_addr[6];
    uint16_t padding;
    uint16_t protocol_type;
} sll_header_t;

// Ethernet header 14B
typedef struct
{
    uint8_t  dst_mac[6];
    uint8_t  src_mac[6];
    uint16_t type_len;
} eth_header_t;

//IP header 20B
typedef struct
{
    uint8_t  ver_hlen; // Version (4 bits) + Internet header length (4 bits)
    uint8_t  tos;      // Type of service
    uint16_t len;      // Total length
    uint16_t id;       // Identification
    uint16_t flags;
    uint8_t  ttl; // Time to live
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dst_ip;
} ip_header_t;

//UDP header 8B
typedef struct _udp_hdr
{
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t len;
    uint16_t checksum;
} udp_header_t;

// FAPI header 8B
typedef struct
{
    uint16_t msg_id;
    uint16_t padding;
    uint32_t msg_len;
} altran_fapi_header_t;

// SCF FAPI header, copied from scf_5g_fapi.h
typedef struct
{
    uint8_t message_count;
    uint8_t handle_id;
    uint8_t payload[0];
} __attribute__((__packed__)) scf_fapi_header_t;

// SCF FAPI body header, copied from scf_5g_fapi.h
typedef struct
{
    uint16_t type_id;
    uint32_t length;
    uint8_t  next[0];
} __attribute__((__packed__)) scf_fapi_body_header_t;

typedef struct
{
    uint16_t sfn;
    uint16_t slot;
    uint8_t  next[0];
} __attribute__((__packed__)) scf_fapi_sfn_slot_t;

typedef struct
{
    scf_fapi_header_t      head;
    scf_fapi_body_header_t body;
    sfn_slot_t             sfn_slot;
} __attribute__((__packed__)) scf_fapi_slot_header_t;

// Get integer system environment value
long get_env_long(const char* name, long def)
{
    if(name == NULL)
    {
        return def;
    }

    // If CUBB_HOME was set in system environment variables, return it
    char* env = getenv(name);
    if(env == NULL)
    {
        return def;
    }

    long  val;
    char* err_ptr = NULL;
    if(strncmp(env, "0b", 2) == 0 || strncmp(env, "0B", 2) == 0)
    {
        val = strtol(env + 2, &err_ptr, 2); // Binary
    }
    else
    {
        val = strtol(env, &err_ptr, 0); // Octal, Decimal, Hex
    }

    if(err_ptr == NULL || *err_ptr != '\0')
    {
        NVLOGI(TAG, "%s: invalid variable: %s=%s", __FUNCTION__, name, env);
        return def;
    }
    else
    {
        return val;
    }
}

void load_debug_config()
{
    fapi_type   = nv_ipc_app_config_get(NV_IPC_CFG_FAPI_TYPE);
    fapi_tb_loc = nv_ipc_app_config_get(NV_IPC_CFG_FAPI_TB_LOC);
    pcap_max_msg_size = nv_ipc_app_config_get(NV_IPC_CFG_PCAP_MAX_MSG_SIZE);
    pcap_max_data_size = nv_ipc_app_config_get(NV_IPC_CFG_PCAP_MAX_DATA_SIZE);
    NVLOGI(TAG, "%s: fapi_type=%d fapi_tb_loc=%d pcap_max_msg_size=%d pcap_max_data_size=%d",
            __func__, fapi_type, fapi_tb_loc, pcap_max_msg_size, pcap_max_data_size);
}

int get_pcap_max_msg_size()
{
    return pcap_max_msg_size;
}

int get_pcap_max_data_size()
{
    return pcap_max_data_size;
}

int get_pcap_max_record_size()
{
    // Stored one additional "int32_t buf_size" for error check
    return sizeof(record_t) + get_pcap_max_msg_size() + get_pcap_max_data_size() + 4;
}

static int32_t parse_fapi_id(uint8_t* fapi_buf)
{
    if(fapi_type == ALTRAN_FAPI)
    {
        altran_fapi_header_t* altran_fapi = (altran_fapi_header_t*)fapi_buf;
        return altran_fapi->msg_id;
    }
    else if(fapi_type == SCF_FAPI)
    {
        scf_fapi_header_t* scf_fapi = (scf_fapi_header_t*)fapi_buf;
        if(scf_fapi->message_count == 0)
        {
            return -1;
        }
        else
        {
            scf_fapi_body_header_t* body = (scf_fapi_body_header_t*)scf_fapi->payload;
            return body->type_id;
        }
    }
    else
    {
        NVLOGE_NO(TAG, AERIAL_FAPI_EVENT, "%s unknown FAPI type %d", __func__, fapi_type);
        return -1;
    }
}

static int32_t parse_fapi_length(uint8_t* fapi_buf)
{
    int32_t fapi_len = 0; // The FAPI payload length, included in FAPI header

    if(fapi_type == ALTRAN_FAPI)
    {
        altran_fapi_header_t* altran_fapi = (altran_fapi_header_t*)fapi_buf;
        fapi_len                          = altran_fapi->msg_len + sizeof(altran_fapi_header_t); // The FAPI payload length, defined in Altran FAPI message
        if(fapi_len <= sizeof(altran_fapi_header_t))
        {
            NVLOGW(TAG, "%s: fapi: msg_id=0x%02X fapi_len=%d", __func__, altran_fapi->msg_id, fapi_len);
            fapi_len = 32 + sizeof(altran_fapi_header_t);
        }
    }
    else if(fapi_type == SCF_FAPI)
    {
        scf_fapi_header_t* scf_fapi = (scf_fapi_header_t*)fapi_buf;
        fapi_len                    = sizeof(scf_fapi_header_t);
        int offset                  = 0;
        for(int i = 0; i < scf_fapi->message_count; i++)
        {
            scf_fapi_body_header_t* body = (scf_fapi_body_header_t*)(scf_fapi->payload + offset);
            fapi_len += sizeof(scf_fapi_body_header_t) + body->length;
            offset += sizeof(scf_fapi_body_header_t) + body->length;
        }
    }
    else
    {
        NVLOGE_NO(TAG, AERIAL_FAPI_EVENT, "%s unknown FAPI type %d", __func__, fapi_type);
        return -1;
    }

    if(fapi_len > get_pcap_max_data_size())
    {
        fapi_len = get_pcap_max_data_size(); // For DL/UL data
    }

    return fapi_len;
}

// Write mocking common headers ahead of FAPI payload: PCAP + SLL/ETH + IP + UDP
int pcap_write_common_headers(record_t* record, int32_t fapi_len)
{
    int         ret = 0;
    net_addr_t *src, *dst;
    int32_t     dir = record->flags >> 16;
    if(dir == IPC_DOWNLINK)
    {
        src = &phy_addr;
        dst = &mac_addr;
    }
    else
    {
        src = &mac_addr;
        dst = &phy_addr;
    }

    if(fapi_type == SCF_FAPI)
    {
        src->port = SCF_PCAP_PROTOCOL_PORT;
        dst->port = SCF_PCAP_PROTOCOL_PORT;
    }

    // PCAP header: 16B
    record_header_t record_hdr;
    record_hdr.ts_sec  = record->tv.tv_sec;
    record_hdr.ts_usec = record->tv.tv_usec;
#if(PCAP_DATA_LINK_TYPE == 1)
    record_hdr.incl_len = fapi_len + 8 + 20 + 14; // FAPI + UDP 8 + IP 20 + ETH 14
#elif(PCAP_DATA_LINK_TYPE == 113)
    record_hdr.incl_len = fapi_len + 8 + 20 + 16; // FAPI + UDP 8 + IP 20 + SLL 16
#else
#error Unsupported PCAP_DATA_LINK_TYPE
#endif
    record_hdr.orig_len = record_hdr.incl_len;
    if(fwrite(&record_hdr, sizeof(record_header_t), 1, pcapfile) != 1)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "Error: %s line %d - %s", __func__, __LINE__, strerror(errno));
        ret = -1;
    }

#if(PCAP_DATA_LINK_TYPE == 1)
    // ETH header: 14B
    eth_header_t eth_hdr;
    for(int i = 0; i < 6; i++)
    {
        eth_hdr.src_mac[i] = src->mac[i];
        eth_hdr.dst_mac[i] = dst->mac[i];
    }
    eth_hdr.type_len = htons(0x0800); // 0x0800 is IP(v4)
    if(fwrite(&eth_hdr, sizeof(eth_header_t), 1, pcapfile) != 1)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "Error: %s line %d - %s", __func__, __LINE__, strerror(errno));
        ret = -1;
    }
#elif(PCAP_DATA_LINK_TYPE == 113)
    // SLL header for "Linux cooked capture": 16B
    sll_header_t sll_header;
    sll_header.packet_type         = 0;
    sll_header.arphrd_type         = htons(772);
    sll_header.link_layer_addr_len = htons(6);
    for(int i = 0; i < 6; i++)
    {
        sll_header.link_layer_addr[i] = 0;
    }
    sll_header.padding       = 0;
    sll_header.protocol_type = htons(0x0800);
    if(fwrite(&sll_header, sizeof(sll_header_t), 1, pcapfile) != 1)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "Error: %s line %d - %s", __func__, __LINE__, strerror(errno));
        ret = -1;
    }
#else
#error Unsupported PCAP_DATA_LINK_TYPE
#endif

    // IP header: 20B
    ip_header_t ip_hdr;
    ip_hdr.ver_hlen = 0x45; // Version and header length
    ip_hdr.tos      = 0x00;
    ip_hdr.len      = htons(fapi_len + 8 + 20); // FAPI + UDP 8 + IP 20
    ip_hdr.id       = htons(id_counter++);
    ip_hdr.flags    = 0x0040;
    ip_hdr.ttl      = 64;
    ip_hdr.protocol = 0x11; //UDP
    ip_hdr.checksum = 0x0996;
    uint32_t addr;
    inet_pton(AF_INET, src->ip, &addr);
    ip_hdr.src_ip = addr;
    inet_pton(AF_INET, dst->ip, &addr);
    ip_hdr.dst_ip = addr;
    if(fwrite(&ip_hdr, sizeof(ip_header_t), 1, pcapfile) != 1)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "Error: %s line %d - %s", __func__, __LINE__, strerror(errno));
        ret = -1;
    }

    // UDP Header: 8B
    udp_header_t udp_hdr;
    udp_hdr.src_port = htons(src->port);
    udp_hdr.dst_port = htons(dst->port);
    udp_hdr.len      = htons(fapi_len + 8); // FAPI + UDP 8
    udp_hdr.checksum = 0xd6db;
    if(fwrite(&udp_hdr, sizeof(udp_header_t), 1, pcapfile) != 1)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "Error: %s line %d - %s", __func__, __LINE__, strerror(errno));
        ret = -1;
    }
    return ret;
}

static int pcap_write_record(record_t* record)
{
    int32_t msg_id  = record->flags & 0xFFFF;
    int32_t fapi_id = parse_fapi_id(record->buf);
    if(msg_id != fapi_id)
    {
        NVLOGW(TAG, "%s: msg_id not match: msg_id=0x%02X fapi_id=0x%02X", __func__, msg_id, fapi_id);
    }

    // The FAPI payload length, included in FAPI header
    int32_t fapi_len = parse_fapi_length(record->buf);
    if(fapi_len != record->buf_size)
    {
        // Except TX_DATA_REQUEST and RX_DATA_INDICATION
        if(fapi_id != 0x84 && fapi_id != 0x85 && fapi_id != 0x14 && fapi_id != 0x16)
        {
            NVLOGW(TAG, "%s: msg_len not match: msg_id=0x%02X msg_len=%d fapi_len=%d", __func__, msg_id, record->buf_size, fapi_len);
        }
    }

    if(pcap_write_common_headers(record, record->buf_size) < 0)
    {
        return -1;
    }

    // Write FAPI payload
    if(fwrite(record->buf, record->buf_size, 1, pcapfile) != 1)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "Error: %s line %d - %s", __func__, __LINE__, strerror(errno));
        return -1;
    }
    else
    {
        NVLOGD(TAG, "%s: write OK. fapi_id=0x%02X record->buf_size=%d", __func__, fapi_id, record->buf_size);
        return 0;
    }
}

static int pcap_write_file_header()
{
    struct pcap_file_header file_header;
    file_header.magic         = 0xA1B2C3D4;
    file_header.version_major = PCAP_VERSION_MAJOR;
    file_header.version_minor = PCAP_VERSION_MINOR;
    file_header.thiszone      = 0;          /* gmt to local correction */
    file_header.sigfigs       = 0;          /* accuracy of timestamps */
    file_header.snaplen       = 0x00040000; /* max length saved portion of each pkt */
    file_header.linktype      = PCAP_DATA_LINK_TYPE;
    NVLOGI(TAG, "%s: write pcap_write_file_header size=%lu", __func__, sizeof(struct pcap_file_header));

    if(fwrite(&file_header, sizeof(struct pcap_file_header), 1, pcapfile) != 1)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: write pcap_write_file_header failed", __func__);
        return -1;
    }
    else
    {
        return 0;
    }
}

int pcap_file_open(const char* filename)
{
    // Create or lookup the semaphore
    pthread_mutex_init(&pcapmutex, NULL);
    char path[LOG_SHM_NAME_LEN * 2];
    snprintf(path, LOG_SHM_NAME_LEN * 2, "%s", filename);

    // Open a temperate file to store the logs
    if((pcapfile = fopen(path, "w")) == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: failed to open file %s", __func__, path);
        return -1;
    }
    pcap_write_file_header();
    NVLOGI(TAG, "%s: opened file %s for PCAP log", __func__, path);
    return 0;
}

#define MAX_SHMLOG_BUF_SIZE (3 * 1024 * 1024)

static int convert_pcap(FILE* record_file, FILE* pcap_file, long start, long end)
{
    int       ret    = 0;
    record_t* record = malloc(MAX_SHMLOG_BUF_SIZE);
    if (record == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: malloc failed", __func__);
        return -1;
    }
    record->buf_size = 0;
    record->flags = 0;

    NVLOGC(TAG, "%s: start=0x%lX end=0x%lX", __func__, start, end);

    long pos_forward = start;
    while(pos_forward + sizeof(record_t) < end)
    {
        // Set file offset
        if(fseek(record_file, pos_forward, SEEK_SET) < 0)
        {
            NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: fseek error: pos_forward=0X%lX err=%d - %s", __func__, pos_forward, errno, strerror(errno));
            ret = -1;
            break;
        }

        // Read the record_t header
        if(fread(record, sizeof(record_t), 1, record_file) != 1)
        {
            NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: fread header error: pos_forward=0X%lX err=%d - %s", __func__, pos_forward, errno, strerror(errno));
            ret = -1;
            break;
        }
        NVLOGV(TAG, "%s: record: dir=%d msg_id=0x%X record->buf_size=%d", __func__, record->flags >> 16, record->flags & 0xFFFF, record->buf_size);

        if(pos_forward + record->buf_size + sizeof(record->buf_size) > end)
        {
            NVLOGI(TAG, "%s: The last record was overridden, skip pos_forward=0x%lX record->buf_size=0x%X", __func__, pos_forward, record->buf_size);
            break;
        }

        // Error check
        if(record->buf_size <= 0 || record->buf_size > get_pcap_max_record_size() - sizeof(record_t) - 4)
        {
            NVLOGC(TAG, "%s: error record: pos_forward=0x%lX record->buf_size=0x%X", __func__, pos_forward, record->buf_size);
            ret = -1;
            break;
        }

        // Read the payload
        if(fread(record->buf, record->buf_size, 1, record_file) != 1)
        {
            NVLOGC(TAG, "%s: fread payload error: pos_forward=0X%lX err=%d - %s", __func__, pos_forward, errno, strerror(errno));
            ret = -1;
            break;
        }

        int32_t buf_size = -1;
        if(fread(&buf_size, sizeof(record->buf_size), 1, record_file) != 1)
        {
            NVLOGC(TAG, "%s: fread size error: pos_forward=0X%lX err=%d - %s", __func__, pos_forward, errno, strerror(errno));
            ret = -1;
            break;
        }

        // For debug: error check
        if(buf_size != record->buf_size)
        {
            NVLOGC(TAG, "%s: record file format error: pos_forward=0x%lX record->buf_size=%d buf_size=%d", __func__, pos_forward, record->buf_size, buf_size);
            ret = -1;
            break;
        }

        // Write to pcap file
        pcap_write_record(record);

        // Check and move file offset
        int record_size = get_record_size(record);
        if(pos_forward + record_size + sizeof(record_t) >= end)
        {
            break;
        }
        else
        {
            pos_forward += record_size;
        }
    }

    if (ret < 0)
    {
        if (end - pos_forward <= get_pcap_max_record_size() * 2)
        {
            // Ignore the last 2 records parsing failure
            ret = 0;
            NVLOGC(TAG, "%s: The last %lu bytes was not integrative. pos_forward=0x%lX record->buf_size=0x%X msg_id=0x%02X",
                    __func__, end - pos_forward, pos_forward, record->buf_size, record->flags & 0xFFFF);
        }
        else
        {
            NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: convert error: remain_size=%lu pos_forward=0x%lX record->buf_size=0x%X msg_id=0x%02X",
                    __func__, end - pos_forward, pos_forward, record->buf_size, record->flags & 0xFFFF);
        }
    }

    free(record);
    fflush(pcapfile);
    NVLOGC(TAG, "%s: ret=%d start=0x%lX end=0x%lX converted_pos=0x%lX - %ld", __func__, ret, start, end, pos_forward, pos_forward);
    return ret;
}

int64_t nv_ipc_convert_pcap(FILE* record_file, char* pcap_filepath, long shm_cache_size, long total_size, uint64_t break_offset)
{
    if(record_file == NULL || fseek(record_file, 0, SEEK_END) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: record_file error", __func__);
        return -1;
    }

    int64_t file_size = ftell(record_file);
    if (file_size < 0)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: record_file size error", __func__);
        return -1;
    }

    NVLOGC(TAG, "%s: fapi_type=%d shm_cache_size=0x%lX file_size=0x%lX total_size=0x%lX", __func__, fapi_type, shm_cache_size, file_size, total_size);

    int ret = 0;
    pcap_file_open(pcap_filepath);

    // Log rotation enabled
    // Convert the first SHM block
    long pcap_end = break_offset == 0 ? file_size : shm_cache_size;
    ret = convert_pcap(record_file, pcapfile, 0, pcap_end);
    if(ret != 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: convert first SHM block failed shm_cache_size=0x%lX file_size=0x%lX", __func__, shm_cache_size, file_size);
    }

    int64_t pos_break = (break_offset & ((shm_cache_size -1) >> 1)) + shm_cache_size;

    NVLOGC(TAG, "%s: converted first block size=0x%lX=%ld MB file_size=0x%lX=%ld MB pos_break=0x%lX", __func__,
            shm_cache_size, shm_cache_size >> 20, file_size, file_size >> 20, pos_break);

    if (break_offset != 0)
    {
        // Some logs may have been overwritten, find the earliest record which hasn't been overwritten
        int64_t pos_backward = file_size;
        record_t record;
    #if 0 // Below code are for debug, do not delete
        while(pos_backward > shm_cache_size)
        {
            if(fseek(record_file, pos_backward - sizeof(record.buf_size), SEEK_SET) < 0)
            {
                NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: fseek error: pos_backward=0x%lX err=%d - %s", __func__, pos_backward, errno, strerror(errno));
                ret = -1;
                break;
            }

            // Read the record size
            if(fread(&record.buf_size, sizeof(record.buf_size), 1, record_file) != 1)
            {
                NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: fread error: pos_backward=0x%lX err=%d - %s", __func__, pos_backward, errno, strerror(errno));
                ret = -1;
                break;
            }

            NVLOGD(TAG, "%s: buf_size=%ld - 0x%X pos_backward=0x%lX", __func__, record.buf_size, record.buf_size, pos_backward);

            if(record.buf_size <= 0)
            {
                NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: error buffer size: pos_backward=0x%lX=%u MB", __func__, pos_backward, pos_backward / 1024/1024);
                ret = -1;
                break;
            }

            // Move backward to previous record
            int record_size = get_record_size(&record);
            if(pos_backward - record_size < shm_cache_size + sizeof(record.buf_size))
            {
                break;
            }
            else
            {
                pos_backward -= record_size;
            }
        }

    #else
        pos_backward = pos_break;
    #endif

        NVLOGC(TAG, "%s: found shm_cache_size=%ld pos_backward=0x%lX break_offset=0x%lX file_size=%ld",
                __func__, shm_cache_size, pos_backward, break_offset, file_size);

        if (pos_backward != pos_break)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: break_offset error: shm_cache_size=%ld pos_backward=0x%lX break_offset=0x%lX file_size=%ld",
                    __func__, shm_cache_size, pos_backward, break_offset, file_size);
        }

        if(ret == 0)
        {
            ret = convert_pcap(record_file, pcapfile, pos_backward, file_size);
        }
    }
    int64_t pcap_size = ftell(pcapfile);
    fclose(pcapfile);
    return ret == 0 ? pcap_size : -1;
}

static void* forward_test_thread(void* arg)
{
    pthread_setname_np(pthread_self(), "forward_test");

    nv_ipc_t* ipc = arg;
    // nv_ipc_t* ipc = nv_ipc_get_instance(NULL);

    if(ipc == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_INVALID_PARAM_EVENT, "%s: get nvipc instance failed", __func__);
        return NULL;
    }

    nv_set_sched_fifo_priority(90);
    nv_assign_thread_cpu_core(CONFIG_FORWARD_TEST_THREAD_CORE);

    char thread_name[16];
    pthread_getname_np(pthread_self(), thread_name, 16);
    NVLOGC(TAG, "%s: thread [%s] started ...", __func__, thread_name);

    // nvipc_fw_start(ipc, 0);

    nv_ipc_msg_t    msg;
    struct timespec ts_abs;
    clock_gettime(CLOCK_REALTIME, &ts_abs);

    while(1)
    {
        nvlog_timespec_add(&ts_abs, 1000LL * 1000 * 1000 * 3);
        if(nvipc_fw_sem_timedwait(ipc, &ts_abs) < 0)
        {
            NVLOGI(TAG, "%s: nvipc_fw_sem_timedwait returned with errno=%d - %s", __func__, errno, strerror(errno));
        }

        while(nvipc_fw_dequeue(ipc, &msg) >= 0)
        {
            // IPC message dequeued from the fw_ring queue
            NVLOGD(TAG, "Forwarder: dequeue msg_id=0x%02X", msg.msg_id);

            // Free the IPC buffers
            nvipc_fw_free(ipc, &msg);
        }
    }

    // nvipc_fw_stop(ipc);

    return NULL;
}

static int ipc_debug_open(nv_ipc_debug_t* ipc_debug)
{
    load_debug_config();

    // Only capture pcap in primary app
    if(ipc_debug->primary && (get_env_long(ENV_NVIPC_DEBUG_PCAP, CONFIG_DEBUG_PCAP) || nv_ipc_app_config_get(NV_IPC_CFG_PCAP_ENABLE)))
    {
        ipc_debug->enable_pcap = 1;
        NVLOGC(TAG, "%s: pcap enabled: fapi_type=%d fapi_tb_loc=%d", __func__, fapi_type, fapi_tb_loc);
    }
    else
    {
        ipc_debug->enable_pcap = 0;
    }

    // Configure timing debug
    if(get_env_long(ENV_NVIPC_DEBUG_TIMING, CONFIG_DEBUG_TIMING) || nv_ipc_app_config_get(NV_IPC_CFG_DEBUG_TIMING))
    {
        ipc_debug->enable_timing = 1;
    }
    else
    {
        ipc_debug->enable_timing = 0;
    }

    char name[NV_NAME_MAX_LEN + NV_NAME_SUFFIX_MAX_LEN];
    nvlog_safe_strncpy(name, ipc_debug->prefix, NV_NAME_MAX_LEN);
    strncat(name, SHM_DEBUG_SUFFIX, NV_NAME_SUFFIX_MAX_LEN);

    // Create a shared memory block
    size_t shm_data_size    = sizeof(debug_shm_data_t);
    size_t msg_timing_size  = sizeof(msg_timing_t) * ipc_debug->msg_pool_len;
    size_t sync_timing_size = sizeof(sync_timing_t) * ipc_debug->msg_pool_len;
    size_t total_shm_size   = shm_data_size + msg_timing_size + sync_timing_size * 2;

    int shm_primary = ipc_debug->transport == NV_IPC_TRANSPORT_SHM ? ipc_debug->primary : 1;
    if((ipc_debug->shmpool = nv_ipc_shm_open(shm_primary, name, total_shm_size)) == NULL)
    {
        return -1;
    }

    int8_t* shm_addr = ipc_debug->shmpool->get_mapped_addr(ipc_debug->shmpool);
    if(shm_primary)
    {
        memset(shm_addr, 0, total_shm_size);
    }

    ipc_debug->shm_data = (debug_shm_data_t*)shm_addr;
    shm_addr += shm_data_size;

    ipc_debug->msg_timing = (msg_timing_t*)shm_addr;
    shm_addr += msg_timing_size;

    ipc_debug->sync_timing_m2s = (sync_timing_t*)shm_addr;
    shm_addr += sync_timing_size;

    ipc_debug->sync_timing_s2m = (sync_timing_t*)shm_addr;
    shm_addr += sync_timing_size;

    if(ipc_debug->enable_timing)
    {
#if 1
        if(ipc_debug->primary)
        {
            ipc_debug->stat_msg_build     = stat_log_open("DL_MSG_BUILD", STAT_MODE_TIMER, TIMING_STAT_INTERVAL);
            ipc_debug->stat_msg_transport = stat_log_open("DL_MSG_TRANSPORT", STAT_MODE_TIMER, TIMING_STAT_INTERVAL);
            ipc_debug->stat_msg_handle    = stat_log_open("DL_MSG_HANDLE", STAT_MODE_TIMER, TIMING_STAT_INTERVAL);
            ipc_debug->stat_msg_total     = stat_log_open("DL_MSG_TOTAL", STAT_MODE_TIMER, TIMING_STAT_INTERVAL);
            ipc_debug->stat_wait_delay    = stat_log_open("DL_WAIT_DELAY", STAT_MODE_TIMER, TIMING_STAT_INTERVAL);
            ipc_debug->stat_post_interval = stat_log_open("UL_POST_INTERVAL", STAT_MODE_TIMER, TIMING_STAT_INTERVAL);
        }
        else
        {
            ipc_debug->stat_msg_build     = stat_log_open("UL_MSG_BUILD", STAT_MODE_TIMER, TIMING_STAT_INTERVAL);
            ipc_debug->stat_msg_transport = stat_log_open("UL_MSG_TRANSPORT", STAT_MODE_TIMER, TIMING_STAT_INTERVAL);
            ipc_debug->stat_msg_handle    = stat_log_open("UL_MSG_HANDLE", STAT_MODE_TIMER, TIMING_STAT_INTERVAL);
            ipc_debug->stat_msg_total     = stat_log_open("UL_MSG_TOTAL", STAT_MODE_TIMER, TIMING_STAT_INTERVAL);
            ipc_debug->stat_wait_delay    = stat_log_open("UL_WAIT_DELAY", STAT_MODE_TIMER, TIMING_STAT_INTERVAL);
            ipc_debug->stat_post_interval = stat_log_open("DL_POST_INTERVAL", STAT_MODE_TIMER, TIMING_STAT_INTERVAL);
        }
#else
        if(ipc_debug->primary)
        {
            ipc_debug->stat_msg_build     = stat_log_open("DL_MSG_BUILD", STAT_MODE_COUNTER, 50 * 1000);
            ipc_debug->stat_msg_transport = stat_log_open("DL_MSG_TRANSPORT", STAT_MODE_COUNTER, 50 * 1000);
            ipc_debug->stat_msg_handle    = stat_log_open("DL_MSG_HANDLE", STAT_MODE_COUNTER, 50 * 1000);
            ipc_debug->stat_msg_total     = stat_log_open("DL_MSG_TOTAL", STAT_MODE_COUNTER, 50 * 1000);
            ipc_debug->stat_wait_delay    = stat_log_open("DL_WAIT_DELAY", STAT_MODE_COUNTER, 10 * 1000);
            ipc_debug->stat_post_interval = stat_log_open("UL_POST_INTERVAL", STAT_MODE_COUNTER, 10 * 1000);
        }
        else
        {
            ipc_debug->stat_msg_build     = stat_log_open("UL_MSG_BUILD", STAT_MODE_COUNTER, 50 * 1000);
            ipc_debug->stat_msg_transport = stat_log_open("UL_MSG_TRANSPORT", STAT_MODE_COUNTER, 50 * 1000);
            ipc_debug->stat_msg_handle    = stat_log_open("UL_MSG_HANDLE", STAT_MODE_COUNTER, 50 * 1000);
            ipc_debug->stat_msg_total     = stat_log_open("UL_MSG_TOTAL", STAT_MODE_COUNTER, 50 * 1000);
            ipc_debug->stat_wait_delay    = stat_log_open("UL_WAIT_DELAY", STAT_MODE_COUNTER, 10 * 1000);
            ipc_debug->stat_post_interval = stat_log_open("DL_POST_INTERVAL", STAT_MODE_COUNTER, 10 * 1000);
        }
#endif
        //    ipc_debug->stat_msg_build->set_limit(ipc_debug->stat_msg_build, 0, 600 * 1000);
        //    ipc_debug->stat_msg_ipc->set_limit(ipc_debug->stat_msg_ipc, 0, 600 * 1000);
        //    ipc_debug->stat_msg_handle->set_limit(ipc_debug->stat_msg_handle, 0, 600 * 1000);
        //    ipc_debug->stat_msg_total->set_limit(ipc_debug->stat_msg_total, 0, 1200 * 1000);
    }

    if(ipc_debug->enable_pcap)
    {
        nvlog_safe_strncpy(name, ipc_debug->prefix, NV_NAME_MAX_LEN);
        strncat(name, "_pcap", NV_NAME_SUFFIX_MAX_LEN);
        shmlogger_config_t cfg;
        cfg.save_to_file     = 1;          // Start a background thread to save SHM cache to file before overflow
        cfg.shm_cache_size   = (1L << nv_ipc_app_config_get(NV_IPC_CFG_PCAP_CACHE_BIT));  // 512MB, shared memory size, a SHM file will be created at /dev/shm/${name}_pcap
        cfg.max_file_size    = (1L << nv_ipc_app_config_get(NV_IPC_CFG_PCAP_FILE_BIT)); // 2GB Max file size, a disk file will be created at /var/log/aerial/${name}_pcap
        cfg.cpu_core_id      = nv_ipc_app_config_get(NV_IPC_CFG_PCAP_CPU_CORE);         // CPU core ID for the background if enabled.
        ipc_debug->shmlogger = shmlogger_open(ipc_debug->primary, name, &cfg);
        NVLOGC(TAG, "Open PCAP logger: prefix=%s cpu_core=%d cache_size=%ld MB round_save_size=%lu MB",
                name, cfg.cpu_core_id, cfg.shm_cache_size >> 20, cfg.max_file_size >> 20);
    }

    if(ipc_debug->primary && CONFIG_FORWARD_TEST_THREAD_CORE >= 0)
    {
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, forward_test_thread, ipc_debug->ipc);
    }

    return 0;
}

static int ipc_debug_close(nv_ipc_debug_t* ipc_debug)
{
    int ret = 0;
    if(ipc_debug->shmpool != NULL)
    {
        if(ipc_debug->shmpool->close(ipc_debug->shmpool) < 0)
        {
            NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: close SHM pool failed", __func__);
            ret = -1;
        }
    }

    free(ipc_debug);

    if(ret == 0)
    {
        NVLOGI(TAG, "%s: OK", __func__);
    }
    return 0;
}

static int ipc_debug_post_hook(nv_ipc_debug_t* ipc_debug)
{
    if(ipc_debug->enable_timing == 0)
    {
        return 0;
    }

    unsigned long  counter;
    sync_timing_t* sync_timing;
    if(ipc_debug->primary)
    {
        counter     = atomic_fetch_add(&ipc_debug->shm_data->post_counter_m2s, 1);
        sync_timing = ipc_debug->sync_timing_m2s + counter % ipc_debug->msg_pool_len;
    }
    else
    {
        counter     = atomic_fetch_add(&ipc_debug->shm_data->post_counter_s2m, 1);
        sync_timing = ipc_debug->sync_timing_s2m + counter % ipc_debug->msg_pool_len;
    }
    debug_get_timestamp(&sync_timing->ts_post);

    //    if(counter == 200 * 1000)
    //    {
    //        ipc_debug->stat_msg_build->set_limit(ipc_debug->stat_msg_build, 0, 500 * 1000);
    //        ipc_debug->stat_msg_ipc->set_limit(ipc_debug->stat_msg_ipc, 0, 500 * 1000);
    //        ipc_debug->stat_msg_handle->set_limit(ipc_debug->stat_msg_handle, 0, 500 * 1000);
    //        ipc_debug->stat_msg_total->set_limit(ipc_debug->stat_msg_total, 0, 1000 * 1000);
    //    }

    if(counter > 0)
    {
        sync_timing_t* sync_timing_last;
        if(counter % ipc_debug->msg_pool_len == 0)
        {
            sync_timing_last = sync_timing + ipc_debug->msg_pool_len - 1;
        }
        else
        {
            sync_timing_last = sync_timing - 1;
        }
        long tti_interval = debug_get_ts_interval(&sync_timing_last->ts_post, &sync_timing->ts_post);
        ipc_debug->stat_post_interval->add(ipc_debug->stat_post_interval, tti_interval);
    }
    return 0;
}

static int ipc_debug_wait_hook(nv_ipc_debug_t* ipc_debug)
{
    if(ipc_debug->enable_timing == 0)
    {
        return 0;
    }

    sync_timing_t* sync_timing;
    if(ipc_debug->primary)
    {
        unsigned long counter = atomic_fetch_add(&ipc_debug->shm_data->wait_counter_s2m, 1) % ipc_debug->msg_pool_len;
        sync_timing           = ipc_debug->sync_timing_s2m + counter;
    }
    else
    {
        unsigned long counter = atomic_fetch_add(&ipc_debug->shm_data->wait_counter_m2s, 1) % ipc_debug->msg_pool_len;
        sync_timing           = ipc_debug->sync_timing_m2s + counter;
    }
    debug_get_timestamp(&sync_timing->ts_wait);
    long sync_delay = debug_get_ts_interval(&sync_timing->ts_post, &sync_timing->ts_wait);
    ipc_debug->stat_wait_delay->add(ipc_debug->stat_wait_delay, sync_delay);
    return 0;
}

static int ipc_debug_alloc_hook(nv_ipc_debug_t* ipc_debug, nv_ipc_msg_t* msg, int32_t buf_index)
{
    if(CONFIG_LOG_ALLOCATE_TIME || ipc_debug->enable_timing)
    {
        msg_timing_t* msg_timing = ipc_debug->msg_timing + buf_index;
        debug_get_timestamp(&msg_timing->ts_alloc);
    }
    return 0;
}

sfn_slot_t nv_ipc_get_sfn_slot(nv_ipc_msg_t* msg)
{
    if(fapi_type == SCF_FAPI && msg->msg_id > SCF_FAPI_RESV_1_END)
    {
        scf_fapi_slot_header_t* header = msg->msg_buf;
        return header->sfn_slot;
    }
    else
    {
        return sfn_slot_invalid;
    }
}

void nv_ipc_set_handle_id(nv_ipc_msg_t* msg, uint8_t handle_id)
{
    if(fapi_type == SCF_FAPI)
    {
        scf_fapi_slot_header_t* header = msg->msg_buf;
        header->head.handle_id = handle_id;
    }
    else
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: unsupported fapi_type %d", __func__, fapi_type);
    }
}

static int ipc_debug_free_hook(nv_ipc_debug_t* ipc_debug, nv_ipc_msg_t* msg, int32_t buf_index)
{
    if(ipc_debug->enable_timing == 0)
    {
        return 0;
    }

    msg_timing_t* msg_timing = ipc_debug->msg_timing + buf_index;
    debug_get_timestamp(&msg_timing->ts_free);

    int64_t build_time     = debug_get_ts_interval(&msg_timing->ts_alloc, &msg_timing->ts_send);
    int64_t transport_time = debug_get_ts_interval(&msg_timing->ts_send, &msg_timing->ts_recv);
    int64_t handle_time    = debug_get_ts_interval(&msg_timing->ts_recv, &msg_timing->ts_free);
    int64_t total_time     = debug_get_ts_interval(&msg_timing->ts_alloc, &msg_timing->ts_free);

    sfn_slot_t ss = nv_ipc_get_sfn_slot(msg);
    NVLOGI(TAG, "SFN %d.%d IPC free FAPI=0x%02X Timing: build=%ld transport=%ld handle=%ld total=%ld", ss.u16.sfn, ss.u16.slot, msg->msg_id, build_time, transport_time, handle_time, total_time);

    int ret = 0;
    if(ipc_debug->stat_msg_build->add(ipc_debug->stat_msg_build, build_time) != 0)
    {
        ret |= 1;
    }
    if(ipc_debug->stat_msg_transport->add(ipc_debug->stat_msg_transport, transport_time) != 0)
    {
        ret |= 2;
    }
    if(ipc_debug->stat_msg_handle->add(ipc_debug->stat_msg_handle, handle_time) != 0)
    {
        ret |= 4;
    }
    if(ipc_debug->stat_msg_total->add(ipc_debug->stat_msg_total, total_time) != 0)
    {
        ret |= 8;
    }

    // Clear all
    memset(msg_timing, 0, sizeof(msg_timing_t));
    return ret;
}

static void save_ipc_msg(nv_ipc_debug_t* ipc_debug, nv_ipc_msg_t* msg, int32_t flags)
{
    // Workaround to get the right buffer address and payload length from Altran FAPI message
    int     ret = 0;
    void*   fapi_msg;
    int32_t ipc_len;

    if(fapi_tb_loc == 0)
    {
        if(msg->data_pool == NV_IPC_MEMPOOL_CPU_DATA)
        {
            fapi_msg = msg->data_buf;
            ipc_len  = msg->data_len;
        }
        else
        {
            fapi_msg = msg->msg_buf;
            ipc_len  = msg->msg_len;
        }
        int32_t fapi_len = parse_fapi_length(fapi_msg);
        shmlogger_save_buffer(ipc_debug->shmlogger, (const char*)fapi_msg, fapi_len, flags);
#if 0 // Do not call to improve performance
        int32_t fapi_id = parse_fapi_id(fapi_msg);
        if(fapi_id != msg->msg_id)
        {
            NVLOGI(TAG, "%s: fapi_id not match: msg_id=0x%02X fapi_id=0x%02X", __func__, msg->msg_id, fapi_id);
        }

        if(fapi_len != ipc_len)
        {
            NVLOGI(TAG, "%s: fapi_len not match: msg_id=0x%02X fapi_id=0x%02X fapi_len=%d ipc_len=%d", __func__, msg->msg_id, fapi_id, fapi_len, ipc_len);
        }
#endif
    }
    else if(fapi_tb_loc == 1)
    {
        shmlogger_save_ipc_msg(ipc_debug->shmlogger, msg, flags);
        return;
    }
    else
    {
        NVLOGI(TAG, "%s: fapi_tb_loc=%d is not supported", __func__, fapi_tb_loc);
        return;
    }
}


int64_t nv_ipc_get_buffer_ts_send(nv_ipc_debug_t* ipc_debug, int32_t buf_index)
{
    msg_timing_t* msg_timing = ipc_debug->msg_timing + buf_index;
    return msg_timing->ts_send.tv_sec * 1000000000LL + msg_timing->ts_send.tv_nsec;
}

static int ipc_debug_send_hook(nv_ipc_debug_t* ipc_debug, nv_ipc_msg_t* msg, int32_t buf_index)
{
    int ret = 0;
    if(CONFIG_LOG_SEND_TIME || ipc_debug->enable_timing)
    {
        msg_timing_t* msg_timing = ipc_debug->msg_timing + buf_index;
        debug_get_timestamp(&msg_timing->ts_send);
    }

    if(ipc_debug->enable_pcap)
    {
        save_ipc_msg(ipc_debug, msg, (IPC_UPLINK << 16) | msg->msg_id);
    }
    return ret;
}

static int ipc_debug_recv_hook(nv_ipc_debug_t* ipc_debug, nv_ipc_msg_t* msg, int32_t buf_index)
{
    if(ipc_debug->enable_timing)
    {
        msg_timing_t* msg_timing = ipc_debug->msg_timing + buf_index;
        debug_get_timestamp(&msg_timing->ts_recv);
    }

    if(ipc_debug->enable_pcap)
    {
        if (msg->msg_id == 0x82) {
            // Skip the loop-back SLOT.indication
            return 0;
        }
        save_ipc_msg(ipc_debug, msg, (IPC_DOWNLINK << 16) | msg->msg_id);
    }
    return 0;
}

static int ipc_debug_fw_deq_hook(nv_ipc_debug_t* ipc_debug, nv_ipc_msg_t* msg, int32_t buf_index)
{
    if(ipc_debug->enable_timing)
    {
        msg_timing_t* msg_timing = ipc_debug->msg_timing + buf_index;
        debug_get_timestamp(&msg_timing->ts_fw_deq);
    }
    return 0;
}

static int ipc_debug_fw_free_hook(nv_ipc_debug_t* ipc_debug, nv_ipc_msg_t* msg, int32_t buf_index)
{
    if(ipc_debug->enable_timing)
    {
        msg_timing_t* msg_timing = ipc_debug->msg_timing + buf_index;
        debug_get_timestamp(&msg_timing->ts_fw_free);
    }
    return 0;
}

int nv_ipc_dump_msg(nv_ipc_debug_t* ipc_debug, nv_ipc_msg_t* msg, int32_t buf_index, const char* info)
{
    char log_buf[1024];
    int  offset = snprintf(log_buf, 128, "%s:", info);

    if(fapi_type == SCF_FAPI)
    {
        sfn_slot_t ss = nv_ipc_get_sfn_slot(msg);
        offset += snprintf(log_buf + offset, 64, " SFN %d.%d", ss.u16.sfn, ss.u16.slot);
    }

    offset += snprintf(log_buf + offset, 64, " cell_id=%d msg_id=0x%02X buf_id=%d", msg->cell_id, msg->msg_id, buf_index);

    if(ipc_debug != NULL)
    {
        msg_timing_t* msg_timing = ipc_debug->msg_timing + buf_index;
        if(msg_timing->ts_alloc.tv_sec != 0)
        {
            offset += snprintf(log_buf + offset, 64, " Allocate: ");

            struct tm _tm;
            if(localtime_r(&msg_timing->ts_alloc.tv_sec, &_tm) != NULL)
            {
                // size +19
                offset += strftime(log_buf + offset, sizeof("1970-01-01 00:00:00"), "%Y-%m-%d %H:%M:%S", &_tm);
#ifdef DEBUG_HIGH_RESOLUTION_TIME
                // size +10
                offset += snprintf(log_buf + offset, 11, ".%09ld", msg_timing->ts_alloc.tv_nsec);
#else
                // size +7
                offset += snprintf(log_buf + offset, 8, ".%06ld", msg_timing->ts_alloc.tv_usec);
#endif
            }

            int64_t build_time     = msg_timing->ts_send.tv_sec == 0 ? -1 : debug_get_ts_interval(&msg_timing->ts_alloc, &msg_timing->ts_send);
            int64_t transport_time = msg_timing->ts_recv.tv_sec == 0 ? -1 : debug_get_ts_interval(&msg_timing->ts_send, &msg_timing->ts_recv);
            int64_t handle_time    = msg_timing->ts_free.tv_sec == 0 ? -1 : debug_get_ts_interval(&msg_timing->ts_recv, &msg_timing->ts_free);
            int64_t total_time     = msg_timing->ts_free.tv_sec == 0 ? -1 : debug_get_ts_interval(&msg_timing->ts_alloc, &msg_timing->ts_free);
            offset += snprintf(log_buf + offset, 256, " Interval: build=%ld transport=%ld handle=%ld total=%ld", build_time, transport_time, handle_time, total_time);
        }
    }

    log_buf[offset] = '\0';

    NVLOGC(TAG, "%s", log_buf);
    return 0;
}

nv_ipc_debug_t* nv_ipc_debug_open(nv_ipc_t* ipc, const nv_ipc_config_t* cfg)
{
    int             size      = sizeof(nv_ipc_debug_t);
    nv_ipc_debug_t* ipc_debug = malloc(size);
    if(ipc_debug == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: memory malloc failed", __func__);
        return NULL;
    }
    memset(ipc_debug, 0, size);
    ipc_debug->ipc       = ipc;
    ipc_debug->transport = cfg->ipc_transport;

    if(ipc_debug->transport == NV_IPC_TRANSPORT_SHM)
    {
        ipc_debug->primary      = cfg->transport_config.shm.primary;
        ipc_debug->msg_pool_len = cfg->transport_config.shm.mempool_size[NV_IPC_MEMPOOL_CPU_MSG].pool_len;
        nvlog_safe_strncpy(ipc_debug->prefix, cfg->transport_config.shm.prefix, NV_NAME_MAX_LEN);
    }
    else if(ipc_debug->transport == NV_IPC_TRANSPORT_DPDK)
    {
        ipc_debug->primary      = cfg->transport_config.dpdk.primary;
        ipc_debug->msg_pool_len = cfg->transport_config.dpdk.mempool_size[NV_IPC_MEMPOOL_CPU_MSG].pool_len;
        nvlog_safe_strncpy(ipc_debug->prefix, cfg->transport_config.dpdk.prefix, NV_NAME_MAX_LEN);
    }
    else if(ipc_debug->transport == NV_IPC_TRANSPORT_DOCA)
    {
        ipc_debug->primary      = cfg->transport_config.doca.primary;
        ipc_debug->msg_pool_len = cfg->transport_config.doca.mempool_size[NV_IPC_MEMPOOL_CPU_MSG].pool_len;
        nvlog_safe_strncpy(ipc_debug->prefix, cfg->transport_config.doca.prefix, NV_NAME_MAX_LEN);
    }
    else
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s unknown transport type %d", __func__, cfg->ipc_transport);
        free(ipc_debug);
        return NULL;
    }

    ipc_debug->alloc_hook = ipc_debug_alloc_hook;
    ipc_debug->free_hook  = ipc_debug_free_hook;
    ipc_debug->send_hook  = ipc_debug_send_hook;
    ipc_debug->recv_hook  = ipc_debug_recv_hook;
    ipc_debug->post_hook  = ipc_debug_post_hook;
    ipc_debug->wait_hook  = ipc_debug_wait_hook;

    ipc_debug->fw_deq_hook  = ipc_debug_fw_deq_hook;
    ipc_debug->fw_free_hook = ipc_debug_fw_free_hook;

    if(ipc_debug_open(ipc_debug) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: prefix=%s Failed", __func__, ipc_debug->prefix);
        ipc_debug_close(ipc_debug);
        return NULL;
    }
    else
    {
        NVLOGI(TAG, "%s: prefix=%s fapi_type=%d OK", __func__, ipc_debug->prefix, fapi_type);
        return ipc_debug;
    }

    return 0;
}
