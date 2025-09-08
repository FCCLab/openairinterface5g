#!/bin/bash

IFNAME="enp2s0f0np1"

# sudo ip address add 192.168.10.100/24 dev $IFNAME
# sudo ip address add 192.168.20.100/24 dev $IFNAME
# sudo ip address add 192.168.30.100/24 dev $IFNAME
# sudo ip address add 192.168.40.100/24 dev $IFNAME

sudo ifconfig $IFNAME mtu 9000

sudo sysctl -w net.core.rmem_max=25000000
sudo sysctl -w net.core.wmem_max=25000000
sudo sysctl -w net.core.rmem_default=25000000
sudo sysctl -w net.core.wmem_default=25000000

sudo ethtool -K $IFNAME gro off gso off tso off
sudo ethtool -G $IFNAME rx 4096 tx 4096

for ((i=0;i<$(nproc);i++)); do sudo cpufreq-set -c $i -r -g performance; done

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "Current script directory: $DIR"


cd ~/openairinterface5g/cmake_targets/ran_build/build
sudo ./nr-uesoftmodem \
  --usrp-args "type=x300,addr=192.168.40.2,clock=internal,time=internal" \
  -r 133 --numerology 1 --band 78 -C 3425010000 \
  --ue-scan-carrier \
  -O $DIR/ue.conf \
  --ue-fo-compensation
#  --ue-txgain -200

#sudo ./nr-uesoftmodem \
#  --usrp-args "type=x300,addr=192.168.40.2,clock=internal,time=internal" \
#  -r 273 --numerology 1 --band 78 -C 3425010000 \
#  --ue-scan-carrier \
#  -O $DIR/ue.conf \
#  --ue-fo-compensation

