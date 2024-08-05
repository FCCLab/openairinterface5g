
FROM ubuntu:jammy AS ran-base
ARG NEEDED_GIT_PROXY
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Paris
ENV BUILD_UHD_FROM_SOURCE=True
ENV UHD_VERSION=4.4.0.0

#install developers pkg/repo
#use gcc-12 to avoid problems of default gcc-11 in FlexRIC
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get upgrade --yes && \
    DEBIAN_FRONTEND=noninteractive apt-get install --yes \
       #gcc needed for build_oai
       build-essential \
       psmisc \
       git \
       gcc-12 \
       g++-12 \
       xxd \
       libpcre2-dev \
       python3-dev \
       bison \
       flex \
       m4 \
       # python3-pip for conf template generation
       python3-pip && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-12 100 --slave /usr/bin/g++ g++ /usr/bin/g++-12 && \
    pip3 install --ignore-installed pyyaml

# Add "Tini - A tiny but valid init for containers", https://github.com/krallin/tini
# it will be copied into target containers, to print exit numbers and handle signals properly
ENV TINI_VERSION v0.19.0
ADD https://github.com/krallin/tini/releases/download/${TINI_VERSION}/tini /tini
RUN chmod +x /tini

# In some network environments, GIT proxy is required
RUN /bin/bash -c "if [[ -v NEEDED_GIT_PROXY ]]; then git config --global http.proxy $NEEDED_GIT_PROXY; fi"

# WORKDIR /oai-ran
# COPY . .

#run build_oai -I to get the builder image
# RUN /bin/sh oaienv && \ 
#     cd cmake_targets && \
#     mkdir -p log && \
#     ./build_oai -I -w USRP --install-optional-packages

RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get upgrade --yes && \
    DEBIAN_FRONTEND=noninteractive apt-get install --yes \
        software-properties-common \
        procps \
        libsctp1 \
        libboost-chrono1.74.0 \
        libboost-date-time1.74.0 \
        libboost-filesystem1.74.0 \
        libboost-regex1.74.0 \
        libboost-serialization1.74.0 \
        libboost-thread1.74.0 \
        libboost-system1.74.0 \
        libboost-program-options1.74.0 \
        tzdata \
        libblas3 \
        libconfig9 \
        openssl \
        net-tools \
        iperf \
        iperf3 \
        iproute2 \
        iputils-ping \
        gdb \
        python3 \
        python3-six \
        python3-requests \
        libusb-1.0-0


RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get upgrade --yes && \
    DEBIAN_FRONTEND=noninteractive apt-get install --yes \
    net-tools \
    cmake \
    ninja-build

WORKDIR /oai-ran
ENV LD_LIBRARY_PATH=/oai-ran/cmake_targets/ran_build/build:$LD_LIBRARY_PATH
