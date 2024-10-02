#!/bin/bash

ip route delete default
ip route add default via 192.168.80.1
ip route add 192.168.71.0/24 dev eth0 via 192.168.70.1

/openair-upf/bin/oai_upf -c /openair-upf/etc/config.yaml -o
