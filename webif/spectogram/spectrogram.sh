#!/bin/bash

# Simple script to run spectrogram.py and forward signals
SCRIPT_DIR="$(dirname "$0")"
LOG_FILE="$SCRIPT_DIR/spectrogram.log"

# Set Python path
export PYTHONPATH=/usr/local/lib/python3.10/site-packages:$PYTHONPATH

# Check UHD installation
python3 -c "import uhd; print('UHD version:', uhd.__version__, uhd.get_version_string())"

# Create log directory if it doesn't exist
mkdir -p "$(dirname "$LOG_FILE")"

# Run spectrogram on the last dedicated core using taskset
exec python3 "$SCRIPT_DIR/spectrogram.py" "$@" 2>&1
