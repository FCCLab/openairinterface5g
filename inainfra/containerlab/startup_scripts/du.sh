#/bin/bash

source $(dirname $0)/utils.sh

wait_interface_up f1
# wait_interface_up f1-u

/tini -v -- /opt/oai-gnb/bin/entrypoint.sh \ 
/opt/oai-gnb/bin/nr-softmodem -O /opt/oai-gnb/etc/gnb.conf --sa --rfsim --log_config.global_log_options level,nocolor,time
