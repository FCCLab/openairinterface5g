#!/bin/bash

sudo brctl addbr oai-du
sudo ifconfig oai-du 10.1.100.10/24
sudo ifconfig oai-du up

cd ~/openairinterface5g/cmake_targets/ran_build/build
sudo ./nr-softmodem -O ~/openairinterface5g/oai-conf/oai-core-ran-sw/conf_ran/gnb-du.sa.conf --sa
