#!/usr/bin/env node

const axios = require('axios');

const BASE_URL = 'http://10.1.100.143:40000/api/ue';

async function testUEAPI() {
  console.log('🧪 Testing UE API endpoints...\n');

  try {
    // Test 1: Get UE Status
    console.log('1. Testing GET /status');
    const statusResponse = await axios.get(`${BASE_URL}/status`);
    console.log('✅ Status:', statusResponse.data);
    console.log('');

    // Test 2: Get UE Configuration
    console.log('2. Testing GET /config');
    const configResponse = await axios.get(`${BASE_URL}/config`);
    console.log('✅ Config:', configResponse.data);
    console.log('');

    // Test 3: Update UE Configuration
    console.log('3. Testing PUT /config');
    const newConfig = {
      frequency: 3500000000,
      imsi: '001010000000004'
    };
    const updateResponse = await axios.put(`${BASE_URL}/config`, newConfig);
    console.log('✅ Config Updated:', updateResponse.data);
    console.log('');

    // Test 4: Get Cell Scan Status
    console.log('4. Testing GET /scan/status');
    const scanStatusResponse = await axios.get(`${BASE_URL}/scan/status`);
    console.log('✅ Scan Status:', scanStatusResponse.data);
    console.log('');

    // Test 5: Start Simulated UE Process
    console.log('5. Testing POST /simulated/start');
    const simulatedStartResponse = await axios.post(`${BASE_URL}/simulated/start`, {
      frequency: 3500000000
    });
    console.log('✅ Simulated UE Started:', {
      success: simulatedStartResponse.data.success,
      message: simulatedStartResponse.data.message,
      frequency: simulatedStartResponse.data.frequency,
      processId: simulatedStartResponse.data.processId
    });
    console.log('');

    // Test 6: Get Simulated UE Status
    console.log('6. Testing GET /simulated/status');
    const simulatedStatusResponse = await axios.get(`${BASE_URL}/simulated/status`);
    console.log('✅ Simulated UE Status:', {
      isRunning: simulatedStatusResponse.data.isRunning,
      processId: simulatedStatusResponse.data.processId,
      connectionState: simulatedStatusResponse.data.connectionState,
      uptime: simulatedStatusResponse.data.uptime ? `${Math.round(simulatedStatusResponse.data.uptime / 1000)}s` : '0s'
    });
    console.log('');

    // Test 7: Get Simulated UE Logs
    console.log('7. Testing GET /simulated/logs');
    const simulatedLogsResponse = await axios.get(`${BASE_URL}/simulated/logs?lines=5`);
    console.log('✅ Simulated UE Logs:', {
      totalLogs: simulatedLogsResponse.data.totalLogs,
      recentLogs: simulatedLogsResponse.data.logs.slice(-3).map(log => ({
        time: log.timestamp,
        level: log.level,
        message: log.message
      }))
    });
    console.log('');

    // Test 8: Stop Simulated UE Process
    console.log('8. Testing POST /simulated/stop');
    const simulatedStopResponse = await axios.post(`${BASE_URL}/simulated/stop`);
    console.log('✅ Simulated UE Stopped:', {
      success: simulatedStopResponse.data.success,
      message: simulatedStopResponse.data.message,
      uptime: simulatedStopResponse.data.uptime ? `${Math.round(simulatedStopResponse.data.uptime / 1000)}s` : '0s'
    });
    console.log('');

    // Test 9: Start UE Process with Configurations
    console.log('9. Testing POST /start with configurations');
    const configData = {
      frequency: 3500000000,
      authentication: {
        imsi: '001010000000004',
        key: '00112233445566778899aabbccddeeff',
        opc: '00112233445566778899aabbccddeeff'
      },
      network: {
        dnn: 'internet',
        nssai_sst: 1
      },
      radio: {
        bandwidth: 100,
        numerology: 0
      }
    };
    const ueStartResponse = await axios.post(`${BASE_URL}/start`, configData);
    console.log('✅ UE Process Started with Config:', {
      success: ueStartResponse.data.success,
      message: ueStartResponse.data.message,
      hasAuth: !!ueStartResponse.data.config?.authentication,
      hasNetwork: !!ueStartResponse.data.config?.network,
      hasRadio: !!ueStartResponse.data.config?.radio
    });
    console.log('');

    // Test 10: Start Cell Scan
    console.log('10. Testing POST /synthetic/start - REMOVED (handled by UE process)');
    console.log('');

    console.log('11. Testing POST /scan/start');
    const scanStartResponse = await axios.post(`${BASE_URL}/scan/start`, {
      centerFrequency: 3500000000
    });
    console.log('✅ Scan Started:', {
      success: scanStartResponse.data.success,
      cellsFound: scanStartResponse.data.cellsFound,
      centerFrequency: scanStartResponse.data.centerFrequency,
      sampleCell: scanStartResponse.data.cells?.[0] ? {
        pci: scanStartResponse.data.cells[0].pci,
        ssRsrp: scanStartResponse.data.cells[0].ssRsrp,
        pss: scanStartResponse.data.cells[0].pss,
        sss: scanStartResponse.data.cells[0].sss
      } : 'No cells found'
    });
    console.log('');

    // Test 12: Get Detected Cells
    console.log('12. Testing GET /scan/cells');
    const cellsResponse = await axios.get(`${BASE_URL}/scan/cells`);
    console.log('✅ Detected Cells:', cellsResponse.data);
    console.log('');

    // Test 13: Get UE Metrics
    console.log('13. Testing GET /metrics');
    const metricsResponse = await axios.get(`${BASE_URL}/metrics`);
    console.log('✅ Metrics:', metricsResponse.data);
    console.log('');

    // Test 14: Get UE Logs
    console.log('14. Testing GET /logs');
    const logsResponse = await axios.get(`${BASE_URL}/logs?lines=10`);
    console.log('✅ Logs:', logsResponse.data);
    console.log('');

    // Test 15: Multiple Cell Scans (demonstrate variety)
    console.log('15. Testing multiple cell scans for variety');
    const frequencies = [3400000000, 3600000000, 3800000000];
    for (let i = 0; i < frequencies.length; i++) {
      const freq = frequencies[i];
      console.log(`   Scan ${i + 1} at ${freq} Hz:`);
      const multiScanResponse = await axios.post(`${BASE_URL}/scan/start`, {
        centerFrequency: freq
      });
      console.log(`   ✅ Found ${multiScanResponse.data.cellsFound} cells, strongest PCI: ${multiScanResponse.data.cells?.[0]?.pci || 'none'}`);
    }
    console.log('');

    // Test 11: Cell Acquisition
    console.log('11. Testing cell acquisition functionality');
    console.log('   Starting acquisition...');
    const acquireStartResponse = await axios.post(`${BASE_URL}/acquire/start`, {
      centerFrequency: 3500000000
    });
    console.log('   ✅ Acquisition started:', acquireStartResponse.data);
    
    // Wait a bit to see some updates
    await new Promise(resolve => setTimeout(resolve, 3000));
    
    // Check acquisition status
    const acquireStatusResponse = await axios.get(`${BASE_URL}/acquire/status`);
    console.log('   ✅ Acquisition status:', acquireStatusResponse.data);
    
    // Stop acquisition
    const acquireStopResponse = await axios.post(`${BASE_URL}/acquire/stop`);
    console.log('   ✅ Acquisition stopped:', acquireStopResponse.data);
    console.log('');

    console.log('🎉 All UE API tests passed!');

  } catch (error) {
    console.error('❌ Test failed:', error.response?.data || error.message);
    process.exit(1);
  }
}

// Run tests if this script is executed directly
if (require.main === module) {
  testUEAPI();
}

module.exports = testUEAPI;
