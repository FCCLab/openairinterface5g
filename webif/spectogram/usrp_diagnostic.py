#!/usr/bin/env python3
"""
USRP Diagnostic Script
Simple test to check USRP hardware functionality
"""

import numpy as np
import uhd
import time

def test_usrp_basic():
    """Test basic USRP functionality"""
    print("=== USRP Basic Diagnostic Test ===")
    
    try:
        # Check UHD version
        print(f"UHD Version: {uhd.get_version_string()}")
        
        # Connect to USRP
        print("\n1. Connecting to USRP...")
        usrp = uhd.usrp.MultiUSRP('addr=192.168.40.2')
        print(f"   Device: {usrp.get_pp_string()}")
        
        # Check device info
        print(f"\n2. Device Information:")
        print(f"   Sample rate range: {usrp.get_rx_rate_range()}")
        print(f"   Frequency range: {usrp.get_rx_freq_range()}")
        print(f"   Gain range: {usrp.get_rx_gain_range(0)}")
        
        # Set basic parameters
        print(f"\n3. Setting basic parameters...")
        usrp.set_rx_rate(1e6, 0)  # 1 MHz sample rate
        usrp.set_rx_freq(100e6, 0)  # 100 MHz frequency
        usrp.set_rx_gain(20, 0)  # 20 dB gain
        
        print(f"   Sample rate: {usrp.get_rx_rate(0)} Hz")
        print(f"   Frequency: {usrp.get_rx_freq(0)} Hz")
        print(f"   Gain: {usrp.get_rx_gain(0)} dB")
        
        # Setup stream
        print(f"\n4. Setting up RX stream...")
        st_args = uhd.usrp.StreamArgs('fc32', 'fc32')
        rx_stream = usrp.get_rx_stream(st_args)
        
        # Start streaming
        print(f"5. Starting continuous streaming...")
        rx_stream.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.start_cont))
        time.sleep(0.1)  # Allow streaming to start
        
        # Collect samples
        print(f"6. Collecting test samples...")
        samples = np.zeros(1000, dtype=np.complex64)
        md = uhd.types.RXMetadata()
        
        n_received = rx_stream.recv(samples, md, timeout=1.0)
        print(f"   Samples received: {n_received}")
        print(f"   Error code: {md.error_code}")
        
        if n_received > 0:
            print(f"   First 5 samples: {samples[:5]}")
            print(f"   Real range: [{np.min(samples.real):.6e}, {np.max(samples.real):.6e}]")
            print(f"   Imag range: [{np.min(samples.imag):.6e}, {np.max(samples.imag):.6e}]")
            print(f"   Magnitude range: [{np.min(np.abs(samples)):.6e}, {np.max(np.abs(samples)):.6e}]")
            print(f"   Average magnitude: {np.mean(np.abs(samples)):.6e}")
            
            # Check for problems
            inf_count = np.sum(np.isinf(samples))
            nan_count = np.sum(np.isnan(samples))
            very_large_count = np.sum(np.abs(samples) > 1e30)
            very_small_count = np.sum(np.abs(samples) < 1e-30)
            
            print(f"\n7. Diagnostic Checks:")
            print(f"   Infinite values: {inf_count}")
            print(f"   NaN values: {nan_count}")
            print(f"   Very large (>1e30): {very_large_count}")
            print(f"   Very small (<1e-30): {very_small_count}")
            
            if inf_count > 0 or very_large_count > 0:
                print(f"\n   ⚠️  WARNING: Hardware issue detected!")
                print(f"      The USRP is outputting invalid data.")
                return False
            else:
                print(f"\n   ✅ USRP appears to be working correctly!")
                return True
        else:
            print(f"   ❌ No samples received!")
            return False
            
    except Exception as e:
        print(f"   ❌ Error: {e}")
        return False

if __name__ == "__main__":
    success = test_usrp_basic()
    if success:
        print("\n=== Test PASSED ===")
    else:
        print("\n=== Test FAILED ===")
        print("Recommendation: Power cycle the USRP device and try again.")
