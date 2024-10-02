#/bin/bash

source $(dirname $0)/utils.sh

wait_interface_up n3
# wait_interface_up cn-cp

/openair-upf/bin/oai_upf -c /openair-upf/etc/config.yaml -o

while true
do
    sleep 1s
done
