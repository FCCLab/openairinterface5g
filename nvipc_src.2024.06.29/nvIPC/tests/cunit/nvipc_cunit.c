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

#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#include "test_timing.h"
#include "test_common.h"
#include "test_ipc.h"
#include "nv_utils.h"

#define MAX_NAME_LEN 32
int TAG = (NVLOG_TAG_BASE_NVIPC + 28); // "TEST"

#ifdef BUILD_NVIPC_ONLY
#define YAML_CONFIG_FILE "../../../../nvIPC/tests/example/nvipc_config.yaml"
#else
#define YAML_CONFIG_FILE "../../../../../../cuPHY-CP/gt_common_libs/nvIPC/tests/example/nvipc_config.yaml"
#endif

// Log level: NVLOG_ERROR, NVLOG_CONSOLE, NVLOG_WARN, NVLOG_INFO, NVLOG_DEBUG, NVLOG_VERBOSE
#define DEFAULT_TEST_LOG_LEVEL NVLOG_INFO
#define DEFAULT_TEST_LOG_LEVEL_CONSOLE NVLOG_CONSOLE

typedef struct
{
    const char* strName;
    CU_TestFunc pTestFunc;
} test_case_t;

static void test_ipc_open(void);
static void test_ipc_close(void);
static void wait_for_finish(void)
{
    usleep(1000L * 1000 * 1);
}

static test_case_t shm_ipc_cases[] = {
    {"test_assign_cpu", test_assign_cpu},
    {"test_ipc_open", test_ipc_open},
    {"test_ring_single_thread", test_ring_single_thread},
    {"test_ring_multi_thread", test_ring_multi_thread},
    {"test_cpu_mempool", test_cpu_mempool},

    {"test_dl_blocking_transfer", test_dl_blocking_transfer},
    {"test_ul_blocking_transfer", test_ul_blocking_transfer},
    {"test_dl_epoll_transfer", test_dl_epoll_transfer},
    {"test_ul_epoll_transfer", test_ul_epoll_transfer},

    {"test_dl_no_sync_transfer", test_dl_no_sync_transfer},
    {"test_ul_no_sync_transfer", test_ul_no_sync_transfer},
    {"test_duplex_no_sync_transfer", test_duplex_no_sync_transfer},
    {"test_duplex_mt_no_sync_transfer", test_duplex_mt_no_sync_transfer},

    {"test_transfer_multi_thread", test_dl_transfer_multi_thread},
    {"test_transfer_duplex_multi_thread", test_transfer_duplex_multi_thread},
    {"test_lockless_queue", test_lockless_queue},

    {"test_ipc_close", test_ipc_close},
    // {"test_stat_log", test_stat_log},
    // {"test_timing", test_timing},
};

static test_case_t dpdk_ipc_cases[] = {
    {"test_assign_cpu", test_assign_cpu},
    {"test_ipc_open", test_ipc_open},

    {"test_dl_no_sync_transfer", test_dl_no_sync_transfer},

    {"wait_for_finish", wait_for_finish},

    {"test_ipc_close", test_ipc_close},
};

static test_case_t doca_ipc_cases[] = {
    {"test_assign_cpu", test_assign_cpu},
    {"test_ipc_open", test_ipc_open},

    {"test_dl_blocking_transfer", test_dl_blocking_transfer},
    {"test_ul_blocking_transfer", test_ul_blocking_transfer},

    {"test_dl_epoll_transfer", test_dl_epoll_transfer},
    {"test_ul_epoll_transfer", test_ul_epoll_transfer},

    {"test_dl_no_sync_transfer", test_dl_no_sync_transfer},
    {"test_ul_no_sync_transfer", test_ul_no_sync_transfer},

    {"test_duplex_no_sync_transfer", test_duplex_no_sync_transfer},
    {"test_duplex_mt_no_sync_transfer", test_duplex_mt_no_sync_transfer},

    {"wait_for_finish", wait_for_finish},

    {"test_ipc_close", test_ipc_close},
};

/***** Global variables **************/
// The CUDA device ID. Can set to -1 to fall back to CPU memory IPC
int                test_cuda_device_id = -1;
int                nproc_num;
nv_ipc_transport_t ipc_transport;
nv_ipc_module_t    module_type;
nv_ipc_t*          ipc;
nv_ipc_config_t    cfg;
nv_ipc_sem_t*      sync_sem;
nv_ipc_efd_t*      sync_efd;

/***** Static variables **************/
static int enable_sync    = 1;
static int global_primary = 0;

struct timespec ts_test_start, ts_test_end;

#define SYNC_BY_SEM 1

int is_primary(void)
{
    if(global_primary)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static int sync_wait(void)
{
    if(SYNC_BY_SEM)
    {
        CU_ASSERT_NOT_EQUAL_FATAL(sync_sem, NULL);
        return sync_sem->sem_wait(sync_sem);
    }
    else
    {
        CU_ASSERT_NOT_EQUAL_FATAL(sync_efd, NULL);
        return sync_efd->get_value(sync_efd);
    }
}

static int sync_notify(void)
{
    if(SYNC_BY_SEM)
    {
        CU_ASSERT_NOT_EQUAL_FATAL(sync_sem, NULL);
        return sync_sem->sem_post(sync_sem);
    }
    else
    {
        CU_ASSERT_NOT_EQUAL_FATAL(sync_efd, NULL);
        return sync_efd->notify(sync_efd, 1);
    }
}

static void sync_open(void)
{
    if(SYNC_BY_SEM)
    {
        sync_sem = nv_ipc_sem_open(is_primary(), "test_sync");
    }
    else
    {
        sync_efd = nv_ipc_efd_open(is_primary(), "test_sync");
    }

    if(!is_primary())
    {
        // Secondary wait 1 second to let primary start first
        usleep(1000 * 1000);
    }
}

static void sync_close(void)
{
    if(SYNC_BY_SEM)
    {
        if(sync_sem != NULL)
        {
            sync_sem->close(sync_sem);
        }
    }
    else
    {
        if(sync_efd != NULL)
        {
            sync_efd->close(sync_efd);
        }
    }
    if(is_primary())
    {
        usleep(100 * 000);
    }
}

void sync_primary_first(const char* info)
{
    if(!is_primary() && enable_sync)
    {
        NVLOGD(TAG, "Wait primary work: %s", info);
        int ret = sync_wait();
        NVLOGD(TAG, "Wait primary work: %s ret=%d", info, ret);
        CU_ASSERT(ret >= 0);
    }
}

void sync_primary_end(const char* info)
{
    if(is_primary() && enable_sync)
    {
        NVLOGD(TAG, "Primary work ended: %s", info);
        int ret = sync_notify();
        CU_ASSERT(ret >= 0);
    }
}

void sync_secondary_first(const char* info)
{
    if(is_primary() && enable_sync)
    {
        NVLOGD(TAG, "Wait secondary work: %s", info);
        int ret = sync_wait();
        NVLOGD(TAG, "Wait salve work: %s ret=%d", info, ret);
        CU_ASSERT(ret >= 0);
    }
}

void sync_secondary_end(const char* info)
{
    if(!is_primary() && enable_sync)
    {
        NVLOGD(TAG, "Secondary work ended: %s", info);
        int ret = sync_notify();
        CU_ASSERT(ret >= 0);
    }
}

void sync_together(const char* info)
{
    if(enable_sync)
    {
        NVLOGD(TAG, "Sync together start: %s", info);
        int ret1 = sync_notify();
        CU_ASSERT(ret1 >= 0);
        int ret2 = sync_wait();
        CU_ASSERT(ret2 >= 0);
        NVLOGD(TAG, "------------Sync together: %s ret1=%d ret2=%d", info, ret1, ret2);
    }
}

static int test_suite_init(void) { return 0; }

static int init_suite_clean(void) { return 0; }

void process_init(int primary)
{
    global_primary = primary ? 1 : 0;

    sync_open();

    if(global_primary)
    {
        module_type = NV_IPC_MODULE_PRIMARY;
        TAG = (NVLOG_TAG_BASE_NVIPC + 29); // "PHY"
    }
    else
    {
        module_type = NV_IPC_MODULE_SECONDARY;
        TAG = (NVLOG_TAG_BASE_NVIPC + 30); // "MAC"
    }
}

void test_process_fork(int module)
{
    if(module == 2)
    {
        pid_t fpid = fork();
        if(fpid < 0)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "error in fork!");
            return;
        }
        else if(fpid == 0)
        {
            NVLOGC(TAG, "Forked Child process: PID=%d", getpid());
            process_init(0);
        }
        else
        {
            NVLOGC(TAG, "Forked Parent process: Child PID=%d", getpid());
            process_init(1);
        }
    }
    else if(module == 1)
    {
        process_init(1);
    }
    else if(module == 0)
    {
        process_init(0);
    }
    else
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "Invalid parameter module=%d transport=%d PID=%d", module, ipc_transport, getpid());
        exit(-1);
    }
}

static void test_ipc_open(void)
{
    if (pthread_setname_np(pthread_self(), "test_task") != 0) {
        NVLOGE_NO(TAG, AERIAL_THREAD_API_EVENT, "%s: set thread name failed", __func__);
    }

    // sync_secondary_first("secondary connection");
    // sync_secondary_end("secondary connection");

    // sync_primary_first("crate ipc interface");

    NVLOGI(TAG, "--------------- Test create ---------------");
    ipc = create_nv_ipc_interface(&cfg);
    NVLOGI(TAG, "-------------------------------------------");

    // Assert
    CU_ASSERT_NOT_EQUAL_FATAL(ipc, NULL);

    // sync_primary_end("crate ipc interface");
    sync_together("IPC open finished");
    sleep(1);

    if(clock_gettime(CLOCK_MONOTONIC, &ts_test_start) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s clock_gettime %s", __func__, strerror(errno));
    }
}

static void test_ipc_close(void)
{
    CU_ASSERT_NOT_EQUAL_FATAL(ipc, NULL);

    sync_secondary_first("close ipc interface");

    NVLOGI(TAG, "--------------- Test destroy ---------------");
    int ret = -1;
    if(ipc != NULL)
    {
        NVLOGI(TAG, "%s open: transport=%d module_type=%d - OK", __func__, cfg.ipc_transport, cfg.module_type);
        ret = ipc->ipc_destroy(ipc);
    }
    else
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s open: transport=%d module_type=%d ipc==NULL - Failed", __func__, cfg.ipc_transport, cfg.module_type);
    }
    CU_ASSERT_NOT_EQUAL(ipc, NULL);
    CU_ASSERT(ret == 0);
    if(ret == 0)
    {
        NVLOGI(TAG, "%s close: transport=%d module_type=%d - OK", __func__, cfg.ipc_transport, cfg.module_type);
    }
    else
    {
        NVLOGI(TAG, "%s close: transport=%d module_type=%d - ret=%d Failed", __func__, cfg.ipc_transport, cfg.module_type, ret);
    }
    NVLOGI(TAG, "-------------------------------------------");

    sync_secondary_end("close ipc interface");
    sync_together("interface close");
    if(clock_gettime(CLOCK_MONOTONIC, &ts_test_end) < 0)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s clock_gettime %s", __func__, strerror(errno));
    }
    sync_close();
}

int add_test_cases(CU_pSuite pSuite, test_case_t test_cases[], int num)
{
    // Add the tests to the suite
    int i;
    for(i = 0; i < num; i++)
    {
        if(CU_add_test(pSuite, test_cases[i].strName, test_cases[i].pTestFunc) == NULL)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: CU_add_test %s failed", __func__, test_cases[i].strName);
            CU_cleanup_registry();
            return CU_get_error();
        }
        else
        {
            NVLOGC(TAG, "%s: Added test case: %s", __func__, test_cases[i].strName);
        }
    }
    return 0;
}

/************* Test Runner Code goes here **************/
int main(int argc, char** argv)
{
    if (pthread_setname_np(pthread_self(), "cunit_fmtlog") != 0) {
        NVLOGE_NO(TAG, AERIAL_THREAD_API_EVENT, "%s: set thread name failed", __func__);
    }

    int module, transport;
    if(argc < 3 || (transport = atoi(argv[1])) < 0 || (module = atoi(argv[2])) < 0)
    {
        fprintf(stderr, "Usage: nvipc_cunit <transport> <module>\n");
        fprintf(stderr, "    transport:      1 - SHM;    2 - DPDK;    3 - Config by YAML\n");
        fprintf(stderr, "    module:         0 - secondary;  1 - primary; 2 - start both by fork.\n");
        exit(1);
    }

    NVLOGC(TAG, "%s: transport=%d module=%d argc=%d argv[0]=%s", __func__, transport, module, argc, argv[0]);
    printf("PRINTF: %s: transport=%d module=%d argc=%d argv[0]=%s\n", __func__, transport, module, argc, argv[0]);

    int use_yaml_config = 0;
    switch(transport)
    {
    case 0:
        ipc_transport = NV_IPC_TRANSPORT_UDP;
        break;
    case 1:
        ipc_transport = NV_IPC_TRANSPORT_SHM;
        break;
    case 2:
        ipc_transport = NV_IPC_TRANSPORT_DPDK;
        break;
    case 3:
        use_yaml_config = 1;
        break;
    default:
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: unsupported transport=%d", __func__, transport);
        return 0;
    }

    // Fork and set module_type
    test_process_fork(module);

    if (pthread_setname_np(pthread_self(), "cunit_init") != 0) {
        NVLOGE_NO(TAG, AERIAL_THREAD_API_EVENT, "%s: set thread name failed", __func__);
    }

    nproc_num = get_nprocs();

    // Get nvipc configuration
    if(use_yaml_config)
    {
        char yaml_path[1024];
        if(argc < 4 || argv[3] == NULL)
        {
            // Use default YAML configuration file
            nv_get_absolute_path(yaml_path, YAML_CONFIG_FILE);
        }
        else
        {
            // Use input YAML configuration file
            nvlog_safe_strncpy(yaml_path, argv[3], 1024);
        }

        NVLOGC(TAG, "YAML configuration file: %s", yaml_path);
        load_nv_ipc_yaml_config(&cfg, yaml_path, module_type);

        ipc_transport = cfg.ipc_transport;
        if(module == 2 && ipc_transport != NV_IPC_TRANSPORT_SHM)
        {
            NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: should not fork for non-SHM IPC test", __func__);
            return -1;
        }
    }
    else
    {
        if(global_primary == 1)
        {
            nvlog_c_init("/var/log/aerial/nvipc_primary.log");
        }
        else
        {
            nvlog_c_init("/var/log/aerial/nvipc_secondary.log");
        }
        nvlog_set_log_level(NVLOG_CONSOLE);

        cfg.ipc_transport = ipc_transport;
        set_nv_ipc_default_config(&cfg, module_type);
    }

    detect_cpu_cores();
    nv_assign_thread_cpu_core(task_info.cpu_cores[task_info.free_core_num-- - 1]);

    if (pthread_setname_np(pthread_self(), "cunit_test") != 0) {
        NVLOGE_NO(TAG, AERIAL_THREAD_API_EVENT, "%s: set thread name failed", __func__);
    }

    if (ipc_transport != NV_IPC_TRANSPORT_SHM)
    {
        enable_sync = 0;
    }

    if (ipc_transport == NV_IPC_TRANSPORT_DPDK) {
        cfg.transport_config.dpdk.lcore_id = task_info.cpu_cores[task_info.free_core_num - 1];
        task_info.free_core_num--;
    } else if (ipc_transport == NV_IPC_TRANSPORT_DOCA) {
        cfg.transport_config.doca.cpu_core = task_info.cpu_cores[task_info.free_core_num - 1];
        task_info.free_core_num--;
    }

    // Create CUnit test registry
    if(CU_initialize_registry() != CUE_SUCCESS)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: CU_initialize_registry failed", __func__);
        return CU_get_error();
    }

    // Add test suite
    CU_pSuite pSuite = CU_add_suite("nvipc", test_suite_init, init_suite_clean);
    if(pSuite == NULL)
    {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: CU_add_suite failed", __func__);
        CU_cleanup_registry();
        return CU_get_error();
    }

    // Add the tests to the suite
    if (ipc_transport == NV_IPC_TRANSPORT_SHM) {
        add_test_cases(pSuite, shm_ipc_cases, sizeof(shm_ipc_cases) / sizeof(test_case_t));
    } else if (ipc_transport == NV_IPC_TRANSPORT_DPDK) {
        add_test_cases(pSuite, dpdk_ipc_cases, sizeof(dpdk_ipc_cases) / sizeof(test_case_t));
    } else if (ipc_transport == NV_IPC_TRANSPORT_DOCA) {
        add_test_cases(pSuite, doca_ipc_cases, sizeof(doca_ipc_cases) / sizeof(test_case_t));
    } else {
        NVLOGE_NO(TAG, AERIAL_NVIPC_API_EVENT, "%s: unsupported transport=%d", __func__, transport);
    }

    init_perf_result();

    // Report mode
    CU_set_output_filename("nvipc_cunit_output.xml");
    // CU_list_tests_to_file();
    // CU_automated_run_tests();

    if (pthread_setname_np(pthread_self(), "run_test") != 0) {
        NVLOGE_NO(TAG, AERIAL_THREAD_API_EVENT, "%s: set thread name failed", __func__);
    }

    // CUnit test basic mode
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();

    // Console mode
    // CU_console_run_tests();
    // CU_console_run_tests();

    if (pthread_setname_np(pthread_self(), "main_end") != 0) {
        NVLOGE_NO(TAG, AERIAL_THREAD_API_EVENT, "%s: set thread name failed", __func__);
    }

    CU_pSuite s     = CU_get_registry()->pSuite;
    int       count = 0;
    while(s)
    {
        CU_pTest t = s->pTest;
        while(t)
        {
            count++;
            t = t->pNext;
        }
        s = s->pNext;
    }

    NVLOGI(TAG, "%d..%d", 1, count);

    NVLOGC(TAG, "===========================================");
    s     = CU_get_registry()->pSuite;
    count = 1;
    int test_result = 0;
    while(s)
    {
        CU_pTest t = s->pTest;
        while(t)
        {
            int               pass     = 1;
            CU_FailureRecord* failures = CU_get_failure_list();
            while(failures)
            {
                if(strcmp(failures->pSuite->pName, s->pName) == 0 &&
                   strcmp(failures->pTest->pName, t->pName) == 0)
                {
                    pass     = 0;
                    failures = 0;
                }
                else
                {
                    failures = failures->pNext;
                }
            }

            if(pass)
            {
                NVLOGC(TAG, "ok %d - %s:%s", count, s->pName, t->pName);
            }
            else
            {
                test_result = 1;
                NVLOGC(TAG, "fail %d - %s:%s", count, s->pName, t->pName);
            }

            count++;
            t = t->pNext;
        }
        s = s->pNext;
    }
    NVLOGC(TAG, "===========================================");
    long ns = nvlog_timespec_interval(&ts_test_start, &ts_test_end);

    // Clean up
    CU_cleanup_registry();

    if (CU_get_error() != CUE_SUCCESS) {
        test_result = 2;
    }

    NVLOGC(TAG, "==== Total time: %ld.%09ld seconds, test_result=%d ====", ns / NUMBER_1E9,
            ns % NUMBER_1E9, test_result);

    print_perf_result();

    nvlog_c_close();

    return test_result;
}
