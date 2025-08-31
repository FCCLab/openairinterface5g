const express = require('express');
const { spawn } = require('child_process');
const path = require('path');
const logger = require('../logger');

const router = express.Router();

// Spectrogram Service Management
let spectrogramServiceProcess = null;
let spectrogramServiceStatus = 'stopped';

// Spectrogram streaming state
let spectrogramStreams = new Map(); // Map to track active streams
let spectrogramInterval = null; // Interval for generating data

// Store connected clients for server-push
let connectedClients = new Map();

// Generate spectrogram data and push to all connected clients
const generateSpectrogramData = () => {
  try {
    // Get the current device serial number if available
    const currentDeviceSerial = spectrogramStreams.get('currentDeviceSerial');
    
    logger.info('Generating spectrogram data', { 
      activeStreams: spectrogramStreams.size,
      connectedClients: connectedClients.size,
      hasInterval: !!spectrogramInterval,
      deviceSerialNumber: currentDeviceSerial || 'none'
    });
    
    // Generate realistic spectrogram data
    const fftSize = 1024;
    const data = new Array(fftSize / 2);
    
    // Base noise floor
    for (let i = 0; i < data.length; i++) {
      data[i] = Math.random() * 10 + 5; // 5-15 dB noise floor
    }
    
    // Add some frequency peaks to simulate signals
    const time = Date.now() / 1000;
    
    // Peak 1: Moving frequency
    const peak1Freq = Math.floor(50 + 30 * Math.sin(time * 0.5));
    const peak1Amp = 60 + 20 * Math.sin(time * 2);
    if (peak1Freq >= 0 && peak1Freq < data.length) {
      data[peak1Freq] = peak1Amp;
      // Add some spread around the peak
      for (let i = Math.max(0, peak1Freq - 2); i <= Math.min(data.length - 1, peak1Freq + 2); i++) {
        const distance = Math.abs(i - peak1Freq);
        data[i] = Math.max(data[i], peak1Amp * Math.exp(-distance * 0.5));
      }
    }
    
    // Peak 2: Fixed frequency with varying amplitude
    const peak2Freq = 150;
    const peak2Amp = 40 + 30 * Math.sin(time * 1.5);
    if (peak2Freq < data.length) {
      data[peak2Freq] = peak2Amp;
      for (let i = Math.max(0, peak2Freq - 1); i <= Math.min(data.length - 1, peak2Freq + 1); i++) {
        const distance = Math.abs(i - peak2Freq);
        data[i] = Math.max(data[i], peak2Amp * Math.exp(-distance * 1.0));
      }
    }
    
    // Peak 3: High frequency burst
    if (Math.sin(time * 3) > 0.8) {
      const peak3Freq = 300 + Math.floor(Math.random() * 50);
      const peak3Amp = 70 + Math.random() * 20;
      if (peak3Freq < data.length) {
        data[peak3Freq] = peak3Amp;
        for (let i = Math.max(0, peak3Freq - 1); i <= Math.min(data.length - 1, peak3Freq + 1); i++) {
          const distance = Math.abs(i - peak3Freq);
          data[i] = Math.max(data[i], peak3Amp * Math.exp(-distance * 1.0));
        }
      }
    }
    
    const spectrogramData = {
      timestamp: Date.now(),
      frequency: 2400,
      sampleRate: 44100,
      fftSize: fftSize,
      data: data,
      deviceSerialNumber: currentDeviceSerial || null
    };
    
    // Send data to all active SSE streams
    let sentCount = 0;
    spectrogramStreams.forEach((res, clientId) => {
      try {
        res.write(`data: ${JSON.stringify(spectrogramData)}\n\n`);
        sentCount++;
      } catch (error) {
        logger.warn('Failed to send data to SSE client', { clientId, error: error.message });
        spectrogramStreams.delete(clientId);
      }
    });
    
    // Send data to all connected HTTP clients (server-push)
    connectedClients.forEach((clientInfo, clientId) => {
      try {
        // Store the latest data for this client
        clientInfo.latestData = spectrogramData;
        sentCount++;
      } catch (error) {
        logger.warn('Failed to store data for HTTP client', { clientId, error: error.message });
        connectedClients.delete(clientId);
      }
    });
    
    logger.info('Spectrogram data generated and sent', { 
      activeStreams: spectrogramStreams.size,
      connectedClients: connectedClients.size,
      sentToClients: sentCount,
      dataPoints: data.length 
    });
  } catch (error) {
    logger.error('Error generating spectrogram data', { error: error.message });
  }
};

// Start spectrogram service
router.post('/service/start', async (req, res) => {
  try {
    logger.spectrogram('Spectrogram service start requested', { ip: req.ip, body: req.body });
    
    if (spectrogramServiceProcess) {
      return res.status(400).json({ 
        success: false, 
        error: 'Spectrogram service is already running' 
      });
    }

    // Path to the spectrogram shell script
    const scriptPath = path.join(__dirname, '../../spectogram/spectrogram.sh');
    
    // Get analysis settings from request body or use defaults
    const {
      frequency = 2.4e9,
      sampleRate = 1e6,
      fftSize = 1024,
      resolution = null,
      windowSize = null,
      hopSize = null,
      windowType = 'hann',
      gain = 20,
      device = '', // Device serial number
      e = ''
    } = req.body || {};
    
    // Build command line arguments
    const args = [scriptPath];
    
    if (frequency) args.push('--freq', frequency.toString());
    if (sampleRate) args.push('--sample_rate', sampleRate.toString());
    if (fftSize) args.push('--fft_size', fftSize.toString());
    if (resolution) args.push('--resolution', resolution.toString());
    if (windowSize) args.push('--window_size', windowSize.toString());
    if (hopSize) args.push('--hop_size', hopSize.toString());
    if (windowType) args.push('--window_type', windowType);
    if (gain) args.push('--gain', gain.toString());
    // Handle device serial number lookup and conversion
    if (!device) {
      return res.json({ success: false, error: 'Device serial number is required' });
    }

    try {
      logger.spectrogram('Processing device parameter', { device: device });

      // Import USRP device manager
      const usrpDeviceManager = require('../UsrpDeviceManager');
      const deviceInfo = await usrpDeviceManager.lookupDeviceBySerial(device);
      
      if (deviceInfo.found && deviceInfo.ipAddress) {
        const deviceArgs = `addr=${deviceInfo.ipAddress}`;
        logger.spectrogram('Serial number lookup successful', { 
          serial: device, 
          ipAddress: deviceInfo.ipAddress,
          deviceArgs: deviceArgs,
          deviceName: deviceInfo.deviceName,
          deviceType: deviceInfo.deviceType
        });
        args.push('--device', deviceArgs);
      } else {
        logger.spectrogram('Serial number not found', { deviceSerial: device });
        return res.json({ 
          success: false, 
          error: `Serial number '${device}' not found. Please check the device serial number or ensure the device is connected.`,
          serialNumber: device
        });
      }
    } catch (error) {
      logger.spectrogram('Error processing device parameter', { 
        device: device, 
        error: error.message,
        stack: error.stack
      });
      return res.json({ 
        success: false, 
        error: `Error processing device parameter for serial '${device}': ${error.message}`,
        serialNumber: device
      });
    }
    
    // Log the complete command that will be executed
    const fullCommand = `bash ${args.join(' ')}`;
    logger.spectrogram('Starting spectrogram service - full command', { 
      command: fullCommand,
      args: args,
      workingDirectory: process.cwd()
    });
    
    // Start the spectrogram service using the shell script with analysis settings
    const spawnOptions = {
      stdio: ['pipe', 'pipe', 'pipe'],
      detached: false,
      cwd: process.cwd(),
      env: { ...process.env }
    };
    
    logger.spectrogram('Spawning spectrogram service process', { 
      command: 'bash',
      args: args,
      options: spawnOptions,
      scriptPath: scriptPath
    });
    
    spectrogramServiceProcess = spawn('bash', args, spawnOptions);

    spectrogramServiceStatus = 'starting';

    // Handle process events
    spectrogramServiceProcess.on('spawn', () => {
      logger.spectrogram('Spectrogram service process spawned', { pid: spectrogramServiceProcess.pid });
      spectrogramServiceStatus = 'running';
    });

    spectrogramServiceProcess.on('error', (error) => {
      logger.spectrogram('Spectrogram service process error', { error: error.message });
      spectrogramServiceStatus = 'error';
      spectrogramServiceProcess = null;
    });

    spectrogramServiceProcess.on('exit', (code, signal) => {
      logger.spectrogram('Spectrogram service process exited', { code, signal });
      spectrogramServiceStatus = 'stopped';
      spectrogramServiceProcess = null;
    });

    // Handle stdout and stderr
    spectrogramServiceProcess.stdout.on('data', (data) => {
      logger.spectrogram('Spectrogram service stdout', { data: data.toString().trim() });
    });

    spectrogramServiceProcess.stderr.on('data', (data) => {
      logger.spectrogram('Spectrogram service stderr', { data: data.toString().trim() });
    });

    const response = {
      success: true,
      message: 'Spectrogram service started successfully',
      pid: spectrogramServiceProcess.pid,
      status: spectrogramServiceStatus
    };
    
    logger.spectrogram('Spectrogram service start successful', { ip: req.ip, pid: spectrogramServiceProcess.pid });
    res.json(response);
  } catch (error) {
    logger.spectrogram('Error starting spectrogram service', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ success: false, error: 'Failed to start spectrogram service' });
  }
});

// Stop spectrogram service
router.post('/service/stop', async (req, res) => {
  try {
    logger.spectrogram('Spectrogram service stop requested', { ip: req.ip });
    
    if (!spectrogramServiceProcess) {
      // Service is not running, return success
      const response = {
        success: true,
        message: 'Spectrogram service is not running',
        status: 'stopped'
      };
      
      logger.spectrogram('Spectrogram service stop - service not running', { ip: req.ip });
      return res.json(response);
    }

    // Kill the process
    spectrogramServiceProcess.kill('SIGTERM');
    
    // Wait a bit for graceful shutdown
    setTimeout(() => {
      if (spectrogramServiceProcess) {
        spectrogramServiceProcess.kill('SIGKILL');
      }
    }, 5000);

    const response = {
      success: true,
      message: 'Spectrogram service stop requested',
      status: 'stopping'
    };
    
    logger.spectrogram('Spectrogram service stop successful', { ip: req.ip });
    res.json(response);
  } catch (error) {
    logger.spectrogram('Error stopping spectrogram service', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ success: false, error: 'Failed to stop spectrogram service' });
  }
});

// Get spectrogram service status
router.get('/service/status', async (req, res) => {
  try {
    logger.spectrogram('Spectrogram service status requested', { ip: req.ip });
    
    const response = {
      success: true,
      status: spectrogramServiceStatus,
      pid: spectrogramServiceProcess ? spectrogramServiceProcess.pid : null,
      running: !!spectrogramServiceProcess
    };
    
    logger.spectrogram('Spectrogram service status retrieved', { ip: req.ip, status: spectrogramServiceStatus });
    res.json(response);
  } catch (error) {
    logger.spectrogram('Error getting spectrogram service status', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ success: false, error: 'Failed to get spectrogram service status' });
  }
});

// Start spectrogram data generation
router.post('/start', async (req, res) => {
  try {
    logger.api('Spectrogram start requested', { ip: req.ip, settings: req.body });
    
    // Extract device serial number from request
    const { deviceSerialNumber, ...spectrogramSettings } = req.body;
    
    if (deviceSerialNumber) {
      logger.debug('USRP device serial number provided for spectrogram', { deviceSerialNumber });
      // Store the device serial number for use in data generation
      spectrogramStreams.set('currentDeviceSerial', deviceSerialNumber);
    } else {
      logger.debug('No USRP device serial number provided for spectrogram');
      spectrogramStreams.delete('currentDeviceSerial');
    }
    
    // Start spectrogram data generation if not already running
    if (!spectrogramInterval) {
      spectrogramInterval = setInterval(() => {
        generateSpectrogramData();
      }, 100); // Generate data every 100ms
      logger.debug('Spectrogram data generation started');
    }
    
    const response = {
      success: true,
      message: 'Spectrogram capture started',
      settings: spectrogramSettings,
      deviceSerialNumber: deviceSerialNumber || null
    };
    
    logger.api('Spectrogram start successful', { ip: req.ip });
    res.json(response);
  } catch (error) {
    logger.error('Error starting spectrogram', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ success: false, error: 'Failed to start spectrogram capture' });
  }
});

// Stop spectrogram data generation
router.post('/stop', async (req, res) => {
  try {
    logger.api('Spectrogram stop requested', { ip: req.ip });
    
    // Stop data generation if running
    if (spectrogramInterval) {
      clearInterval(spectrogramInterval);
      spectrogramInterval = null;
      logger.debug('Spectrogram data generation stopped');
    }
    
    const response = {
      success: true,
      message: 'Spectrogram capture stopped'
    };
    
    logger.api('Spectrogram stop successful', { ip: req.ip });
    res.json(response);
  } catch (error) {
    logger.error('Error stopping spectrogram', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ success: false, error: 'Failed to stop spectrogram capture' });
  }
});

// Handle preflight OPTIONS request for SSE
router.options('/stream', (req, res) => {
  res.writeHead(200, {
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Credentials': 'false',
    'Access-Control-Allow-Headers': 'Cache-Control, Content-Type',
    'Access-Control-Allow-Methods': 'GET, OPTIONS',
    'Access-Control-Max-Age': '86400'
  });
  res.end();
});

// Server-Sent Events endpoint for real-time spectrogram data
router.get('/stream', async (req, res) => {
  try {
    logger.api('Spectrogram stream requested', { ip: req.ip });
    
    // Set SSE headers with proper CORS for Chrome
    res.writeHead(200, {
      'Content-Type': 'text/event-stream',
      'Cache-Control': 'no-cache, no-store, must-revalidate',
      'Connection': 'keep-alive',
      'Access-Control-Allow-Origin': '*',
      'Access-Control-Allow-Credentials': 'false',
      'Access-Control-Allow-Headers': 'Cache-Control, Content-Type',
      'Access-Control-Allow-Methods': 'GET, OPTIONS',
      'X-Accel-Buffering': 'no' // Disable nginx buffering if present
    });
    
    // Send initial connection message
    res.write('data: {"type": "connected", "message": "Spectrogram stream connected"}\n\n');
    
    // Generate unique client ID
    const clientId = `${req.ip}-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
    
    // Add client to active streams
    spectrogramStreams.set(clientId, res);
    
    logger.debug('Spectrogram stream connected', { clientId, ip: req.ip, activeStreams: spectrogramStreams.size });
    
    // Log the current state
    logger.info('SSE Connection established', { 
      clientId, 
      ip: req.ip, 
      activeStreams: spectrogramStreams.size,
      hasInterval: !!spectrogramInterval,
      userAgent: req.get('User-Agent')
    });
    
    // Handle client disconnect
    req.on('close', () => {
      spectrogramStreams.delete(clientId);
      logger.info('SSE Connection closed', { clientId, ip: req.ip, activeStreams: spectrogramStreams.size });
      
      // Stop data generation if no active streams
      if (spectrogramStreams.size === 0 && spectrogramInterval) {
        clearInterval(spectrogramInterval);
        spectrogramInterval = null;
        logger.info('Spectrogram data generation stopped - no active streams');
      }
    });
    
    // Handle client error
    req.on('error', (error) => {
      logger.warn('SSE Connection error', { clientId, ip: req.ip, error: error.message });
      spectrogramStreams.delete(clientId);
    });
    
  } catch (error) {
    logger.error('Error setting up spectrogram stream', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ error: 'Failed to setup spectrogram stream' });
  }
});

// Register client for server-push data
router.post('/register', async (req, res) => {
  try {
    const clientId = `${req.ip}-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
    connectedClients.set(clientId, {
      ip: req.ip,
      registeredAt: Date.now(),
      latestData: null
    });
    
    logger.info('Client registered for server-push', { clientId, ip: req.ip });
    
    res.json({ 
      success: true, 
      clientId,
      message: 'Registered for server-push data' 
    });
  } catch (error) {
    logger.error('Error registering client', { error: error.message, ip: req.ip });
    res.status(500).json({ error: 'Failed to register client' });
  }
});

// Get latest data for registered client (server-push)
router.get('/data/:clientId', async (req, res) => {
  try {
    const { clientId } = req.params;
    const clientInfo = connectedClients.get(clientId);
    
    if (!clientInfo) {
      return res.status(404).json({ error: 'Client not found' });
    }
    
    if (clientInfo.latestData) {
      // Return the data and clear it (one-time delivery)
      const data = clientInfo.latestData;
      clientInfo.latestData = null;
      res.json(data);
    } else {
      // No new data available
      res.status(204).json({ message: 'No new data available' });
    }
  } catch (error) {
    logger.error('Error getting client data', { error: error.message, clientId: req.params.clientId });
    res.status(500).json({ error: 'Failed to get data' });
  }
});

// Legacy endpoint for backward compatibility (single request)
router.get('/data', async (req, res) => {
  try {
    logger.debug('Spectrogram data requested', { ip: req.ip });
    
    // Generate more realistic spectrogram data with some frequency peaks
    const fftSize = 1024;
    const data = new Array(fftSize / 2);
    
    // Base noise floor
    for (let i = 0; i < data.length; i++) {
      data[i] = Math.random() * 10 + 5; // 5-15 dB noise floor
    }
    
    // Add some frequency peaks to simulate signals
    const time = Date.now() / 1000;
    
    // Peak 1: Moving frequency
    const peak1Freq = Math.floor(50 + 30 * Math.sin(time * 0.5));
    const peak1Amp = 60 + 20 * Math.sin(time * 2);
    if (peak1Freq >= 0 && peak1Freq < data.length) {
      data[peak1Freq] = peak1Amp;
      // Add some spread around the peak
      for (let i = Math.max(0, peak1Freq - 2); i <= Math.min(data.length - 1, peak1Freq + 2); i++) {
        const distance = Math.abs(i - peak1Freq);
        data[i] = Math.max(data[i], peak1Amp * Math.exp(-distance * 0.5));
      }
    }
    
    // Peak 2: Fixed frequency with varying amplitude
    const peak2Freq = 150;
    const peak2Amp = 40 + 30 * Math.sin(time * 1.5);
    if (peak2Freq < data.length) {
      data[peak2Freq] = peak2Amp;
      for (let i = Math.max(0, peak2Freq - 1); i <= Math.min(data.length - 1, peak2Freq + 1); i++) {
        const distance = Math.abs(i - peak2Freq);
        data[i] = Math.max(data[i], peak2Amp * Math.exp(-distance * 0.8));
      }
    }
    
    // Peak 3: High frequency burst
    if (Math.sin(time * 3) > 0.8) {
      const peak3Freq = 300 + Math.floor(Math.random() * 50);
      const peak3Amp = 70 + Math.random() * 20;
      if (peak3Freq < data.length) {
        data[peak3Freq] = peak3Amp;
        for (let i = Math.max(0, peak3Freq - 1); i <= Math.min(data.length - 1, peak3Freq + 1); i++) {
          const distance = Math.abs(i - peak3Freq);
          data[i] = Math.max(data[i], peak3Amp * Math.exp(-distance * 1.0));
        }
      }
    }
    
    const mockData = {
      timestamp: Date.now(),
      frequency: 2400,
      sampleRate: 44100,
      fftSize: fftSize,
      data: data
    };
    
    res.json(mockData);
  } catch (error) {
    logger.error('Error getting spectrogram data', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ error: 'Failed to get spectrogram data' });
  }
});

module.exports = router;
