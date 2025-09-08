const { spawn } = require('child_process');
const path = require('path');
const grpc = require('@grpc/grpc-js');
const protoLoader = require('@grpc/proto-loader');
const logger = require('./logger');

const UeInterface = require('./UeInterface');

class UeManagerSimulated extends UeInterface {
  constructor() {
    super();
    this.process = null;
    this.isRunningFlag = false;
    this.startTime = null;
    this.config = {
      frequency: 3425010000,
      port: 50051,
      imsi: '001010000000003'
    };
    this.consoleOutput = [];
    this.scanStatus = {
      isScanning: false,
      scanId: null,
      detectedCells: []
    };
    this.grpcClient = null;
    this.protoPath = path.join(__dirname, '..', 'proto', 'ue_service.proto');
  }

  // Get current UE status
  getStatus() {
    const now = Date.now();
    const uptime = this.startTime ? Math.floor((now - this.startTime) / 1000) : 0;
    
    return {
      isRunning: this.isRunningFlag,
      processId: this.isRunningFlag ? `sim_ue_${this.startTime || now}` : null,
      connectionState: this.isRunningFlag ? 'connected' : 'disconnected',
      grpcPort: this.config.port,
      uptime: uptime,
      status: this.isRunningFlag ? 'Active' : 'Inactive'
    };
  }

  // Get UE configuration
  getConfig() {
    return { ...this.config };
  }

  // Update UE configuration
  updateConfig(newConfig) {
    this.config = { ...this.config, ...newConfig };
    logger.info('Simulated UE configuration updated', { newConfig });
    return this.config;
  }

  // Start UE process
  async start(config = {}) {
    try {
      if (this.isRunningFlag) {
        throw new Error('Simulated UE process is already running');
      }

      // Update configuration
      this.config = { ...this.config, ...config };

      logger.info('Starting simulated UE process', { config: this.config });

      // Start Python simulated UE process
      const pythonScript = path.join(__dirname, '..', 'ue', 'simulated_ue.py');
      const args = [
        '--frequency', (this.config.radio?.freq || this.config.frequency || 3425010000).toString(),
        '--port', this.config.port.toString()
      ];

      // Add authentication parameters
      if (this.config.authentication) {
        if (this.config.authentication.imsi) args.push('--imsi', this.config.authentication.imsi);
        if (this.config.authentication.key) args.push('--key', this.config.authentication.key);
        if (this.config.authentication.opc) args.push('--opc', this.config.authentication.opc);
      }

      // Add network parameters
      if (this.config.network) {
        if (this.config.network.dnn) args.push('--dnn', this.config.network.dnn);
        if (this.config.network.nssai_sst) args.push('--nssai-sst', this.config.network.nssai_sst.toString());
        if (this.config.network.nssai_sd) {
          args.push('--nssai-sd', this.config.network.nssai_sd.toString());
        }
      }

      // Add radio parameters
      if (this.config.radio) {
        args.push('--bandwidth', (this.config.radio.bw || this.config.radio.bandwidth || 100).toString());
        args.push('--numerology', (this.config.radio.numerology || 1).toString());
      }

      this.process = spawn('python3', [pythonScript, ...args], {
        cwd: path.join(__dirname, '..', 'ue'),
        stdio: ['pipe', 'pipe', 'pipe']
      });

      this.process.on('spawn', () => {
        logger.info('Simulated UE process spawned', { pid: this.process.pid });
        this.isRunningFlag = true;
        this.startTime = Date.now();
      });

      this.process.stdout.on('data', (data) => {
        const output = data.toString().trim();
        logger.info('Simulated UE stdout', { output });
        this.addConsoleOutput(output, 'stdout');
      });

      this.process.stderr.on('data', (data) => {
        const output = data.toString().trim();
        logger.warn('Simulated UE stderr', { error: output });
        this.addConsoleOutput(output, 'stderr');
      });

      this.process.on('close', (code) => {
        logger.info('Simulated UE process closed', { code });
        this.isRunningFlag = false;
        this.startTime = null;
        this.process = null;
      });

      this.process.on('error', (error) => {
        logger.error('Simulated UE process error', { error: error.message });
        this.isRunningFlag = false;
        this.startTime = null;
        this.process = null;
      });

      // Wait a moment for the process to start
      await new Promise(resolve => setTimeout(resolve, 2000));

      return {
        success: true,
        processId: `sim_ue_${this.startTime}`,
        message: 'Simulated UE Python process started successfully',
        grpcPort: this.config.port,
        frequency: this.config.radio?.freq || this.config.frequency
      };

    } catch (error) {
      logger.error('Error starting simulated UE', { error: error.message });
      throw error;
    }
  }

  // Stop UE process
  async stop() {
    try {
      if (!this.isRunningFlag) {
        return { success: true, message: 'Simulated UE process is not running' };
      }

      logger.info('Stopping simulated UE process');

      if (this.process && !this.process.killed) {
        this.process.kill('SIGTERM');
        
        // Wait for graceful shutdown
        await new Promise((resolve) => {
          const timeout = setTimeout(() => {
            if (this.process && !this.process.killed) {
              this.process.kill('SIGKILL');
            }
            resolve();
          }, 5000);

          this.process.on('close', () => {
            clearTimeout(timeout);
            resolve();
          });
        });
      }

      this.isRunningFlag = false;
      this.startTime = null;
      this.process = null;

      return { success: true, message: 'Simulated UE process stopped successfully' };

    } catch (error) {
      logger.error('Error stopping simulated UE', { error: error.message });
      throw error;
    }
  }

  // Restart UE process
  async restart(config = {}) {
    try {
      await this.stop();
      await new Promise(resolve => setTimeout(resolve, 2000));
      return await this.start(config);
    } catch (error) {
      logger.error('Error restarting simulated UE', { error: error.message });
      throw error;
    }
  }

  // Check if UE process is running
  isRunning() {
    return this.isRunningFlag;
  }

  // Get UE logs (simulated - return empty for now)
  getLogs(lines = 100) {
    // For simulated UE, we could read from log files
    return [];
  }

  addConsoleOutput(output, type = 'stdout') {
    if (output) {
      const timestamp = new Date().toISOString();
      this.consoleOutput.push({
        timestamp,
        type,
        message: output
      });
      
      // Keep only last 1000 lines
      if (this.consoleOutput.length > 1000) {
        this.consoleOutput = this.consoleOutput.slice(-1000);
      }
    }
  }

  getConsoleOutput(lines = 100) {
    return this.consoleOutput.slice(-lines);
  }

  // Get UE performance metrics
  getMetrics() {
    const now = Date.now();
    const uptime = this.startTime ? now - this.startTime : 0;
    
    return {
      uptime: uptime,
      status: this.isRunningFlag ? 'Active' : 'Inactive',
      connected: this.isRunningFlag,
      processId: this.isRunningFlag ? `sim_ue_${this.startTime}` : null,
      lastActivity: new Date().toISOString(),
      error: null,
      logEntries: 0,
      timestamp: new Date().toISOString()
    };
  }

  // Create gRPC client
  createGrpcClient() {
    try {
      const packageDefinition = protoLoader.loadSync(this.protoPath, {
        keepCase: true,
        longs: String,
        enums: String,
        defaults: true,
        oneofs: true
      });

      const ueService = grpc.loadPackageDefinition(packageDefinition).ue_service;
      const client = new ueService.UEControlService(
        `localhost:${this.config.port}`,
        grpc.credentials.createInsecure()
      );

      return client;
    } catch (error) {
      logger.error('Failed to create gRPC client', { error: error.message });
      return null;
    }
  }

  // Start cell scanning
  async startScan() {
    try {
      if (!this.isRunningFlag) {
        throw new Error('Simulated UE process is not running');
      }

      if (this.scanStatus.isScanning) {
        return {
          success: true,
          message: 'Cell scan is already running',
          scanId: this.scanStatus.scanId
        };
      }

      const client = this.createGrpcClient();
      if (!client) {
        throw new Error('Failed to create gRPC client');
      }

      const scanId = `scan_${Date.now()}`;
      
      return new Promise((resolve, reject) => {
        client.StartScanning({
          process_id: `sim_ue_${this.startTime}`
        }, (error, response) => {
          if (error) {
            logger.error('gRPC StartScanning error', { error: error.message });
            reject(new Error(`gRPC error: ${error.code} ${error.details}`));
          } else {
            // Check the response from the Python script
            if (response.success) {
              this.scanStatus.isScanning = true;
              this.scanStatus.scanId = response.scan_id || scanId;
              
              resolve({
                success: true,
                message: response.message || 'Cell scanning started successfully',
                scanId: response.scan_id || scanId
              });
            } else {
              reject(new Error(response.message || 'Failed to start cell scan'));
            }
          }
        });
      });

    } catch (error) {
      logger.error('Error starting cell scan', { error: error.message });
      throw error;
    }
  }

  // Stop cell scanning
  async stopScan() {
    try {
      if (!this.scanStatus.isScanning) {
        return { success: true, message: 'Cell scan is not in progress' };
      }

      const client = this.createGrpcClient();
      if (!client) {
        throw new Error('Failed to create gRPC client');
      }

      return new Promise((resolve, reject) => {
        client.StopScanning({
          process_id: `sim_ue_${this.startTime}`,
          scan_id: this.scanStatus.scanId
        }, (error, response) => {
          if (error) {
            logger.error('gRPC StopScanning error', { error: error.message });
            reject(new Error(`gRPC error: ${error.code} ${error.details}`));
          } else {
            this.scanStatus.isScanning = false;
            this.scanStatus.scanId = null;
            
            resolve({
              success: true,
              message: 'Cell scanning stopped successfully'
            });
          }
        });
      });

    } catch (error) {
      logger.error('Error stopping cell scan', { error: error.message });
      throw error;
    }
  }

  // Get detected cells
  async getDetectedCells() {
    try {
      if (!this.isRunningFlag) {
        throw new Error('Simulated UE process is not running');
      }

      const packageDefinition = protoLoader.loadSync(this.protoPath, {
        keepCase: true,
        longs: String,
        enums: String,
        defaults: true,
        oneofs: true
      });

      const ueService = grpc.loadPackageDefinition(packageDefinition).ue_service;
      const dataClient = new ueService.UEDataService(
        `localhost:${this.config.port}`,
        grpc.credentials.createInsecure()
      );

      return new Promise((resolve, reject) => {
        dataClient.GetCurrentCells({}, (error, response) => {
          if (error) {
            logger.error('gRPC GetCurrentCells error', { error: error.message });
            reject(new Error(`gRPC error: ${error.code} ${error.details}`));
          } else {
            const cells = response.cells.map(cell => ({
              id: Math.floor(Math.random() * 1000), // Generate random ID
              pci: cell.pci,
              pss: cell.pss,
              sss: cell.sss,
              ss_rsrp: cell.ss_rsrp,
              ss_rsrq: cell.ss_rsrq,
              ss_sinr: cell.ss_sinr,
              pbch_decoded: cell.pbch_decoded,
              mib: cell.mib,
              sib1_detected: cell.sib1_detected,
              sib1: cell.sib1,
              timestamp: response.timestamp
            }));

            resolve({
              success: true,
              cells: cells,
              timestamp: response.timestamp.toString(),
              count: cells.length
            });
          }
        });
      });

    } catch (error) {
      logger.error('Error getting detected cells', { error: error.message });
      throw error;
    }
  }

  // Get scan status
  getScanStatus() {
    return {
      isScanning: this.scanStatus.isScanning,
      detectedCells: this.scanStatus.detectedCells,
      scanId: this.scanStatus.scanId
    };
  }

  // Send command to UE process (not supported for simulated UE)
  async sendCommand(command) {
    throw new Error('sendCommand() is not supported for simulated UE');
  }
}

module.exports = UeManagerSimulated;
