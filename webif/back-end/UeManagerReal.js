const { spawn, exec } = require('child_process');
const fs = require('fs');
const path = require('path');
const logger = require('./logger');

const UeInterface = require('./UeInterface');

class UeManagerReal extends UeInterface {
  constructor() {
    super();
    this.process = null;
    this.status = {
      connected: false,
      state: 'disconnected',
      processId: null,
      startTime: null,
      lastActivity: null,
      error: null
    };
    this.config = {
      imsi: '001010000000003',
      key: 'fec86ba6eb707ed08905757b1bb44b8f',
      opc: 'c42449363bbad02b66d16bc975d77cc1',
      dnn: 'oai',
      nssai_sst: 1,
      usrpArgs: 'type=x300,addr=192.168.40.2,clock=internal,time=internal',
      frequency: 3425010000,
      numerology: 1,
      band: 78
    };
    this.consoleOutput = [];
    this.logBuffer = [];
    this.maxLogBufferSize = 1000;
  }

  // Get current status (unified interface)
  getStatus() {
    const now = Date.now();
    const uptime = this.status.startTime ? Math.floor((now - new Date(this.status.startTime).getTime()) / 1000) : 0;
    
    return {
      isRunning: this.status.connected && this.status.state !== 'disconnected',
      processId: this.status.processId,
      connectionState: this.status.connected ? 'connected' : 'disconnected',
      grpcPort: null, // Real UE doesn't use gRPC
      uptime: uptime,
      status: this.status.state === 'connected' ? 'Active' : 'Inactive'
    };
  }

  // Get configuration
  getConfig() {
    return { ...this.config };
  }

  // Update configuration
  updateConfig(newConfig) {
    this.config = { ...this.config, ...newConfig };
    logger.info('UE configuration updated', { newConfig });
    return this.config;
  }

  // Start UE process
  async start(config = {}) {
    try {
      if (this.process && !this.process.killed) {
        throw new Error('UE process is already running');
      }

      // Update configuration
      this.config = { ...this.config, ...config };

      logger.info('Starting UE process', { config: this.config });

      // Update UE configuration file
      await this.updateConfigFile();

      // Build UE command
      const command = this.buildUECommand();

      logger.info('UE command', { command });

      // Start UE process using the script
      this.process = spawn('bash', ['-c', command], {
        stdio: ['pipe', 'pipe', 'pipe'],
        detached: false,
        cwd: path.join(__dirname, '../ue') // Script will handle the build directory change
      });

      this.process.on('spawn', () => {
        logger.info('UE process spawned', { pid: this.process.pid });
        this.status = {
          connected: true,
          state: 'starting',
          processId: this.process.pid,
          startTime: new Date().toISOString(),
          lastActivity: new Date().toISOString(),
          error: null
        };
      });

      this.process.stdout.on('data', (data) => {
        const output = data.toString();
        this.addToLogBuffer('stdout', output);
        this.status.lastActivity = new Date().toISOString();
        
        // Also print to backend console for visibility
        console.log('[UE OUTPUT]', output.trim());
        
        // Parse UE output for status updates
        this.parseUEOutput(output);
      });

      this.process.stderr.on('data', (data) => {
        const error = data.toString();
        this.addToLogBuffer('stderr', error);
        this.status.lastActivity = new Date().toISOString();
        
        // Also print to backend console for visibility
        console.error('[UE ERROR]', error.trim());
        
        // Check for critical errors
        if (error.includes('ERROR') || error.includes('FATAL')) {
          this.status.error = error.trim();
        }
      });

      this.process.on('close', (code) => {
        logger.info('UE process closed', { code });
        this.addToLogBuffer('system', `Process closed with code: ${code}`);
        this.status = {
          connected: false,
          state: 'disconnected',
          processId: null,
          startTime: null,
          lastActivity: new Date().toISOString(),
          error: code !== 0 ? `Process exited with code ${code}` : null
        };
        this.process = null;
      });

      this.process.on('error', (error) => {
        logger.error('UE process error', { error: error.message });
        this.addToLogBuffer('system', `Process error: ${error.message}`);
        this.status = {
          connected: false,
          state: 'error',
          processId: null,
          startTime: null,
          lastActivity: new Date().toISOString(),
          error: error.message
        };
        this.process = null;
      });

      return { 
        success: true, 
        message: 'UE process started', 
        pid: this.process.pid,
        config: this.config
      };

    } catch (error) {
      logger.error('Error starting UE', { error: error.message });
      this.status.error = error.message;
      throw error;
    }
  }

  // Stop UE process
  async stop() {
    try {
      if (!this.process || this.process.killed) {
        return { success: true, message: 'UE process is not running' };
      }

      logger.info('Stopping UE process', { pid: this.process.pid });

      // Try graceful shutdown first
      this.process.kill('SIGTERM');
      this.addToLogBuffer('system', 'Sending SIGTERM to UE process');

      // Wait for graceful shutdown
      await new Promise((resolve) => {
        const timeout = setTimeout(() => {
          if (this.process && !this.process.killed) {
            logger.warn('Force killing UE process');
            this.process.kill('SIGKILL');
            this.addToLogBuffer('system', 'Force killing UE process with SIGKILL');
          }
          resolve();
        }, 5000);

        this.process.on('close', () => {
          clearTimeout(timeout);
          resolve();
        });
      });

      this.status = {
        connected: false,
        state: 'disconnected',
        processId: null,
        startTime: null,
        lastActivity: new Date().toISOString(),
        error: null
      };

      return { success: true, message: 'UE process stopped' };

    } catch (error) {
      logger.error('Error stopping UE', { error: error.message });
      throw error;
    }
  }

  // Restart UE process
  async restart(config = {}) {
    try {
      await this.stop();
      await new Promise(resolve => setTimeout(resolve, 2000)); // Wait 2 seconds
      return await this.start(config);
    } catch (error) {
      logger.error('Error restarting UE', { error: error.message });
      throw error;
    }
  }

  // Build UE command using ue_usrp.sh script
  buildUECommand() {
    const scriptPath = path.join(__dirname, '../ue/ue_usrp.sh');
    return `bash ${scriptPath}`;
  }

  // Update UE configuration file
  async updateConfigFile() {
    try {
      const configPath = path.join(__dirname, '../ue/ue.conf');
      const configContent = `uicc0 = {
  imsi = "${this.config.authentication?.imsi || this.config.imsi || '001010000000003'}";
  key = "${this.config.authentication?.key || this.config.key || 'fec86ba6eb707ed08905757b1bb44b8f'}";
  opc= "${this.config.authentication?.opc || this.config.opc || 'c42449363bbad02b66d16bc975d77cc1'}";
  dnn= "${this.config.network?.dnn || this.config.dnn || 'oai'}";
  nssai_sst=${this.config.network?.nssai_sst || this.config.nssai_sst || 1};
  nssai_sd=${this.config.network?.nssai_sd || this.config.nssai_sd || 1};
}`;

      await fs.promises.writeFile(configPath, configContent, 'utf8');
      logger.info('UE configuration file updated', { configPath });

    } catch (error) {
      logger.error('Error updating UE config file', { error: error.message });
      throw error;
    }
  }

  // Parse UE output for status updates
  parseUEOutput(output) {
    const outputLower = output.toLowerCase();
    
    if (outputLower.includes('ue is connected') || outputLower.includes('connected to network')) {
      this.status.state = 'connected';
    } else if (outputLower.includes('cell search') || outputLower.includes('scanning')) {
      this.status.state = 'scanning';
    } else if (outputLower.includes('initializing') || outputLower.includes('starting')) {
      this.status.state = 'starting';
    } else if (outputLower.includes('error') || outputLower.includes('failed')) {
      this.status.state = 'error';
    }
  }

  // Add to log buffer
  addToLogBuffer(type, message) {
    const logEntry = {
      timestamp: new Date().toISOString(),
      type: type,
      message: message.trim()
    };

    this.logBuffer.push(logEntry);

    // Also add to console output
    this.consoleOutput.push(logEntry);

    // Keep buffer size manageable
    if (this.logBuffer.length > this.maxLogBufferSize) {
      this.logBuffer = this.logBuffer.slice(-this.maxLogBufferSize);
    }
    
    if (this.consoleOutput.length > this.maxLogBufferSize) {
      this.consoleOutput = this.consoleOutput.slice(-this.maxLogBufferSize);
    }
  }

  // Get logs
  getLogs(lines = 100) {
    return this.logBuffer.slice(-lines);
  }

  // Get console output
  getConsoleOutput(lines = 100) {
    return this.consoleOutput.slice(-lines);
  }

  // Get performance metrics
  getMetrics() {
    return {
      uptime: this.status.startTime ? Date.now() - new Date(this.status.startTime).getTime() : 0,
      status: this.status.state,
      connected: this.status.connected,
      processId: this.status.processId,
      lastActivity: this.status.lastActivity,
      error: this.status.error,
      logEntries: this.logBuffer.length,
      timestamp: new Date().toISOString()
    };
  }

  // Check if process is running
  isRunning() {
    return this.process && !this.process.killed;
  }

  // Send command to UE process
  sendCommand(command) {
    if (!this.process || this.process.killed) {
      throw new Error('UE process is not running');
    }

    this.process.stdin.write(command + '\n');
    this.addToLogBuffer('command', command);
  }

  // Start cell scanning (for real UE - not implemented yet)
  async startScan(scanConfig) {
    throw new Error('Cell scanning not yet implemented for real UE');
  }

  // Stop cell scanning (for real UE - not implemented yet)
  async stopScan() {
    throw new Error('Cell scanning not yet implemented for real UE');
  }

  // Get detected cells (for real UE - not implemented yet)
  async getDetectedCells() {
    throw new Error('Cell detection not yet implemented for real UE');
  }

  // Get scan status (for real UE - not implemented yet)
  getScanStatus() {
    return {
      isScanning: false,
      centerFrequency: null,
      detectedCells: [],
      scanId: null
    };
  }
}

module.exports = new UeManagerReal();
