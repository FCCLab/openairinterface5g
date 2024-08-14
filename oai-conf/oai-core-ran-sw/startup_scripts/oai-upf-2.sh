#/bin/bash

source $(dirname $0)/utils.sh

wait_interface_up if-upf-2-cn
ifconfig if-upf-2-cn 10.1.20.102/24

/openair-upf/bin/oai_upf -c /openair-upf/etc/config.yaml -o

while true
do
    sleep 1s
done
