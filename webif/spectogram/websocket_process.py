#!/usr/bin/env python3
"""
WebSocket Process - Self-contained WebSocket server process
Handles real-time broadcasting of processed spectrogram data
"""

import json
import time
import asyncio
import websockets
import numpy as np
import threading
import os
import signal
import gc
import queue
from pathlib import Path

# Add the spectogram directory to Python path for imports
script_dir = Path(__file__).parent

from process_base import ProcessBase

class WebSocketProcess(ProcessBase):
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
        # Call parent constructor first
        super().__init__("WebSocket", stop_event, timestamp, cpu_core, log_dir)
        
        # Process-specific attributes
        self.websocket_port = websocket_port
        self.stft_queue = stft_queue
        
        # WebSocket state (global variables like old code)
        self.websocket_clients = set()
        self.websocket_server = None
        self.frames_broadcast = 0
        
        # Set WebSocket-specific memory threshold (800MB for client management)
        self.set_memory_threshold(800)
        self.set_memory_check_interval(15.0)  # Check every 15 seconds
        
        # Reusable data structures to avoid creating new objects every frame
        self.spectrogram_data_template = {
            'f': 0,
            'freq': [],
            't': [],
            'mag': [],
            'ts': 0
        }
    
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
        """Broadcast spectrogram data - optimized with new key names"""
        if not self.websocket_clients:
            return
        
        try:
            # Check for NaN values in magnitude data before sending
            nan_check_start = time.time()
            if np.any(np.isnan(data['mag'])):
                self.logger.error(f"Frame {data['f']}: NaN values in mag before sending!")
                self.logger.error(f"  Magnitude shape: {data['mag'].shape}")
                self.logger.error(f"  NaN count: {np.sum(np.isnan(data['mag']))}")
                # Replace NaN with -120 dB
                data['mag'] = np.nan_to_num(data['mag'], nan=-120.0)
            nan_check_time = time.time() - nan_check_start
            
            # Prepare data for transmission - simple JSON since we have pre-converted lists
            json_start = time.time()
            
            # Debug: Check data types to see if we're actually sending lists
            if data['f'] % 1000 == 0:  # Log every 1000th frame
                self.logger.info(f"Broadcast data types (frame {data['f']}):")
                self.logger.info(f"  freq type: {type(data['freq'])}, length: {len(data['freq']) if hasattr(data['freq'], '__len__') else 'N/A'}")
                self.logger.info(f"  t type: {type(data['t'])}, length: {len(data['t']) if hasattr(data['t'], '__len__') else 'N/A'}")
                self.logger.info(f"  mag type: {type(data['mag'])}, length: {len(data['mag']) if hasattr(data['mag'], '__len__') else 'N/A'}")
                if hasattr(data['freq'], 'tolist'):
                    self.logger.info(f"  freq is numpy array with shape: {data['freq'].shape}")
                if hasattr(data['t'], 'tolist'):
                    self.logger.info(f"  t is numpy array with shape: {data['t'].shape}")
                if hasattr(data['mag'], 'tolist'):
                    self.logger.info(f"  mag is numpy array with shape: {data['mag'].shape}")
            
            json_data = json.dumps(data)  # Much faster - no conversions needed
            json_time = time.time() - json_start
            
            # Broadcast to all connected clients
            client_start = time.time()
            client_count = 0
            for client in self.websocket_clients.copy():
                try:
                    client_count += 1
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
            client_time = time.time() - client_start
            
            # Log detailed timing every 1000 frames
            if data['f'] % 1000 == 0:
                total_broadcast_time = nan_check_time + json_time + client_time
                self.logger.info(f"Broadcast timing breakdown (frame {data['f']}):")
                self.logger.info(f"  NaN check: {nan_check_time*1000:.2f}ms")
                self.logger.info(f"  JSON serialize: {json_time*1000:.2f}ms")
                self.logger.info(f"  Client send ({client_count} clients): {client_time*1000:.2f}ms")
                self.logger.info(f"  Total broadcast: {total_broadcast_time*1000:.2f}ms")
                    
        except Exception as e:
            self.logger.error(f"Error broadcasting spectrogram data: {e}")
    

    
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
        
        # Reset start time when we actually start processing
        self.start_time = time.time()
        self.last_fps_log_time = time.time()
        
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
                    
                    # Get current time for monitoring
                    current_time = time.time()
                    
                    # Use base class memory monitoring
                    self._check_memory_periodic(current_time)
                    
                    # Skip processing if no clients are connected (performance optimization)
                    if not self.websocket_clients:
                        # Just consume data to prevent queue buildup, but don't process
                        try:
                            self.stft_queue.get(timeout=0.1)  # Short timeout
                            # Don't increment counters since we're not processing
                            continue
                        except queue.Empty:
                            time.sleep(0.01)  # Very short sleep when no clients
                            continue
                    
                    # Get processed data from stft queue
                    queue_start = time.time()
                    frame_data = self.stft_queue.get(timeout=0.1)  # 1 second timeout
                    queue_time = time.time() - queue_start
                    
                    frame_num, f, t, magnitude_db, timestamp = frame_data
                    
                    # Check if we're falling behind - log warning if queue is building up
                    queue_size = self.stft_queue.qsize()
                    if queue_size > 20:  # If more than 20 frames waiting
                        if frame_count % 1000 == 0:  # Log every 1000th frame
                            self.logger.warning(f"WebSocket falling behind: {queue_size} frames in queue, processing frame {frame_num}")
                    
                    # Prepare data for WebSocket broadcast - OPTIMIZED MEMORY USAGE
                    # Reuse template instead of creating new dict every frame
                    prep_start = time.time()
                    
                    # Convert numpy arrays to lists once - much faster than doing it in JSON
                    freq_list = f.tolist() if hasattr(f, 'tolist') else f
                    t_list = t.tolist() if hasattr(t, 'tolist') else t
                    mag_list = magnitude_db.tolist() if hasattr(magnitude_db, 'tolist') else magnitude_db
                    
                    # Update template instead of creating new dict - saves memory allocation
                    self.spectrogram_data_template['f'] = frame_num
                    self.spectrogram_data_template['freq'] = freq_list
                    self.spectrogram_data_template['t'] = t_list
                    self.spectrogram_data_template['mag'] = mag_list
                    self.spectrogram_data_template['ts'] = timestamp
                    
                    # Create a copy for sending to avoid modification issues
                    spectrogram_data = self.spectrogram_data_template.copy()
                    prep_time = time.time() - prep_start
                    
                    # Debug: Verify we're sending lists, not numpy arrays
                    if frame_count % 1000 == 0:  # Log every 1000th frame
                        self.logger.info(f"Main loop data types (frame {frame_num}):")
                        self.logger.info(f"  freq_list type: {type(freq_list)}, length: {len(freq_list) if hasattr(freq_list, '__len__') else 'N/A'}")
                        self.logger.info(f"  t_list type: {type(t_list)}, length: {len(t_list) if hasattr(t_list, '__len__') else 'N/A'}")
                        self.logger.info(f"  mag_list type: {type(mag_list)}, length: {len(mag_list) if hasattr(mag_list, '__len__') else 'N/A'}")
                        if hasattr(freq_list, 'tolist'):
                            self.logger.info(f"  freq_list is still numpy array with shape: {freq_list.shape}")
                        if hasattr(t_list, 'tolist'):
                            self.logger.info(f"  t_list is still numpy array with shape: {t_list.shape}")
                        if hasattr(mag_list, 'tolist'):
                            self.logger.info(f"  mag_list is still numpy array with shape: {mag_list.shape}")
                    
                    # Broadcast to all connected clients
                    broadcast_start = time.time()
                    self.broadcast_spectrogram_data(spectrogram_data)
                    broadcast_time = time.time() - broadcast_start
                    
                    frame_count += 1
                    self.frame_count += 1  # Update global frame count for FPS tracking
                    
                    # Log timing details every 1000 frames
                    if frame_count % 1000 == 0:
                        total_time = queue_time + prep_time + broadcast_time
                        self.logger.info(f"WebSocket timing breakdown (frame {frame_num}):")
                        self.logger.info(f"  Queue get: {queue_time*1000:.2f}ms")
                        self.logger.info(f"  Data prep: {prep_time*1000:.2f}ms")
                        self.logger.info(f"  Broadcast: {broadcast_time*1000:.2f}ms")
                        self.logger.info(f"  Total: {total_time*1000:.2f}ms")
                    
                    # Log progress and FPS occasionally
                    if frame_count % 5000 == 0:
                        self.logger.info(f"WebSocket process: broadcast {frame_count} frames")
                    
                    # Use base class FPS logging
                    self._log_fps(current_time)
                    
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
    
    def get_stats(self):
        """Get process statistics"""
        stats = super().get_stats()
        stats.update({
            'frames_broadcast': self.frames_broadcast,
            'clients_connected': len(self.websocket_clients)
        })
        return stats
    
    def stop(self):
        """Stop the WebSocket process"""
        # Close server if running
        if self.websocket_server:
            self.websocket_server.close()
        
        # Call parent stop method
        super().stop()

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
