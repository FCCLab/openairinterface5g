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
import gc
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
    
    def __init__(self, rx_queue, sample_rate, window_size, fft_size, hop_size, window_type, stop_event, stft_queue, timestamp, cpu_core=1, log_dir=None):
        """
        Initialize STFT process
        
        Args:
            rx_queue: Queue for RX data input
            sample_rate: Sample rate
            window_size: Window size for STFT
            fft_size: FFT size
            hop_size: Hop size between windows
            window_type: Window function type (hann, hamming, blackman, etc.)
            stop_event: Multiprocessing event to signal stop
            stft_queue: Queue for processed STFT data
            timestamp: Main process timestamp
            cpu_core: CPU core to bind to
        """
        self.rx_queue = rx_queue
        self.sample_rate = sample_rate
        self.fft_size = fft_size
        self.window_size = window_size
        self.hop_size = hop_size
        self.window_type = window_type
        self.stop_event = stop_event
        self.stft_queue = stft_queue
        self.timestamp = timestamp
        self.cpu_core = cpu_core
        self.log_dir = log_dir

        # Setup logging
        self._setup_logging()
        
        # Log STFT process initialization parameters
        self.logger.info(f"STFT [INFO] Initializing STFTProcess: rx_queue={type(rx_queue)}, sample_rate={self.sample_rate}, window_size={self.window_size}, fft_size={self.fft_size}, hop_size={self.hop_size}, window_type={self.window_type}, cpu_core={self.cpu_core}, log_dir={self.log_dir}, timestamp={self.timestamp}")
        
        # Process state
        self.frame_count = 0
        self.skipped_frames = 0
        
        # FPS tracking
        self.start_time = time.time()
        self.last_fps_log_time = time.time()
        self.fps_log_interval = 5.0  # Log FPS every 5 seconds

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
            if not frame_data or len(frame_data) < 3:
                self.logger.warning(f"Invalid frame data: {frame_data}")
                return None
            
            # Extract frame data
            if len(frame_data) == 4:
                frame_num, samples, timestamp, thread_id = frame_data
            else:
                frame_num, samples, timestamp = frame_data
                thread_id = 0  # Default for backward compatibility
            
            # Check if we have enough samples for STFT
            actual_samples = len(samples)
            if actual_samples == 0:
                self.logger.warning(f"Frame {frame_num}: No samples received")
                return None
            
            # Ensure window size doesn't exceed signal length
            effective_window_size = min(self.window_size, actual_samples)
            if effective_window_size < self.window_size:
                self.logger.debug(f"Frame {frame_num}: Reducing window size from {self.window_size} to {effective_window_size} (signal length: {actual_samples})")
            
            # Ensure hop size doesn't exceed window size
            effective_hop_size = min(self.hop_size, effective_window_size)
            if effective_hop_size < self.hop_size:
                self.logger.debug(f"Frame {frame_num}: Reducing hop size from {self.hop_size} to {effective_hop_size}")
            
            # Calculate overlap samples from hop size
            overlap_samples = effective_window_size - effective_hop_size
            
            # STFT calculation with adjusted parameters
            f, t, Zxx = scipy_signal.stft(
                samples, 
                fs=self.sample_rate, 
                nperseg=effective_window_size, 
                noverlap=overlap_samples,
                nfft=self.fft_size,
                window=self.window_type.lower(),
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
        
        # Performance monitoring variables
        last_gc_time = time.time()
        gc_interval = 10.0  # Run garbage collection every 10 seconds
        
        try:
            while not self.stop_event.is_set():
                try:
                    # Check if parent process is still alive
                    if not self._check_parent_alive():
                        self.logger.warning("Parent process died or is orphaned, sending SIGKILL to self...")
                        os.kill(os.getpid(), signal.SIGKILL)  # Force kill self
                        break
                    
                    # Periodic garbage collection to prevent memory buildup
                    current_time = time.time()
                    if current_time - last_gc_time >= gc_interval:
                        collected = gc.collect()
                        if collected > 0:
                            self.logger.debug(f"Garbage collection: collected {collected} objects")
                        last_gc_time = current_time
                    
                    # Get IQ data from RX queue
                    frame_data = self.rx_queue.get(timeout=1.0)  # 1 second timeout
                    
                    # Check if STFT queue is getting too full - skip frames to maintain real-time performance
                    queue_size = self.stft_queue.qsize()
                    if queue_size > 200:  # Increased threshold from 50 to 200 for larger queue
                        # Dynamic frame skipping based on queue size
                        skip_frames = min(queue_size // 100, 10)  # Skip 1-10 frames based on queue size
                        if self.frame_count % 100 == 0:  # Log every 100th skipped frame
                            self.logger.warning(f"STFT queue full ({queue_size} frames), skipping {skip_frames} frames to maintain real-time performance")
                        # Skip processing this frame to prevent queue buildup
                        self.skipped_frames += 1
                        del frame_data
                        continue
                    
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
                                self.logger.info(f"STFT FPS: {fps:.2f} frames/sec (Total: {self.frame_count} frames, Skipped: {self.skipped_frames} frames in {elapsed_time:.1f}s)")
                                self.last_fps_log_time = current_time
                    
                    # Clear frame_data and result to help garbage collection
                    del frame_data
                    if result is not None:
                        del result
                    
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
