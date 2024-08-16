#!/bin/bash

/tini -v -- /opt/oai-nr-ue/bin/entrypoint.sh /opt/oai-nr-ue/bin/nr-uesoftmodem -O /opt/oai-nr-ue/etc/nr-ue.conf ${USE_ADDITIONAL_OPTIONS} &

python3 /startup_scripts/oai-nr-ue-rfsim.py

while true; do sleep 1; done