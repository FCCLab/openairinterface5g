#!/bin/bash
GNB_COMMAND='./ran_build/build/nr-softmodem -O ../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.usrpb210.conf --gNBs.[0].min_rxtxtime 6 --rfsim --sa'
UE_COMMAND='./ran_build/build/nr-uesoftmodem -r 106 --numerology 1 --band 78 -C 3619200000 --ssb 516 --rfsim --sa'
sudo tmux \
    new-session "${GNB_COMMAND} | tee gnb.log" \; \
    split-window -h "${UE_COMMAND} | tee ue.log" \; \
    split-window "tail -f ue.log | grep MAC" \;
