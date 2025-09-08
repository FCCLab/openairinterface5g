const express = require('express');
const { exec } = require('child_process');
const fs = require('fs');
const fsPromises = require('fs').promises;
const path = require('path');
const logger = require('../logger');
const ueManager = require('../UeManager');
const { spawn } = require('child_process');

// gRPC imports
const grpc = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');

// Load protobuf definitions
const PROTO_PATH = path.join(__dirname, '..', '..', 'proto', 'ue_service.proto');
const packageDefinition = protoLoader.loadSync(PROTO_PATH, {
  keepCase: true,
  longs: String,
  enums: String,
  defaults: true,
  oneofs: true
});
const ueService = grpc.loadPackageDefinition(packageDefinition).ue_service;

const router = express.Router();

// Cell scan data (simulated for now)
let cellScanData = {
  isScanning: false,
  detectedCells: []
};

// Cell data retrieval interval
let cellRetrievalInterval = null;

// Start automatic cell data retrieval
const startCellDataRetrieval = async () => {
  if (cellRetrievalInterval) {
    logger.info('Cell data retrieval already running');
    return;
  }

  logger.info('Starting automatic cell data retrieval every 1 second');
  
  cellRetrievalInterval = setInterval(async () => {
    try {
      const manager = ueManager.getCurrentManager();
      const result = await manager.getDetectedCells();
      
      // Update the global cell scan data
      cellScanData.detectedCells = result.cells || [];
      
      logger.debug('Cell data retrieved', { 
        cellCount: cellScanData.detectedCells.length,
        timestamp: new Date().toISOString()
      });
      
    } catch (error) {
      logger.error('Error retrieving cell data', { error: error.message });
    }
  }, 1000); // Every 1 second
};

// Stop automatic cell data retrieval
const stopCellDataRetrieval = () => {
  if (cellRetrievalInterval) {
    logger.info('Stopping automatic cell data retrieval');
    clearInterval(cellRetrievalInterval);
    cellRetrievalInterval = null;
  }
};

// Old simulated UE code removed - now using unified interface

// Helper function to execute shell commands
const execCommand = (command, options = {}) => {
  return new Promise((resolve, reject) => {
    logger.debug(`Executing UE command: ${command}`);
    exec(command, { timeout: 10000, ...options }, (error, stdout, stderr) => {
      if (error) {
        logger.error(`UE command failed: ${command}`, { error: error.message, stderr });
        reject(error);
        return;
      }
      logger.debug(`UE command succeeded: ${command}`, { stdout: stdout.trim() });
      resolve(stdout.trim());
    });
  });
};

// Get UE status
const getUEStatus = () => {
  const manager = ueManager.getCurrentManager();
  const status = manager.getStatus();
  return {
    ...status,
    timestamp: new Date().toISOString()
  };
};








// Start continuous cell acquisition

// API Endpoints

// Get UE status
router.get('/status', (req, res) => {
  try {
    const status = getUEStatus();
    res.json(status);
  } catch (error) {
    logger.error('Error getting UE status', { error: error.message });
    res.status(500).json({ error: 'Failed to get UE status' });
  }
});

// Get UE console output (stdout/stderr)
router.get('/console', (req, res) => {
  try {
    const manager = ueManager.getCurrentManager();
    const lines = parseInt(req.query.lines) || 100; // Default to last 100 lines
    const consoleOutput = manager.getConsoleOutput(lines);
    res.json({
      success: true,
      output: consoleOutput,
      count: consoleOutput.length
    });
  } catch (error) {
    logger.error('Error getting UE console output', { error: error.message });
    res.status(500).json({ error: error.message });
  }
});

// Old simulated UE endpoints removed - now using unified interface

// Old simulated UE endpoints removed - now using unified interface

// Get UE configuration
router.get('/config', (req, res) => {
  try {
    const manager = ueManager.getCurrentManager();
    const config = manager.getConfig();
    res.json(config);
  } catch (error) {
    logger.error('Error getting UE config', { error: error.message });
    res.status(500).json({ error: 'Failed to get UE configuration' });
  }
});

// Update UE configuration
router.put('/config', (req, res) => {
  try {
    const newConfig = req.body;
    const manager = ueManager.getCurrentManager();
    const updatedConfig = manager.updateConfig(newConfig);
    res.json({ success: true, config: updatedConfig });
  } catch (error) {
    logger.error('Error updating UE config', { error: error.message });
    res.status(500).json({ error: 'Failed to update UE configuration' });
  }
});

// Start UE process with configurations
router.post('/start', async (req, res) => {
  try {
    const { mode, ...config } = req.body || {};
    
    // Validate mode if provided
    if (mode && mode !== 'simulated' && mode !== 'real') {
      return res.status(400).json({ error: 'Mode must be "simulated" or "real"' });
    }
    
    // Set mode if provided
    if (mode) {
      ueManager.setMode(mode);
    }
    
    logger.info('Starting UE process with configurations', { 
      mode: mode || ueManager.getMode(),
      hasAuth: !!config.authentication,
      hasNetwork: !!config.network,
      hasRadio: !!config.radio,
      frequency: config.radio?.freq || config.frequency,
      fullConfig: config,
      radioConfig: config.radio
    });
    
    const manager = ueManager.getCurrentManager();
    const result = await manager.start(config);
    res.json({
      ...result,
      mode: ueManager.getMode()
    });
  } catch (error) {
    logger.error('Error starting UE process', { error: error.message });
    res.status(500).json({ error: error.message });
  }
});

// Stop UE
router.post('/stop', async (req, res) => {
  try {
    const manager = ueManager.getCurrentManager();
    const result = await manager.stop();
    res.json(result);
  } catch (error) {
    logger.error('Error stopping UE', { error: error.message });
    res.status(500).json({ error: error.message });
  }
});

// Old simulated UE endpoints removed - now using unified interface

// Old simulated UE endpoints removed - now using unified interface

// Restart UE
router.post('/restart', async (req, res) => {
  try {
    const { mode, ...config } = req.body || {};
    
    // Validate mode if provided
    if (mode && mode !== 'simulated' && mode !== 'real') {
      return res.status(400).json({ error: 'Mode must be "simulated" or "real"' });
    }
    
    // Set mode if provided
    if (mode) {
      ueManager.setMode(mode);
    }
    
    const manager = ueManager.getCurrentManager();
    const result = await manager.restart(config);
    res.json({
      ...result,
      mode: ueManager.getMode()
    });
  } catch (error) {
    logger.error('Error restarting UE', { error: error.message });
    res.status(500).json({ error: error.message });
  }
});


// Start cell scan
router.post('/scan/start', async (req, res) => {
  try {
    const manager = ueManager.getCurrentManager();
    const result = await manager.startScan();
    
    // Start automatic cell data retrieval
    await startCellDataRetrieval();
    
    res.json(result);

  } catch (error) {
    logger.error('Error starting cell scan', { error: error.message });
    res.status(500).json({ error: error.message });
  }
});

// Stop cell scan
router.post('/scan/stop', async (req, res) => {
  try {
    const manager = ueManager.getCurrentManager();
    const result = await manager.stopScan();
    
    // Stop automatic cell data retrieval
    stopCellDataRetrieval();
    
    res.json(result);

  } catch (error) {
    logger.error('Error stopping cell scan', { error: error.message });
    res.status(500).json({ error: error.message });
  }
});

// Get cell scan status
router.get('/scan/status', (req, res) => {
  try {
    const manager = ueManager.getCurrentManager();
    const scanStatus = manager.getScanStatus();
    
    // Add cell data retrieval status
    const status = {
      ...scanStatus,
      cellDataRetrieval: {
        isActive: cellRetrievalInterval !== null,
        interval: '1 second'
      }
    };
    
    res.json(status);
  } catch (error) {
    logger.error('Error getting cell scan status', { error: error.message });
    res.status(500).json({ error: 'Failed to get cell scan status' });
  }
});

// Get detected cells
router.get('/scan/cells', async (req, res) => {
  try {
    const manager = ueManager.getCurrentManager();
    const result = await manager.getDetectedCells();
    res.json(result);

  } catch (error) {
    logger.error('Error getting detected cells', { error: error.message });
    res.status(500).json({ error: error.message });
  }
});

// Mode switching removed - mode is now specified in the start request

// Get singleton manager status
router.get('/manager/status', (req, res) => {
  try {
    const managerStatus = ueManager.getManagerStatus();
    res.json({ 
      success: true, 
      managerStatus: managerStatus
    });
  } catch (error) {
    logger.error('Error getting manager status:', error);
    res.status(500).json({
      success: false,
      error: 'Failed to get manager status',
      message: error.message
    });
  }
});

// Reinitialize singleton manager (for debugging)
router.post('/manager/reinitialize', (req, res) => {
  try {
    ueManager.reinitialize();
    res.json({
      success: true,
      message: 'Singleton UE Manager reinitialized successfully'
    });
  } catch (error) {
    logger.error('Error reinitializing manager:', error);
    res.status(500).json({
      success: false,
      error: 'Failed to reinitialize manager',
      message: error.message
    });
  }
});




// Get UE performance metrics
router.get('/metrics', (req, res) => {
  try {
    const manager = ueManager.getCurrentManager();
    const processMetrics = manager.getMetrics();
    const acquisitionStatus = getCellAcquisitionStatus();
    const metrics = {
      ...processMetrics,
      cellsDetected: cellScanData.detectedCells.length,
      scanInProgress: cellScanData.isScanning,
      acquisitionActive: acquisitionStatus.isAcquiring,
      acquisitionUpdateCount: acquisitionStatus.updateCount,
      lastAcquisitionUpdate: acquisitionStatus.lastUpdate
    };

    res.json(metrics);
  } catch (error) {
    logger.error('Error getting UE metrics', { error: error.message });
    res.status(500).json({ error: 'Failed to get UE metrics' });
  }
});

// Make status function available to app
module.exports = router;
module.exports.getCurrentUEStatus = getUEStatus;
