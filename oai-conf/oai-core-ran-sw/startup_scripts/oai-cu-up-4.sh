#/bin/bash

source $(dirname $0)/utils.sh

wait_interface_up if-cu-up-4-cn
wait_interface_up if-cu-up-4-e1
wait_interface_up if-cu-up-4-f1u

ifconfig if-cu-up-4-cn  10.1.20.204/24
ifconfig if-cu-up-4-e1  10.1.150.204/24
ifconfig if-cu-up-4-f1u 10.1.100.204/24

/tini -v -- /opt/oai-gnb/bin/entrypoint.sh \ 
/opt/oai-gnb/bin/nr-cuup -O /opt/oai-gnb/etc/gnb.conf --sa &

nice -n -19 python3 /startup_scripts/cpu-time.py

while true
do
    sleep 1s
done
