#/bin/bash

source $(dirname $0)/utils.sh

nice -n -19 python3 /startup_scripts/oai_stats.py &

wait_interface_up if-upf-5-cn
ifconfig if-upf-5-cn 10.1.20.105/24

/openair-upf/bin/oai_upf -c /openair-upf/etc/config.yaml -o

while true
do
    sleep 1s
done
