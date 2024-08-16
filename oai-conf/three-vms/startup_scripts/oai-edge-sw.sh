#!/bin/bash

source $(dirname $0)/utils.sh

echo "##### Linux Simple Switch #####"

# Create 3 Bridges
brctl addbr cn5g
brctl addbr e1
brctl addbr f1uc

# Create trunks
ip link add link if-edge-region name if-er.cn type vlan id 20
ip link add link if-edge-region name if-er.e1 type vlan id 150
ip link add link if-edge-region name if-er.f1uc type vlan id 100
ifconfig if-er.cn up
ifconfig if-er.e1 up
ifconfig if-er.f1uc up
##################################### Add interfaces to Bridges ####################################
# Add trunks to bridge
brctl addif cn5g if-er.cn
brctl addif e1   if-er.e1
brctl addif f1uc if-er.f1uc

# UPF + CU-UP 1
brctl addif cn5g if-upf-1-cn
brctl addif cn5g if-cu-up-1-cn
brctl addif e1 if-cu-up-1-e1
brctl addif f1uc if-cu-up-1-f1u

# UPF + CU-UP 4
brctl addif cn5g if-upf-4-cn
brctl addif cn5g if-cu-up-4-cn
brctl addif e1 if-cu-up-4-e1
brctl addif f1uc if-cu-up-4-f1u

# DU
brctl addif f1uc if-du-f1uc
##################################### Add interfaces to Bridges ####################################

# Up + Show
ifconfig cn5g up
ifconfig e1 up
ifconfig f1uc up
echo "### Interfaces ###"
brctl show
echo "### Interfaces ###"

######################################## Simulate Throughput and Delay ###############################
# tc qdisc del dev if-edge-region root
# tc qdisc add dev if-edge-region root handle 1: netem delay 10ms
# tc qdisc add dev if-edge-region parent 1:1 handle 10: tbf rate 50mbit burst 32kbit latency 400ms
# echo "### Similated Throughput and Delay ###"
# tc qdisc show dev if-edge-region
# echo "### Similated Throughput and Delay ###"
######################################## Simulate Throughput and Delay ###############################

#################################### Loop #########################################
while true
do
    sleep 1
done
#################################### Loop #########################################

