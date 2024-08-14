#!/bin/bash

source $(dirname $0)/utils.sh

echo "##### Linux Simple Switch #####"

######################################### Wait Interfaces #########################################
wait_interface_up if-region-core

# CN5G
wait_interface_up if-oai-cn5g

# CU-CP
wait_interface_up if-cu-cp-cn
wait_interface_up if-cu-cp-e1
wait_interface_up if-cu-cp-f1c

# UPF + CU-UP 3
wait_interface_up if-upf-3-cn
wait_interface_up if-cu-up-3-cn
wait_interface_up if-cu-up-3-e1
wait_interface_up if-cu-up-3-f1u
######################################### Wait Interfaces #########################################

# Create 3 Bridges
brctl addbr cn5g
brctl addbr e1
brctl addbr f1uc

# Create trunks
ip link add link if-region-core name if-rc.cn type vlan id 20
ip link add link if-region-core name if-rc.e1 type vlan id 150
ip link add link if-region-core name if-rc.f1uc type vlan id 100
ifconfig if-rc.cn up
ifconfig if-rc.e1 up
ifconfig if-rc.f1uc up
##################################### Add interfaces to Bridges ####################################
# Add trunks to bridge
brctl addif cn5g if-rc.cn
brctl addif e1   if-rc.e1
brctl addif f1uc if-rc.f1uc

# CN5G
brctl addif cn5g if-oai-cn5g

# CU-CP
brctl addif cn5g if-cu-cp-cn
brctl addif e1 if-cu-cp-e1
brctl addif f1uc if-cu-cp-f1c

# UPF + CU-UP 3
brctl addif cn5g if-upf-3-cn
brctl addif cn5g if-cu-up-3-cn
brctl addif e1 if-cu-up-3-e1
brctl addif f1uc if-cu-up-3-f1u
##################################### Add interfaces to Bridges ####################################

# Up + Show
ifconfig cn5g up
ifconfig e1 up
ifconfig f1uc up
echo "### Interfaces ###"
brctl show
echo "### Interfaces ###"

######################################## Simulate Throughput and Delay ###############################
tc qdisc del dev if-region-core root
tc qdisc add dev if-region-core root handle 1: netem delay 10ms
tc qdisc add dev if-region-core parent 1:1 handle 10: tbf rate 100mbit burst 32kbit latency 400ms
echo "### Similated Throughput and Delay ###"
tc qdisc show dev if-region-core
echo "### Similated Throughput and Delay ###"
######################################## Simulate Throughput and Delay ###############################

#################################### Loop #########################################
while true
do
    sleep 1
done
