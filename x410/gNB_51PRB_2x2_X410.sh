#! /bin/bash

sudo ifconfig enp12s0f0np0 192.168.10.100/24
sudo ifconfig enp12s0f0np mtu 9000

sudo cpupower idle-set -D 0

for ((i=0;i<$(nproc);i++)); do sudo cpufreq-set -c $i -r -g performance; done
sudo sysctl -w net.core.wmem_max=62500000
sudo sysctl -w net.core.rmem_max=62500000
sudo sysctl -w net.core.wmem_default=62500000
sudo sysctl -w net.core.rmem_default=62500000
sudo ethtool -G enp12s0f0np tx 4096 rx 4096

sudo ../cmake_targets/ran_build/build/nr-softmodem -O ./gnb.sa.band78.fr1.51PRB.2x2.usrpx410.352501.conf --gNBs.[0].min_rxtxtime 6 | tee gnb.log
