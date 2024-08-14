#!/bin/bash

source $(dirname $0)/utils.sh

wait_interface_up if-du-f1uc
ifconfig if-du-f1uc 10.1.100.10/24

set -uo pipefail

PREFIX=/opt/oai-gnb
CONFIGFILE=$PREFIX/etc/gnb.conf

echo "=================================="
echo "/proc/sys/kernel/core_pattern=$(cat /proc/sys/kernel/core_pattern)"

if [ ! -f $CONFIGFILE ]; then
  echo "No configuration file found: please mount at $CONFIGFILE"
  exit 255
fi

echo "=================================="
echo "== Configuration file:"
cat $CONFIGFILE

ifconfig

sysctl -w net.core.wmem_max=62500000
sysctl -w net.core.rmem_max=62500000
sysctl -w net.core.wmem_default=62500000
sysctl -w net.core.rmem_default=62500000
ethtool -G eth1 tx 9000 rx 9000


# Load the USRP binaries
echo "=================================="
echo "== Load USRP binaries"
if [[ -v USE_B2XX ]]; then
    $PREFIX/bin/uhd_images_downloader.py -t b2xx
elif [[ -v USE_X3XX ]]; then
    $PREFIX/bin/uhd_images_downloader.py -t x3xx
elif [[ -v USE_N3XX ]]; then
    $PREFIX/bin/uhd_images_downloader.py -t n3xx
fi

# enable printing of stack traces on assert
export OAI_GDBSTACKS=1

echo "=================================="
echo "== Starting gNB soft modem"
# if [[ -v USE_ADDITIONAL_OPTIONS ]]; then
#     echo "Additional option(s): ${USE_ADDITIONAL_OPTIONS}"
#     new_args=()
#     while [[ $# -gt 0 ]]; do
#         new_args+=("$1")
#         shift
#     done
#     for word in ${USE_ADDITIONAL_OPTIONS}; do
#         new_args+=("$word")
#     done
#     echo "${new_args[@]}"
#     exec "${new_args[@]}"
# else
#     echo "$@"
#     exec "$@"
# fi
/opt/oai-gnb/bin/nr-softmodem -O /opt/oai-gnb/etc/gnb.conf --sa

while true; do sleep 1; done
