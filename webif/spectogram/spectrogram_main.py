#!/usr/bin/env python3
"""
Spectrogram Main - Main orchestrator for the spectrogram service
Manages TX/RX, STFT, and WebSocket processes with proper isolation
"""

import logging
import time
import signal
import os
import sys
import multiprocessing
import psutil
from pathlib import Path

# Add the spectogram directory to Python path for imports
script_dir = Path(__file__).parent
sys.path.insert(0, str(script_dir))

try:
    import numpy as np
    from cpu_pe import get_p_e_cores
    from tx_rx_process import TXRXProcess
    from stft_process import STFTProcess
    from websocket_process import WebSocketProcess
except ImportError as e:
    logging.error(f"Import error: {e}")
    sys.exit(1)

class SpectrogramMain:
    """Main orchestrator for the spectrogram service"""
    
    def __init__(self, args):
        """
        Initialize the main orchestrator
        
        Args:
            args: Command line arguments
        """
        self.args = args
        self.main_timestamp = time.strftime("%Y%m%d_%H%M%S")
        
        # Setup logging and get log directory
        self.log_dir = self._setup_logging()

        # Process management
        self.tx_rx_process = None
        self.stft_process = None
        self.websocket_process = None
        self.stop_event = multiprocessing.Event()
        
        # Queues for inter-process communication
        self.rx_queue = multiprocessing.Queue(maxsize=100)
        self.stft_queue = multiprocessing.Queue(maxsize=100)
        
        # CPU core assignments
        self.cpu_cores = self._setup_cpu_cores()
        
        # Restart mechanism
        self.restart_attempts = {
            'TX/RX': {'count': 0, 'last_restart': 0, 'process': None},
            'STFT': {'count': 0, 'last_restart': 0, 'process': None},
            'WebSocket': {'count': 0, 'last_restart': 0, 'process': None}
        }
        self.max_restart_attempts = 10
        self.restart_time_window = 60000  # 1 minute in milliseconds
        
        # Setup signal handlers
        self._setup_signal_handlers()

    
    def _setup_logging(self):
        """Setup main process logging"""
        log_filename = f"spectrogram_{self.main_timestamp}.log"
        # Use absolute path based on script directory
        log_path = script_dir / "logs" / log_filename
        
        # Create logs directory if it doesn't exist
        log_path.parent.mkdir(exist_ok=True)
        
        # Create a logger specific to this process
        logger = logging.getLogger(f"MainProcess_{self.main_timestamp}")
        logger.setLevel(logging.INFO)
        
        # Clear any existing handlers to avoid duplicates
        logger.handlers.clear()
        
        # Create formatter
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        
        # Add console handler (this ensures output goes to console)
        console_handler = logging.StreamHandler()
        console_handler.setLevel(logging.INFO)
        console_handler.setFormatter(formatter)
        logger.addHandler(console_handler)
        
        # Add file handler
        file_handler = logging.FileHandler(log_path)
        file_handler.setLevel(logging.INFO)
        file_handler.setFormatter(formatter)
        logger.addHandler(file_handler)
        
        # Store the logger in self
        self.logger = logger
        
        self.logger.info(f"Main process logging setup complete: {log_path}")
        self.logger.info(f"Current working directory: {os.getcwd()}")
        self.logger.info(f"Script directory: {script_dir}")
        
        return log_path.parent  # Return the logs directory path
    
    def _setup_signal_handlers(self):
        """Setup signal handlers for graceful shutdown"""
        def signal_handler(signum, frame):
            self.logger.info(f"Main process received signal {signum}, initiating graceful shutdown...")
            # Set stop event and trigger cleanup
            self.stop_event.set()
            # Force cleanup if signal is SIGTERM (backend termination)
            if signum == signal.SIGTERM:
                self.logger.info("SIGTERM received, forcing immediate cleanup...")
                self._force_cleanup()
        
        signal.signal(signal.SIGINT, signal_handler)
        signal.signal(signal.SIGTERM, signal_handler)
        # Add SIGHUP for backend control
        signal.signal(signal.SIGHUP, signal_handler)
        self.logger.info("Main process: Signal handlers set up successfully")
    
    def _force_cleanup(self):
        """Force immediate cleanup for backend termination"""
        try:
            self.logger.info("Force cleanup initiated...")
            
            # Method 1: Try to terminate process group (most effective)
            if hasattr(self, 'process_group_id'):
                try:
                    self.logger.info(f"Terminating entire process group {self.process_group_id}...")
                    os.killpg(self.process_group_id, signal.SIGTERM)
                    
                    # Wait briefly for process group termination
                    time.sleep(2)
                    
                    # Check if any processes are still alive
                    if not self._all_processes_dead():
                        self.logger.info("Process group termination incomplete, force killing...")
                        os.killpg(self.process_group_id, signal.SIGKILL)
                        time.sleep(1)
                except Exception as e:
                    self.logger.warning(f"Process group termination failed: {e}")
            
            # Method 2: Individual process termination (fallback)
            if not self._all_processes_dead():
                self.logger.info("Falling back to individual process termination...")
                processes_to_cleanup = [
                    ("TX/RX", self.tx_rx_process),
                    ("STFT", self.stft_process),
                    ("WebSocket", self.websocket_process)
                ]
                
                for name, process in processes_to_cleanup:
                    if process and process.is_alive():
                        self.logger.info(f"Force terminating {name} process...")
                        process.terminate()
                        # Give a very short time for graceful exit
                        process.join(timeout=1.0)
                        if process.is_alive():
                            self.logger.info(f"Force killing {name} process...")
                            process.kill()
            
            self.logger.info("Force cleanup completed")
            
        except Exception as e:
            self.logger.error(f"Error during force cleanup: {e}")
    
    def _all_processes_dead(self):
        """Check if all child processes are dead"""
        processes = [self.tx_rx_process, self.stft_process, self.websocket_process]
        return all(not p or not p.is_alive() for p in processes)
    
    def _setup_cpu_cores(self):
        """Setup CPU core assignments using P-core/E-core detection"""
        try:
            # Get P-cores and E-cores
            p_cores, e_cores = get_p_e_cores()
            
            self.logger.info(f"Total CPU cores detected: {len(p_cores) + len(e_cores)}")
            self.logger.info(f"P core (number): {p_cores}")
            self.logger.info(f"E core (number): {e_cores}")
            
            # Assign cores: Main (E-core), TX/RX (P-core), STFT (P-core), WebSocket (P-core)
            if len(p_cores) >= 3 and len(e_cores) >= 1:
                cpu_cores = {
                    'main': e_cores[0],      # Main process on first E-core
                    'tx_rx': p_cores[0],     # TX/RX on first P-core
                    'stft': p_cores[1],      # STFT on second P-core
                    'websocket': p_cores[2]  # WebSocket on third P-core
                }
            else:
                # Fallback to default cores if not enough P/E cores
                self.logger.warning("Not enough P/E cores detected, using default assignment")
                cpu_cores = {
                    'main': 0,
                    'tx_rx': 1,
                    'stft': 2,
                    'websocket': 3
                }
            
            self.logger.info("CPU core assignment:")
            self.logger.info(f"  Main Process (E core): {cpu_cores['main']}")
            self.logger.info(f"  TX/RX Process (P core): {cpu_cores['tx_rx']}")
            self.logger.info(f"  STFT Process (P core): {cpu_cores['stft']}")
            self.logger.info(f"  WebSocket Process (P core): {cpu_cores['websocket']}")
            
            return cpu_cores
            
        except Exception as e:
            self.logger.error(f"Error in CPU core detection: {e}")
            # Fallback to default cores
            cpu_cores = {
                'main': 0,
                'tx_rx': 1,
                'stft': 2,
                'websocket': 3
            }
            self.logger.info("Using fallback CPU core assignment")
            return cpu_cores
    
    def _set_main_process_affinity(self):
        """Set CPU affinity for the main process"""
        try:
            process = psutil.Process()
            process.cpu_affinity([self.cpu_cores['main']])
            self.logger.info(f"Successfully set main process affinity to core {self.cpu_cores['main']}")
        except Exception as e:
            self.logger.error(f"Error setting main process CPU affinity: {e}")
    
    def start_processes(self):
        """Start all child processes with process group management"""
        self.logger.info("Starting 3-process architecture...")
        
        try:
            # Create a new process group for all child processes
            # This allows us to terminate all children together if needed
            os.setpgrp()
            self.logger.info(f"Created process group: {os.getpgid(0)}")
            
            # Create TX wave for transmission
            tx_wave = self._create_tx_tone()
            
            # Process 1: TX/RX process
            self.tx_rx_process = multiprocessing.Process(
                target=self._run_tx_rx_process,
                args=(tx_wave, self.args.hop_size, self.stop_event, 
                      self.rx_queue, self.main_timestamp, self.args.device, 
                      self.args.freq, self.args.sample_rate, self.args.gain, 
                      self.cpu_cores['tx_rx'], self.log_dir)
            )
            self.tx_rx_process.start()
            self.restart_attempts['TX/RX']['process'] = self.tx_rx_process
            self.logger.info(f"Started TX/RX process on CPU core {self.cpu_cores['tx_rx']} (PID: {self.tx_rx_process.pid})")
            
            # Process 2: STFT process
            self.stft_process = multiprocessing.Process(
                target=self._run_stft_process,
                args=(self.args.sample_rate, self.args.fft_size, self.stop_event,
                      self.rx_queue, self.stft_queue, self.main_timestamp,
                      self.cpu_cores['stft'], self.log_dir)
            )
            self.stft_process.start()
            self.restart_attempts['STFT']['process'] = self.stft_process
            self.logger.info(f"Started STFT process on CPU core {self.cpu_cores['stft']} (PID: {self.stft_process.pid})")
            
            # Process 3: WebSocket process
            self.websocket_process = multiprocessing.Process(
                target=self._run_websocket_process,
                args=(self.args.websocket_port, self.stop_event, self.stft_queue,
                      self.main_timestamp, self.cpu_cores['websocket'], self.log_dir)
            )
            self.websocket_process.start()
            self.restart_attempts['WebSocket']['process'] = self.websocket_process
            self.logger.info(f"Started WebSocket process on CPU core {self.cpu_cores['websocket']} (PID: {self.websocket_process.pid})")
            
            # Store process group ID for cleanup
            self.process_group_id = os.getpgid(0)
            self.logger.info(f"All processes started successfully in process group {self.process_group_id}")
            
            # Validate that all processes started successfully
            if self._validate_process_startup():
                self.logger.info("3-process architecture started successfully. Main process monitoring child processes...")
                return True
            else:
                self.logger.error("Failed to start all processes. Cleaning up and exiting...")
                self._cleanup_processes()
                return False
                
        except Exception as e:
            self.logger.error(f"Error starting processes: {e}")
            self._cleanup_processes()
            return False
    
    def _run_tx_rx_process(self, tx_wave, hop_size, stop_event, rx_queue, timestamp, 
                           device, freq, sample_rate, gain, cpu_core, log_dir):
        """Run TX/RX process in separate process"""
        process = TXRXProcess(
            tx_wave=tx_wave,
            hop_size=hop_size,
            stop_event=stop_event,
            rx_queue=rx_queue,
            timestamp=timestamp,
            device=device,
            freq=freq,
            sample_rate=sample_rate,
            gain=gain,
            cpu_core=cpu_core,
            log_dir=log_dir
        )
        process.start()
    
    def _run_stft_process(self, sample_rate, fft_size, stop_event, rx_queue, 
                          stft_queue, timestamp, cpu_core, log_dir):
        """Run STFT process in separate process"""
        process = STFTProcess(
            sample_rate=sample_rate,
            fft_size=fft_size,
            stop_event=stop_event,
            rx_queue=rx_queue,
            stft_queue=stft_queue,
            timestamp=timestamp,
            cpu_core=cpu_core,
            log_dir=log_dir
        )
        process.start()
    
    def _run_websocket_process(self, websocket_port, stop_event, stft_queue, 
                               timestamp, cpu_core, log_dir):
        """Run WebSocket process in separate process"""
        process = WebSocketProcess(
            websocket_port=websocket_port,
            stop_event=stop_event,
            stft_queue=stft_queue,
            timestamp=timestamp,
            cpu_core=cpu_core,
            log_dir=log_dir
        )
        process.start()
    
    def _validate_process_startup(self):
        """Validate that all processes started successfully"""
        time.sleep(2)  # Give processes time to start
        
        processes = [
            ('TX/RX', self.tx_rx_process),
            ('STFT', self.stft_process),
            ('WebSocket', self.websocket_process)
        ]
        
        for name, process in processes:
            if not process or not process.is_alive():
                self.logger.error(f"Process {name} failed to start or died immediately")
                return False
            self.logger.info(f"Process {name} started successfully (PID: {process.pid})")
        
        return True
    
    def _cleanup_processes(self):
        """Clean up all processes in stages with better backend responsiveness"""
        self.logger.info("Stopping 3-process architecture...")
        
        # Stage 1: Attempt graceful termination (shorter timeout for backend)
        self.logger.info("Stage 1: Attempting graceful termination...")
        self.stop_event.set()
        
        processes_to_cleanup = [
            ("TX/RX", self.tx_rx_process),
            ("STFT", self.stft_process),
            ("WebSocket", self.websocket_process)
        ]
        
        # Shorter timeout for backend responsiveness
        graceful_timeout = 3.0  # Reduced from 5.0 seconds
        
        for name, process in processes_to_cleanup:
            if process and process.is_alive():
                self.logger.info(f"Stopping {name} process gracefully...")
                process.join(timeout=graceful_timeout)
                if process.is_alive():
                    self.logger.warning(f"{name} process did not exit gracefully within {graceful_timeout}s")
        
        # Stage 2: Force terminate remaining processes
        self.logger.info("Stage 2: Attempting terminate() for remaining processes...")
        for name, process in processes_to_cleanup:
            if process and process.is_alive():
                self.logger.info(f"Terminating {name} process...")
                process.terminate()
                # Shorter timeout for terminate
                process.join(timeout=2.0)
        
        # Stage 3: Force kill remaining processes
        self.logger.info("Stage 3: Force killing remaining processes...")
        for name, process in processes_to_cleanup:
            if process and process.is_alive():
                self.logger.info(f"Force killing {name} process...")
                process.kill()
                process.join(timeout=1.0)  # Very short timeout for kill
        
        self.logger.info("All processes successfully terminated")
    
    def _check_process_health(self):
        """Check health of all processes and restart if needed"""
        current_time = time.time() * 1000  # Convert to milliseconds
        
        processes = [
            ('TX/RX', self.tx_rx_process),
            ('STFT', self.stft_process),
            ('WebSocket', self.websocket_process)
        ]
        
        alive_processes = []
        
        for name, process in processes:
            if process and process.is_alive():
                alive_processes.append(name)
                # Reset restart counter for healthy processes
                if self.restart_attempts[name]['count'] > 0:
                    self.logger.info(f"Process {name} recovered, resetting restart counter")
                    self.restart_attempts[name]['count'] = 0
                    self.restart_attempts[name]['last_restart'] = 0
            else:
                # Process is dead, attempt restart
                if self.restart_attempts[name]['count'] < self.max_restart_attempts:
                    # Check if we're within the restart time window
                    if (current_time - self.restart_attempts[name]['last_restart']) > self.restart_time_window:
                        self.restart_attempts[name]['count'] = 0  # Reset counter if outside window
                    
                    if self.restart_attempts[name]['count'] < self.max_restart_attempts:
                        self.logger.warning(f"Process {name} has died. Attempting restart {self.restart_attempts[name]['count'] + 1}/{self.max_restart_attempts}")
                        
                        if self._restart_process(name):
                            self.restart_attempts[name]['count'] += 1
                            self.restart_attempts[name]['last_restart'] = current_time
                        else:
                            self.logger.error(f"Failed to restart {name} process")
                    else:
                        self.logger.error(f"Process {name} exceeded maximum restart attempts")
                else:
                    self.logger.error(f"Process {name} exceeded maximum restart attempts within time window")
        
        # Check if we have enough processes alive
        if len(alive_processes) < 3:
            self.logger.error(f"Critical: Only {len(alive_processes)} processes alive: {alive_processes}")
            if len(alive_processes) < 2:
                self.logger.error("Too few processes alive, shutting down service")
                return False
        
        return True
    
    def _restart_process(self, process_name):
        """Restart a specific process"""
        try:
            self.logger.info(f"Attempting to restart {process_name} process...")
            
            # Create new process based on name
            if process_name == "TX/RX":
                tx_wave = self._create_tx_tone()
                new_process = multiprocessing.Process(
                    target=self._run_tx_rx_process,
                    args=(tx_wave, self.args.hop_size, self.stop_event,
                          self.rx_queue, self.main_timestamp, self.args.device,
                          self.args.freq, self.args.sample_rate, self.args.gain,
                          self.cpu_cores['tx_rx'], self.log_dir)
                )
                new_process.start()
                self.tx_rx_process = new_process
                self.restart_attempts['TX/RX']['process'] = new_process
                
            elif process_name == "STFT":
                new_process = multiprocessing.Process(
                    target=self._run_stft_process,
                    args=(self.args.sample_rate, self.args.fft_size, self.stop_event,
                          self.rx_queue, self.stft_queue, self.main_timestamp,
                          self.cpu_cores['stft'], self.log_dir)
                )
                new_process.start()
                self.stft_process = new_process
                self.restart_attempts['STFT']['process'] = new_process
                
            elif process_name == "WebSocket":
                new_process = multiprocessing.Process(
                    target=self._run_websocket_process,
                    args=(self.args.websocket_port, self.stop_event, self.stft_queue,
                          self.main_timestamp, self.cpu_cores['websocket'], self.log_dir)
                )
                new_process.start()
                self.websocket_process = new_process
                self.restart_attempts['WebSocket']['process'] = new_process
            
            # Wait a moment for process to start
            time.sleep(1)
            
            # Check if restart was successful
            if new_process.is_alive():
                self.logger.info(f"Successfully restarted {process_name} process (PID: {new_process.pid})")
                return True
            else:
                self.logger.error(f"Failed to restart {process_name} process - process died immediately")
                return False
                
        except Exception as e:
            self.logger.error(f"Error restarting {process_name} process: {e}")
            return False
    
    def _get_queue_status(self):
        """Get status of all queues"""
        try:
            rx_size = self.rx_queue.qsize()
            stft_size = self.stft_queue.qsize()
            return rx_size, stft_size
        except Exception as e:
            self.logger.error(f"Error getting queue status: {e}")
            return 0, 0
    
    def _log_statistics(self, uptime):
        """Log system statistics"""
        try:
            rx_size, stft_size = self._get_queue_status()
            
            # Get memory usage
            process = psutil.Process()
            memory_mb = process.memory_info().rss / 1024 / 1024
            
            self.logger.info("=" * 50)
            self.logger.info("STATISTICS UPDATE (Every 5s)")
            self.logger.info("=" * 50)
            self.logger.info(f"Queue Status - RX: {rx_size}, STFT: {stft_size}")
            self.logger.info(f"Uptime: {uptime:.1f}s, Memory Usage: {memory_mb:.1f} MB")
            
            # Process status
            alive_processes = []
            for name, process in [('TX/RX', self.tx_rx_process), ('STFT', self.stft_process), ('WebSocket', self.websocket_process)]:
                if process and process.is_alive():
                    alive_processes.append(name)
            
            self.logger.info(f"Processes Alive: {', '.join(alive_processes)}")
            
            # Queue warnings
            if rx_size == 0:
                self.logger.warning("RX queue is empty - RX thread may be having issues")
            if stft_size == 0:
                self.logger.warning("STFT queue is empty - STFT process may be having issues")
            
            # Restart info
            restart_info = []
            for name, info in self.restart_attempts.items():
                if info['count'] > 0:
                    restart_info.append(f"{name}: {info['count']}/{self.max_restart_attempts}")
            
            if restart_info:
                self.logger.info(f"Restart Info: {', '.join(restart_info)}")
            
            self.logger.info("=" * 50)
            
        except Exception as e:
            self.logger.error(f"Error logging statistics: {e}")
    
    def run(self):
        """Main monitoring loop with improved status tracking"""
        try:
            # Set main process CPU affinity
            self._set_main_process_affinity()
            
            # Record start time for status tracking
            self._start_time = time.time()
            self.logger.info(f"Service start time recorded: {self._start_time}")
            
            # Start all processes
            if not self.start_processes():
                self.logger.error("Failed to start processes, exiting")
                return False
            
            # Main monitoring loop
            start_time = time.time()
            last_health_check = time.time()
            
            self.logger.info("Main monitoring loop started")
            
            while not self.stop_event.is_set():
                try:
                    # Check if launcher process is still alive
                    if not self._check_launcher_alive():
                        self.logger.warning("Launcher process died, stopping main process...")
                        break
                    
                    # Check if this process itself is orphaned (more aggressive)
                    if self._check_self_orphaned():
                        self.logger.error("CRITICAL: Process is orphaned, forcing immediate exit...")
                        break
                    
                    # Check process health
                    if not self._check_process_health():
                        self.logger.error("Critical process failure, shutting down")
                        break
                    
                    # Log statistics every 5 seconds
                    uptime = time.time() - start_time
                    if int(uptime) % 5 == 0:
                        self._log_statistics(uptime)
                    
                    # Health check for backend monitoring (every 10 seconds)
                    current_time = time.time()
                    if current_time - last_health_check >= 10.0:
                        health_status = self.is_healthy()
                        if not health_status:
                            self.logger.warning("Service health check failed")
                        last_health_check = current_time
                    
                    # Sleep for a short time
                    time.sleep(1)
                    
                except Exception as e:
                    self.logger.error(f"Error in main monitoring loop: {e}")
                    # Continue monitoring instead of breaking
                    time.sleep(1)
                    continue
            
            self.logger.info("Main monitoring loop ended")
            return True
            
        except Exception as e:
            self.logger.error(f"Critical error in main process: {e}")
            return False
        finally:
            self.logger.info("Initiating cleanup procedures...")
            self._cleanup_processes()
            self.logger.info("Cleanup procedures completed")
    
    def start(self):
        """Start the spectrogram service"""
        try:
            return self.run()
        except Exception as e:
            self.logger.error(f"Error starting spectrogram service: {e}")
            return False
    
    def stop(self):
        """Stop the spectrogram service"""
        self.logger.info("Stopping spectrogram service...")
        self.stop_event.set()
    
    def get_service_status(self):
        """Get current service status for backend monitoring"""
        try:
            status = {
                'main_process_alive': not self.stop_event.is_set(),
                'processes': {},
                'queues': {},
                'uptime': time.time() - getattr(self, '_start_time', time.time()),
                'restart_attempts': self.restart_attempts.copy()
            }
            
            # Process status
            for name, process in [('TX/RX', self.tx_rx_process), ('STFT', self.stft_process), ('WebSocket', self.websocket_process)]:
                if process:
                    status['processes'][name] = {
                        'alive': process.is_alive(),
                        'pid': process.pid if process.is_alive() else None,
                        'exitcode': process.exitcode if hasattr(process, 'exitcode') else None
                    }
                else:
                    status['processes'][name] = {'alive': False, 'pid': None, 'exitcode': None}
            
            # Queue status
            try:
                status['queues'] = {
                    'rx_size': self.rx_queue.qsize(),
                    'stft_size': self.stft_queue.qsize()
                }
            except Exception as e:
                status['queues'] = {'error': str(e)}
            
            return status
            
        except Exception as e:
            return {'error': f'Failed to get status: {e}'}
    
    def is_healthy(self):
        """Check if service is healthy for backend health checks"""
        try:
            # Check if main process is running
            if self.stop_event.is_set():
                return False
            
            # Check if all child processes are alive
            processes = [self.tx_rx_process, self.stft_process, self.websocket_process]
            alive_count = sum(1 for p in processes if p and p.is_alive())
            
            # Service is healthy if at least 2 out of 3 processes are alive
            return alive_count >= 2
            
        except Exception as e:
            self.logger.error(f"Health check failed: {e}")
            return False
    
    def _create_tx_tone(self):
        """Create TX waveform for loopback testing - EXACT same as old working code"""
        def make_tone(fs, f0, nsamps, amp=0.5):
            """
            Create a complex tone for loopback testing.
            
            Args:
                fs: Sample rate
                f0: Tone frequency
                nsamps: Number of samples
                amp: Amplitude (0.0 to 1.0)
                
            Returns:
                Complex tone array
            """
            t = np.arange(nsamps, dtype=np.float64) / fs
            amp = float(np.clip(amp, 0.0, 1.0))
            return (amp * np.exp(2j * np.pi * f0 * t)).astype(np.complex64)
        
        # Create TX waveform for loopback testing - EXACT same as old working code
        tone_freq = self.args.freq - self.args.sample_rate / 4
        tx_wave = make_tone(self.args.sample_rate, tone_freq, 10000, amp=0.3)
        self.logger.info(f"Created TX tone: {tone_freq/1e6:.1f} MHz, {len(tx_wave)} samples")
        
        return tx_wave
    
    def _check_launcher_alive(self):
        """Check if the process that launched spectrogram_main is still alive"""
        try:
            # Get parent PID (launcher)
            parent_pid = os.getppid()
            
            # Check if parent is still running
            if parent_pid == 1:  # Adopted by init - launcher died
                self.logger.warning("Launcher process died, spectrogram_main is orphaned, stopping...")
                return False
            
            # Check if parent process exists
            try:
                os.kill(parent_pid, 0)  # Signal 0 just checks if process exists
                return True
            except OSError:
                self.logger.warning("Launcher process no longer exists, stopping...")
                return False
                
        except Exception as e:
            self.logger.error(f"Error checking launcher process: {e}")
            return False
    
    def _check_self_orphaned(self):
        """Check if this process itself is orphaned (more aggressive check)"""
        try:
            # Get parent PID
            parent_pid = os.getppid()
            
            # If PPID is 1, we're orphaned
            if parent_pid == 1:
                self.logger.error("CRITICAL: This process is orphaned (PPID=1), forcing immediate exit...")
                return True
            
            # Check if parent process exists and is not init
            try:
                os.kill(parent_pid, 0)
                return False  # Parent exists
            except OSError:
                self.logger.error("CRITICAL: Parent process no longer exists, forcing immediate exit...")
                return True
            
        except Exception as e:
            self.logger.error(f"Error in orphaned check: {e}")
            return False

def main():
    """Main entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Spectrogram Service')
    parser.add_argument('--device', default='', help='USRP device string')
    parser.add_argument('--freq', type=float, default=2.4e9, help='Center frequency')
    parser.add_argument('--sample_rate', type=float, default=1e6, help='Sample rate')
    parser.add_argument('--gain', type=float, default=37.5, help='RX gain')
    parser.add_argument('--fft_size', type=int, default=1024, help='FFT size')
    parser.add_argument('--hop_size', type=int, default=1000, help='Hop size')
    parser.add_argument('--websocket-port', type=int, default=40001, help='WebSocket port')
    
    # Additional parameters for backward compatibility
    parser.add_argument('--sample-rate', type=float, help='Sample rate (alternative format)')
    parser.add_argument('--fft-size', type=int, help='FFT size (alternative format)')
    parser.add_argument('--hop-size', type=int, help='Hop size (alternative format)')
    parser.add_argument('--resolution', type=int, help='Resolution (legacy parameter)')
    parser.add_argument('--window_size', type=int, help='Window size (legacy parameter)')
    parser.add_argument('--window_type', type=str, help='Window type (legacy parameter)')
    
    args = parser.parse_args()
    
    # Handle argument mapping and legacy parameters
    def normalize_args(args):
        """Normalize arguments for backward compatibility"""
        # Handle legacy parameter names
        if args.sample_rate is None and hasattr(args, 'sample-rate') and args.sample_rate is not None:
            args.sample_rate = args.sample_rate
        if args.fft_size is None and hasattr(args, 'fft-size') and args.fft_size is not None:
            args.fft_size = args.fft_size
        if args.hop_size is None and hasattr(args, 'hop-size') and args.hop_size is not None:
            args.hop_size = args.hop_size
            
        # Handle legacy resolution parameter (map to hop_size if not set)
        if args.resolution and args.hop_size == 1000:  # Only if hop_size is still default
            args.hop_size = args.resolution
            
        # Handle legacy window_size parameter (map to fft_size if not set)
        if args.window_size and args.fft_size == 1024:  # Only if fft_size is still default
            args.fft_size = args.window_size
            
        # Window type is not currently used but accepted for compatibility
        if args.window_type:
            print(f"Note: Window type '{args.window_type}' is accepted but not currently implemented")
            
        return args
    
    # Normalize arguments
    args = normalize_args(args)
    
    # Create and start service
    service = SpectrogramMain(args)
    
    try:
        if service.start():
            print("Spectrogram service completed successfully")
        else:
            print("Spectrogram service failed")
            sys.exit(1)
            
    except KeyboardInterrupt:
        print("Received keyboard interrupt")
    finally:
        service.stop()

if __name__ == "__main__":
    main()
