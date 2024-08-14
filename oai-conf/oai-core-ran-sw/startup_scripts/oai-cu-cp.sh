#/bin/bash

source $(dirname $0)/utils.sh

wait_interface_up if-cu-cp-cn
wait_interface_up if-cu-cp-e1
wait_interface_up if-cu-cp-f1c

ifconfig if-cu-cp-cn  10.1.20.50/24
ifconfig if-cu-cp-e1  10.1.150.50/24
ifconfig if-cu-cp-f1c 10.1.100.50/24

/tini -v -- /opt/oai-gnb/bin/entrypoint.sh \ 
/opt/oai-gnb/bin/nr-softmodem -O /opt/oai-gnb/etc/gnb.conf --sa --telnetsrv --telnetsrv.shrmod ci --log_config.global_log_options level,nocolor,time,line_num,function

# while true
# do
#     sleep 1s
# done
