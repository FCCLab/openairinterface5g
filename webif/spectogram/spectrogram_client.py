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
from collections import deque

class SpectrogramClient:
    def __init__(self, host='localhost', port=40001, max_frames=100, sample_rate=10e6):
        """
        Initialize the spectrogram client.
        
        Args:
            host: WebSocket server host
            port: WebSocket server port
            max_frames: Maximum number of frames to keep in memory
            sample_rate: Sample rate in Hz (default: 10 MHz)
        """
        self.host = host
        self.port = port
        self.websocket = None
        self.connected = False
        self.data_queue = deque(maxlen=max_frames)
        self.latest_data = None
        self.sample_rate = sample_rate
        
        # Create frequency map from -sample_rate/2 to +sample_rate/2
        self.freq_min = -sample_rate / 2
        self.freq_max = sample_rate / 2
        self.freq_map_size = 1024  # Number of frequency bins
        self.freq_step = sample_rate / self.freq_map_size
        
        # Initialize frequency-magnitude map
        self.freq_mag_map = np.full(self.freq_map_size, -120.0)  # Initialize with noise floor
        self.freq_map = np.linspace(self.freq_min, self.freq_max, self.freq_map_size)
        
        print(f"00 spectrogram_client.py: Created frequency map: {self.freq_min/1e6:.1f} to {self.freq_max/1e6:.1f} MHz ({self.freq_map_size} bins)")
        print(f"00 spectrogram_client.py: Frequency step: {self.freq_step/1e3:.1f} kHz per bin")
        
    async def connect(self):
        """Connect to the WebSocket server."""
        try:
            uri = f"ws://{self.host}:{self.port}"
            print(f"00 spectrogram_client.py: Connecting to {uri}")
            self.websocket = await websockets.connect(uri)
            self.connected = True
            print(f"00 spectrogram_client.py: Connected to spectrogram server")
            return True
        except Exception as e:
            print(f"00 spectrogram_client.py: Failed to connect: {e}")
            return False
            
    async def disconnect(self):
        """Disconnect from the WebSocket server."""
        if self.websocket:
            await self.websocket.close()
            self.connected = False
            print(f"00 spectrogram_client.py: Disconnected from server")
            
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
                    # Process all received data as spectrogram data (no type check needed)
                    
                    # Store packet info every 10 packets for display
                    if frame_count % 10 == 0:
                        frequencies = data.get('frequencies', [])
                        magnitude = data.get('magnitude_db', [])
                        self.last_packet_info = {
                            'frame_num': data.get('frame_num'),
                            'timestamp': data.get('timestamp'),
                            'frequencies': frequencies,
                            'times': data.get('times', []),
                            'magnitude': magnitude,
                            'frequencies_shape': f"{len(frequencies)}",
                            'times_shape': f"{len(data.get('times', []))}",
                            'magnitude_shape': f"{len(magnitude)}x{len(magnitude[0]) if isinstance(magnitude, list) and len(magnitude) > 0 and isinstance(magnitude[0], list) else 1}"
                        }
                    
                    self.process_spectrogram_data(data)
                except json.JSONDecodeError as e:
                    print(f"00 spectrogram_client.py: Invalid JSON received: {e}")
                except Exception as e:
                    print(f"00 spectrogram_client.py: Error processing message: {e}")
        except websockets.exceptions.ConnectionClosed:
            print(f"00 spectrogram_client.py: Connection closed by server")
            self.connected = False
        except Exception as e:
            print(f"00 spectrogram_client.py: Error receiving data: {e}")
            self.connected = False
            
    def process_spectrogram_data(self, data):
        """
        Process received spectrogram data and draw ASCII spectrogram.
        
        Args:
            data: Dictionary containing spectrogram data
        """
        try:
            # Extract data
            frame = data.get('frame_num', 0)
            timestamp = data.get('timestamp', time.time())
            frequencies = np.array(data.get('frequencies', []))
            times = np.array(data.get('times', []))
            magnitude = np.array(data.get('magnitude_db', []))
            
            # Use frequencies in Hz (no conversion)
            frequencies_hz = frequencies
            freq_max = np.max(np.abs(frequencies))
            print(f"00 spectrogram_client.py: Using frequencies in Hz: {freq_max:.6f} Hz max")
            
            # Store data
            self.latest_data = {
                'frame': frame,
                'timestamp': timestamp,
                'frequencies': frequencies_hz,
                'times': times,
                'magnitude': magnitude
            }
            
            self.data_queue.append(self.latest_data)
            
            # Update frequency-magnitude map with received data
            self.update_freq_mag_map(frequencies_hz, magnitude)
            
            # Draw ASCII spectrogram
            self.draw_ascii_spectrogram(frame)
            
            # Print summary info
            if frame % 10 == 0:  # Print every 10th frame to avoid spam
                print(f"00 spectrogram_client.py: Frame {frame} at {timestamp:.3f}s")
                print(f"  Frequencies: {len(frequencies)} bins ({frequencies_hz[0]:.3f} to {frequencies_hz[-1]:.3f} Hz)")
                print(f"  Magnitude: {magnitude.shape} (min: {np.min(magnitude):.1f} dB, max: {np.max(magnitude):.1f} dB)")
                
                # Find peak frequency and amplitude
                max_idx = np.unravel_index(np.argmax(magnitude), magnitude.shape)
                peak_freq = frequencies_hz[max_idx[0]]
                peak_amplitude = magnitude[max_idx]
                print(f"  Peak: {peak_freq:.3f} Hz at {peak_amplitude:.1f} dB")
                print("  " + "-" * 50)
                      
        except Exception as e:
            print(f"00 spectrogram_client.py: Error processing spectrogram data: {e}")
    
    def update_freq_mag_map(self, frequencies, magnitude):
        """
        Update the frequency-magnitude map with received data.
        
        Args:
            frequencies: Frequency array from received data
            magnitude: Magnitude array from received data
        """
        try:
            # Convert magnitude to 1D if 2D
            if magnitude.ndim == 2:
                mag_1d = magnitude[:, -1] if magnitude.shape[1] > 0 else magnitude[:, 0]
            else:
                mag_1d = magnitude
            
            print(f"00 spectrogram_client.py: Updating map with {len(frequencies)} frequency points")
            print(f"00 spectrogram_client.py: Frequency range: {frequencies[0]:.1f} to {frequencies[-1]:.1f} Hz")
            print(f"00 spectrogram_client.py: Magnitude range: {np.min(mag_1d):.1f} to {np.max(mag_1d):.1f} dB")
            
            # Map received frequencies to our frequency map
            updated_count = 0
            out_of_range_count = 0
            
            for i, freq in enumerate(frequencies):
                # Find the closest bin in our frequency map
                bin_idx = int((freq - self.freq_min) / self.freq_step)
                
                # Ensure bin index is within bounds
                if 0 <= bin_idx < self.freq_map_size:
                    # Update the magnitude at this frequency bin
                    self.freq_mag_map[bin_idx] = mag_1d[i]
                    updated_count += 1
                else:
                    out_of_range_count += 1
                    if out_of_range_count <= 5:  # Show first few out-of-range frequencies
                        print(f"00 spectrogram_client.py: Frequency {freq:.1f} Hz out of range (bin {bin_idx})")
            
            print(f"00 spectrogram_client.py: Updated {updated_count} bins, {out_of_range_count} out of range")
            print(f"00 spectrogram_client.py: Map magnitude range: {np.min(self.freq_mag_map):.1f} to {np.max(self.freq_mag_map):.1f} dB")
            
            # Apply some smoothing/decay to prevent sharp transitions
            # Simple exponential decay: new_value = 0.9 * old_value + 0.1 * new_value
            # But only apply to bins that weren't updated
            mask = np.ones(self.freq_map_size, dtype=bool)
            for i, freq in enumerate(frequencies):
                bin_idx = int((freq - self.freq_min) / self.freq_step)
                if 0 <= bin_idx < self.freq_map_size:
                    mask[bin_idx] = False
            
            # Apply decay only to non-updated bins
            self.freq_mag_map[mask] = 0.9 * self.freq_mag_map[mask] + 0.1 * (-120.0)
            
        except Exception as e:
            print(f"00 spectrogram_client.py: Error updating frequency map: {e}")
            import traceback
            traceback.print_exc()
    
    def draw_ascii_spectrogram(self, frame):
        """
        Draw ASCII spectrogram in console using the frequency-magnitude map.
        
        Args:
            frame: Frame number
        """
        try:
            # Clear screen and move cursor to top
            print("\033[2J\033[H", end="")
            
            # Print packet info at the top if available (updated every 10 packets)
            if hasattr(self, 'last_packet_info') and self.last_packet_info:
                print("📦 LAST PACKET INFO (updated every 10 packets):")
                print(f"   Frame: {self.last_packet_info.get('frame_num', 'N/A')}")
                print(f"   Timestamp: {self.last_packet_info.get('timestamp', 'N/A')}")
                print(f"   Frequencies: {self.last_packet_info.get('frequencies_shape', 'N/A')} bins")
                print(f"   Times: {self.last_packet_info.get('times_shape', 'N/A')} points")
                print(f"   Magnitude: {self.last_packet_info.get('magnitude_shape', 'N/A')}")
                print("  " + "=" * 50)
            else:
                print("DEBUG: No packet info available for display")
                print(f"DEBUG: hasattr(self, 'last_packet_info'): {hasattr(self, 'last_packet_info')}")
                if hasattr(self, 'last_packet_info'):
                    print(f"DEBUG: self.last_packet_info: {self.last_packet_info}")
            
            # Get terminal size
            import os
            try:
                terminal_width = os.get_terminal_size().columns
                terminal_height = os.get_terminal_size().lines
            except:
                terminal_width = 80
                terminal_height = 24
            
            # Determine display dimensions
            display_width = min(terminal_width - 20, 60)  # Leave space for labels
            display_height = min(terminal_height - 10, 20)  # Leave space for header/footer
            
            # Use the frequency-magnitude map
            frequencies = self.freq_map
            mag_1d = self.freq_mag_map
            
            # Downsample to fit display
            if len(mag_1d) > display_width:
                step = len(mag_1d) // display_width
                mag_1d = mag_1d[::step][:display_width]
                frequencies = frequencies[::step][:display_width]
            
            # Normalize magnitude to 0-1 range
            mag_min = np.min(mag_1d)
            mag_max = np.max(mag_1d)
            if mag_max > mag_min:
                mag_norm = (mag_1d - mag_min) / (mag_max - mag_min)
            else:
                mag_norm = np.zeros_like(mag_1d)
            
            # ASCII characters for intensity (from dark to bright)
            ascii_chars = " .:-=+*#%@"
            
            # Draw header
            print(f"🎵 ASCII Spectrogram - Frame {frame}")
            print(f"📡 Frequency Range: {frequencies[0]:.2f} to {frequencies[-1]:.2f} Hz")
            print(f"📊 Magnitude Range: {mag_min:.1f} to {mag_max:.1f} dB")
            print("=" * (display_width + 20))
            
            # Draw spectrogram
            for i in range(display_height):
                # Calculate threshold for this row (bottom to top)
                threshold = 1.0 - (i / display_height)
                
                # Draw row
                row = ""
                for j, mag in enumerate(mag_norm):
                    if mag >= threshold:
                        # Calculate character index
                        char_idx = int(mag * (len(ascii_chars) - 1))
                        char_idx = min(char_idx, len(ascii_chars) - 1)
                        row += ascii_chars[char_idx]
                    else:
                        row += " "
                
                # Add amplitude label on the right
                if i == 0:
                    row += f"  {mag_max:.0f} dB"
                elif i == display_height // 2:
                    row += f"  {(mag_max + mag_min) / 2:.0f} dB"
                elif i == display_height - 1:
                    row += f"  {mag_min:.0f} dB"
                else:
                    row += "     "
                
                print(row)
            
            # Draw frequency axis
            freq_axis = ""
            for j in range(display_width):
                if j == 0 or j == display_width // 2 or j == display_width - 1:
                    freq_axis += "|"
                else:
                    freq_axis += " "
            
            print("-" * display_width)
            print(freq_axis)
            
            # Draw frequency labels
            freq_labels = ""
            for j in range(display_width):
                if j == 0:
                    freq_labels += f"{frequencies[0]:.1f}"
                elif j == display_width // 2:
                    mid_freq = frequencies[len(frequencies) // 2]
                    freq_labels += f"{mid_freq:.1f}"
                elif j == display_width - 1:
                    freq_labels += f"{frequencies[-1]:.1f}"
                else:
                    freq_labels += " "
            
            # Ensure 0 Hz is properly marked at center
            center_pos = display_width // 2
            if center_pos < len(freq_labels):
                # Find the actual center frequency
                center_freq = frequencies[len(frequencies) // 2]
                if abs(center_freq) < 0.1:  # If close to 0, mark as 0
                    freq_labels = freq_labels[:center_pos] + "0" + freq_labels[center_pos+1:]
            
            print(freq_labels)
            print(f"Hz{' ' * (display_width - 2)}")
            
            # Draw legend
            print("\nLegend:")
            for i, char in enumerate(ascii_chars):
                intensity = i / (len(ascii_chars) - 1)
                db_value = mag_min + intensity * (mag_max - mag_min)
                print(f"'{char}' = {db_value:.0f} dB", end="  ")
                if (i + 1) % 5 == 0:
                    print()
            print()
            
        except Exception as e:
            print(f"00 spectrogram_client.py: Error drawing ASCII spectrogram: {e}")
            
    def print_summary(self):
        """Print summary of received data."""
        if self.data_queue:
            print(f"00 spectrogram_client.py: Data summary:")
            print(f"  Total frames received: {len(self.data_queue)}")
            print(f"  Latest frame: {self.latest_data['frame']}")
            print(f"  Time span: {self.latest_data['timestamp'] - self.data_queue[0]['timestamp']:.3f} seconds")
            print(f"  Average frame rate: {len(self.data_queue) / (self.latest_data['timestamp'] - self.data_queue[0]['timestamp']):.2f} fps")
        
    async def run(self):
        """Main client loop."""
        if not await self.connect():
            return
            
        try:
            print(f"00 spectrogram_client.py: Receiving spectrogram data...")
            print(f"00 spectrogram_client.py: Press Ctrl+C to stop")
            print("  " + "=" * 50)
            
            # Receive data
            await self.receive_data()
            
        except KeyboardInterrupt:
            print(f"\n00 spectrogram_client.py: Received interrupt signal")
            self.print_summary()
        except Exception as e:
            print(f"00 spectrogram_client.py: Error in main loop: {e}")
        finally:
            await self.disconnect()

def main():
    """Main function."""
    parser = argparse.ArgumentParser(description='Spectrogram WebSocket Client')
    parser.add_argument('--host', type=str, default='localhost', 
                       help='WebSocket server host (default: localhost)')
    parser.add_argument('--port', type=int, default=40001, 
                       help='WebSocket server port (default: 40001)')
    parser.add_argument('--max_frames', type=int, default=100, 
                       help='Maximum frames to keep in memory (default: 100)')
    parser.add_argument('--sample_rate', type=float, default=10e6, 
                       help='Sample rate in Hz (default: 10 MHz)')
    
    args = parser.parse_args()
    
    print(f"00 spectrogram_client.py: Starting spectrogram client")
    print(f"  Server: {args.host}:{args.port}")
    print(f"  Max frames: {args.max_frames}")
    print(f"  Sample rate: {args.sample_rate/1e6:.1f} MHz")
    
    # Create and run client
    client = SpectrogramClient(args.host, args.port, args.max_frames, args.sample_rate)
    
    try:
        asyncio.run(client.run())
    except KeyboardInterrupt:
        print(f"\n00 spectrogram_client.py: Client stopped by user")
    except Exception as e:
        print(f"00 spectrogram_client.py: Client error: {e}")

if __name__ == "__main__":
    main()
