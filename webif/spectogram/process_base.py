#!/usr/bin/env python3
"""
Base class for spectrogram service processes.
Provides common functionality for memory management, logging, and process lifecycle.
"""

import os
import sys
import time
import signal
import logging
import multiprocessing
from pathlib import Path

# Add script directory to path for imports
script_dir = Path(__file__).parent
sys.path.insert(0, str(script_dir))


class ProcessBase:
    """Base class for all spectrogram service processes"""
    
    def __init__(self, process_name, stop_event, timestamp, cpu_core=1, log_dir=None):
        """
        Initialize base process
        
        Args:
            process_name: Name of the process (e.g., 'TX/RX', 'STFT', 'WebSocket')
            stop_event: Multiprocessing event to signal stop
            timestamp: Main process timestamp for logging
            cpu_core: CPU core to bind to
            log_dir: Directory for log files
        """
        self.process_name = process_name
        self.stop_event = stop_event
        self.timestamp = timestamp
        self.cpu_core = cpu_core
        self.log_dir = log_dir
        
        # Process state
        self.frame_count = 0
        self.start_time = time.time()
        self.last_fps_log_time = time.time()
        self.fps_log_interval = 5.0  # Log FPS every 5 seconds
        
        # Memory management
        self.last_memory_check = time.time()
        self.memory_check_interval = 15.0  # Check memory every 15 seconds
        self.memory_threshold = 500  # Default 500MB threshold
        
        # Signal handling
        self._signal_received = False
        
        # Setup logging first
        self._setup_logging()
        
        # Setup signal handlers
        self._setup_signal_handlers()
        
        # Setup CPU affinity
        self._setup_cpu_affinity()
        
        self.logger.info(f"{self.process_name} Process base initialization complete")
    
    def _setup_logging(self):
        """Setup process-specific logging"""
        log_filename = f"spectrogram_{self.timestamp}_{self.process_name.lower().replace('/', '_')}.log"
        
        # Use log directory passed from main process, or fallback to script directory
        if self.log_dir:
            log_path = self.log_dir / log_filename
        else:
            log_path = script_dir / "logs" / log_filename
        
        # Create logs directory if it doesn't exist
        log_path.parent.mkdir(exist_ok=True)
        
        # Create a logger specific to this process
        self.logger = logging.getLogger(f"{self.process_name}Process_{self.timestamp}")
        self.logger.setLevel(logging.INFO)
        
        # Clear any existing handlers to avoid duplicates
        self.logger.handlers.clear()
        
        # Create formatter
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        
        # Add console handler
        console_handler = logging.StreamHandler()
        console_handler.setLevel(logging.INFO)
        console_handler.setFormatter(formatter)
        self.logger.addHandler(console_handler)
        
        # Add file handler
        file_handler = logging.FileHandler(log_path)
        file_handler.setLevel(logging.INFO)
        file_handler.setFormatter(formatter)
        self.logger.addHandler(file_handler)
        
        self.logger.info(f"{self.process_name} Process logging setup complete: {log_path}")
        self.logger.info(f"Current working directory: {os.getcwd()}")
        self.logger.info(f"Script directory: {script_dir}")
    
    def _setup_signal_handlers(self):
        """Setup signal handlers for graceful shutdown"""
        def signal_handler(signum, frame):
            # Only process signal once
            if self._signal_received:
                return
            
            self._signal_received = True
            self.logger.info(f"{self.process_name} Process received signal {signum}, stopping...")
            self.stop_event.set()
        
        signal.signal(signal.SIGINT, signal_handler)
        signal.signal(signal.SIGTERM, signal_handler)
        signal.signal(signal.SIGHUP, signal_handler)
        self.logger.info(f"{self.process_name} Process: Signal handlers set up successfully")
    
    def _setup_cpu_affinity(self):
        """Set CPU affinity for this process"""
        try:
            import psutil
            process = psutil.Process()
            process.cpu_affinity([self.cpu_core])
            self.logger.info(f"Successfully set {self.process_name} process affinity to core {self.cpu_core}")
        except Exception as e:
            self.logger.error(f"Error setting CPU affinity: {e}")
    
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
    
    def _monitor_memory(self):
        """Monitor memory usage and trigger cleanup if needed"""
        try:
            import psutil
            process = psutil.Process()
            memory_mb = process.memory_info().rss / 1024 / 1024
            
            if memory_mb > self.memory_threshold:
                self.logger.warning(f"High memory usage: {memory_mb:.1f} MB, triggering cleanup")
                # Force garbage collection
                collected = gc.collect()
                if collected > 0:
                    self.logger.info(f"Garbage collection freed {collected} objects")
                
                # Check memory after cleanup
                memory_after = process.memory_info().rss / 1024 / 1024
                self.logger.info(f"Memory after cleanup: {memory_after:.1f} MB")
            
            return memory_mb
            
        except Exception as e:
            self.logger.debug(f"Memory monitoring failed: {e}")
            return 0
    
    def _log_fps(self, current_time):
        """Log FPS information if enough time has passed"""
        if current_time - self.last_fps_log_time >= self.fps_log_interval:
            elapsed_time = current_time - self.start_time
            if elapsed_time > 0:
                fps = self.frame_count / elapsed_time
                self.logger.info(f"{self.process_name} FPS: {fps:.2f} frames/sec (Total: {self.frame_count} frames in {elapsed_time:.1f}s)")
                self.last_fps_log_time = current_time
    
    def _check_memory_periodic(self, current_time):
        """Check memory usage periodically"""
        if current_time - self.last_memory_check >= self.memory_check_interval:
            memory_mb = self._monitor_memory()
            if memory_mb > 0:
                self.logger.debug(f"Current memory usage: {memory_mb:.1f} MB")
            self.last_memory_check = current_time
    
    def _force_garbage_collection(self, current_time, last_gc_time, gc_interval):
        """Force garbage collection periodically"""
        if current_time - last_gc_time >= gc_interval:
            import gc
            collected = gc.collect()
            if collected > 0:
                self.logger.debug(f"Garbage collection: collected {collected} objects")
            return current_time
        return last_gc_time
    
    def set_memory_threshold(self, threshold_mb):
        """Set memory threshold for this process"""
        self.memory_threshold = threshold_mb
        self.logger.info(f"Memory threshold set to {threshold_mb} MB")
    
    def set_memory_check_interval(self, interval_seconds):
        """Set memory check interval"""
        self.memory_check_interval = interval_seconds
        self.logger.info(f"Memory check interval set to {interval_seconds} seconds")
    
    def get_stats(self):
        """Get process statistics"""
        return {
            'process_name': self.process_name,
            'frames_processed': self.frame_count,
            'uptime': time.time() - self.start_time,
            'is_alive': not self.stop_event.is_set(),
            'cpu_core': self.cpu_core
        }
    
    def is_alive(self):
        """Check if the process is alive"""
        return not self.stop_event.is_set()
    
    def stop(self):
        """Stop the process"""
        self.logger.info(f"Stopping {self.process_name} process...")
        self.stop_event.set()
    
    def cleanup(self):
        """Clean up process resources - override in subclasses"""
        self.logger.info(f"{self.process_name} Process cleanup completed")
    
    def run(self):
        """Main process loop - override in subclasses"""
        raise NotImplementedError("Subclasses must implement run() method")
    
    def start(self):
        """Start the process"""
        try:
            self.logger.info(f"Starting {self.process_name} process...")
            result = self.run()
            self.logger.info(f"{self.process_name} process completed")
            return result
        except Exception as e:
            self.logger.error(f"Error in {self.process_name} process: {e}")
            return False
        finally:
            self.cleanup()


if __name__ == "__main__":
    """Test the base class"""
    import multiprocessing
    
    # Create stop event
    stop_event = multiprocessing.Event()
    
    # Create test process
    class TestProcess(ProcessBase):
        def run(self):
            self.logger.info("Test process running...")
            time.sleep(2)
            return True
    
    # Test the base class
    process = TestProcess("Test", stop_event, "test", cpu_core=0)
    process.start()
