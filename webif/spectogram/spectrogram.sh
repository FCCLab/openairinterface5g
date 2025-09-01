#!/bin/bash

# Spectrogram Service Launcher Script
# Starts the new modular architecture with proper environment setup

SCRIPT_DIR="$(dirname "$0")"
LOG_FILE="$SCRIPT_DIR/logs/spectrogram.log"

# Set Python path for UHD module
export PYTHONPATH=/usr/local/lib/python3.10/site-packages:$PYTHONPATH

# Run the new modular spectrogram service directly
# Forward all arguments and signals
cd "$SCRIPT_DIR"
exec python3 -m spectrogram_main "$@" 2>&1
