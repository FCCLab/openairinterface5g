#!/bin/bash

# Test script to verify spectrogram service startup
# This script tests the startup process without running the full service

SCRIPT_DIR="$(dirname "$0")"

echo "🧪 Testing Spectrogram Service Startup..."
echo "========================================"

# Set Python path for UHD module
export PYTHONPATH=/usr/local/lib/python3.10/site-packages:$PYTHONPATH

# Test 1: Check UHD availability
echo "1️⃣ Testing UHD module..."
python3 -c "import uhd; print('   ✅ UHD available:', uhd.__version__)" || {
    echo "   ❌ UHD module failed"
    exit 1
}

# Test 2: Check individual process imports
echo "2️⃣ Testing process module imports..."
python3 -c "import tx_rx_process; print('   ✅ TX/RX Process module OK')" || {
    echo "   ❌ TX/RX Process module failed"
    exit 1
}

python3 -c "import stft_process; print('   ✅ STFT Process module OK')" || {
    echo "   ❌ STFT Process module failed"
    exit 1
}

python3 -c "import websocket_process; print('   ✅ WebSocket Process module OK')" || {
    echo "   ❌ WebSocket Process module failed"
    exit 1
}

# Test 3: Check main orchestrator
echo "3️⃣ Testing main orchestrator..."
python3 -c "import spectrogram_main; print('   ✅ Main Orchestrator module OK')" || {
    echo "   ❌ Main Orchestrator module failed"
    exit 1
}

# Test 4: Check argument parsing
echo "4️⃣ Testing argument parsing..."
python3 run_spectrogram.py --help > /dev/null 2>&1 || {
    echo "   ❌ Argument parsing failed"
    exit 1
}
echo "   ✅ Argument parsing OK"

# Test 5: Test process creation (without starting)
echo "5️⃣ Testing process creation..."
python3 -c "
import multiprocessing
import spectrogram_main
import argparse

# Create mock args
args = argparse.Namespace()
args.device = ''
args.freq = 2.4e9
args.sample_rate = 1e6
args.gain = 37.5
args.fft_size = 1024
args.hop_size = 1000
args.websocket_port = 40001

# Test service creation
service = spectrogram_main.SpectrogramMain(args)
print('   ✅ Service creation OK')
print('   ✅ CPU cores assigned:', service.cpu_cores)
" || {
    echo "   ❌ Service creation failed"
    exit 1
}

echo ""
echo "🎉 All startup tests passed! The service should start successfully."
echo "🚀 You can now run: ./spectrogram.sh [args]"
