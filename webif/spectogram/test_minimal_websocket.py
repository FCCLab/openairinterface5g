#!/usr/bin/env python3
"""
Minimal WebSocket server test to isolate the connection issue
"""

import asyncio
import websockets
import logging

# Setup basic logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

async def handler(websocket, path):
    """Simple WebSocket handler"""
    logger.info(f"Client connected from {websocket.remote_address}")
    try:
        async for message in websocket:
            logger.info(f"Received: {message}")
            await websocket.send(f"Echo: {message}")
    except websockets.exceptions.ConnectionClosed:
        logger.info("Client disconnected")
    finally:
        logger.info("Handler finished")

async def main():
    """Main server function"""
    logger.info("Starting minimal WebSocket server on port 40002...")
    
    # Start server
    server = await websockets.serve(handler, "0.0.0.0", 40002)
    logger.info("Server started successfully")
    
    # Keep server running
    await server.wait_closed()

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        logger.info("Server stopped by user")
    except Exception as e:
        logger.error(f"Server error: {e}")
