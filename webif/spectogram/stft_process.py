#!/usr/bin/env python3
"""
STFT Process - Self-contained signal processing process
Handles Short-Time Fourier Transform on received IQ samples
"""

import time
import gc
import queue
import os
import signal
from pathlib import Path

# Add the spectogram directory to Python path for imports
script_dir = Path(__file__).parent

try:
    import numpy as np
    from scipy import signal as scipy_signal
except ImportError as e:
    print(f"Import error: {e}")
    sys.exit(1)

from process_base import ProcessBase

class STFTProcess(ProcessBase):
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
        # Call parent constructor first
        super().__init__("STFT", stop_event, timestamp, cpu_core, log_dir)
        
        # Process-specific attributes
        self.rx_queue = rx_queue
        self.sample_rate = sample_rate
        self.fft_size = fft_size
        self.window_size = window_size
        self.hop_size = hop_size
        self.window_type = window_type
        self.stft_queue = stft_queue
        
        # STFT-specific state
        self.skipped_frames = 0
        
        # Set STFT-specific memory threshold (500MB for large array processing)
        self.set_memory_threshold(500)
        self.set_memory_check_interval(10.0)  # Check every 10 seconds
        
        # Cache window function to avoid recreating it every frame
        self.window_cache = self._create_window(self.window_size, self.window_type)
        self.logger.info(f"Window function '{self.window_type}' cached for size {self.window_size}")
        
        # Log STFT process initialization parameters
        self.logger.info(f"STFT [INFO] Initializing STFTProcess: rx_queue={type(rx_queue)}, sample_rate={self.sample_rate}, window_size={self.window_size}, fft_size={self.fft_size}, hop_size={self.hop_size}, window_type={self.window_type}, cpu_core={self.cpu_core}, log_dir={self.log_dir}, timestamp={self.timestamp}")
    
    def _create_window(self, window_size, window_type):
        """Create window function for STFT"""
        try:
            if window_type.lower() == 'hann':
                return scipy_signal.windows.hann(window_size)
            elif window_type.lower() == 'hamming':
                return scipy_signal.windows.hamming(window_size)
            elif window_type.lower() == 'blackman':
                return scipy_signal.windows.blackman(window_size)
            elif window_type.lower() == 'bartlett':
                return scipy_signal.windows.bartlett(window_size)
            elif window_type.lower() == 'flattop':
                return scipy_signal.windows.flattop(window_size)
            else:
                # Default to Hann window
                self.logger.warning(f"Unknown window type '{window_type}', using Hann window")
                return scipy_signal.windows.hann(window_size)
        except Exception as e:
            self.logger.error(f"Error creating window function: {e}, using Hann window")
            return scipy_signal.windows.hann(window_size)
    
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
            
            # Check if window size exceeds input signal length
            if self.window_size > actual_samples:
                self.logger.warning(f"Frame {frame_num}: Skipping - window_size ({self.window_size}) > input_samples ({actual_samples})")
                return None
            
            # Use configured window and hop sizes directly
            overlap_samples = self.window_size - self.hop_size
            
            # STFT calculation with configured parameters - use cached window
            f, t, Zxx = scipy_signal.stft(
                samples, 
                fs=self.sample_rate, 
                nperseg=self.window_size, 
                noverlap=overlap_samples,
                nfft=self.fft_size,
                window=self.window_cache,  # Use cached window instead of recreating
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
            
            # Create result tuple
            result = (frame_num, f, t, magnitude_db, timestamp)
            
            # Explicit cleanup of intermediate arrays to help garbage collection
            del Zxx
            
            return result
            
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
                    
                    # Get current time for monitoring
                    current_time = time.time()
                    
                    # Use base class garbage collection
                    last_gc_time = self._force_garbage_collection(current_time, last_gc_time, gc_interval)
                    
                    # Use base class memory monitoring
                    self._check_memory_periodic(current_time)
                    
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
                        
                        # Use base class FPS logging
                        self._log_fps(current_time)
                    
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
    
    def get_stats(self):
        """Get process statistics"""
        stats = super().get_stats()
        stats.update({
            'frames_processed': self.frame_count,
            'skipped_frames': self.skipped_frames
        })
        return stats

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
