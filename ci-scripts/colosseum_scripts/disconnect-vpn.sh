#!/bin/bash
set -xeu

PID_FILE=./openconnect_pid

# close vpn connection
sudo kill -2 $(cat ${PID_FILE})
