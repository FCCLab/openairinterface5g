#/bin/bash

source $(dirname $0)/utils.sh

wait_interface_up f1-u
wait_interface_up e1
wait_interface_up n3

/tini -v -- /opt/oai-gnb/bin/entrypoint.sh \ 
/opt/oai-gnb/bin/nr-cuup -O /opt/oai-gnb/etc/gnb.conf --sa

while true
do
    sleep 1s
done
