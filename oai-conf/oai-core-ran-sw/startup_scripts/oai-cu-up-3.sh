#/bin/bash

source $(dirname $0)/utils.sh

wait_interface_up if-cu-up-3-cn
wait_interface_up if-cu-up-3-e1
wait_interface_up if-cu-up-3-f1u

ifconfig if-cu-up-3-cn  10.1.20.203/24
ifconfig if-cu-up-3-e1  10.1.150.203/24
ifconfig if-cu-up-3-f1u 10.1.100.203/24

/tini -v -- /opt/oai-gnb/bin/entrypoint.sh \ 
/opt/oai-gnb/bin/nr-cuup -O /opt/oai-gnb/etc/gnb.conf --sa

while true
do
    sleep 1s
done
