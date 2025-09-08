#!/bin/bash

IFNAME="enp2s0f0np1"


# Set up sudo command with password
if [ "$EUID" -ne 0 ]; then
    SUDO_CMD="echo 'fcpsutd' | sudo -S"
    echo "Running as user, will use sudo with password"
else
    SUDO_CMD=""
    echo "Running as root, no sudo needed"
fi

echo "Setting network interface MTU..."
$SUDO_CMD ifconfig $IFNAME mtu 9000 2>/dev/null || echo "Warning: Could not set MTU (may need sudo)"

echo "Setting network buffer sizes..."
$SUDO_CMD sysctl -w net.core.rmem_max=25000000 2>/dev/null || echo "Warning: Could not set rmem_max (may need sudo)"
$SUDO_CMD sysctl -w net.core.wmem_max=25000000 2>/dev/null || echo "Warning: Could not set wmem_max (may need sudo)"
$SUDO_CMD sysctl -w net.core.rmem_default=25000000 2>/dev/null || echo "Warning: Could not set rmem_default (may need sudo)"
$SUDO_CMD sysctl -w net.core.wmem_default=25000000 2>/dev/null || echo "Warning: Could not set wmem_default (may need sudo)"

echo "Configuring network interface offloads..."
$SUDO_CMD ethtool -K $IFNAME gro off gso off tso off 2>/dev/null || echo "Warning: Could not configure offloads (may need sudo)"
$SUDO_CMD ethtool -G $IFNAME rx 4096 tx 4096 2>/dev/null || echo "Warning: Could not set ring buffers (may need sudo)"

echo "Setting CPU performance mode..."
for ((i=0;i<$(nproc);i++)); do $SUDO_CMD cpufreq-set -c $i -r -g performance 2>/dev/null || echo "Warning: Could not set CPU $i to performance mode (may need sudo)"; done

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
echo "Current script directory: $DIR"


cd ~/openairinterface5g/cmake_targets/ran_build/build
echo "Starting nr-uesoftmodem..."
echo "Working directory: $(pwd)"

echo "Attempting to run nr-uesoftmodem..."
echo "Command: ./nr-uesoftmodem --usrp-args \"type=x300,addr=192.168.40.2,clock=internal,time=internal\" --ue-scan-carrier -O $DIR/ue.conf --ue-fo-compensation $@"

if [ -x "./nr-uesoftmodem" ]; then
    echo "nr-uesoftmodem is executable, running..."
    if [ "$EUID" -ne 0 ]; then
        echo "Running with sudo and password..."
        exec echo 'fcpsutd' | sudo -S ./nr-uesoftmodem --usrp-args "type=x300,addr=192.168.40.2,clock=internal,time=internal" --ue-scan-carrier -O $DIR/ue.conf --ue-fo-compensation "$@"
    else
        echo "Running as root..."
        exec ./nr-uesoftmodem --usrp-args "type=x300,addr=192.168.40.2,clock=internal,time=internal" --ue-scan-carrier -O $DIR/ue.conf --ue-fo-compensation "$@"
    fi
else
    echo "nr-uesoftmodem not executable or not found."
    exit 1
fi
