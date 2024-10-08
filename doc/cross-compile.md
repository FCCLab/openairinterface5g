[[_TOC_]]


# OpenAirInterface Cross-Compiler User Guide

## Environment

- OS: ubuntu 22.04

### Set up the environment

Set up for install the package for aarch64.

```shell
sudo dpkg --add-architecture arm64

echo -e \
        "deb [arch=arm64] http://ports.ubuntu.com/ jammy main restricted\n"\
        "deb [arch=arm64] http://ports.ubuntu.com/ jammy-updates main restricted\n"\
        "deb [arch=arm64] http://ports.ubuntu.com/ jammy universe\n"\
        "deb [arch=arm64] http://ports.ubuntu.com/ jammy-updates universe\n"\
        "deb [arch=arm64] http://ports.ubuntu.com/ jammy multiverse\n"\
        "deb [arch=arm64] http://ports.ubuntu.com/ jammy-updates multiverse\n"\
        "deb [arch=arm64] http://ports.ubuntu.com/ jammy-backports main restricted universe multiverse"\
    | sudo tee /etc/apt/sources.list.d/arm-cross-compile-sources.list

sudo cp /etc/apt/sources.list "/etc/apt/sources.list.`date`.backup"
sudo sed -i -E "s/(deb)\ (http:.+)/\1\ [arch=amd64]\ \2/" /etc/apt/sources.list

sudo apt update
sudo apt install -y gcc-11-aarch64-linux-gnu \
                    g++-11-aarch64-linux-gnu

sudo apt-get install -y \
    libatlas-base-dev:arm64 \
    libblas-dev:arm64 \
    libc6-dev-i386 \
    liblapack-dev:arm64 \
    liblapacke-dev:arm64 \
    libreadline-dev:arm64 \
    libgnutls28-dev:arm64 \
    libconfig-dev:arm64 \
    libsctp-dev:arm64 \
    libssl-dev:arm64 \
    libtool:arm64 \
    zlib1g-dev:arm64
```

The above enables apt to download packages for arm64. It also installs
gcc cross-compilers for aarch64 in version 11. This version needs to match the
versions of gcc defined in the cmake cross-compilation file (`cross-arm.cmake`).

## Install and Build

### Install required packages

Use the host compiler to install its dependencies.

```shell
cd cmake_targets
./build_oai -I
```

### Build the LDPC generators

Use the x86 compiler to build the `ldpc_generators` and generate the header
file in the `ran_build/build` folder.  They are necessary during a build for
code generation, and therefore need to be created for the x86 architecture.

```shell
rm -r ran_build
mkdir ran_build
mkdir ran_build/build
mkdir ran_build/build-cross

cd ran_build/build
cmake ../../..
make -j`nproc` ldpc_generators generate_T
```

### Build the Other Executables for aarch64

Switch to the `ran_build/build-cross` folder to build the target executables
for ARM. The `cross-arm.cmake` file defines some ARM-specific build tools
(e.g., compilers) that you might need to adapt. Further, it defines cmake
variables that define in this step where the host tools (such as LDPC
generators) are to be found. For the latter, the `NATIVE_DIR` option has to
be defined in order to tell cmake where the host tools have been built.

```shell
cd ../build-cross
cmake ../../.. -GNinja -DCMAKE_TOOLCHAIN_FILE=../../../cmake_targets/cross-arm.cmake -DNATIVE_DIR=../build

ninja -j`nproc` dlsim ulsim ldpctest polartest smallblocktest nr_pbchsim nr_dlschsim nr_ulschsim nr_dlsim nr_ulsim nr_pucchsim nr_prachsim
ninja -j`nproc` lte-softmodem nr-softmodem nr-cuup oairu lte-uesoftmodem nr-uesoftmodem
ninja -j`nproc` params_libconfig coding rfsimulator
```
