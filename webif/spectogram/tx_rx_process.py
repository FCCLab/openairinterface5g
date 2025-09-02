#!/usr/bin/env python3
"""
TX/RX Process - Self-contained USRP communication process
Handles continuous transmission and reception of IQ samples
"""

import logging
import time
import threading
import queue
import signal
import os
import sys
from pathlib import Path

# Add the spectogram directory to Python path for imports
script_dir = Path(__file__).parent
sys.path.insert(0, str(script_dir))

try:
    import uhd
    import numpy as np
    from cpu_pe import get_p_e_cores
except ImportError as e:
    logging.error(f"Import error: {e}")
    sys.exit(1)

class TXRXProcess:
    """Self-contained TX/RX process for USRP communication"""
    
    def __init__(self, tx_wave, hop_size, stop_event, rx_queue, timestamp, 
                 device, freq, sample_rate, gain, cpu_core=1, log_dir=None):
        """
        Initialize TX/RX process
        
        Args:
            tx_wave: Transmit waveform data
            hop_size: Hop size for frequency hopping
            stop_event: Multiprocessing event to signal stop
            rx_queue: Queue for RX data
            timestamp: Main process timestamp
            device: USRP device string
            freq: Center frequency
            sample_rate: Sample rate
            gain: RX gain
            cpu_core: CPU core to bind to
        """
        # Store instance variables first
        self.tx_wave = tx_wave
        self.hop_size = hop_size
        self.stop_event = stop_event
        self.rx_queue = rx_queue
        self.timestamp = timestamp
        self.device = device
        self.freq = freq
        self.sample_rate = sample_rate
        self.gain = gain
        self.cpu_core = cpu_core
        self.log_dir = log_dir
        
        # Process state
        self.usrp = None
        self.tx_streamer = None
        self.rx_streamer = None
        self.tx_thread = None
        self.rx_thread = None
        self.tx_samples_sent = 0
        self.rx_frames_collected = 0
        
        # Setup logging first (before any logging calls)
        self._setup_logging()
        
        # Now we can log the initialization details
        self.logger.info("TX/RX Process initialization:")
        self.logger.info(f"  tx_wave: {type(tx_wave)}, length: {len(tx_wave) if hasattr(tx_wave, '__len__') else 'N/A'}")
        self.logger.info(f"  hop_size: {hop_size}")
        self.logger.info(f"  stop_event: {type(stop_event)}")
        self.logger.info(f"  rx_queue: {type(rx_queue)}")
        self.logger.info(f"  timestamp: {timestamp}")
        self.logger.info(f"  device: '{device}' (type: {type(device)}, repr: {repr(device)})")
        self.logger.info(f"  freq: {freq}")
        self.logger.info(f"  sample_rate: {sample_rate}")
        self.logger.info(f"  gain: {gain}")
        self.logger.info(f"  cpu_core: {cpu_core}")
        
        # Setup signal handlers
        self._setup_signal_handlers()
        
        # Setup CPU affinity
        self._setup_cpu_affinity()
    
    def _setup_logging(self):
        """Setup process-specific logging"""
        # Create a logger specific to this process
        self.logger = logging.getLogger(f"TXRXProcess_{self.timestamp}")
        self.logger.setLevel(logging.INFO)
        
        # Clear any existing handlers to avoid duplicates
        self.logger.handlers.clear()
        
        # Create formatter
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        
        # Add console handler (this ensures output goes to console)
        console_handler = logging.StreamHandler()
        console_handler.setLevel(logging.INFO)
        console_handler.setFormatter(formatter)
        self.logger.addHandler(console_handler)
        
        # Add file handler
        log_filename = f"spectrogram_{self.timestamp}_tx_rx.log"
        # Use log directory passed from main process, or fallback to script directory
        if self.log_dir:
            log_path = self.log_dir / log_filename
        else:
            log_path = script_dir / "logs" / log_filename
        
        # Create logs directory if it doesn't exist
        log_path.parent.mkdir(exist_ok=True)
        
        file_handler = logging.FileHandler(log_path)
        file_handler.setLevel(logging.INFO)
        file_handler.setFormatter(formatter)
        self.logger.addHandler(file_handler)
        
        self.logger.info(f"TX/RX Process logging setup complete: {log_path}")
        self.logger.info(f"Current working directory: {os.getcwd()}")
        self.logger.info(f"Script directory: {script_dir}")
    
    def _setup_signal_handlers(self):
        """Setup signal handlers for graceful shutdown"""
        def signal_handler(signum, frame):
            self.logger.info(f"TX/RX Process received signal {signum}, stopping...")
            self.stop_event.set()
        
        signal.signal(signal.SIGINT, signal_handler)
        signal.signal(signal.SIGTERM, signal_handler)
        # Add SIGHUP for backend control
        signal.signal(signal.SIGHUP, signal_handler)
        self.logger.info("TX/RX Process: Signal handlers set up successfully")
    
    def _check_parent_alive(self):
        """Check if parent process is still alive"""
        try:
            # Get parent PID
            parent_pid = os.getppid()
            
            # Check if parent is still running
            if parent_pid == 1:  # Adopted by init - parent died
                self.logger.warning("Parent process died, initiating self-cleanup...")
                return False
            
            # Check if parent process exists
            try:
                os.kill(parent_pid, 0)  # Signal 0 just checks if process exists
                return True
            except OSError:
                self.logger.warning("Parent process no longer exists, initiating self-cleanup...")
                return False
                
        except Exception as e:
            self.logger.error(f"Error checking parent process: {e}")
            return False
    
    def _cleanup_usrp(self):
        """Clean up USRP resources"""
        try:
            if hasattr(self, 'tx_streamer') and self.tx_streamer:
                self.logger.info("Cleaning up TX streamer...")
                del self.tx_streamer
                self.tx_streamer = None
            
            if hasattr(self, 'rx_streamer') and self.rx_streamer:
                self.logger.info("Cleaning up RX streamer...")
                self.rx_streamer.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont))
                del self.rx_streamer
                self.rx_streamer = None
            
            if hasattr(self, 'usrp') and self.usrp:
                self.logger.info("Cleaning up USRP device...")
                del self.usrp
                self.usrp = None
                
            self.logger.info("USRP cleanup completed")
            
        except Exception as e:
            self.logger.error(f"Error during USRP cleanup: {e}")
    
    def _setup_cpu_affinity(self):
        """Set CPU affinity for this process"""
        try:
            import psutil
            process = psutil.Process()
            process.cpu_affinity([self.cpu_core])
            self.logger.info(f"Successfully set TX/RX process affinity to core {self.cpu_core}")
        except Exception as e:
            self.logger.error(f"Error setting CPU affinity: {e}")
    
    def setup_usrp(self):
        """Setup USRP device and streams"""
        try:
            # Setup USRP device using exactly the same logic as the old code
            self.logger.info(f"Setting up USRP device: '{self.device}'")
            self.logger.info(f"Device type: {type(self.device)}")
            self.logger.info(f"Device length: {len(self.device) if self.device else 0}")
            self.logger.info(f"Device repr: {repr(self.device)}")
            
            # Ensure proper device address format (copy from old code)
            device_addr = self.device
            self.logger.info(f"Initial device_addr: '{device_addr}'")
            
            if device_addr:
                if not device_addr.startswith("addr="):
                    device_addr = f"addr={device_addr}"
                    self.logger.info(f"Added 'addr=' prefix: '{device_addr}'")
            else:
                device_addr = ""  # Auto-detect
                self.logger.info("No device specified, using auto-detect")
            
            self.logger.info(f"Final device address: '{device_addr}'")
            self.logger.info(f"Final device address repr: {repr(device_addr)}")
            
            if device_addr:
                self.logger.info(f"Creating USRP with device string: '{device_addr}'")
                # Try different USRP constructors to avoid the device string corruption issue
                try:
                    # First try the MultiUSRP constructor (this is the correct one)
                    self.logger.info("Trying uhd.usrp.MultiUSRP constructor...")
                    self.usrp = uhd.usrp.MultiUSRP(device_addr)
                    self.logger.info("Successfully created USRP using uhd.usrp.MultiUSRP")
                except Exception as e1:
                    self.logger.warning(f"MultiUSRP constructor failed: {e1}")
                    try:
                        # Fall back to auto-detect
                        self.logger.info("Trying auto-detect...")
                        self.usrp = uhd.usrp.MultiUSRP()
                        self.logger.info("Successfully created USRP using auto-detect")
                    except Exception as e2:
                        self.logger.error(f"Auto-detect also failed: {e2}")
                        return False
            else:
                self.logger.info("Creating USRP with auto-detect")
                self.usrp = uhd.usrp.MultiUSRP()
                
            self.logger.info("Successfully connected to USRP device")
            self.logger.info(f"USRP device: {self.usrp.get_pp_string()}")
            
            # Configure device using the same approach as old code
            self.usrp.set_tx_rate(self.sample_rate, 0)
            self.usrp.set_tx_freq(self.freq, 0)
            self.usrp.set_tx_gain(10, 0)  # Moderate TX gain for loopback
            
            self.usrp.set_rx_rate(self.sample_rate, 0)
            self.usrp.set_rx_freq(self.freq, 0)
            
            # Get and set maximum RX gain (like old code)
            rx_gain_range = self.usrp.get_rx_gain_range(0)
            max_rx_gain = rx_gain_range.stop()
            self.usrp.set_rx_gain(max_rx_gain, 0)
            self.logger.info(f"RX gain set to maximum: {max_rx_gain} dB (range: {rx_gain_range.start():.1f} to {rx_gain_range.stop():.1f} dB)")
            
            # Create streamers using exactly the same approach as the old code
            tx_st_args = uhd.usrp.StreamArgs("fc32", "sc16")
            tx_st_args.channels = [0]
            
            rx_st_args = uhd.usrp.StreamArgs("fc32", "sc16")
            rx_st_args.channels = [0]
            
            # Create streamers
            self.tx_streamer = self.usrp.get_tx_stream(tx_st_args)
            self.rx_streamer = self.usrp.get_rx_stream(rx_st_args)
            
            self.logger.info("TX streamer setup complete")
            self.logger.info("RX streamer setup complete")
            
            return True
            
        except Exception as e:
            self.logger.error(f"Error setting up USRP: {e}")
            return False
    
    def tx_thread_function(self):
        """Transmit thread function - EXACT same as old working code"""
        thread_id = 1
        samps_sent = 0
        max_tx = self.tx_streamer.get_max_num_samps()
        
        self.logger.info(f"TX thread {thread_id}: Starting with max_tx={max_tx}, tx_wave length={len(self.tx_wave)}")
        self.logger.info(f"TX thread {thread_id}: Will log progress every 5,000 samples")
        
        try:
            # Create TX metadata like the old code
            tx_md = uhd.types.TXMetadata()
            
            while not self.stop_event.is_set():
                try:
                    # Send the waveform in chunks - EXACT same as old code
                    chunk = self.tx_wave[samps_sent % len(self.tx_wave):(samps_sent % len(self.tx_wave)) + max_tx]
                    if len(chunk) < max_tx:
                        # Wrap around to beginning of waveform - EXACT same as old code
                        remaining = max_tx - len(chunk)
                        chunk = np.concatenate([chunk, self.tx_wave[:remaining]])
                    
                    # # Transmit the chunk with metadata
                    # sent = self.tx_streamer.send(chunk, tx_md)
                    # samps_sent += sent
                    
                    # # Log progress occasionally
                    # if samps_sent % 5000 == 0:
                    #     self.logger.info(f"TX thread {thread_id}: sent {samps_sent} samples")
                    
                    # Small delay to prevent overwhelming
                    time.sleep(0.001)
                    
                except Exception as e:
                    self.logger.error(f"Error in TX thread {thread_id}: {e}")
                    break
                    
        except Exception as e:
            self.logger.error(f"Error in TX thread {thread_id}: {e}")
        finally:
            self.logger.info(f"TX thread {thread_id}: Stopping")
    
    def rx_thread_function(self):
        """Receive thread function"""
        thread_id = 1
        frame_size = 5000
        local_frame_count = 0
        
        try:
            # Start continuous streaming like the old code
            self.rx_streamer.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.start_cont))
            self.logger.info(f"RX thread {thread_id}: Started continuous streaming")
            
            # Create RX metadata like the old code
            rx_md = uhd.types.RXMetadata()
            
            while not self.stop_event.is_set():
                try:
                    # Create buffer for samples
                    samples = np.zeros(frame_size, dtype=np.complex64)
                    
                    # Receive samples with metadata
                    num_samps = self.rx_streamer.recv(samples, rx_md)
                    
                    if num_samps > 0:
                        # Create frame data
                        frame_data = (local_frame_count, samples[:num_samps], time.time(), thread_id)
                        
                        # Put in RX queue
                        try:
                            self.rx_queue.put(frame_data, timeout=0.1)
                            local_frame_count += 1
                            
                            # Log progress
                            if local_frame_count % 5000 == 0:
                                self.logger.info(f"RX thread {thread_id}: collected {local_frame_count} local frames")
                                
                        except queue.Full:
                            self.logger.warning("RX queue is full, dropping frame")
                            continue
                    else:
                        # Log when no samples received
                        if local_frame_count % 1000 == 0:
                            self.logger.debug(f"RX thread {thread_id}: no samples received, num_samps={num_samps}")
                    
                    # Small delay to prevent overwhelming
                    time.sleep(0.001)
                        
                except Exception as e:
                    self.logger.error(f"Error in RX thread {thread_id} receive loop: {e}")
                    time.sleep(0.1)  # Wait a bit before retrying
                    continue
                        
        except Exception as e:
            self.logger.error(f"Error in RX thread {thread_id}: {e}")
        finally:
            self.logger.info(f"RX thread {thread_id}: Stopping")
    
    def start(self):
        """Start the TX/RX process"""
        self.logger.info(f"Starting TX/RX process on CPU core {self.cpu_core}...")
        
        try:
            # Setup USRP
            self.logger.info("Setting up USRP device...")
            if not self.setup_usrp():
                self.logger.error("Failed to setup USRP, exiting")
                return False
            
            self.logger.info("USRP setup complete, starting threads...")
            
            # Start TX thread
            self.tx_thread = threading.Thread(target=self.tx_thread_function)
            self.tx_thread.daemon = True
            self.tx_thread.start()
            self.logger.info("TX thread started")
            
            # Start RX thread
            self.rx_thread = threading.Thread(target=self.rx_thread_function)
            self.rx_thread.daemon = True
            self.rx_thread.start()
            self.logger.info("RX thread started")
            
            self.logger.info(f"Started 1 TX thread and 1 RX thread successfully")
            self.logger.info(f"Total threads running: {threading.active_count()}")
            
            # Keep the process alive
            self.logger.info("TX/RX process main thread entering wait loop...")
            while not self.stop_event.is_set():
                # Check if parent process is still alive
                if not self._check_parent_alive():
                    self.logger.warning("Parent process died or is orphaned, stopping TX/RX process...")
                    break
                
                time.sleep(1)
                
                # Check if threads are still alive (only if we're not stopping)
                if not self.stop_event.is_set():
                    if not self.tx_thread.is_alive():
                        self.logger.error("TX thread died unexpectedly")
                        break
                    if not self.rx_thread.is_alive():
                        self.logger.error("RX thread died unexpectedly")
                        break
                else:
                    # We're stopping, threads are expected to die
                    self.logger.info("Stop signal received, threads are expected to terminate")
                    break
            
            # Log the shutdown process
            if self.stop_event.is_set():
                self.logger.info("TX/RX process shutting down normally (stop signal received)")
            else:
                self.logger.info("TX/RX process shutting down due to parent/thread issues")
            
            self.logger.info("TX/RX process main thread exiting")
            return True
            
        except Exception as e:
            self.logger.error(f"Error starting TX/RX process: {e}")
            return False
    
    def stop(self):
        """Stop the TX/RX process"""
        self.logger.info("Stopping TX/RX process...")
        
        # Signal threads to stop
        self.stop_event.set()
        
        # Wait for threads to finish
        if self.tx_thread and self.tx_thread.is_alive():
            self.tx_thread.join(timeout=2.0)
        
        if self.rx_thread and self.rx_thread.is_alive():
            self.rx_thread.join(timeout=2.0)
        
        # Cleanup USRP resources
        if self.usrp:
            try:
                del self.tx_streamer
                del self.rx_streamer
                del self.usrp
                self.logger.info("USRP resources cleaned up successfully")
            except Exception as e:
                self.logger.error(f"Error cleaning up USRP resources: {e}")
        
        self.logger.info("TX/RX process stopped")
    
    def is_alive(self):
        """Check if the process is alive"""
        return (self.tx_thread and self.tx_thread.is_alive()) or \
               (self.rx_thread and self.rx_thread.is_alive())
    
    def get_stats(self):
        """Get process statistics"""
        return {
            'tx_samples_sent': self.tx_samples_sent,
            'rx_frames_collected': self.rx_frames_collected,
            'tx_thread_alive': self.tx_thread.is_alive() if self.tx_thread else False,
            'rx_thread_alive': self.rx_thread.is_alive() if self.rx_thread else False
        }

def main():
    """Main function for standalone testing"""
    import argparse
    
    parser = argparse.ArgumentParser(description='TX/RX Process')
    parser.add_argument('--device', default='', help='USRP device string')
    parser.add_argument('--freq', type=float, default=2.4e9, help='Center frequency')
    parser.add_argument('--sample-rate', type=float, default=1e6, help='Sample rate')
    parser.add_argument('--gain', type=float, default=37.5, help='RX gain')
    parser.add_argument('--cpu-core', type=int, default=1, help='CPU core to bind to')
    
    args = parser.parse_args()
    
    # Create stop event
    import multiprocessing
    stop_event = multiprocessing.Event()
    
    # Create RX queue
    rx_queue = multiprocessing.Queue(maxsize=100)
    
    # Create TX wave
    tx_wave = np.random.randn(10000).astype(np.complex64)
    
    # Create and start process
    process = TXRXProcess(
        tx_wave=tx_wave,
        hop_size=1000,
        stop_event=stop_event,
        rx_queue=rx_queue,
        timestamp="test",
        device=args.device,
        freq=args.freq,
        sample_rate=args.sample_rate,
        gain=args.gain,
        cpu_core=args.cpu_core
    )
    
    try:
        if process.start():
            process.logger.info("TX/RX Process started successfully")
            
            # Keep running until stop signal
            while not stop_event.is_set():
                time.sleep(1)
                
        else:
            process.logger.error("Failed to start TX/RX Process")
            
    except KeyboardInterrupt:
        process.logger.info("Received keyboard interrupt")
    finally:
        process.stop()

if __name__ == "__main__":
    main()
