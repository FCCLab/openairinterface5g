#!/usr/bin/env python3
"""
Test WebSocket server functionality
"""

import asyncio
import websockets
import json
import time

async def test_websocket_client():
    """Test WebSocket client connection"""
    uri = "ws://localhost:40001"
    
    try:
        print(f"Connecting to {uri}...")
        async with websockets.connect(uri) as websocket:
            print("Connected successfully!")
            
            # Wait for some data
            print("Waiting for data...")
            for i in range(10):
                try:
                    message = await asyncio.wait_for(websocket.recv(), timeout=2.0)
                    data = json.loads(message)
                    print(f"Received frame {data['frame_num']} at {time.time()}")
                except asyncio.TimeoutError:
                    print(f"No data received in 2 seconds (attempt {i+1}/10)")
                except Exception as e:
                    print(f"Error receiving data: {e}")
                    break
                    
    except Exception as e:
        print(f"Connection failed: {e}")

if __name__ == "__main__":
    print("Testing WebSocket connection...")
    asyncio.run(test_websocket_client())
