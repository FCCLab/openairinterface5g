Build nvipc for partner
=======================
The cuPHY-CP/gt_common_libs folder includes the common libraries which used in multiple modules. This guide provide instructions for build libnvipc.so for L2/MAC partner integration.

Prepare source code
-------------------
Related sources are as below, can run ./pack_nvipc.sh to get a tarball of them. Source code of external libs fmtlog_flat and libyaml will copied automatically.

.
├── CMakeLists.txt
├── external
│   ├── fmtlog_flat
│   └── libyaml
├── nvIPC
├── nvlog
└── README.md

Install dependencies
--------------------
The system requires basic build tools like cmake, gcc and below additional libiaries:
sudo apt-get install libpcap-dev libcunit1-dev libnuma-dev -y

Additional feature requirement:
(1) To support CUDA memory pool, requires CUDA version >= 11.3 to be installed.
(2) To support DPDK IPC, requires DPDK to be installed (DPDK IPC is in developing, only for trial).
(3) To support DOCA IPC, requires DOCA version >= 2.2 to be installed.
(4) To support fmtlog based nvlog (C++17), requires gcc, g++ version greter than 7.0
(5) To support nvIPC build with lower versions of gcc and g++ (7.0 or less), fmtlog will not be included.

Default build
-------------
mkdir build && cd build
cmake ..
make -j$(nproc)
make install

Configurable options
--------------------
(1) NVIPC_DPDK_ENABLE: default is ON, will be turned off if DPDK not detected
(2) NVIPC_DOCA_ENABLE: default is ON, will be turned off if DOCA not detected
(3) NVIPC_CUDA_ENABLE: default is ON, depends on CUDA version >= 11.3
(4) NVIPC_FMTLOG_ENABLE: default is ON.
(5) CMAKE_BUILD_TYPE: default is "Release". Config to "Debug" if want to debug by GDB.

Note:
NVIPC_DPDK_ENABLE=ON depends on DPDK libs to be installed and can be detected by PkgConfig
NVIPC_DOCA_ENABLE=ON depends on DOCA libs to be installed and can be detected by PkgConfig
May export the PkgConfig searching path like below if DPDK or DOCA not properly found:
export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:<DPDK/DOCA pkgconfig file path>
NVIPC_FMTLOG_ENABLE=OFF to disable fmtlog or support lower versions of gcc(7.0 or less).

# Example 1: use default configurations (Recommended)
cmake ..

# Example 2: explicitly disable DPDK IPC
cmake .. -DNVIPC_DPDK_ENABLE=OFF

# Example 3: explicitly disable DOCA IPC
cmake .. -DNVIPC_DOCA_ENABLE=OFF

# Example 4: disable CUDA
cmake .. -DNVIPC_CUDA_ENABLE=OFF

# Example 5: disable NVIPC_FMTLOG_ENABLE
cmake .. -DNVIPC_FMTLOG_ENABLE=OFF

# Example 6: enable GDB debug info
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Test 1: SHM IPC example

Run below 2 processes in different terminals: 1 primary process and 1 secondary process, they communicate with each other:

sudo ./build/nvIPC/tests/example/test_ipc 3 1 1    # Primary process
sudo ./build/nvIPC/tests/example/test_ipc 3 1 0    # Secondary process

# Test 2: SHM IPC unit test
Run below command, it will start a primary process and fork a secondary process. The 2 processes communicate with each other. Should see all pass in console output.

sudo ./build/nvIPC/tests/cunit/nvipc_cunit 3 2

Integration instructions
------------------------
(1) Refer to nvIPC/tests/example/test_ipc.c as example of how to use libnvipc.so
(2) Strongly recommend to copy nvIPC/tests/example/nvipc_config.yaml to L2 side and let L2 call API load_nv_ipc_yaml_config() to automatically load configurations for nvipc interface and log.
