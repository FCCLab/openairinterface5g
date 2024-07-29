
/*
 * Copyright (c) 2020-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <stdatomic.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include <libgen.h>
#include <linux/limits.h>

#include "aerial_event_code.h"
#include "nvlog.h"

#define TAG (NVLOG_TAG_BASE_NVLOG + 9) // "NVLOG.C"
#define TAG_LOG_COLLECT (NVLOG_TAG_BASE_NVLOG + 9) // "NVLOG.C"

static const char str[8][3] = {"N", "F", "E", "C", "W", "I", "D", "V"};

static int nvlog_c_log_level = NVLOG_CONSOLE;

#ifdef NVIPC_FMTLOG_ENABLE
int is_fmt_log_initiated();
int fmt_log_level_validate(int level, int itag, const char** stag);
void nvlog_vprint_fmt(int level, const char* stag, const char* format, va_list va);
void nvlog_e_vprint_fmt(int level, const char* event, const char* stag, const char* format, va_list va);
void nvlog_c_init_fmt(const char *file);
void nvlog_c_close_fmt();
void nvlog_set_log_level_fmt(int log_level);
void nvlog_set_max_file_size_fmt(size_t size);
#endif

static inline int getIndex(int level)
{
    int index = 0;
    switch(level)
    {
        case NVLOG_FATAL:
        {
            index = 1;
            break;
        }
        case NVLOG_ERROR:
        {
            index = 2;
            break;
        }
        case NVLOG_CONSOLE:
        {
            index = 3;
            break;
        }
        case NVLOG_WARN:
        {
            index = 4;
            break;
        }
        case NVLOG_INFO:
        {
            index = 5;
            break;
        }
        case NVLOG_DEBUG:
        {
            index = 6;
            break;
        }
        case NVLOG_VERBOSE:
        {
            index = 7;
            break;
        }
        case NVLOG_NONE:
        default:
        {
            printf("invalid log level %d, setting to WRN level\n", level);
            index = 0;
            break;
        }
    }
    return index;
}

void nvlog_c_print(int level, int itag, const char* format, ...)
{
#ifdef NVIPC_FMTLOG_ENABLE
    if (is_fmt_log_initiated())
    {
        const char* stag = NULL;
        if(fmt_log_level_validate(level,itag,&stag))
        {
            if(stag != NULL)
            {
                va_list va;
                va_start(va, format);
                nvlog_vprint_fmt(level, stag, format, va);
                va_end(va);
            }
        }
        return;
    }
#endif

    if (level <= nvlog_c_log_level)
    {
        printf("[%s]: ", str[getIndex(level)]);
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");
    }
}

void nvlog_e_c_print(int level, int itag, const char *event, const char* format, ...)
{
#ifdef NVIPC_FMTLOG_ENABLE
    if (is_fmt_log_initiated())
    {
        const char* stag = NULL;
        if(fmt_log_level_validate(level,itag,&stag))
        {
            if(stag != NULL)
            {
                va_list va;
                va_start(va, format);
                nvlog_e_vprint_fmt(level, event, stag, format, va);
                va_end(va);
            }
        }
        return;
    }
#endif

    if (level <= nvlog_c_log_level)
    {
        printf("[%s][%s]: ", str[getIndex(level)], event);
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");
    }
}

void nvlog_set_log_level(int log_level)
{
    nvlog_c_log_level = log_level;
#ifdef NVIPC_FMTLOG_ENABLE
    nvlog_set_log_level_fmt(log_level);
#endif
}

void nvlog_set_max_file_size(size_t size)
{
#ifdef NVIPC_FMTLOG_ENABLE
    nvlog_set_max_file_size_fmt(size);
#endif
}

void nvlog_c_init(const char *file)
{
    // Get directory of the file
    char path_copy[PATH_MAX];
    nvlog_safe_strncpy(path_copy, file, PATH_MAX);
    char* dir = dirname(path_copy);

    // Create folder if not exist
    char cmd[PATH_MAX];
    snprintf(cmd, PATH_MAX, "mkdir -p %s", dir);
    if(system(cmd) != 0)
    {
        NVLOGE_NO(TAG, AERIAL_SYSTEM_API_EVENT, "%s: [%s] err=%d - %s\n", __func__, cmd, errno, strerror(errno));
    }

#ifdef NVIPC_FMTLOG_ENABLE
    nvlog_c_init_fmt(file);
#endif
}

void nvlog_c_close()
{
#ifdef NVIPC_FMTLOG_ENABLE
    nvlog_c_close_fmt();
#endif
}
