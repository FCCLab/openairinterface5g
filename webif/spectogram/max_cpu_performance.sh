#!/bin/bash

# Simple script to set maximum CPU performance using cpupower
# This is the easiest way to set CPU to maximum performance

echo "Setting maximum CPU performance for spectrogram processing..."

# Check if cpupower is available
if ! command -v cpupower &> /dev/null; then
    echo "Error: cpupower not found. Please install it with:"
    echo "sudo apt-get install linux-tools-common linux-tools-generic"
    exit 1
fi

# Show current status
echo "Current CPU status:"
cpupower frequency-info
echo ""

# Set performance governor
echo "Setting CPU governor to performance mode..."
sudo cpupower frequency-set -g performance

# Set maximum frequency
echo "Setting CPU frequency to maximum..."
sudo cpupower frequency-set -f max

# Show final status
echo ""
echo "Final CPU status:"
cpupower frequency-info

echo ""
echo "=== Maximum CPU Performance Set ==="
echo "CPU is now running at maximum performance"
echo "Note: These settings will reset after reboot"

# Show the last core that will be used for spectrogram
LAST_CORE=$(($(nproc) - 1))
echo ""
echo "Spectrogram will run on dedicated core: $LAST_CORE"
echo "Core $LAST_CORE frequency: $(cat /sys/devices/system/cpu/cpu$LAST_CORE/cpufreq/scaling_cur_freq 2>/dev/null || echo "N/A") Hz"
