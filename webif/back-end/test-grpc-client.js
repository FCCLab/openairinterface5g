const grpc = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');
const path = require('path');

// Load proto file
const protoPath = path.join(__dirname, 'proto', 'ue_service.proto');
const packageDefinition = protoLoader.loadSync(protoPath, {
  keepCase: true,
  longs: String,
  enums: String,
  defaults: true,
  oneofs: true
});

const ueService = grpc.loadPackageDefinition(packageDefinition).ue_service;

// Create gRPC client
const client = new ueService.UEControlService('localhost:50051', grpc.credentials.createInsecure());
const dataClient = new ueService.UEDataService('localhost:50051', grpc.credentials.createInsecure());

async function testGrpcClient() {
  console.log('Testing gRPC client for Simulated UE Process...\n');

  try {
    // Test 1: Get Status
    console.log('1. Testing GetStatus...');
    const statusResponse = await new Promise((resolve, reject) => {
      client.GetStatus({ process_id: 'test' }, (err, response) => {
        if (err) reject(err);
        else resolve(response);
      });
    });
    console.log('Status Response:', statusResponse);
    console.log('');

    // Test 2: Start UE
    console.log('2. Testing StartUE...');
    const startResponse = await new Promise((resolve, reject) => {
      client.StartUE({
        frequency: '3425010000',
        config: {
          frequency: '3425010000',
          auth: {
            imsi: '001010000000003',
            key: 'fec86ba6eb707ed08905757b1bb44b8f',
            opc: 'c42449363bbad02b66d16bc975d77cc1'
          },
          network: {
            mcc: '001',
            mnc: '01',
            tac: '1'
          },
          radio: {
            frequency: '3425010000',
            bandwidth: '100',
            power: '23'
          }
        }
      }, (err, response) => {
        if (err) reject(err);
        else resolve(response);
      });
    });
    console.log('Start Response:', startResponse);
    console.log('');

    // Test 3: Get Current Cells
    console.log('3. Testing GetCurrentCells...');
    const cellsResponse = await new Promise((resolve, reject) => {
      dataClient.GetCurrentCells({ process_id: 'test' }, (err, response) => {
        if (err) reject(err);
        else resolve(response);
      });
    });
    console.log('Cells Response:', JSON.stringify(cellsResponse, null, 2));
    console.log('');

    // Test 4: Stream Cell Data (for 10 seconds)
    console.log('4. Testing StreamCellData (10 seconds)...');
    const streamCall = dataClient.StreamCellData({ 
      process_id: 'test',
      continuous: true 
    });

    let cellDataCount = 0;
    const streamTimeout = setTimeout(() => {
      streamCall.cancel();
      console.log(`Received ${cellDataCount} cell data updates\n`);
    }, 10000);

    streamCall.on('data', (data) => {
      cellDataCount++;
      console.log(`Cell Data Update #${cellDataCount}:`, {
        timestamp: new Date(data.timestamp).toISOString(),
        cellCount: data.cells.length,
        strongestPCI: data.cells[0]?.pci || 'N/A'
      });
    });

    streamCall.on('end', () => {
      clearTimeout(streamTimeout);
      console.log('Stream ended\n');
    });

    streamCall.on('error', (err) => {
      clearTimeout(streamTimeout);
      console.error('Stream error:', err.message);
    });

    // Test 5: Stream Logs (for 5 seconds)
    console.log('5. Testing StreamLogs (5 seconds)...');
    const logStreamCall = dataClient.StreamLogs({ 
      process_id: 'test',
      level: 'INFO'
    });

    let logCount = 0;
    const logTimeout = setTimeout(() => {
      logStreamCall.cancel();
      console.log(`Received ${logCount} log entries\n`);
    }, 5000);

    logStreamCall.on('data', (log) => {
      logCount++;
      console.log(`Log #${logCount}: [${log.level}] ${log.message}`);
    });

    logStreamCall.on('end', () => {
      clearTimeout(logTimeout);
      console.log('Log stream ended\n');
    });

    logStreamCall.on('error', (err) => {
      clearTimeout(logTimeout);
      console.error('Log stream error:', err.message);
    });

    // Test 6: Stop UE
    setTimeout(async () => {
      console.log('6. Testing StopUE...');
      const stopResponse = await new Promise((resolve, reject) => {
        client.StopUE({ process_id: 'test' }, (err, response) => {
          if (err) reject(err);
          else resolve(response);
        });
      });
      console.log('Stop Response:', stopResponse);
      console.log('\nAll tests completed!');
    }, 15000);

  } catch (error) {
    console.error('Test error:', error.message);
  }
}

// Run the test
testGrpcClient().catch(console.error);
