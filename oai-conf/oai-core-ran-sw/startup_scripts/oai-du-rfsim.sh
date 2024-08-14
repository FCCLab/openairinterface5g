#/bin/bash

source $(dirname $0)/utils.sh

wait_interface_up if-du-f1uc
ifconfig if-du-f1uc 10.1.100.10/24

/tini -v -- /opt/oai-gnb/bin/entrypoint.sh \ 
/opt/oai-gnb/bin/nr-softmodem -O /opt/oai-gnb/etc/gnb.conf --sa --rfsim --log_config.global_log_options level,nocolor,time
