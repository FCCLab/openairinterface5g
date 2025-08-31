import numpy as np
import uhd
import time
from scipy import signal
import asyncio
import websockets
import multiprocessing
import json
import logging
import os
import sys
import signal as signal_module
from datetime import datetime
import queue

# Global variables for 4-process architecture
websocket_clients = set()
websocket_server = None
stop_event = None

# Process-specific queues and data
tx_queue = None          # Queue for TX data
rx_queue = None          # Queue for RX data  
process_queue = None     # Queue for processed data
websocket_queue = None   # Queue for WebSocket broadcasting

# Process objects
tx_process = None
rx_process = None
process_process = None
websocket_process = None

# CPU cores for each process (last 4 cores)
TX_CPU_CORE = None       # Will be set to last core - 3
RX_CPU_CORE = None       # Will be set to last core - 2  
PROCESS_CPU_CORE = None  # Will be set to last core - 1
WEBSOCKET_CPU_CORE = None # Will be set to last core

# CPU core assignments for 3-process architecture
TX_RX_CPU_CORE = 10      # Combined TX/RX process
IQ_PROCESS_CPU_CORE = 11 # IQ processing process  
WEBSOCKET_CPU_CORE = 12  # WebSocket broadcasting process

def set_cpu_affinity():
    """
    Set CPU affinity to the last available core for better performance.
    """
    try:
        import psutil
        # Get the last CPU core (0-indexed)
        last_core = psutil.cpu_count() - 1
        current_process = psutil.Process()
        current_process.cpu_affinity([last_core])
        logging.info(f"Set CPU affinity to core {last_core}")
        
        # Try to set high priority (requires root privileges)
        try:
            current_process.nice(psutil.HIGH_PRIORITY_CLASS)
            logging.info("Set process to high priority")
        except (PermissionError, AttributeError):
            logging.info("Could not set high priority (requires root privileges)")
        
        return True
    except ImportError:
        logging.warning("psutil not available, cannot set CPU affinity")
        return False
    except Exception as e:
        logging.warning(f"Failed to set CPU affinity: {e}")
        return False

def setup_cpu_cores():
    """
    Setup CPU cores for the 3-process architecture.
    """
    global TX_RX_CPU_CORE, IQ_PROCESS_CPU_CORE, WEBSOCKET_CPU_CORE
    
    try:
        import psutil
        total_cores = psutil.cpu_count()
        
        # Assign cores for 3-process architecture
        TX_RX_CPU_CORE = 10        # Combined TX/RX process
        IQ_PROCESS_CPU_CORE = 11   # IQ processing process
        WEBSOCKET_CPU_CORE = 12    # WebSocket broadcasting process
        
        logging.info(f"CPU core assignment for 3-process architecture:")
        logging.info(f"  TX/RX Process: Core {TX_RX_CPU_CORE}")
        logging.info(f"  IQ Process: Core {IQ_PROCESS_CPU_CORE}")
        logging.info(f"  WebSocket Process: Core {WEBSOCKET_CPU_CORE}")
        
        # Set initial process affinity to all cores
        process = psutil.Process(os.getpid())
        all_cores = list(range(total_cores))
        process.cpu_affinity(all_cores)
        logging.info(f"Initial process affinity set to all cores: {all_cores}")
        
        return True
    except ImportError:
        logging.warning("psutil not available, using default core assignment")
        TX_RX_CPU_CORE = 10
        IQ_PROCESS_CPU_CORE = 11
        WEBSOCKET_CPU_CORE = 12
        return False
    except Exception as e:
        logging.warning(f"Failed to setup CPU cores: {e}")
        return False

def set_process_cpu_affinity(process_name, core_id):
    """
    Set CPU affinity for a specific process.
    """
    try:
        import psutil
        import os
        
        # Get the current process
        process = psutil.Process(os.getpid())
        
        # Set affinity for the entire process
        process.cpu_affinity([core_id])
        
        # Verify the affinity was set
        current_affinity = process.cpu_affinity()
        if core_id in current_affinity:
            logging.info(f"Successfully set {process_name} process affinity to core {core_id}")
            return True
        else:
            logging.warning(f"Failed to verify {process_name} process affinity to core {core_id}")
            return False
            
    except Exception as e:
        logging.warning(f"Failed to set {process_name} process affinity: {e}")
        return False

def verify_process_affinity():
    """
    Verify that processes are running on their assigned cores.
    """
    try:
        import psutil
        import os
        
        process = psutil.Process(os.getpid())
        current_affinity = process.cpu_affinity()
        
        logging.info(f"Current process affinity: {current_affinity}")
        logging.info(f"Process can run on cores: {current_affinity}")
        
        # Log the 3-process architecture core assignments
        logging.info(f"3-process architecture core assignments:")
        logging.info(f"  TX/RX Process: Core {TX_RX_CPU_CORE}")
        logging.info(f"  IQ Process: Core {IQ_PROCESS_CPU_CORE}")
        logging.info(f"  WebSocket Process: Core {WEBSOCKET_CPU_CORE}")
        
        return True
    except Exception as e:
        logging.warning(f"Failed to verify process affinity: {e}")
        return False

def generate_child_process_log_filename(process_name, timestamp):
    """
    Generate a timestamped log filename for child processes using the main process timestamp.
    
    Args:
        process_name: Name of the child process (e.g., 'tx', 'rx', 'process', 'websocket')
        timestamp: Timestamp from the main process (format: YYYYMMDD_HHMMSS)
        
    Returns:
        Log filename in format: spectrogram_YYYYMMDD_HHMMSS_process_name.log
    """
    return f"spectrogram_{timestamp}_{process_name}.log"

def setup_process_logging(log_file_path):
    """
    Setup logging for a child process with both file and console output.
    """
    try:
        # Make sure the log file path is absolute
        if not os.path.isabs(log_file_path):
            # Get the directory where the spectrogram script is located
            script_dir = os.path.dirname(os.path.abspath(__file__))
            log_file_path = os.path.join(script_dir, log_file_path)
        
        # Create the log directory if it doesn't exist
        log_dir = os.path.dirname(log_file_path)
        os.makedirs(log_dir, exist_ok=True)
        
        # Create formatter
        formatter = logging.Formatter(
            '%(asctime)s - %(name)s - %(levelname)s - %(message)s'
        )
        
        # Setup file handler
        file_handler = logging.FileHandler(log_file_path)
        file_handler.setLevel(logging.DEBUG)
        file_handler.setFormatter(formatter)
        
        # Setup console handler
        console_handler = logging.StreamHandler()
        console_handler.setLevel(logging.INFO)
        console_handler.setFormatter(formatter)
        
        # Setup root logger
        root_logger = logging.getLogger()
        root_logger.setLevel(logging.DEBUG)
        root_logger.addHandler(file_handler)
        root_logger.addHandler(console_handler)
        
        logging.info(f"Process logging setup complete: {log_file_path}")
        logging.info(f"Current working directory: {os.getcwd()}")
        logging.info(f"Script directory: {os.path.dirname(os.path.abspath(__file__))}")
        return True
    except Exception as e:
        print(f"Failed to setup process logging: {e}")
        return False

def signal_handler(signum, frame):
    """
    Handle termination signals to ensure proper cleanup.
    """
    logging.info(f"Received signal {signum}, initiating graceful shutdown...")
    
    # Set the stop event to signal all processes and threads to stop
    global stop_event, tx_process, rx_process, process_process, websocket_process
    if stop_event:
        stop_event.set()
    
    # Force kill all processes immediately
    def force_kill_processes():
        time.sleep(1)  # Give processes 1 second to terminate gracefully
        logging.info("Force killing all processes...")
        
        processes_to_kill = []
        if tx_process and tx_process.is_alive():
            processes_to_kill.append(("TX", tx_process))
        if rx_process and rx_process.is_alive():
            processes_to_kill.append(("RX", rx_process))
        if process_process and process_process.is_alive():
            processes_to_kill.append(("Process", process_process))
        if websocket_process and websocket_process.is_alive():
            processes_to_kill.append(("WebSocket", websocket_process))
        
        for name, process in processes_to_kill:
            try:
                logging.warning(f"Force killing {name} process (PID: {process.pid})")
                process.kill()
                process.join(timeout=1)
            except Exception as e:
                logging.error(f"Error killing {name} process: {e}")
        
        # Clean up any orphaned processes
        cleanup_orphaned_processes()
        
        # Force exit after killing processes
        time.sleep(1)
        logging.info("Force exiting main process...")
        os._exit(0)
    
    # Start force kill timer
    import threading
    kill_thread = threading.Thread(target=force_kill_processes)
    kill_thread.daemon = True
    kill_thread.start()

def cleanup_orphaned_processes():
    """
    Clean up any orphaned spectrogram processes that might still be running.
    """
    try:
        import subprocess
        import psutil
        
        # Find all Python processes that might be spectrogram-related
        current_pid = os.getpid()
        orphaned_pids = []
        
        for proc in psutil.process_iter(['pid', 'name', 'cmdline']):
            try:
                if (proc.info['name'] == 'python3' and 
                    proc.info['pid'] != current_pid and
                    proc.info['cmdline'] and
                    any('spectrogram' in cmd for cmd in proc.info['cmdline'])):
                    orphaned_pids.append(proc.info['pid'])
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                continue
        
        if orphaned_pids:
            logging.warning(f"Found {len(orphaned_pids)} orphaned spectrogram processes: {orphaned_pids}")
            for pid in orphaned_pids:
                try:
                    logging.warning(f"Killing orphaned process PID: {pid}")
                    subprocess.run(['kill', '-9', str(pid)], timeout=5)
                except Exception as e:
                    logging.error(f"Failed to kill orphaned process {pid}: {e}")
        else:
            logging.info("No orphaned spectrogram processes found")
            
    except Exception as e:
        logging.error(f"Error during orphaned process cleanup: {e}")

def setup_logging():
    """
    Setup logging to stdout, stderr, and file.
    """
    # Create logs directory if it doesn't exist
    log_dir = os.path.join(os.path.dirname(__file__), 'logs')
    os.makedirs(log_dir, exist_ok=True)
    
    # Create log filename with timestamp
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    log_file = os.path.join(log_dir, f'spectrogram_{timestamp}.log')
    
    # Configure logging
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(levelname)s - %(message)s',
        handlers=[
            logging.FileHandler(log_file),
            logging.StreamHandler(sys.stdout),  # stdout
        ]
    )
    
    print(f"00 spectrogram.py: Logging to file: {log_file}")
    return log_file

def setup_tx_for_loopback(usrp, center_freq, sample_rate, fft_size):
    """
    Setup TX for loopback testing.
    
    Args:
        usrp: USRP device object
        center_freq: Center frequency
        sample_rate: Sample rate
        fft_size: FFT size for frequency calculations
        
    Returns:
        tuple: (tx_streamer, tx_wave) or (None, None) on error
    """
    try:
        # Configure TX parameters
        usrp.set_tx_rate(sample_rate, 0)
        usrp.set_tx_freq(center_freq, 0)
        usrp.set_tx_gain(10, 0)  # Moderate TX gain for loopback
        
        # Setup TX streamer
        tx_st_args = uhd.usrp.StreamArgs("fc32", "sc16")
        tx_st_args.channels = [0]
        tx_streamer = usrp.get_tx_stream(tx_st_args)
        
        # Create test tone at freq + sample_rate/4 for loopback
        tone_freq = center_freq - sample_rate / 4
        tx_wave = make_tone(sample_rate, tone_freq, 10000, amp=0.3)
        logging.info(f"Created TX tone: {tone_freq/1e6:.1f} MHz (freq + sample_rate/4), {len(tx_wave)} samples")
        
        # Calculate expected frequency bin for the tone
        freq_resolution = sample_rate / fft_size
        tone_offset = sample_rate / 4  # 250 kHz
        expected_bin = int(tone_offset / freq_resolution)
        logging.info(f"Expected tone at frequency bin {expected_bin} (offset {tone_offset/1e3:.1f} kHz, resolution {freq_resolution/1e3:.1f} kHz)")
        
        return tx_streamer, tx_wave
        
    except Exception as e:
        logging.error(f"TX setup failed: {e}")
        return None, None

def setup_usrp_device(device_addr, center_freq, sample_rate, gain, fft_size=None):
    """
    Setup USRP device and return device objects.
    
    Args:
        device_addr: USRP device address
        center_freq: Center frequency
        sample_rate: Sample rate
        gain: Gain in dB
        fft_size: FFT size for TX tone calculations (optional)
        
    Returns:
        tuple: (usrp, rx_streamer, tx_streamer, tx_wave) or (None, None, None, None) on error
    """
    try:
        logging.info(f"Setting up USRP device: '{device_addr}'")
        usrp = uhd.usrp.MultiUSRP(device_addr) if device_addr else uhd.usrp.MultiUSRP()
        logging.info("Successfully connected to USRP device")
        
        # Print device information
        logging.info(f"USRP device: {usrp.get_pp_string()}")
        logging.info(f"Device time: {usrp.get_time_now()}")
        
        # Set parameters (specify channel 0)
        usrp.set_rx_rate(sample_rate, 0)
        usrp.set_rx_freq(center_freq, 0)
        usrp.set_rx_gain(gain, 0)

        usrp.set_clock_source("internal")
        usrp.set_time_source("internal")
        # usrp.set_rx_antenna("RX2", 0)   # or "TX/RX" depending on your model
        # logging.info(usrp.get_rx_antenna(0))

        # Verify settings
        logging.info(f"Sample rate: {usrp.get_rx_rate(0)}")
        logging.info(f"Center frequency: {usrp.get_rx_freq(0)}")
        logging.info(f"Gain: {usrp.get_rx_gain(0)}")
        
        # Allow for tuning/settling
        time.sleep(0.5)
        
        # Setup RX streamer
        st_args = uhd.usrp.StreamArgs("fc32", "sc16")
        st_args.channels = [0]
        rx_streamer = usrp.get_rx_stream(st_args)

        # Add device status and sensor information
        try:
            logging.info(f"Device sensors: {usrp.get_rx_sensor_names(0)}")
            for sensor_name in usrp.get_rx_sensor_names(0):
                try:
                    sensor_value = usrp.get_rx_sensor(sensor_name, 0)
                    logging.info(f"  {sensor_name}: {sensor_value}")
                except Exception as e:
                    logging.warning(f"  Could not read sensor {sensor_name}: {e}")
        except Exception as e:
            logging.warning(f"Could not read device sensors: {e}")
        

        
        logging.info("USRP device setup complete")
        
        # Setup TX for loopback testing
        tx_streamer, tx_wave = setup_tx_for_loopback(usrp, center_freq, sample_rate, fft_size)
        if tx_streamer is None or tx_wave is None:
            logging.error("Failed to setup TX for loopback testing")
            return None, None, None, None
        
        return usrp, rx_streamer, tx_streamer, tx_wave
        
    except Exception as e:
        error_msg = f"Failed to setup USRP device '{device_addr}'={str(e)}"
        logging.error(error_msg)
        return None, None, None, None

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

def cleanup_usrp_device(usrp, rx_streamer):
    """
    Cleanup USRP device resources.
    
    Args:
        usrp: USRP device object
        rx_streamer: USRP RX streamer object
    """
    try:
        logging.info("Cleaning up USRP resources...")
        if rx_streamer is not None:
            rx_streamer.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont))
        if usrp is not None:
            del usrp
        logging.info("USRP cleanup complete")
    except Exception as e:
        logging.error(f"Error during cleanup: {str(e)}")

async def websocket_handler(websocket):
    """
    Handle WebSocket connections.
    
    Args:
        websocket: WebSocket connection object
    """
    global websocket_clients
    logging.info(f"WebSocket client connected: {websocket.remote_address}")
    websocket_clients.add(websocket)
    logging.info(f"Total WebSocket clients: {len(websocket_clients)}")
    
    try:
        async for message in websocket:
            # Handle any incoming messages if needed
            logging.info(f"Received WebSocket message: {message}")
    except websockets.exceptions.ConnectionClosed:
        logging.info(f"WebSocket client disconnected: {websocket.remote_address}")
    finally:
        websocket_clients.discard(websocket)
        logging.info(f"Total WebSocket clients: {len(websocket_clients)}")

async def start_websocket_server(port=40001):
    """
    Start WebSocket server.
    
    Args:
        port: WebSocket server port
    """
    global websocket_server
    logging.info(f"Starting WebSocket server on port {port}")
    websocket_server = await websockets.serve(websocket_handler, "localhost", port)
    logging.info(f"WebSocket server started on ws://localhost:{port}")
    await websocket_server.wait_closed()

def start_websocket_server_direct(port=40001):
    """
    Start WebSocket server directly (for use in multiprocessing).
    
    Args:
        port: WebSocket server port
    """
    try:
        asyncio.run(start_websocket_server(port))
    except Exception as e:
        logging.error(f"WebSocket server error: {e}")

def tx_rx_process_function(tx_wave, hop_size, stop_event, rx_queue, timestamp, device_addr, center_freq, sample_rate, gain):
    """
    Process 1: Combined TX/RX process function that handles both transmission and reception.
    Dedicated to CPU core TX_RX_CPU_CORE.
    
    Args:
        tx_wave: TX waveform to transmit
        hop_size: Number of samples to collect per hop
        stop_event: Multiprocessing event to signal stop
        rx_queue: Multiprocessing queue for RX data
        timestamp: Main process timestamp for log filename
        device_addr: USRP device address
        center_freq: Center frequency
        sample_rate: Sample rate
        gain: RX gain
    """
    # Setup logging for this process
    log_filename = generate_child_process_log_filename("tx_rx", timestamp)
    setup_process_logging(f"logs/{log_filename}")
    
    # Setup signal handlers for this process
    signal_module.signal(signal_module.SIGINT, lambda s, f: None)   # Ignore SIGINT in child
    signal_module.signal(signal_module.SIGTERM, lambda s, f: stop_event.set())  # Forward SIGTERM
    
    # Set CPU affinity for this process
    set_process_cpu_affinity("TX_RX", TX_RX_CPU_CORE)
    
    logging.info(f"Starting TX/RX process on CPU core {TX_RX_CPU_CORE}...")
    
    try:
        # Setup USRP device
        logging.info(f"Setting up USRP device: '{device_addr}'")
        
        # Ensure proper device address format
        if device_addr:
            if not device_addr.startswith("addr="):
                device_addr = f"addr={device_addr}"
        else:
            device_addr = ""  # Auto-detect
        
        logging.info(f"Using device address: '{device_addr}'")
        if device_addr:
            usrp = uhd.usrp.MultiUSRP(device_addr)
        else:
            usrp = uhd.usrp.MultiUSRP()
        logging.info("Successfully connected to USRP device")
        logging.info(f"USRP device: {usrp.get_pp_string()}")
        
        # Setup TX streamer
        usrp.set_tx_rate(sample_rate, 0)
        usrp.set_tx_freq(center_freq, 0)
        usrp.set_tx_gain(10, 0)  # Moderate TX gain for loopback
        
        tx_st_args = uhd.usrp.StreamArgs("fc32", "sc16")
        tx_st_args.channels = [0]
        tx_streamer = usrp.get_tx_stream(tx_st_args)
        logging.info("TX streamer setup complete")
        
        # Setup RX streamer
        usrp.set_rx_rate(sample_rate, 0)
        usrp.set_rx_freq(center_freq, 0)
        usrp.set_rx_gain(gain, 0)
        
        rx_st_args = uhd.usrp.StreamArgs("fc32", "sc16")
        rx_st_args.channels = [0]
        rx_streamer = usrp.get_rx_stream(rx_st_args)
        logging.info("RX streamer setup complete")
        
        # Start TX and RX threads
        import threading
        
        # TX thread
        def tx_thread():
            tx_md = uhd.types.TXMetadata()
            samps_sent = 0
            max_tx = tx_streamer.get_max_num_samps()
            
            logging.info(f"TX thread: max_tx={max_tx}, tx_wave length={len(tx_wave)}")
            
            while not stop_event.is_set():
                try:
                    # Send the waveform in chunks
                    chunk = tx_wave[samps_sent % len(tx_wave):(samps_sent % len(tx_wave)) + max_tx]
                    if len(chunk) < max_tx:
                        # Wrap around to beginning of waveform
                        remaining = max_tx - len(chunk)
                        chunk = np.concatenate([chunk, tx_wave[:remaining]])
                    
                    sent = tx_streamer.send(chunk, tx_md)
                    samps_sent += sent
                    
                    # Log progress occasionally
                    if samps_sent % 100000 == 0:
                        logging.info(f"TX thread: sent {samps_sent} samples")
                    
                    # Small delay to prevent overwhelming the system
                    time.sleep(0.001)
                    
                except Exception as e:
                    logging.error(f"Error in TX thread: {e}")
                    break
        
        # RX thread
        def rx_thread():
            # Start continuous streaming
            rx_streamer.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.start_cont))
            logging.info("RX thread: Started continuous streaming")
            
            rx_md = uhd.types.RXMetadata()
            frame_count = 0
            
            while not stop_event.is_set():
                try:
                    # Collect samples
                    samples = np.zeros(hop_size, dtype=np.complex64)
                    num_samps = rx_streamer.recv(samples, rx_md)
                    
                    if num_samps > 0:
                        # Put samples into RX queue for processing
                        rx_queue.put((frame_count, samples[:num_samps], time.time()))
                        frame_count += 1
                        
                        # Log progress occasionally
                        if frame_count % 1000 == 0:
                            logging.info(f"RX thread: collected {frame_count} frames")
                        
                except Exception as e:
                    error_msg = str(e)
                    if "OpTimeout" in error_msg or "RfnocError" in error_msg:
                        logging.warning(f"RX thread: USRP timeout/communication error: {e}")
                        logging.warning("RX thread: Attempting to recover by restarting streaming...")
                        try:
                            # Restart continuous streaming
                            rx_streamer.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont))
                            time.sleep(0.1)  # Brief pause
                            rx_streamer.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.start_cont))
                            logging.info("RX thread: Streaming restarted successfully")
                        except Exception as restart_error:
                            logging.error(f"RX thread: Failed to restart streaming: {restart_error}")
                            time.sleep(1.0)  # Wait longer before retrying
                    else:
                        logging.error(f"RX thread: Error receiving samples: {e}")
                    # Continue instead of breaking to keep the thread alive
                    continue
        
        # Start TX and RX threads
        tx_thread_obj = threading.Thread(target=tx_thread)
        rx_thread_obj = threading.Thread(target=rx_thread)
        tx_thread_obj.daemon = True
        rx_thread_obj.daemon = True
        tx_thread_obj.start()
        rx_thread_obj.start()
        logging.info("TX and RX threads started successfully")
        
        # Wait for stop event
        while not stop_event.is_set():
            time.sleep(1)
            
    except Exception as e:
        logging.error(f"Error in TX/RX process: {e}")
    finally:
        logging.info("Stopping TX/RX process...")

def iq_process_function(sample_rate, fft_size, stop_event, rx_queue, process_queue, timestamp):
    """
    Process 2: IQ processing function that performs STFT on received IQ data.
    Dedicated to CPU core IQ_PROCESS_CPU_CORE.
    
    Args:
        sample_rate: Sample rate
        fft_size: FFT size
        stop_event: Multiprocessing event to signal stop
        rx_queue: Multiprocessing queue for RX data
        process_queue: Multiprocessing queue for processed data
        timestamp: Main process timestamp for log filename
    """
    # Setup logging for this process
    log_filename = generate_child_process_log_filename("iq_process", timestamp)
    setup_process_logging(f"logs/{log_filename}")
    
    # Setup signal handlers for this process
    signal_module.signal(signal_module.SIGINT, lambda s, f: None)   # Ignore SIGINT in child
    signal_module.signal(signal_module.SIGTERM, lambda s, f: stop_event.set())  # Forward SIGTERM
    
    # Set CPU affinity for this process
    set_process_cpu_affinity("IQ_Process", IQ_PROCESS_CPU_CORE)
    
    logging.info(f"Starting IQ process on CPU core {IQ_PROCESS_CPU_CORE}...")
    
    try:
        frame_count = 0
        logging.info("IQ process: Starting main processing loop")
        
        while not stop_event.is_set():
            try:
                # Get IQ data from RX queue
                logging.debug("IQ process: Waiting for data from RX queue...")
                frame_data = rx_queue.get(timeout=1.0)  # 1 second timeout
                frame_num, samples, timestamp = frame_data
                logging.debug(f"IQ process: Received frame {frame_num} with {len(samples)} samples")
                
                # Perform STFT processing
                window_size = fft_size  # Use FFT size as window size
                overlap_samples = window_size // 4  # 25% overlap instead of 50%
                
                # STFT calculation
                f, t, Zxx = signal.stft(
                    samples, 
                    fs=sample_rate, 
                    nperseg=window_size, 
                    noverlap=overlap_samples,
                    nfft=fft_size,
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
                
                # Put processed data into process queue
                process_queue.put((frame_num, f, t, magnitude_db, timestamp))
                frame_count += 1
                logging.debug(f"IQ process: Processed frame {frame_num}, total processed: {frame_count}")
                
                # Log progress occasionally
                if frame_count % 1000 == 0:
                    logging.info(f"IQ process: processed {frame_count} frames")
                
            except queue.Empty:
                # No data available, continue
                logging.debug("IQ process: No data available in RX queue, continuing...")
                continue
            except Exception as e:
                logging.error(f"Error in IQ process: {e}")
                continue
                
    except Exception as e:
        logging.error(f"Error in IQ process: {e}")
    finally:
        logging.info("Stopping IQ process...")

def websocket_process_function(websocket_port, stop_event, process_queue, timestamp):
    """
    Process 3: WebSocket process function that broadcasts processed spectrogram data.
    Dedicated to CPU core WEBSOCKET_CPU_CORE.
    
    Args:
        websocket_port: WebSocket server port
        stop_event: Multiprocessing event to signal stop
        process_queue: Multiprocessing queue for processed data
        timestamp: Main process timestamp for log filename
    """
    # Setup logging for this process
    log_filename = generate_child_process_log_filename("websocket", timestamp)
    setup_process_logging(f"logs/{log_filename}")
    
    # Setup signal handlers for this process
    signal_module.signal(signal_module.SIGINT, lambda s, f: None)   # Ignore SIGINT in child
    signal_module.signal(signal_module.SIGTERM, lambda s, f: stop_event.set())  # Forward SIGTERM
    
    # Set CPU affinity for this process
    set_process_cpu_affinity("WebSocket", WEBSOCKET_CPU_CORE)
    
    logging.info(f"Starting WebSocket process on CPU core {WEBSOCKET_CPU_CORE}...")
    
    try:
        # Start WebSocket server in a separate thread
        import threading
        
        def websocket_server_thread():
            try:
                start_websocket_server_direct(websocket_port)
            except Exception as e:
                logging.error(f"WebSocket server thread error: {e}")
        
        # Start WebSocket server thread
        server_thread = threading.Thread(target=websocket_server_thread)
        server_thread.daemon = True
        server_thread.start()
        
        # Wait a moment for server to start
        time.sleep(1)
        
        # Broadcast loop
        frame_count = 0
        
        while not stop_event.is_set():
            try:
                # Get processed data from process queue
                frame_data = process_queue.get(timeout=1.0)  # 1 second timeout
                frame_num, f, t, magnitude_db, timestamp = frame_data
                
                # Prepare data for WebSocket broadcast
                spectrogram_data = {
                    'frame_num': frame_num,
                    'frequencies': f.tolist(),
                    'times': t.tolist(),
                    'magnitude_db': magnitude_db.tolist(),
                    'timestamp': timestamp
                }
                
                # Broadcast to all connected clients
                broadcast_spectrogram_data(spectrogram_data)
                frame_count += 1
                
                # Log progress occasionally
                if frame_count % 10000 == 0:  # Log every 10000 frames
                    logging.info(f"WebSocket process: broadcast {frame_count} frames")
                
            except queue.Empty:
                # No data available, continue
                continue
            except Exception as e:
                logging.error(f"Error in WebSocket process: {e}")
                continue
                
    except Exception as e:
        logging.error(f"Error in WebSocket process: {e}")
    finally:
        logging.info("Stopping WebSocket process...")

def broadcast_spectrogram_data(data):
    """
    Broadcast spectrogram data to all connected WebSocket clients.
    
    Args:
        data: Dictionary containing spectrogram data
    """
    global websocket_clients
    
    if not websocket_clients:
        return
    
    try:
        # Check for NaN values in magnitude data before sending
        if np.any(np.isnan(data['magnitude_db'])):
            logging.error(f"Frame {data['frame_num']}: NaN values in magnitude_db before sending!")
            logging.error(f"  Magnitude shape: {data['magnitude_db'].shape}")
            logging.error(f"  NaN count: {np.sum(np.isnan(data['magnitude_db']))}")
            # Replace NaN with -120 dB
            data['magnitude_db'] = np.nan_to_num(data['magnitude_db'], nan=-120.0)
        
        # Prepare data for transmission
        json_data = json.dumps(data)
        
        # Broadcast to all connected clients
        for client in websocket_clients.copy():
            try:
                # Get the event loop from the websocket server
                if websocket_server and websocket_server.is_serving():
                    loop = websocket_server.loop
                    asyncio.run_coroutine_threadsafe(
                        client.send(json_data), 
                        loop
                    )
                else:
                    # Fallback: try to get current event loop
                    try:
                        loop = asyncio.get_event_loop()
                        asyncio.run_coroutine_threadsafe(
                            client.send(json_data), 
                            loop
                        )
                    except RuntimeError:
                        logging.warning(f"No event loop available for client {client.remote_address}")
                        websocket_clients.discard(client)
            except Exception as e:
                logging.error(f"Error sending to WebSocket client: {e}")
                websocket_clients.discard(client)
                
    except Exception as e:
        logging.error(f"Error broadcasting spectrogram data: {e}")

def data_collection_thread(usrp, rx_streamer, hop_size, stop_event, data_queue):
    """
    Thread function for continuous data collection from USRP.
    
    Args:
        usrp: USRP device object
        rx_streamer: USRP RX streamer object
        hop_size: Number of samples to collect per frame
        stop_event: Multiprocessing event to signal stop
        data_queue: Queue to store collected data
    """
    logging.info("Starting data collection thread...")
    
    try:
        # Start RX streaming
        logging.info("Starting RX streaming in data collection thread...")
        rx_streamer.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.start_cont))
        time.sleep(0.2)  # Allow streaming to start
        logging.info("RX streaming started successfully")
        
        frame_count = 0
        
        while not stop_event.is_set():
            try:
                # Collect samples
                result = collect_samples(usrp, rx_streamer, hop_size)
                
                if result['success']:
                    frame_count += 1
                    
                    # Debug first few frames
                    if frame_count <= 3:
                        samples = result['samples']
                        logging.info(f"Frame {frame_count} - First 5 samples: {samples[:5]}")
                        logging.info(f"Frame {frame_count} - Magnitude range: [{np.min(np.abs(samples)):.6f}, {np.max(np.abs(samples)):.6f}]")
                        logging.info(f"Frame {frame_count} - Average magnitude: {np.mean(np.abs(samples)):.6f}")
                    
                    data_queue.put({
                        'frame': frame_count,
                        'samples': result['samples'],
                        'timestamp': time.time(),
                        'success': True
                    })
                else:
                    logging.warning(f"Frame {frame_count} failed: {result['error']}")
                    data_queue.put({
                        'frame': frame_count,
                        'error': result['error'],
                        'timestamp': time.time(),
                        'success': False
                    })
                    
            except Exception as e:
                logging.error(f"Error in data collection thread: {e}")
                time.sleep(0.1)
                
    except Exception as e:
        logging.error(f"Fatal error in data collection thread: {e}")
    finally:
        logging.info("Stopping data collection thread...")
        try:
            rx_streamer.issue_stream_cmd(uhd.types.StreamCMD(uhd.types.StreamMode.stop_cont))
        except:
            pass

def collect_samples(usrp, rx_streamer, num_samples):
    """
    Collect samples from USRP device.
    
    Args:
        usrp: USRP device object
        rx_streamer: USRP RX streamer object
        num_samples: Number of samples to collect
        
    Returns:
        dict: Collection result with success status
    """
    try:
        md = uhd.types.RXMetadata()
        buffer_samps = min(num_samples, rx_streamer.get_max_num_samps())
        
        samples = np.zeros(num_samples, dtype=np.complex64)
        samples_collected = 0
        
        while samples_collected < num_samples:
            num_to_recv = min(buffer_samps, num_samples - samples_collected)
            recv_buffer = np.zeros((num_to_recv,), dtype=np.complex64)
            n = rx_streamer.recv(recv_buffer, md, timeout=5.0)
            
            # Log debugging information occasionally
            if hasattr(collect_samples, 'debug_count'):
                collect_samples.debug_count += 1
            else:
                collect_samples.debug_count = 1
                
            if collect_samples.debug_count % 1000 == 0:  # Log every 1000th collection (reduced frequency)
                logging.info(f"RX Debug - Samples received: {n}, Error code: {md.error_code}, Time: {md.time_spec}")
            
            # Check for RX errors
            if md.error_code != uhd.types.RXMetadataErrorCode.none:
                if md.error_code == uhd.types.RXMetadataErrorCode.timeout:
                    if samples_collected > 0:
                        break
                    else:
                        return {
                            'success': False,
                            'error': f"RX timeout: No data received from device",
                            'samples': None
                        }
                elif md.error_code == uhd.types.RXMetadataErrorCode.overflow:
                    logging.warning(f"RX overflow detected, clearing buffer and retrying...")
                    # Clear the buffer by reading more samples
                    clear_buffer = np.zeros((1000,), dtype=np.complex64)
                    try:
                        rx_streamer.recv(clear_buffer, md, timeout=0.1)
                    except:
                        pass
                    return {
                        'success': False,
                        'error': f"RX overflow: Buffer cleared, retry on next frame",
                        'samples': None
                    }
                else:
                    return {
                        'success': False,
                        'error': f"RX error: {md.strerror()}",
                        'samples': None
                    }
            
            # Add to data array
            end_idx = min(samples_collected + n, num_samples)
            samples[samples_collected:end_idx] = recv_buffer[:end_idx - samples_collected]
            samples_collected = end_idx
        
        # Clean any NaN/Inf values by replacing with zeros (silently)
        if np.any(np.isnan(samples)) or np.any(np.isinf(samples)):
            samples = np.nan_to_num(samples, nan=0.0, posinf=0.0, neginf=0.0)
        
        # Occasionally log received IQ values for debugging
        if hasattr(collect_samples, 'print_count'):
            collect_samples.print_count += 1
        else:
            collect_samples.print_count = 1
            
        if collect_samples.print_count % 1000 == 0:  # Log every 1000th collection
            logging.info(f"IQ Sample Stats - Count: {collect_samples.print_count}")
            logging.info(f"  First 5 samples: {samples[:5]}")
            logging.info(f"  Real range: [{np.min(samples.real):.6f}, {np.max(samples.real):.6f}]")
            logging.info(f"  Imag range: [{np.min(samples.imag):.6f}, {np.max(samples.imag):.6f}]")
            logging.info(f"  Magnitude range: [{np.min(np.abs(samples)):.6f}, {np.max(np.abs(samples)):.6f}]")
            logging.info(f"  Average magnitude: {np.mean(np.abs(samples)):.6f}")
            logging.info(f"  Sample count: {len(samples)}")
            
            # Add diagnostic checks
            zero_count = np.sum(np.abs(samples) == 0)
            inf_count = np.sum(np.isinf(samples))
            nan_count = np.sum(np.isnan(samples))
            very_small_count = np.sum(np.abs(samples) < 1e-30)
            very_large_count = np.sum(np.abs(samples) > 1e30)
            
            logging.info(f"  Diagnostic - Zero: {zero_count}, Inf: {inf_count}, NaN: {nan_count}")
            logging.info(f"  Diagnostic - Very small (<1e-30): {very_small_count}, Very large (>1e30): {very_large_count}")
            logging.info("---")
        
        return {
            'success': True,
            'samples': samples,
            'error': None
        }
    except Exception as e:
        return {
            'success': False,
            'error': str(e),
            'samples': None
        }

# Example usage of read_iq_and_fft for quick test/demo
if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Run IQ and STFT capture example")
    parser.add_argument("--device", type=str, default="", help="USRP device address (e.g., addr=192.168.10.2, serial=xxxx, or type=x300)")
    parser.add_argument("--freq", type=float, default=2.4e9, help="Center frequency in Hz")
    parser.add_argument("--sample_rate", type=float, default=1e6, help="Sample rate (bandwidth) in Hz")
    parser.add_argument("--fft_size", type=int, default=128, help="FFT size (number of frequency bins)")
    parser.add_argument("--resolution", type=float, default=None, help="Desired frequency resolution in Hz (overrides fft_size if provided)")
    parser.add_argument("--window_size", type=int, default=None, help="Window size in samples (defaults to fft_size)")
    parser.add_argument("--hop_size", type=int, default=None, help="Hop size in samples (defaults to window_size/2)")
    parser.add_argument("--window_type", type=str, default="hamming", choices=["hann", "hamming", "blackman", "rectangular"], help="Window type")
    parser.add_argument("--gain", type=float, default=20, help="Gain in dB")
    parser.add_argument("--websocket_port", type=int, default=40001, help="WebSocket server port (default: 40001)")

    args = parser.parse_args()

    # Setup logging to stdout, stderr, and file
    log_file = setup_logging()
    
    # Extract timestamp from main process log filename for child processes
    # log_file format: /path/to/spectrogram_YYYYMMDD_HHMMSS.log
    import re
    timestamp_match = re.search(r'spectrogram_(\d{8}_\d{6})\.log', log_file)
    if timestamp_match:
        main_timestamp = timestamp_match.group(1)
        logging.info(f"Main process timestamp: {main_timestamp}")
    else:
        # Fallback: generate current timestamp
        from datetime import datetime
        main_timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        logging.warning(f"Could not extract timestamp from log file, using current time: {main_timestamp}")

    # Setup signal handlers for graceful shutdown
    signal_module.signal(signal_module.SIGINT, signal_handler)   # Ctrl+C
    signal_module.signal(signal_module.SIGTERM, signal_handler)  # Termination signal
    signal_module.signal(signal_module.SIGHUP, signal_handler)   # Hangup signal

    # Setup CPU cores for 4-thread architecture
    setup_cpu_cores()

    logging.info(f"Using FFT size: {args.fft_size}")
    
    # Set window and hop sizes
    window_size = args.window_size if args.window_size else args.fft_size
    hop_size = args.hop_size if args.hop_size else window_size // 2
    overlap_samples = window_size - hop_size

    # Continuous spectrogram processing loop
    logging.info("Starting continuous spectrogram processing...")
    logging.info(f"  Center Frequency: {args.freq/1e9:.3f} GHz")
    logging.info(f"  Sample Rate: {args.sample_rate/1e6:.1f} MHz")
    logging.info(f"  FFT Size: {args.fft_size} bins")
    if args.resolution:
        logging.info(f"  Resolution: {args.resolution/1e3:.1f} kHz")
    else:
        logging.info(f"  Resolution: {args.sample_rate/args.fft_size/1e3:.1f} kHz (calculated)")
    logging.info(f"  Window Size: {window_size} samples")
    logging.info(f"  Hop Size: {hop_size} samples")
    logging.info(f"  Window Type: {args.window_type}")
    logging.info(f"  Gain: {args.gain} dB")
    logging.info(f"  WebSocket Port: {args.websocket_port}")
    logging.info("Process will run continuously until terminated by parent...")
    
    # Initialize global variables for 4-process architecture
    
    # Use multiprocessing queues and events
    tx_queue = multiprocessing.Queue(maxsize=50)        # TX data queue
    rx_queue = multiprocessing.Queue(maxsize=100)       # RX data queue  
    process_queue = multiprocessing.Queue(maxsize=50)   # Processed data queue
    websocket_queue = multiprocessing.Queue(maxsize=50) # WebSocket data queue
    stop_event = multiprocessing.Event()
        
    # WebSocket server will be started in the WebSocket process
    
    # Main processing loop with multiprocessing
    frame_count = 0
    start_time = time.time()
    
    try:
        # Create TX waveform for loopback testing
        tx_wave = make_tone(args.sample_rate, args.freq - args.sample_rate / 4, 10000, amp=0.3)
        logging.info(f"Created TX tone: {(args.freq - args.sample_rate / 4)/1e6:.1f} MHz, {len(tx_wave)} samples")
        
        # USRP device setup is handled in the TX/RX process
        logging.info("USRP device setup will be handled in TX/RX process")
        
        # Start 3 processes with dedicated CPU cores
        logging.info("Starting 3-process architecture...")
        
        # Create log directory
        log_dir = "logs"
        os.makedirs(log_dir, exist_ok=True)
        
        # Initialize process variables
        tx_rx_process = None
        iq_process = None
        websocket_process = None
        
        # Process 1: TX/RX process (CPU core TX_RX_CPU_CORE)
        tx_rx_process = multiprocessing.Process(
            target=tx_rx_process_function,
            args=(tx_wave, hop_size, stop_event, rx_queue, main_timestamp, args.device, args.freq, args.sample_rate, args.gain)
        )
        tx_rx_process.start()
        tx_rx_log_filename = generate_child_process_log_filename("tx_rx", main_timestamp)
        logging.info(f"Started TX/RX process on CPU core {TX_RX_CPU_CORE} (logs: logs/{tx_rx_log_filename})")
        
        # Process 2: IQ process (CPU core IQ_PROCESS_CPU_CORE)
        iq_process = multiprocessing.Process(
            target=iq_process_function,
            args=(args.sample_rate, args.fft_size, stop_event, rx_queue, process_queue, main_timestamp)
        )
        iq_process.start()
        iq_log_filename = generate_child_process_log_filename("iq_process", main_timestamp)
        logging.info(f"Started IQ process on CPU core {IQ_PROCESS_CPU_CORE} (logs: logs/{iq_log_filename})")
        
        # Process 3: WebSocket process (CPU core WEBSOCKET_CPU_CORE)
        websocket_process = multiprocessing.Process(
            target=websocket_process_function,
            args=(args.websocket_port, stop_event, process_queue, main_timestamp)
        )
        websocket_process.start()
        websocket_log_filename = generate_child_process_log_filename("websocket", main_timestamp)
        logging.info(f"Started WebSocket process on CPU core {WEBSOCKET_CPU_CORE} (logs: logs/{websocket_log_filename})")
        
        # Main monitoring loop
        logging.info("3-process architecture started successfully. Main process monitoring child processes...")
        
        # Verify process affinity
        verify_process_affinity()
        
        last_rx_count = 0
        last_process_count = 0
        last_websocket_count = 0
        
        while True:
            try:
                # Monitor thread health and queue status
                time.sleep(10)  # Check every 10 seconds
                
                # Log queue status occasionally
                if frame_count % 50 == 0:  # More frequent monitoring
                    try:
                        current_rx_size = rx_queue.qsize()
                        current_process_size = process_queue.qsize()
                        current_websocket_size = websocket_queue.qsize()
                        
                        logging.info(f"Queue Status - RX: {current_rx_size}, Process: {current_process_size}, WebSocket: {current_websocket_size}")
                        logging.info(f"Active WebSocket clients: {len(websocket_clients)}")
                        
                        # Check for potential issues
                        if current_rx_size == 0 and last_rx_count > 0:
                            logging.warning("RX queue is empty - RX thread may be having issues")
                        if current_process_size == 0 and current_rx_size > 10:
                            logging.warning("Process queue is empty but RX queue has data - Process thread may be having issues")
                        if current_websocket_size == 0 and current_process_size > 10:
                            logging.warning("WebSocket queue is empty but Process queue has data - WebSocket process may be having issues")
                        
                        # More detailed debugging
                        if current_rx_size > 0:
                            logging.info(f"RX queue has {current_rx_size} items waiting to be processed")
                        if current_process_size > 0:
                            logging.info(f"Process queue has {current_process_size} items waiting to be broadcast")
                        
                        last_rx_count = current_rx_size
                        last_process_count = current_process_size
                        last_websocket_count = current_websocket_size
                    except Exception as e:
                        logging.warning(f"Error checking queue status: {e}")
                
                frame_count += 1
                
            except KeyboardInterrupt:
                break
                
    except KeyboardInterrupt:
        logging.info("Received interrupt signal, stopping continuous processing")
        logging.info(f"Total frames processed: {frame_count}")
        logging.info(f"Total processing time: {time.time() - start_time:.1f} seconds")
        if frame_count > 0:
            logging.info(f"Average processing rate: {frame_count / (time.time() - start_time):.2f} frames/second")
    except Exception as e:
        logging.error(f"Unexpected error in continuous processing: {str(e)}")
        logging.info(f"Total frames processed: {frame_count}")
        logging.info(f"Total processing time: {time.time() - start_time:.1f} seconds")
    finally:
        # Stop all 4 processes
        logging.info("Stopping 4-process architecture...")
        stop_event.set()
        
        # Collect all processes
        processes_to_cleanup = []
        if tx_rx_process:
            processes_to_cleanup.append(("TX_RX", tx_rx_process))
        if iq_process:
            processes_to_cleanup.append(("IQ_Process", iq_process))
        if websocket_process:
            processes_to_cleanup.append(("WebSocket", websocket_process))
        
        # First attempt: graceful termination
        logging.info("Attempting graceful termination...")
        for name, process in processes_to_cleanup:
            if process.is_alive():
                logging.info(f"Stopping {name} process gracefully...")
                process.join(timeout=3)
        
        # Second attempt: terminate
        logging.info("Attempting terminate() for remaining processes...")
        for name, process in processes_to_cleanup:
            if process.is_alive():
                logging.warning(f"{name} process did not terminate gracefully, calling terminate()")
                try:
                    process.terminate()
                    process.join(timeout=2)
                except Exception as e:
                    logging.error(f"Error terminating {name} process: {e}")
        
        # Third attempt: force kill
        logging.info("Force killing remaining processes...")
        for name, process in processes_to_cleanup:
            if process.is_alive():
                logging.warning(f"Force killing {name} process (PID: {process.pid})")
                try:
                    process.kill()
                    process.join(timeout=1)
                except Exception as e:
                    logging.error(f"Error killing {name} process: {e}")
        
        # Final verification
        still_alive = []
        for name, process in processes_to_cleanup:
            if process.is_alive():
                still_alive.append(name)
        
        if still_alive:
            logging.error(f"WARNING: These processes are still alive: {still_alive}")
            # Use system kill as last resort
            for name, process in processes_to_cleanup:
                if process.is_alive():
                    try:
                        import subprocess
                        subprocess.run(['kill', '-9', str(process.pid)], timeout=5)
                        logging.warning(f"Used system kill -9 on {name} process (PID: {process.pid})")
                    except Exception as e:
                        logging.error(f"Failed to system kill {name} process: {e}")
        else:
            logging.info("All processes successfully terminated")
        
        # USRP resources are cleaned up in the TX/RX process
        logging.info("USRP resources will be cleaned up in TX/RX process")
        
        # Final cleanup: check for orphaned processes
        cleanup_orphaned_processes()
        
        logging.info("Cleanup complete")
