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
# Using CUDA support inn CMake 3.8+
# https://devblogs.nvidia.com/building-cuda-applications/cmake/

# string(APPEND CMAKE_CUDA_FLAGS " --cudart shared -lineinfo ")
set(CMAKE_CUDA_STANDARD 17)

# include_directories("/usr/local/cuda-10.1/samples/common/inc")
# include_directories("/usr/local/cuda/include")

set(SOURCES test_cuda.cu)
add_library(cudaipctest STATIC ${SOURCES})
target_link_libraries(cudaipctest nvipc)

set(SOURCES test_memcpy.cu)
add_executable(test_memcopy ${SOURCES})

set (SOURCES cuda_ipc.cu)
add_executable(cuda_ipc ${SOURCES})
target_link_libraries(cuda_ipc nvipc cudaipctest -pthread)
