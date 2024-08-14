#/bin/bash

source $(dirname $0)/utils.sh

wait_interface_up if-cu-up-2-cn
wait_interface_up if-cu-up-2-e1
wait_interface_up if-cu-up-2-f1u

ifconfig if-cu-up-2-cn  10.1.20.202/24
ifconfig if-cu-up-2-e1  10.1.150.202/24
ifconfig if-cu-up-2-f1u 10.1.100.202/24

/tini -v -- /opt/oai-gnb/bin/entrypoint.sh \ 
/opt/oai-gnb/bin/nr-cuup -O /opt/oai-gnb/etc/gnb.conf --sa

while true
do
    sleep 1s
done
