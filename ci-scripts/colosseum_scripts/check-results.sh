#!/bin/bash
set -x

RESULT_FILE=$1

grep -A 1 "Final Status" ${RESULT_FILE} | grep "PASS"

if [ "$?" = 0 ]; then
    echo '{"status": "successful"}' > results.json
else
    echo '{"status": "failed"}' > results.json
fi
