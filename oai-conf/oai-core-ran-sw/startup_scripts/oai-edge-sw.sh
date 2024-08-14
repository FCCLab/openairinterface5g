#!/bin/bash

source $(dirname $0)/utils.sh

echo "##### Linux Simple Switch #####"

######################################### Wait Interfaces #########################################
wait_interface_up if-edge-region

# UPF + CU-UP 1
wait_interface_up if-upf-1-cn
wait_interface_up if-cu-up-1-cn
wait_interface_up if-cu-up-1-e1
wait_interface_up if-cu-up-1-f1u
######################################### Wait Interfaces #########################################

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

# DU
brctl addif f1uc if-du-f1uc
##################################### Add interfaces to Bridges ####################################

# Up + Show + Loop
ifconfig cn5g up
ifconfig e1 up
ifconfig f1uc up
brctl show
while true
do
    sleep 1
done
