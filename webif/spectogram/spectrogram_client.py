#!/usr/bin/env python3
"""
Spectrogram WebSocket Client

This client connects to the spectrogram server via WebSocket and displays
real-time spectrogram data using matplotlib.

Usage:
    python spectrogram_client.py [--host localhost] [--port 8765]
"""

import asyncio
import websockets
import json
import numpy as np
import argparse
import time
import logging
import os
from datetime import datetime

class SpectrogramClient:
    def __init__(self, host='127.0.0.1', port=40001, sample_rate=1e6):
        """
        Initialize the spectrogram client.
        
        Args:
            host: WebSocket server host
            port: WebSocket server port
            sample_rate: Sample rate in Hz (default: 1 MHz)
        """
        self.host = host
        self.port = port
        self.websocket = None
        self.connected = False
        self.latest_data = None
        self.sample_rate = sample_rate
        
        # Setup logging
        self.setup_logging()
        
        # Create frequency map from -sample_rate/2 to +sample_rate/2
        self.freq_min = -sample_rate / 2
        self.freq_max = sample_rate / 2
        self.freq_map_size = 1024  # Number of frequency bins
        self.freq_step = sample_rate / self.freq_map_size
        
        # For 1 MHz sample rate, this should be -500 kHz to +500 kHz
        # But the STFT is returning frequencies in this range, so we need to match it
        
        # Initialize frequency-magnitude map
        self.freq_mag_map = np.full(self.freq_map_size, -120.0)  # Initialize with noise floor
        self.freq_map = np.linspace(self.freq_min, self.freq_max, self.freq_map_size)
        
        self.logger.info(f"Created frequency map: {self.freq_min/1e3:.1f} to {self.freq_max/1e3:.1f} kHz ({self.freq_map_size} bins)")
        self.logger.info(f"Frequency step: {self.freq_step:.1f} Hz per bin")
        self.logger.info(f"Sample rate: {self.sample_rate/1e6:.1f} MHz")
        
    def setup_logging(self):
        """Setup logging to file."""
        # Create logs directory if it doesn't exist
        log_dir = "logs"
        os.makedirs(log_dir, exist_ok=True)
        
        # Create log filename with timestamp
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        log_filename = f"spectrogram_client_{timestamp}.log"
        log_path = os.path.join(log_dir, log_filename)
        
        # Configure logging
        self.logger = logging.getLogger(f"spectrogram_client_{timestamp}")
        self.logger.setLevel(logging.INFO)
        
        # Create file handler
        file_handler = logging.FileHandler(log_path)
        file_handler.setLevel(logging.INFO)
        
        # Create formatter
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        file_handler.setFormatter(formatter)
        
        # Add handler to logger
        self.logger.addHandler(file_handler)
        
        print(f"Logging to: {log_path}")
        
    async def connect(self):
        """Connect to the WebSocket server."""
        try:
            uri = f"ws://{self.host}:{self.port}"
            self.logger.info(f"Connecting to {uri}")
            self.websocket = await websockets.connect(uri)
            self.connected = True
            self.logger.info(f"Connected to spectrogram server")
            return True
        except Exception as e:
            self.logger.error(f"Failed to connect: {e}")
            return False
            
    async def disconnect(self):
        """Disconnect from the WebSocket server."""
        if self.websocket:
            await self.websocket.close()
            self.connected = False
            self.logger.info(f"Disconnected from server")
            
    async def receive_data(self):
        """Receive data from the WebSocket server."""
        if not self.websocket:
            return
            
        frame_count = 0
        
        try:
            async for message in self.websocket:
                frame_count += 1
                try:
                    data = json.loads(message)
                    
                    # Debug: Log the first few messages to see what we're receiving
                    if frame_count <= 3:
                        self.logger.info(f"📦 Received data frame {frame_count}:")
                        self.logger.info(f"   Keys: {list(data.keys())}")
                        self.logger.info(f"   Frame: {data.get('f', 'N/A')}")
                        self.logger.info(f"   Timestamp: {data.get('ts', 'N/A')}")
                        self.logger.info(f"   Freq length: {len(data.get('freq', []))}")
                        self.logger.info(f"   Mag length: {len(data.get('mag', []))}")
                    
                    # Process all received data as spectrogram data (no type check needed)
                    
                    # Store packet info every 10 packets for display
                    if frame_count % 10 == 0:
                        frequencies = data.get('freq', [])  # Server sends 'freq'
                        magnitude = data.get('mag', [])    # Server sends 'mag'
                        self.last_packet_info = {
                            'frame_num': data.get('f', 0),      # Server sends 'f'
                            'timestamp': data.get('ts', 0),     # Server sends 'ts'
                            'frequencies': frequencies,
                            'times': data.get('t', []),         # Server sends 't'
                            'magnitude': magnitude,
                            'frequencies_shape': f"{len(frequencies)}",
                            'times_shape': f"{len(data.get('t', []))}",
                            'magnitude_shape': f"{len(magnitude)}x{len(magnitude[0]) if isinstance(magnitude, list) and len(magnitude) > 0 and isinstance(magnitude[0], list) else 1}"
                        }
                    
                    # Print example of received data every 100 packets
                    if frame_count % 10 == 0:
                        self.print_data_example(data, frame_count)
                    
                    self.process_spectrogram_data(data)
                except json.JSONDecodeError as e:
                    self.logger.warning(f"Invalid JSON received: {e}")
                except Exception as e:
                    self.logger.error(f"Error processing message: {e}")
        except websockets.exceptions.ConnectionClosed:
            self.logger.warning(f"Connection closed by server")
            self.connected = False
        except Exception as e:
            self.logger.error(f"Error receiving data: {e}")
            self.connected = False
            
    def process_spectrogram_data(self, data):
        """
        Process received spectrogram data and draw ASCII spectrogram.
        
        Args:
            data: Dictionary containing spectrogram data
        """
        try:
            # Extract data - use server's key names
            frame = data.get('f', 0)                    # Server sends 'f'
            frequencies = np.array(data.get('freq', [])) # Server sends 'freq'
            timestamps = np.array(data.get('t', []))    # Server sends 't'
            magnitudes = np.array(data.get('mag', []))  # Server sends 'mag'
            # print(frequencies.shape)
            # print(timestamps.shape)
            # print(magnitudes.shape)
            # Draw ASCII spectrogram for each data point
            for idx, timestamp in enumerate(timestamps):
                magnitude = magnitudes[:,idx] if idx < len(magnitudes) else 0
                self.draw_ascii_spectrogram(timestamp, frequencies, magnitude)
            
        except Exception as e:
            print(f"Error processing spectrogram data: {e}")

    def draw_ascii_spectrogram(self, timestamp, frequency, magnitude):
        """
        Draw ASCII spectrogram display for a time slice.
        
        Args:
            timestamp: Timestamp value
            frequency: Frequency array
            magnitude: Magnitude array
        """
        # print(timestamp)
        # print(frequency)
        # print(magnitude)
        try:

            # Clear screen and move cursor to top
            print("\033[2J\033[H", end="")

            # Display single data point
            print(f"Timestamp: {timestamp:.3f} s")
            print(f"Frequencies: {len(frequency)} points, range: {np.min(frequency):.1f} to {np.max(frequency):.1f} Hz")
            print(f"Magnitudes: {len(magnitude)} points, range: {np.min(magnitude):.1f} to {np.max(magnitude):.1f} dB")
            print("-" * 50)
            
            # Create frequency vs magnitude plot
            if len(frequency) > 0 and len(magnitude) > 0:
                # Get terminal size for plot dimensions
                try:
                    terminal_width = os.get_terminal_size().columns
                    terminal_height = os.get_terminal_size().lines
                except:
                    terminal_width = 80
                    terminal_height = 24
                
                # Plot dimensions
                plot_width = min(terminal_width - 20, 60)
                plot_height = min(terminal_height - 15, 15)
                
                # Downsample data to fit plot
                if len(frequency) > plot_width:
                    step = len(frequency) // plot_width
                    freq_plot = frequency[::step][:plot_width]
                    mag_plot = magnitude[::step][:plot_width]
                else:
                    freq_plot = frequency
                    mag_plot = magnitude
                
                # Normalize magnitude to plot height
                mag_min = np.min(mag_plot)
                mag_max = np.max(mag_plot)
                mag_range = mag_max - mag_min if mag_max > mag_min else 1
                
                # Create plot
                print(f"📊 Frequency vs Magnitude Plot")
                print(f"📡 Frequency Range: {freq_plot[0]:.1f} to {freq_plot[-1]:.1f} Hz")
                print(f"📈 Magnitude Range: {mag_min:.1f} to {mag_max:.1f} dB")
                print("=" * (plot_width + 10))
                
                # Draw plot rows (y-axis)
                for y in range(plot_height):
                    # Calculate threshold for this row (top to bottom)
                    threshold = mag_max - (y / plot_height) * mag_range
                    
                    # Draw row
                    row = ""
                    for x, mag in enumerate(mag_plot):
                        if mag >= threshold:
                            row += "█"
                        else:
                            row += " "
                    
                    # Add magnitude label
                    row += f"  {threshold:.0f} dB"
                    print(row)
                
                # Draw x-axis
                print("-" * plot_width)
                
                # Draw frequency labels
                freq_labels = ""
                for x in range(plot_width):
                    if x == 0 or x == plot_width // 2 or x == plot_width - 1:
                        freq_labels += "|"
                    else:
                        freq_labels += " "
                print(freq_labels)
                
                # Draw frequency values
                freq_values = ""
                for x in range(plot_width):
                    if x == 0:
                        freq_values += f"{freq_plot[0]:.0f}"
                    elif x == plot_width // 2:
                        mid_idx = len(freq_plot) // 2
                        freq_values += f"{freq_plot[mid_idx]:.0f}"
                    elif x == plot_width - 1:
                        freq_values += f"{freq_plot[-1]:.0f}"
                    else:
                        freq_values += " "
                print(freq_values)
                print(f"Hz{' ' * (plot_width - 2)}")
                
                # Show peak information
                peak_idx = np.argmax(mag_plot)
                peak_freq = freq_plot[peak_idx]
                peak_mag = mag_plot[peak_idx]
                print(f"Peak: {peak_freq:.1f} Hz at {peak_mag:.1f} dB")
            else:
                print("No data available for plotting")
            
            print("-" * 50)
            time.sleep(0.1)
        except Exception as e:
            print(f"Error drawing ASCII spectrogram: {e}")
            
    def print_summary(self):
        """Print summary of received data."""
        if self.latest_data:
            self.logger.info(f"Data summary:")
            self.logger.info(f"  Latest frame: {self.latest_data.get('frame_num', 'N/A')}")
            self.logger.info(f"  Latest timestamp: {self.latest_data.get('timestamp', 'N/A')}")
        
    async def run(self):
        """Main client loop."""
        if not await self.connect():
            return
            
        try:
            self.logger.info(f"Receiving spectrogram data...")
            self.logger.info(f"Press Ctrl+C to stop")
            self.logger.info("  " + "=" * 50)
            
            # Receive data
            await self.receive_data()
            
        except KeyboardInterrupt:
            self.logger.info(f"\nReceived interrupt signal")
            self.print_summary()
        except Exception as e:
            self.logger.error(f"Error in main loop: {e}")
        finally:
            await self.disconnect()

    def print_data_example(self, data, frame_count):
        """Print an example of received data."""
        try:
            frame_num = data.get('f', 'N/A')        # Server sends 'f'
            timestamp = data.get('ts', 'N/A')       # Server sends 'ts'
            frequencies = data.get('freq', [])      # Server sends 'freq'
            magnitude = data.get('mag', [])         # Server sends 'mag'
            
            # Convert to numpy arrays for analysis
            freq_array = np.array(frequencies)
            mag_array = np.array(magnitude)
            
            # Find peak frequency and magnitude
            if len(mag_array) > 0:
                if mag_array.ndim == 2:
                    mag_1d = mag_array[:, -1] if mag_array.shape[1] > 0 else mag_array[:, 0]
                else:
                    mag_1d = mag_array
                
                max_idx = np.argmax(mag_1d)
                peak_freq = freq_array[max_idx] if len(freq_array) > max_idx else 'N/A'
                peak_mag = mag_1d[max_idx]
                
                # Calculate statistics
                freq_min = np.min(freq_array) if len(freq_array) > 0 else 'N/A'
                freq_max = np.max(freq_array) if len(freq_array) > 0 else 'N/A'
                mag_min = np.min(mag_1d) if len(mag_1d) > 0 else 'N/A'
                mag_max = np.max(mag_1d) if len(mag_1d) > 0 else 'N/A'
                
                self.logger.info(f"\n📦 DATA EXAMPLE (Frame {frame_count}):")
                self.logger.info(f"   Frame Number: {frame_num}")
                self.logger.info(f"   Timestamp: {timestamp}")
                self.logger.info(f"   Frequency Range: {freq_min:.1f} to {freq_max:.1f} Hz")
                self.logger.info(f"   Magnitude Range: {mag_min:.1f} to {mag_max:.1f} dB")
                self.logger.info(f"   Peak Frequency: {peak_freq:.1f} Hz at {peak_mag:.1f} dB")
                self.logger.info(f"   Data Shape: {len(frequencies)} freq bins, {len(magnitude)} mag bins")
                
                # Show first few frequency and magnitude values
                if len(frequencies) > 0:
                    self.logger.info(f"   First 5 Frequencies: {frequencies[:5]}")
                if len(magnitude) > 0:
                    if isinstance(magnitude[0], list):
                        self.logger.info(f"   First 5 Magnitudes: {magnitude[0][:5] if len(magnitude[0]) > 0 else magnitude[0]}")
                    else:
                        self.logger.info(f"   First 5 Magnitudes: {magnitude[:5]}")
                
                self.logger.info("   " + "=" * 50)
                
        except Exception as e:
            self.logger.error(f"Error printing data example: {e}")

def main():
    """Main function."""
    parser = argparse.ArgumentParser(description='Spectrogram WebSocket Client')
    parser.add_argument('--host', type=str, default='localhost', 
                       help='WebSocket server host (default: localhost)')
    parser.add_argument('--port', type=int, default=40001, 
                       help='WebSocket server port (default: 40001)')
    parser.add_argument('--sample_rate', type=float, default=1e6, 
                                                help='Sample rate in Hz (default: 1 MHz)')
    
    args = parser.parse_args()
    
    print(f"Starting spectrogram client")
    print(f"  Server: {args.host}:{args.port}")
    print(f"  Sample rate: {args.sample_rate/1e6:.1f} MHz")
    
    # Create and run client
    client = SpectrogramClient(args.host, args.port, args.sample_rate)
    
    try:
        asyncio.run(client.run())
    except KeyboardInterrupt:
        print(f"\nClient stopped by user")
    except Exception as e:
        print(f"Client error: {e}")

if __name__ == "__main__":
    main()
