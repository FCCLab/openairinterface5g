#!/usr/bin/env python3
"""
Minimal WebSocket client test
"""

import asyncio
import websockets
import logging

# Setup basic logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

async def test_client():
    """Test WebSocket client"""
    uri = "ws://localhost:40002"
    logger.info(f"Connecting to {uri}...")
    
    try:
        async with websockets.connect(uri) as websocket:
            logger.info("Connected successfully!")
            
            # Send a test message
            await websocket.send("Hello, WebSocket!")
            logger.info("Sent: Hello, WebSocket!")
            
            # Receive response
            response = await websocket.recv()
            logger.info(f"Received: {response}")
            
            # Wait a bit
            await asyncio.sleep(2)
            
    except Exception as e:
        logger.error(f"Connection failed: {e}")

if __name__ == "__main__":
    try:
        asyncio.run(test_client())
    except KeyboardInterrupt:
        logger.info("Client stopped by user")
    except Exception as e:
        logger.error(f"Client error: {e}")
