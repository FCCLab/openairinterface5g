#!/usr/bin/env node
/**
 * WebSocket client to test UE status updates
 */

const WebSocket = require('ws');

const ws = new WebSocket('ws://localhost:40000');

ws.on('open', function open() {
  console.log('🔌 Connected to WebSocket server');
  console.log('📊 Listening for UE status updates every second...\n');
});

ws.on('message', function message(data) {
  try {
    const parsed = JSON.parse(data);
    
    if (parsed.type === 'ue_status_update') {
      const { simulatedUE, realUE, timestamp } = parsed.data;
      
      console.log(`⏰ ${new Date(timestamp).toLocaleTimeString()}`);
      console.log(`📱 Simulated UE: ${simulatedUE.status} (${simulatedUE.isRunning ? 'Running' : 'Stopped'})`);
      console.log(`🔗 Real UE: ${realUE.status} (${realUE.isRunning ? 'Running' : 'Stopped'})`);
      
      if (simulatedUE.isRunning) {
        console.log(`   Process ID: ${simulatedUE.processId}`);
        console.log(`   Uptime: ${simulatedUE.uptime}s`);
        console.log(`   gRPC Port: ${simulatedUE.grpcPort}`);
      }
      console.log('---');
    } else if (parsed.type === 'connected') {
      console.log(`✅ ${parsed.message}`);
    }
  } catch (error) {
    console.log('📨 Raw message:', data.toString());
  }
});

ws.on('close', function close() {
  console.log('🔌 WebSocket connection closed');
});

ws.on('error', function error(err) {
  console.error('❌ WebSocket error:', err.message);
});

// Keep the process running
process.on('SIGINT', () => {
  console.log('\n👋 Closing WebSocket connection...');
  ws.close();
  process.exit(0);
});
