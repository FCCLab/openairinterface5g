#!/bin/bash
set -eu

COL_USER=$1
COL_PASS=$2
COL_VPN_URL=vpn.colosseum.net/open6g
PID_FILE=./openconnect_pid

# open vpn connection
sudo openconnect ${COL_VPN_URL} --useragent=AnyConnect --user=${COL_USER} --quiet --background --pid-file=${PID_FILE} --passwd-on-stdin < <(echo ${COL_PASS})

sleep 10
