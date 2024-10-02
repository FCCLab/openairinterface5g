#/bin/bash

source $(dirname $0)/utils.sh

wait_interface_up n2

/openair-amf/bin/oai_amf -c /openair-amf/etc/config.yaml -o
