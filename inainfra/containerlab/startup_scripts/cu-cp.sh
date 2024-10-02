#/bin/bash

source $(dirname $0)/utils.sh

wait_interface_up n2
wait_interface_up e1
wait_interface_up f1-c

/tini -v -- /opt/oai-gnb/bin/entrypoint.sh \ 
/opt/oai-gnb/bin/nr-softmodem -O /opt/oai-gnb/etc/gnb.conf --sa --telnetsrv --telnetsrv.shrmod ci --log_config.global_log_options level,nocolor,time,line_num,function

# while true
# do
#     sleep 1s
# done
