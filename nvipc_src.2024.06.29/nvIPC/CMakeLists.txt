# Copyright (c) 2017-2024, NVIDIA CORPORATION.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted
# provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright notice, this list of
#       conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright notice, this list of
#       conditions and the following disclaimer in the documentation and/or other materials
#       provided with the distribution.
#     * Neither the name of the NVIDIA CORPORATION nor the names of its contributors may be used
#       to endorse or promote products derived from this software without specific prior written
#       permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
# FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL NVIDIA CORPORATION BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TOR (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

include(GNUInstallDirs)

# lib: nvipc
set(SOURCES configs/nv_ipc_config.c
            configs/nv_ipc_yaml_parser.cpp
            common/fd_share.c
            common/array_queue.c
            common/nv_ipc_debug.c
            common/nv_ipc_ring.c
            common/nv_ipc_mempool.c
            common/nv_ipc_shm.c
            common/nv_ipc_sem.c
            common/nv_ipc_efd.c
            common/nv_ipc_epoll.c
            common/shm_logger.c
            shm/nv_ipc_shm_if.c
            udp/nv_ipc_udp_if.c
            nv_ipc_common.c)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} --forward-unknown-to-host-compiler")

if(NVIPC_CUDA_ENABLE)
    set(SOURCES ${SOURCES} cuda/nv_ipc_cudapool.cu cuda/nv_ipc_cuda_utils.cu)
endif()

if(NVIPC_DPDK_ENABLE)
    set(SOURCES ${SOURCES} dpdk/nv_ipc_dpdk_if.c dpdk/nv_ipc_dpdk_utils.c)
endif()

if(NVIPC_GDRCPY_ENABLE)
    set(SOURCES ${SOURCES} gpudata/nv_ipc_gpudatapool.cu gpudata/nv_ipc_gpudata_utils.cu)
endif()

if(NVIPC_DOCA_ENABLE)
    set(SOURCES ${SOURCES} doca/nv_ipc_doca_if.c doca/nv_ipc_doca_utils.c)
endif()

add_library(nvipc SHARED ${SOURCES})
target_include_directories(nvipc PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_include_directories(nvipc PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/common>)
target_include_directories(nvipc PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cuda>)
target_include_directories(nvipc PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gpudata>)

set_target_properties(nvipc PROPERTIES PUBLIC_HEADER
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_compile_options(nvipc PRIVATE ${AERIAL_ARCH_TUNE_FLAGS})

if(NVIPC_DPDK_ENABLE OR NVIPC_DOCA_GPUNETIO)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(DPDK REQUIRED libdpdk)
    # Link to DPDK shared libraries
    target_compile_options(nvipc PRIVATE ${DPDK_CFLAGS})
    target_include_directories(nvipc PRIVATE ${DPDK_INCLUDE_DIRS})
    target_link_directories(nvipc PRIVATE ${DPDK_LIBRARY_DIRS})
    target_link_libraries(nvipc PRIVATE -Wl,--no-as-needed)
    target_link_libraries(nvipc PRIVATE ${DPDK_LIBRARIES})
    if(DEFINED DPDK_TARGET)
        add_dependencies(nvipc ${DPDK_TARGET})
    endif()
endif()

if(NVIPC_DOCA_ENABLE)
    find_package(PkgConfig REQUIRED)
    pkg_search_module(DOCA REQUIRED doca doca-libs)
    add_definitions(-DNVIPC_DOCA_ENABLE)
    message("DOCA_CFLAGS=${DOCA_CFLAGS}")
    message("DOCA_INCLUDE_DIRS=${DOCA_INCLUDE_DIRS}")
    message("DOCA_LIBRARY_DIRS=${DOCA_LIBRARY_DIRS}")
    message("DOCA_LIBRARY_DIRS=${DOCA_LIBRARIES}")

    target_compile_options(nvipc PUBLIC ${DOCA_CFLAGS})
    target_include_directories(nvipc PUBLIC ${DOCA_INCLUDE_DIRS})
    target_link_directories(nvipc PUBLIC ${DOCA_LIBRARY_DIRS})
    target_link_libraries(nvipc PUBLIC ${DOCA_LIBRARIES})

    add_definitions(-Wno-deprecated-declarations)
endif()

if(NVIPC_DOCA_GPUNETIO)
    add_definitions(-DNVIPC_DOCA_GPUNETIO)
    target_link_libraries(nvipc PUBLIC -ldoca_gpunetio -lcuda)
endif()

if(NVIPC_GDRCPY_ENABLE)
    target_include_directories(nvipc /usr/local/gdrcopy/include)
    target_link_libraries(nvipc PRIVATE cudai gdrapi)
endif()

if(BUILD_NVIPC_ONLY)
    # Build a independent libnvipc.so for release to partner
    target_link_libraries(nvipc PUBLIC rt -pthread nvlog_static)
    target_link_libraries(nvipc PUBLIC -static-libstdc++)
else()
    target_link_libraries(nvipc PUBLIC rt -pthread nvlog)
endif()

install(TARGETS nvipc
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/..)

add_subdirectory(tests)
