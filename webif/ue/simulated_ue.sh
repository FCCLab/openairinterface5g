#!/bin/bash

# Set PYTHONPATH to include both the ue directory and proto subdirectory
UE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export PYTHONPATH="$UE_DIR:$UE_DIR/proto:$PYTHONPATH"
echo "PYTHONPATH: $PYTHONPATH"

exec python3 $UE_DIR/simulated_ue.py "$@"
