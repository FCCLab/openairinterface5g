#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

NR_SOFTMODEM_DIR=$SCRIPT_DIR/../cmake_targets/ran_build/build/nr-softmodem
CONF_FILE=$SCRIPT_DIR/gnb.sa.band78.133prb.fhi72.4x2-delta.conf

sudo $NR_SOFTMODEM_DIR -O $CONF_FILE --thread-pool 8,9,10,11 | tee gnb.log
