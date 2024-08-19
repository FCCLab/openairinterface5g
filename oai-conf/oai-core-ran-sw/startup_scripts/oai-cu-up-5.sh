#/bin/bash

source $(dirname $0)/utils.sh

wait_interface_up if-cu-up-5-cn
wait_interface_up if-cu-up-5-e1
wait_interface_up if-cu-up-5-f1u

ifconfig if-cu-up-5-cn  10.1.20.205/24
ifconfig if-cu-up-5-e1  10.1.150.205/24
ifconfig if-cu-up-5-f1u 10.1.100.205/24

/tini -v -- /opt/oai-gnb/bin/entrypoint.sh \ 
/opt/oai-gnb/bin/nr-cuup -O /opt/oai-gnb/etc/gnb.conf --sa &

nice -n -19 python3 /startup_scripts/oai-cu-up-stats.py

while true
do
    sleep 1s
done
