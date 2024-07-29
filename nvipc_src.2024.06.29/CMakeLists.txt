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

# Minimum CMake version required.
# Using CUDA support in CMake 3.8+
# https://devblogs.nvidia.com/building-cuda-applications/cmake/
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
# option(CUPHY_HDF5_VERSION_CHECK "Force HDF5 library version checking" ON)

project(gt_common_libs LANGUAGES C CXX)

# ----------------------------------------------------------------------
# Global C++ options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------------------------------------------------------------------
# Below options doesn't take effect if had been defined in cuBB_SDK top CMakeLists.txt
OPTION(BUILD_NVIPC_ONLY "Build libnvipc.so only" ON)
OPTION(NVIPC_CUDA_ENABLE "Build libnvipc.so with CUDA" ON)
OPTION(NVIPC_GDRCPY_ENABLE "Build libnvipc.so with GDRCOPY" OFF)
OPTION(NVIPC_DPDK_ENABLE "Build libnvipc.so with DPDK" ON)
OPTION(DOCA_GPU_DPDK "Enable DOCA GPUNetIO" ON)
OPTION(NVIPC_DOCA_ENABLE "Build libnvipc.so with DOCA" ON)
OPTION(NVIPC_DOCA_GPUNETIO "Use DOCA gpunetio in DOCA GPU DMA" OFF)
OPTION(NVIPC_FMTLOG_ENABLE "Enable nvIPC logging with fmtlog" ON)

if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
   set(NVIPC_FMTLOG_ENABLE OFF)
endif()

# Set default CMAKE_BUILD_TYPE=Release, can be overwritten by cmake .. -DCMAKE_BUILD_TYPE=Debug
if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    SET(CMAKE_BUILD_TYPE Release)
endif()

find_package(PkgConfig REQUIRED)

# Disable NVIPC_DOCA_ENABLE if DOCA version < 2.5
pkg_search_module(DOCA QUIET doca)
if("${DOCA_VERSION}" STREQUAL "" OR DOCA_VERSION LESS 2.5)
    message("DOCA_VERSION=${DOCA_VERSION}, set NVIPC_DOCA_ENABLE=OFF")
    set(NVIPC_DOCA_ENABLE OFF)
endif()

# Enable NVIPC_DPDK_ENABLE if DPDK exist
pkg_search_module(DPDK QUIET libdpdk)
if("${DPDK_VERSION}" STREQUAL "")
    message("DPDK_VERSION not defined, set NVIPC_DPDK_ENABLE=OFF")
    set(NVIPC_DPDK_ENABLE OFF)
endif()

if(NVIPC_DPDK_ENABLE)
    add_definitions(-DNVIPC_DPDK_ENABLE)
    if(DPDK_VERSION LESS 22.11)
        message("DPDK_VERSION=${DPDK_VERSION}, set DOCA_GPU_DPDK_OLD=1")
        add_definitions(-DDOCA_GPU_DPDK_OLD=1)
    else()
        message("DPDK_VERSION=${DPDK_VERSION}, set DOCA_GPU_DPDK_OLD=0")
        add_definitions(-DDOCA_GPU_DPDK_OLD=0)
    endif()
endif()

find_package(CUDAToolkit QUIET)
if(NOT CUDAToolkit_FOUND OR CUDAToolkit_VERSION LESS 11.3)
    message("CUDAToolkit_FOUND=${CUDAToolkit_FOUND} CUDAToolkit_VERSION=${CUDAToolkit_VERSION}: Disable CUDA")
    set(NVIPC_CUDA_ENABLE OFF)
endif()

if(NVIPC_CUDA_ENABLE)
    enable_language(CUDA)
    add_definitions(-DNVIPC_CUDA_ENABLE)
endif()

# Get system ARCH
execute_process(COMMAND arch OUTPUT_VARIABLE ARCH)
string(STRIP ${ARCH} ARCH)

message("=========================================================")
message("Build nvipc for [${ARCH}] CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
message("NVIPC_CUDA_ENABLE=${NVIPC_CUDA_ENABLE} CUDAToolkit_VERSION=${CUDAToolkit_VERSION}")
message("NVIPC_DPDK_ENABLE=${NVIPC_DPDK_ENABLE} DPDK_VERSION=${DPDK_VERSION}")
message("NVIPC_DOCA_ENABLE=${NVIPC_DOCA_ENABLE} DOCA_VERSION=${DOCA_VERSION}")
message("DOCA_GPU_DPDK=${DOCA_GPU_DPDK}")
message("NVIPC_DOCA_GPUNETIO=${NVIPC_DOCA_GPUNETIO}")
message("NVIPC_GDRCPY_ENABLE=${NVIPC_GDRCPY_ENABLE}")
message("NVIPC_FMTLOG_ENABLE=${NVIPC_FMTLOG_ENABLE}")
message("=========================================================")

if(NVIPC_GDRCPY_ENABLE)
    add_definitions(-DNVIPC_GDRCPY_ENABLE)
endif()

if (DOCA_GPU_DPDK)
    add_definitions(-DDOCA_GPU_DPDK=1)
endif (DOCA_GPU_DPDK)

# ====== Build libnvipc.so only for partner ============================
if(BUILD_NVIPC_ONLY)
    message("Build nvipc only for [${ARCH}]")

    # Please export PKG_CONFIG_PATH to the DPDK pkgconfig path
    message(STATUS "PKG_CONFIG_PATH=$ENV{PKG_CONFIG_PATH}")
    add_definitions(-DBUILD_NVIPC_ONLY)

    if (NOT (TARGET yaml))
        message(STATUS "including libyaml")
        add_subdirectory(external/libyaml)
    endif()
    set_property(TARGET yaml PROPERTY POSITION_INDEPENDENT_CODE ON)

    if(NVIPC_FMTLOG_ENABLE)
        if (NOT (TARGET fmt))
            message(STATUS "including fmtlog")
            add_subdirectory(external/fmtlog_flat)
            set_property(TARGET fmt PROPERTY POSITION_INDEPENDENT_CODE ON)
            set_property(TARGET fmtlog-static PROPERTY POSITION_INDEPENDENT_CODE ON)
        endif()
        add_definitions(-DNVIPC_FMTLOG_ENABLE)
    endif()
    if (NOT TARGET nvlog)
        add_subdirectory(nvlog)
    endif()

    if (NOT TARGET nvipc)
        message(STATUS "including nvipc")
        add_subdirectory(nvIPC)
    endif()

    RETURN()
endif()

# ====== Build libnvipc.so inside cuBB_SDK =============================

# ----------------------------------------------------------------------
# Additional packages
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

set(HDF5_USE_STATIC_LIBRARIES ON)
if (CUPHY_HDF5_VERSION_CHECK)
    find_package(HDF5 1.10 REQUIRED COMPONENTS C)
else (CUPHY_HDF5_VERSION_CHECK)
    find_package(HDF5 REQUIRED COMPONENTS C)
endif (CUPHY_HDF5_VERSION_CHECK)

# check for CURL
find_package(CURL REQUIRED)

# check for SCTP
find_library(Sctp NAMES sctp)
if(Sctp MATCHES Sctp-NOTFOUND)
  message(FATAL_ERROR "libsctp not found. RPC over SCTP support will not be compiled")
endif()


# ----------------------------------------------------------------------
# Subdirectories

add_subdirectory(aerial_metrics)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cuPHY/CMakeLists.txt")
  add_subdirectory(cuPHY)
endif()

if (NOT TARGET nvipc)
    message(STATUS "including nvipc")
    add_subdirectory(nvIPC)
endif()

if (NOT TARGET oran_utils)
  message(STATUS "including oran_utils")
  add_subdirectory(oran_utils)
endif()

if (NOT TARGET slot_command)
    message(STATUS "including slot_comamnd")
    add_subdirectory(slot_command)
endif()
