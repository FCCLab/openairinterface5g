#!/usr/bin/env python3
"""
STFT Process - Self-contained signal processing process
Handles Short-Time Fourier Transform on received IQ samples
"""

import logging
import time
import signal
import os
import sys
from pathlib import Path

# Add the spectogram directory to Python path for imports
script_dir = Path(__file__).parent
sys.path.insert(0, str(script_dir))

try:
    import numpy as np
    from scipy import signal as scipy_signal
    import queue
except ImportError as e:
    logging.error(f"Import error: {e}")
    sys.exit(1)

class STFTProcess:
    """Self-contained STFT process for signal processing"""
    
    def __init__(self, sample_rate, fft_size, stop_event, rx_queue, stft_queue, timestamp, cpu_core=1, log_dir=None):
        """
        Initialize STFT process
        
        Args:
            sample_rate: Sample rate
            fft_size: FFT size
            stop_event: Multiprocessing event to signal stop
            rx_queue: Queue for RX data
            stft_queue: Queue for processed STFT data
            timestamp: Main process timestamp
            cpu_core: CPU core to bind to
        """
        self.sample_rate = sample_rate
        self.fft_size = fft_size
        self.stop_event = stop_event
        self.rx_queue = rx_queue
        self.stft_queue = stft_queue
        self.timestamp = timestamp
        self.cpu_core = cpu_core
        self.log_dir = log_dir
        
        # Process state
        self.frame_count = 0
        
        # FPS tracking
        self.start_time = time.time()
        self.last_fps_log_time = time.time()
        self.fps_log_interval = 5.0  # Log FPS every 5 seconds
        
        # Setup logging
        self._setup_logging()
        
        # Setup signal handlers
        self._setup_signal_handlers()
        
        # Setup CPU affinity
        self._setup_cpu_affinity()
    
    def _setup_logging(self):
        """Setup process-specific logging"""
        log_filename = f"spectrogram_{self.timestamp}_stft.log"
        # Use log directory passed from main process, or fallback to script directory
        if self.log_dir:
            log_path = self.log_dir / log_filename
        else:
            log_path = script_dir / "logs" / log_filename
        
        # Create logs directory if it doesn't exist
        log_path.parent.mkdir(exist_ok=True)
        
        # Create a logger specific to this process
        self.logger = logging.getLogger(f"STFTProcess_{self.timestamp}")
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
        file_handler = logging.FileHandler(log_path)
        file_handler.setLevel(logging.INFO)
        file_handler.setFormatter(formatter)
        self.logger.addHandler(file_handler)
        
        self.logger.info(f"STFT Process logging setup complete: {log_path}")
        self.logger.info(f"Current working directory: {os.getcwd()}")
        self.logger.info(f"Script directory: {script_dir}")
    
    def _setup_signal_handlers(self):
        """Setup signal handlers for graceful shutdown"""
        # Flag to prevent multiple signal processing
        self._signal_received = False
        
        def signal_handler(signum, frame):
            # Only process signal once
            if self._signal_received:
                return
            
            self._signal_received = True
            self.logger.info(f"STFT Process received signal {signum}, stopping...")
            self.stop_event.set()
        
        signal.signal(signal.SIGINT, signal_handler)
        signal.signal(signal.SIGTERM, signal_handler)
        # Add SIGHUP for backend control
        signal.signal(signal.SIGHUP, signal_handler)
        self.logger.info("STFT Process: Signal handlers set up successfully")
    
    def _check_parent_alive(self):
        """Check if parent process (spectrogram_main) is still alive"""
        try:
            # Get parent PID (spectrogram_main)
            parent_pid = os.getppid()
            
            # Check if parent is still running
            if parent_pid == 1:  # Adopted by init - parent died or is orphaned
                self.logger.warning("Parent process died or is orphaned, initiating self-cleanup...")
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
    
    def _setup_cpu_affinity(self):
        """Set CPU affinity for this process"""
        try:
            import psutil
            process = psutil.Process()
            process.cpu_affinity([self.cpu_core])
            self.logger.info(f"Successfully set STFT process affinity to core {self.cpu_core}")
        except Exception as e:
            self.logger.error(f"Error setting CPU affinity: {e}")
    
    def process_frame(self, frame_data):
        """Process a single frame of IQ data"""
        try:
            # Handle both old format (3 items) and new format (4 items with thread_id)
            if len(frame_data) == 4:
                frame_num, samples, timestamp, thread_id = frame_data
            else:
                frame_num, samples, timestamp = frame_data
                thread_id = 0  # Default for backward compatibility
            
            # Perform STFT processing
            window_size = self.fft_size  # Use FFT size as window size
            overlap_samples = window_size // 4  # 25% overlap instead of 50%
            
            # STFT calculation
            f, t, Zxx = scipy_signal.stft(
                samples, 
                fs=self.sample_rate, 
                nperseg=window_size, 
                noverlap=overlap_samples,
                nfft=self.fft_size,
                return_onesided=False
            )
            
            # Force to use only 1 time point (first time slice)
            if Zxx.shape[1] > 1:
                Zxx = Zxx[:, 0:1]  # Take only the first time slice
                t = t[0:1]  # Take only the first time point
            
            # Convert to magnitude in dB
            magnitude_db = 20 * np.log10(np.abs(Zxx) + 1e-10)
            
            # Replace any remaining NaN/Inf values
            magnitude_db = np.nan_to_num(magnitude_db, nan=-120.0, posinf=-120.0, neginf=-120.0)
            
            return frame_num, f, t, magnitude_db, timestamp
            
        except Exception as e:
            self.logger.error(f"Error processing frame {frame_data[0] if frame_data else 'unknown'}: {e}")
            return None
    
    def run(self):
        """Main processing loop with parent monitoring"""
        self.logger.info(f"Starting STFT process on CPU core {self.cpu_core}...")
        
        try:
            while not self.stop_event.is_set():
                try:
                    # Check if parent process is still alive
                    if not self._check_parent_alive():
                        self.logger.warning("Parent process died or is orphaned, sending SIGKILL to self...")
                        os.kill(os.getpid(), signal.SIGKILL)  # Force kill self
                        break
                    
                    # Get IQ data from RX queue
                    frame_data = self.rx_queue.get(timeout=1.0)  # 1 second timeout
                    
                    # Process the frame
                    result = self.process_frame(frame_data)
                    
                    if result is not None:
                        # Put processed data into STFT queue
                        self.stft_queue.put(result)
                        self.frame_count += 1
                        
                        # Log progress and FPS occasionally
                        if self.frame_count % 5000 == 0:
                            self.logger.info(f"STFT process: Processed {self.frame_count} frames, latest frame {result[0]}")
                        
                        # Log FPS periodically
                        current_time = time.time()
                        if current_time - self.last_fps_log_time >= self.fps_log_interval:
                            elapsed_time = current_time - self.start_time
                            if elapsed_time > 0:
                                fps = self.frame_count / elapsed_time
                                self.logger.info(f"STFT FPS: {fps:.2f} frames/sec (Total: {self.frame_count} frames in {elapsed_time:.1f}s)")
                                self.last_fps_log_time = current_time
                    
                except queue.Empty:
                    # No data available, continue
                    continue
                except Exception as e:
                    self.logger.error(f"Error in STFT process main loop: {e}")
                    time.sleep(0.1)  # Brief pause before retrying
                    continue
            
            return True
                    
        except Exception as e:
            self.logger.error(f"Critical error in STFT process: {e}")
            return False
        finally:
            self.logger.info("STFT process cleanup completed")
    
    def start(self):
        """Start the STFT process"""
        try:
            self.run()
            return True
        except Exception as e:
            self.logger.error(f"Error starting STFT process: {e}")
            return False
    
    def stop(self):
        """Stop the STFT process"""
        self.logger.info("Stopping STFT process...")
        self.stop_event.set()
    
    def is_alive(self):
        """Check if the process is alive"""
        return not self.stop_event.is_set()
    
    def get_stats(self):
        """Get process statistics"""
        return {
            'frames_processed': self.frame_count,
            'is_alive': self.is_alive()
        }

def main():
    """Main function for standalone testing"""
    import argparse
    import multiprocessing
    
    parser = argparse.ArgumentParser(description='STFT Process')
    parser.add_argument('--sample-rate', type=float, default=1e6, help='Sample rate')
    parser.add_argument('--fft-size', type=int, default=1024, help='FFT size')
    parser.add_argument('--cpu-core', type=int, default=1, help='CPU core to bind to')
    
    args = parser.parse_args()
    
    # Create stop event
    stop_event = multiprocessing.Event()
    
    # Create queues
    rx_queue = multiprocessing.Queue(maxsize=100)
    stft_queue = multiprocessing.Queue(maxsize=100)
    
    # Create and start process
    process = STFTProcess(
        sample_rate=args.sample_rate,
        fft_size=args.fft_size,
        stop_event=stop_event,
        rx_queue=rx_queue,
        stft_queue=stft_queue,
        timestamp="test",
        cpu_core=args.cpu_core
    )
    
    try:
        if process.start():
            process.logger.info("STFT Process started successfully")
        else:
            process.logger.error("Failed to start STFT Process")
            
    except KeyboardInterrupt:
        process.logger.info("Received keyboard interrupt")
    finally:
        process.stop()

if __name__ == "__main__":
    main()
