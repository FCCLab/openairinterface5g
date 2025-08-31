#!/usr/bin/env python3
"""
Simple tone test script to verify USRP transmission.
This script creates a simple tone and transmits it continuously for testing.
"""

import uhd
import numpy as np
import time
import argparse
import logging

def make_tone(fs, f0, nsamps, amp=0.5):
    """Create a complex tone."""
    t = np.arange(nsamps, dtype=np.float64) / fs
    amp = float(np.clip(amp, 0.0, 1.0))
    return (amp * np.exp(2j * np.pi * f0 * t)).astype(np.complex64)

def main():
    parser = argparse.ArgumentParser(description="Simple USRP tone test")
    parser.add_argument("--device", type=str, default="", help="USRP device address")
    parser.add_argument("--freq", type=float, default=2.4e9, help="Center frequency in Hz")
    parser.add_argument("--sample_rate", type=float, default=1e6, help="Sample rate in Hz")
    parser.add_argument("--tone_offset", type=float, default=500e3, help="Tone offset from center in Hz")
    parser.add_argument("--amplitude", type=float, default=0.8, help="Tone amplitude (0.0 to 1.0)")
    parser.add_argument("--duration", type=float, default=30, help="Transmission duration in seconds")
    
    args = parser.parse_args()
    
    # Setup logging
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
    
    try:
        # Setup USRP
        logging.info(f"Setting up USRP device: '{args.device}'")
        usrp = uhd.usrp.MultiUSRP(args.device) if args.device else uhd.usrp.MultiUSRP()
        logging.info(f"USRP device: {usrp.get_pp_string()}")
        
        # Configure TX
        usrp.set_tx_rate(args.sample_rate, 0)
        usrp.set_tx_freq(args.freq, 0)
        
        # Get and set maximum TX gain
        max_tx_gain = usrp.get_tx_gain_range(0).stop()
        usrp.set_tx_gain(max_tx_gain, 0)
        logging.info(f"TX gain set to maximum: {max_tx_gain} dB")
        
        # Setup TX streamer
        tx_st_args = uhd.usrp.StreamArgs("fc32", "sc16")
        tx_st_args.channels = [0]
        tx_streamer = usrp.get_tx_stream(tx_st_args)
        
        # Create tone
        tone_freq = args.tone_offset  # Tone frequency relative to center
        tx_wave = make_tone(args.sample_rate, tone_freq, 10000, amp=args.amplitude)
        
        logging.info(f"Center frequency: {args.freq/1e6:.3f} MHz")
        logging.info(f"Sample rate: {args.sample_rate/1e6:.1f} MHz")
        logging.info(f"Tone offset: {args.tone_offset/1e3:.1f} kHz")
        logging.info(f"Tone frequency: {tone_freq/1e3:.1f} kHz (relative to center)")
        logging.info(f"Tone amplitude: {args.amplitude}")
        logging.info(f"Transmission duration: {args.duration} seconds")
        
        # Start transmission
        logging.info("Starting transmission...")
        tx_streamer.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.start_cont))
        
        # Transmit for specified duration
        start_time = time.time()
        tx_md = uhd.types.TXMetadata()
        samps_sent = 0
        max_tx = tx_streamer.get_max_num_samps()
        
        while time.time() - start_time < args.duration:
            # Send the waveform in chunks
            chunk = tx_wave[samps_sent % len(tx_wave):(samps_sent % len(tx_wave)) + max_tx]
            if len(chunk) < max_tx:
                remaining = max_tx - len(chunk)
                chunk = np.concatenate([chunk, tx_wave[:remaining]])
            
            sent = tx_streamer.send(chunk, tx_md)
            samps_sent += sent
            
            # Log progress every second
            if int(time.time() - start_time) % 5 == 0 and int(time.time() - start_time) != int(time.time() - start_time - 0.1):
                logging.info(f"Transmitted {samps_sent} samples ({time.time() - start_time:.1f}s elapsed)")
        
        # Stop transmission
        logging.info("Stopping transmission...")
        tx_streamer.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont))
        
        logging.info(f"Transmission complete. Total samples sent: {samps_sent}")
        
    except KeyboardInterrupt:
        logging.info("Interrupted by user")
    except Exception as e:
        logging.error(f"Error: {e}")

if __name__ == "__main__":
    main()
