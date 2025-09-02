#!/usr/bin/env python3
"""
WebSocket Process - Self-contained WebSocket server process
Handles real-time broadcasting of processed spectrogram data
"""

import os
import sys
import json
import time
import signal
import logging
import asyncio
import websockets
import multiprocessing
import threading
import numpy as np
from pathlib import Path

# Add the spectogram directory to Python path for imports
script_dir = Path(__file__).parent
sys.path.insert(0, str(script_dir))

try:
    import queue
except ImportError as e:
    logging.error(f"Import error: {e}")
    sys.exit(1)

class WebSocketProcess:
    """WebSocket process using the exact same hybrid approach as old working code"""
    
    def __init__(self, websocket_port, stop_event, stft_queue, timestamp, cpu_core=2, log_dir=None):
        """
        Initialize WebSocket process
        
        Args:
            websocket_port: WebSocket server port
            stop_event: Multiprocessing event to signal stop
            stft_queue: Queue for processed STFT data
            timestamp: Main process timestamp
            cpu_core: CPU core to bind to
        """
        self.websocket_port = websocket_port
        self.stop_event = stop_event
        self.stft_queue = stft_queue
        self.timestamp = timestamp
        self.cpu_core = cpu_core
        self.log_dir = log_dir
        
        # WebSocket state (global variables like old code)
        self.websocket_clients = set()
        self.websocket_server = None
        self.frames_broadcast = 0
        
        # FPS tracking
        self.frame_count = 0
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
        log_filename = f"spectrogram_{self.timestamp}_websocket.log"
        # Use log directory passed from main process, or fallback to script directory
        if self.log_dir:
            log_path = self.log_dir / log_filename
        else:
            log_path = script_dir / "logs" / log_filename
        
        # Create logs directory if it doesn't exist
        log_path.parent.mkdir(exist_ok=True)
        
        # Create a logger specific to this process
        self.logger = logging.getLogger(f"WebSocketProcess_{self.timestamp}")
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
        
        self.logger.info(f"WebSocket Process logging setup complete: {log_path}")
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
            self.logger.info(f"WebSocket Process received signal {signum}, stopping...")
            self.stop_event.set()
        
        signal.signal(signal.SIGINT, signal_handler)
        signal.signal(signal.SIGTERM, signal_handler)
        # Add SIGHUP for backend control
        signal.signal(signal.SIGHUP, signal_handler)
        self.logger.info("WebSocket Process: Signal handlers set up successfully")
    
    def _setup_cpu_affinity(self):
        """Set CPU affinity for this process"""
        try:
            import psutil
            process = psutil.Process()
            process.cpu_affinity([self.cpu_core])
            self.logger.info(f"Successfully set WebSocket process affinity to core {self.cpu_core}")
        except Exception as e:
            self.logger.error(f"Error setting CPU affinity: {e}")
    
    async def websocket_handler(self, websocket):
        """Handle WebSocket connections - EXACT same as old working code"""
        self.logger.info(f"WebSocket client connected: {websocket.remote_address}")
        self.websocket_clients.add(websocket)
        self.logger.info(f"Total WebSocket clients: {len(self.websocket_clients)}")
        
        try:
            async for message in websocket:
                # Handle any incoming messages if needed
                self.logger.info(f"Received WebSocket message: {message}")
        except websockets.exceptions.ConnectionClosed:
            self.logger.info(f"WebSocket client disconnected: {websocket.remote_address}")
        finally:
            self.websocket_clients.discard(websocket)
            self.logger.info(f"Total WebSocket clients: {len(self.websocket_clients)}")
    
    async def start_websocket_server(self):
        """Start WebSocket server - EXACT same as old working code"""
        # Reduce WebSocket library logging to prevent spam
        import logging as ws_logging
        ws_logging.getLogger('websockets.server').setLevel(ws_logging.WARNING)
        ws_logging.getLogger('websockets.protocol').setLevel(ws_logging.WARNING)
        
        self.logger.info(f"Starting WebSocket server on port {self.websocket_port}")
        self.websocket_server = await websockets.serve(self.websocket_handler, "0.0.0.0", self.websocket_port)
        self.logger.info(f"WebSocket server started on ws://0.0.0.0:{self.websocket_port}")
        await self.websocket_server.wait_closed()
    
    def start_websocket_server_direct(self):
        """Start WebSocket server directly - EXACT same as old working code"""
        try:
            # Reduce WebSocket library logging to prevent spam
            import logging as ws_logging
            ws_logging.getLogger('websockets.server').setLevel(ws_logging.WARNING)
            ws_logging.getLogger('websockets.protocol').setLevel(ws_logging.WARNING)
            
            asyncio.run(self.start_websocket_server())
        except Exception as e:
            self.logger.error(f"WebSocket server error: {e}")
    
    def broadcast_spectrogram_data(self, data):
        """Broadcast spectrogram data - EXACT same as old working code"""
        if not self.websocket_clients:
            return
        
        try:
            # Check for NaN values in magnitude data before sending
            if np.any(np.isnan(data['magnitude_db'])):
                self.logger.error(f"Frame {data['frame_num']}: NaN values in magnitude_db before sending!")
                self.logger.error(f"  Magnitude shape: {data['magnitude_db'].shape}")
                self.logger.error(f"  NaN count: {np.sum(np.isnan(data['magnitude_db']))}")
                # Replace NaN with -120 dB
                data['magnitude_db'] = np.nan_to_num(data['magnitude_db'], nan=-120.0)
            
            # Prepare data for transmission
            json_data = json.dumps(data)
            
            # Broadcast to all connected clients
            for client in self.websocket_clients.copy():
                try:
                    # Get the event loop from the websocket server
                    if self.websocket_server and self.websocket_server.is_serving():
                        loop = self.websocket_server.loop
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
                            self.logger.warning(f"No event loop available for client {client.remote_address}")
                            self.websocket_clients.discard(client)
                except Exception as e:
                    self.logger.error(f"Error sending to WebSocket client: {e}")
                    self.websocket_clients.discard(client)
                    
        except Exception as e:
            self.logger.error(f"Error broadcasting spectrogram data: {e}")
    
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
    
    def _cleanup_websocket(self):
        """Clean up WebSocket resources"""
        try:
            if hasattr(self, 'websocket_server') and self.websocket_server:
                self.logger.info("Cleaning up WebSocket server...")
                self.websocket_server.close()
                self.websocket_server = None
            
            # Close all client connections
            if hasattr(self, 'websocket_clients') and self.websocket_clients:
                self.logger.info(f"Closing {len(self.websocket_clients)} WebSocket client connections...")
                for client in self.websocket_clients.copy():
                    try:
                        # Note: We can't await in this context, but the client will be closed when the process exits
                        pass
                    except Exception as e:
                        self.logger.error(f"Error closing client connection: {e}")
                self.websocket_clients.clear()
                
            self.logger.info("WebSocket cleanup completed")
            
        except Exception as e:
            self.logger.error(f"Error during WebSocket cleanup: {e}")
    
    def run(self):
        """Main processing loop with parent monitoring"""
        self.logger.info(f"Starting WebSocket process on CPU core {self.cpu_core}...")
        
        try:
            # Start WebSocket server in a separate thread - EXACT same as old code
            def websocket_server_thread():
                try:
                    self.start_websocket_server_direct()
                except Exception as e:
                    self.logger.error(f"WebSocket server thread error: {e}")
            
            # Start WebSocket server thread
            server_thread = threading.Thread(target=websocket_server_thread)
            server_thread.daemon = True
            server_thread.start()
            
            # Wait a moment for server to start
            time.sleep(1)
            
            # Broadcast loop - EXACT same as old code
            frame_count = 0
            
            while not self.stop_event.is_set():
                try:
                    # Check if parent process is still alive
                    if not self._check_parent_alive():
                        self.logger.warning("Parent process died or is orphaned, sending SIGKILL to self...")
                        os.kill(os.getpid(), signal.SIGKILL)  # Force kill self
                        break
                    
                    # Get processed data from stft queue
                    frame_data = self.stft_queue.get(timeout=1.0)  # 1 second timeout
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
                    self.broadcast_spectrogram_data(spectrogram_data)
                    frame_count += 1
                    self.frame_count += 1  # Update global frame count for FPS tracking
                    
                    # Log progress and FPS occasionally
                    if frame_count % 5000 == 0:
                        self.logger.info(f"WebSocket process: broadcast {frame_count} frames")
                    
                    # Log FPS periodically
                    current_time = time.time()
                    if current_time - self.last_fps_log_time >= self.fps_log_interval:
                        elapsed_time = current_time - self.start_time
                        if elapsed_time > 0:
                            fps = self.frame_count / elapsed_time
                            self.logger.info(f"WebSocket FPS: {fps:.2f} frames/sec (Total: {self.frame_count} frames in {elapsed_time:.1f}s)")
                            self.last_fps_log_time = current_time
                    
                except queue.Empty:
                    # No data available, continue
                    continue
                except Exception as e:
                    self.logger.error(f"Error in WebSocket process: {e}")
                    time.sleep(0.1)  # Brief pause before retrying
                    continue
                    
            return True
            
        except Exception as e:
            self.logger.error(f"Error in WebSocket process: {e}")
            return False
        finally:
            self.logger.info("WebSocket process cleanup initiated...")
            self._cleanup_websocket()
            self.logger.info("WebSocket process cleanup completed")
    
    def start(self):
        """Start the WebSocket process"""
        try:
            self.run()
            return True
        except Exception as e:
            self.logger.error(f"Error starting WebSocket process: {e}")
            return False
    
    def stop(self):
        """Stop the WebSocket process"""
        self.logger.info("Stopping WebSocket process...")
        self.stop_event.set()
        
        # Close server if running
        if self.websocket_server:
            self.websocket_server.close()
    
    def is_alive(self):
        """Check if the process is alive"""
        return not self.stop_event.is_set()
    
    def get_stats(self):
        """Get process statistics"""
        return {
            'frames_broadcast': self.frames_broadcast,
            'clients_connected': len(self.websocket_clients),
            'is_alive': self.is_alive()
        }

def main():
    """Main function for standalone testing"""
    import argparse
    import multiprocessing
    
    parser = argparse.ArgumentParser(description='WebSocket Process')
    parser.add_argument('--port', type=int, default=40001, help='WebSocket port')
    parser.add_argument('--cpu-core', type=int, default=2, help='CPU core to bind to')
    
    args = parser.parse_args()
    
    # Create stop event
    stop_event = multiprocessing.Event()
    
    # Create STFT queue
    stft_queue = multiprocessing.Queue(maxsize=100)
    
    # Create and start process
    process = WebSocketProcess(
        websocket_port=args.port,
        stop_event=stop_event,
        stft_queue=stft_queue,
        timestamp="test",
        cpu_core=args.cpu_core
    )
    
    try:
        if process.start():
            process.logger.info("WebSocket Process started successfully")
        else:
            process.logger.error("Failed to start WebSocket Process")
            
    except KeyboardInterrupt:
        process.logger.info("Received keyboard interrupt")
    finally:
        process.stop()

if __name__ == "__main__":
    main()
