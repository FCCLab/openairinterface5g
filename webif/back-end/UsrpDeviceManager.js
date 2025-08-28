const { exec } = require('child_process');
const logger = require('./logger');

class UsrpDeviceManager {
  constructor() {
    this.cache = {
      devices: [],
      lastUpdate: null,
      cacheDuration: 30000, // 30 seconds cache
      isUpdating: false
    };
    this.detectionMethods = {
      uhd_find_devices: true,
      lsusb: true,
      dev: true
    };
  }

  /**
   * Check if cache is still valid
   */
  isCacheValid() {
    if (!this.cache.lastUpdate) return false;
    return (Date.now() - this.cache.lastUpdate) < this.cache.cacheDuration;
  }

  /**
   * Get cached USRP devices or update if cache is expired
   */
  async getUsrpDevices() {
    // Return cached data if still valid
    if (this.isCacheValid()) {
      logger.debug('Returning cached USRP devices', { 
        deviceCount: this.cache.devices.length,
        cacheAge: Date.now() - this.cache.lastUpdate
      });
      return this.getFormattedResponse();
    }

    // Update cache if not already updating
    if (!this.cache.isUpdating) {
      return await this.updateCache();
    }

    // If already updating, return current cache
    logger.debug('USRP cache update in progress, returning current cache');
    return this.getFormattedResponse();
  }

  /**
   * Force update the USRP device cache
   */
  async forceUpdate() {
    logger.debug('Forcing USRP cache update');
    this.cache.lastUpdate = null;
    return await this.updateCache();
  }

  /**
   * Update the USRP device cache
   */
  async updateCache() {
    if (this.cache.isUpdating) {
      logger.debug('USRP cache update already in progress');
      return this.getFormattedResponse();
    }

    this.cache.isUpdating = true;
    
    try {
      logger.debug('Starting USRP device cache update');
      const startTime = Date.now();

      const devices = await this.detectUsrpDevices();
      
      this.cache.devices = devices;
      this.cache.lastUpdate = Date.now();
      
      const updateTime = Date.now() - startTime;
      logger.debug('USRP device cache updated', { 
        deviceCount: devices.length,
        updateTime: `${updateTime}ms`,
        devices: devices.map(d => ({ name: d.name, method: d.method }))
      });

      return this.getFormattedResponse();
    } catch (error) {
      logger.error('Error updating USRP device cache', { 
        error: error.message, 
        stack: error.stack 
      });
      return this.getFormattedResponse(true, error.message);
    } finally {
      this.cache.isUpdating = false;
    }
  }

  /**
   * Detect USRP devices using multiple methods
   */
  async detectUsrpDevices() {
    const devices = [];
    
    // Check for USRP devices using uhd_find_devices
    if (this.detectionMethods.uhd_find_devices) {
      const usrpFindDevices = await this.detectWithUhdFindDevices();
      devices.push(...usrpFindDevices);
    }

    // Check for USRP devices using lsusb
    if (this.detectionMethods.lsusb) {
      const lsusbDevices = await this.detectWithLsusb();
      devices.push(...lsusbDevices);
    }

    // Check for USRP devices in /dev
    if (this.detectionMethods.dev) {
      const devDevices = await this.detectWithDev();
      devices.push(...devDevices);
    }

    // Remove duplicates based on name and method
    const uniqueDevices = this.removeDuplicates(devices);
    
    return uniqueDevices;
  }

  /**
   * Detect USRP devices using uhd_find_devices command
   */
  async detectWithUhdFindDevices() {
    return new Promise((resolve) => {
      exec('uhd_find_devices', { timeout: 15000 }, (error, stdout, stderr) => {
        if (error) {
          logger.debug('uhd_find_devices not found or failed', { 
            error: error.message, 
            stderr: stderr 
          });
          resolve([]);
        } else {
          logger.debug('uhd_find_devices output received', { 
            outputLength: stdout.length 
          });
          const devices = this.parseUhdFindDevicesOutput(stdout);
          resolve(devices);
        }
      });
    });
  }

  /**
   * Parse uhd_find_devices output
   */
  parseUhdFindDevicesOutput(output) {
    if (!output) return [];

    const devices = [];
    const lines = output.split('\n');
    let currentDevice = null;

    for (const line of lines) {
      // Look for device header: -- UHD Device 0
      const deviceMatch = line.match(/-- UHD Device (\d+)/);
      if (deviceMatch) {
        if (currentDevice) {
          devices.push(currentDevice);
          logger.debug('Added USRP device from uhd_find_devices', { 
            device: currentDevice.name, 
            method: currentDevice.method 
          });
        }
        currentDevice = {
          type: 'USRP',
          name: 'Unknown USRP',
          method: 'uhd_find_devices',
          details: {
            'Device Index': deviceMatch[1]
          }
        };
        logger.debug('Found USRP device header', { deviceIndex: deviceMatch[1] });
      }

      // Parse device details
      if (currentDevice && line.includes(':')) {
        const [key, value] = line.split(':').map(s => s.trim());
        if (key && value) {
          if (key === 'product') {
            currentDevice.name = value;
            logger.debug('Set USRP device name', { name: value });
          } else if (key === 'addr') {
            currentDevice.details['IP Address'] = value;
          } else if (key === 'serial') {
            currentDevice.details['Serial Number'] = value;
          } else if (key === 'fpga') {
            currentDevice.details['FPGA'] = value;
          } else if (key === 'type') {
            currentDevice.details['Type'] = value;
          } else {
            currentDevice.details[key] = value;
          }
        }
      }
    }

    // Don't forget the last device
    if (currentDevice) {
      devices.push(currentDevice);
      logger.debug('Added final USRP device from uhd_find_devices', { 
        device: currentDevice.name, 
        method: currentDevice.method 
      });
    }

    return devices;
  }

  /**
   * Detect USRP devices using lsusb command
   */
  async detectWithLsusb() {
    return new Promise((resolve) => {
      exec('lsusb', { timeout: 5000 }, (error, stdout, stderr) => {
        if (error) {
          logger.debug('lsusb command failed', { error: error.message });
          resolve([]);
        } else {
          const devices = this.parseLsusbOutput(stdout);
          resolve(devices);
        }
      });
    });
  }

  /**
   * Parse lsusb output for USRP devices
   */
  parseLsusbOutput(output) {
    if (!output) return [];

    const devices = [];
    const lines = output.split('\n');
    
    for (const line of lines) {
      if (line.toLowerCase().includes('usrp') || line.toLowerCase().includes('ettus')) {
        const match = line.match(/Bus (\d+) Device (\d+): ID ([a-f0-9]{4}):([a-f0-9]{4}) (.+)/);
        if (match) {
          devices.push({
            type: 'USRP',
            name: match[5].trim(),
            method: 'lsusb',
            details: {
              'Bus': match[1],
              'Device': match[2],
              'Vendor ID': match[3],
              'Product ID': match[4]
            }
          });
        }
      }
    }

    return devices;
  }

  /**
   * Detect USRP devices in /dev directory
   */
  async detectWithDev() {
    return new Promise((resolve) => {
      exec('ls /dev/ | grep -i usrp', { timeout: 5000 }, (error, stdout, stderr) => {
        if (error) {
          logger.debug('No USRP devices found in /dev', { error: error.message });
          resolve([]);
        } else {
          const devFiles = stdout.trim().split('\n').filter(line => line.trim());
          if (devFiles.length > 0) {
            resolve([{
              type: 'USRP',
              name: 'USRP Device Files',
              method: '/dev',
              details: {
                'Device Files': devFiles.join(', ')
              }
            }]);
          } else {
            resolve([]);
          }
        }
      });
    });
  }

  /**
   * Remove duplicate devices based on name and method
   */
  removeDuplicates(devices) {
    const seen = new Set();
    return devices.filter(device => {
      const key = `${device.name}-${device.method}`;
      if (seen.has(key)) {
        return false;
      }
      seen.add(key);
      return true;
    });
  }

  /**
   * Get detailed device information using uhd_usrp_probe
   */
  async getDeviceDetails(deviceArgs) {
    try {
      logger.debug('Getting detailed device information', { deviceArgs });
      
      return new Promise((resolve, reject) => {
        const command = `uhd_usrp_probe --args "${deviceArgs}"`;
        
        exec(command, { timeout: 30000 }, (error, stdout, stderr) => {
          if (error) {
            logger.debug('uhd_usrp_probe failed', { 
              error: error.message, 
              stderr: stderr,
              deviceArgs 
            });
            resolve({
              success: false,
              error: error.message,
              output: stderr || stdout
            });
          } else {
            logger.debug('uhd_usrp_probe output received', { 
              outputLength: stdout.length,
              deviceArgs 
            });
            
            const parsedDetails = this.parseUsrpProbeOutput(stdout);
            resolve({
              success: true,
              details: parsedDetails,
              rawOutput: stdout
            });
          }
        });
      });
    } catch (error) {
      logger.error('Error getting device details', { 
        error: error.message, 
        stack: error.stack,
        deviceArgs 
      });
      return {
        success: false,
        error: error.message
      };
    }
  }

  /**
   * Parse uhd_usrp_probe output
   */
  parseUsrpProbeOutput(output) {
    if (!output) return {};

    // Helper function to clean up pipe characters and whitespace
    const cleanText = (text) => {
      return text
        .replace(/^\s*\|+\s*/, '')   // Remove leading pipes and spaces
        .replace(/\s*\|+\s*$/, '')   // Remove trailing pipes and spaces
        .replace(/\|\s*/g, '')       // Remove any remaining pipe characters
        .replace(/\s+/g, ' ')        // Normalize whitespace
        .trim();
    };

    const details = {
      mboard: {},
      rfBlocks: [],
      connections: [],
      dboards: []
    };

    const lines = output.split('\n');
    let currentSection = null;
    let currentDboard = null;

    for (const line of lines) {
      const trimmedLine = line.trim();
      
      // Skip empty lines and info messages
      if (!trimmedLine || trimmedLine.startsWith('[INFO]') || trimmedLine.startsWith('[WARNING]')) {
        continue;
      }

      logger.debug('Processing line', { 
        line: trimmedLine, 
        currentSection, 
        hasColon: trimmedLine.includes(':') 
      });

      // Detect mainboard section
      if (trimmedLine.includes('Mboard:') || trimmedLine.includes('Device:')) {
        currentSection = 'mboard';
        logger.debug('Detected mainboard section', { line: trimmedLine });
        continue;
      }

      // Detect RFNoC blocks section
      if (trimmedLine.includes('RFNoC blocks on this device:')) {
        currentSection = 'rfBlocks';
        continue;
      }

      // Detect static connections section
      if (trimmedLine.includes('Static connections on this device:')) {
        currentSection = 'connections';
        continue;
      }

      // Detect dboard sections
      if (trimmedLine.includes('TX Dboard:') || trimmedLine.includes('RX Dboard:')) {
        currentSection = 'dboard';
        currentDboard = {
          type: trimmedLine.includes('TX Dboard:') ? 'TX' : 'RX',
          id: '',
          serial: '',
          frontend: {}
        };
        details.dboards.push(currentDboard);
        continue;
      }

      // Parse mainboard details
      if (currentSection === 'mboard' && trimmedLine.includes(':')) {
        const colonIndex = trimmedLine.indexOf(':');
        if (colonIndex > 0) {
          const key = trimmedLine.substring(0, colonIndex).trim();
          const value = trimmedLine.substring(colonIndex + 1).trim();
          if (key && value) {
            // Clean up pipe characters and extra whitespace from both key and value
            const cleanKey = cleanText(key);
            const cleanValue = cleanText(value);
            if (cleanKey && cleanValue) {
              details.mboard[cleanKey] = cleanValue;
              logger.debug('Added mainboard detail', { key: cleanKey, value: cleanValue });
            }
          }
        }
      }

      // Parse RFNoC blocks
      if (currentSection === 'rfBlocks' && trimmedLine.startsWith('*')) {
        const blockName = trimmedLine.replace('*', '').trim();
        if (blockName) {
          details.rfBlocks.push(blockName);
        }
      }

      // Parse static connections
      if (currentSection === 'connections' && trimmedLine.startsWith('*')) {
        const connection = trimmedLine.replace('*', '').trim();
        if (connection) {
          details.connections.push(connection);
        }
      }

      // Parse dboard details
      if (currentSection === 'dboard' && currentDboard) {
        if (trimmedLine.includes('ID:')) {
          const cleanValue = cleanText(trimmedLine.split('ID:')[1].trim());
          currentDboard.id = cleanValue;
        } else if (trimmedLine.includes('Serial:')) {
          const cleanValue = cleanText(trimmedLine.split('Serial:')[1].trim());
          currentDboard.serial = cleanValue;
        } else if (trimmedLine.includes('Frontend:') && trimmedLine.includes('Name:')) {
          const nameMatch = trimmedLine.match(/Name:\s*(.+)/);
          if (nameMatch) {
            const cleanValue = cleanText(nameMatch[1]);
            currentDboard.frontend.name = cleanValue;
          }
        } else if (trimmedLine.includes('Freq range:')) {
          const freqMatch = trimmedLine.match(/Freq range:\s*(.+)/);
          if (freqMatch) {
            const cleanValue = cleanText(freqMatch[1]);
            currentDboard.frontend.freqRange = cleanValue;
          }
        } else if (trimmedLine.includes('Gain range')) {
          const gainMatch = trimmedLine.match(/Gain range.*:\s*(.+)/);
          if (gainMatch) {
            const cleanValue = cleanText(gainMatch[1]);
            currentDboard.frontend.gainRange = cleanValue;
          }
        }
      }
    }

    return details;
  }

  /**
   * Get formatted response for API
   */
  getFormattedResponse(hasError = false, errorMessage = null) {
    const response = {
      detected: this.cache.devices.length > 0,
      count: this.cache.devices.length,
      devices: this.cache.devices,
      detectionMethods: {
        uhd_find_devices: this.detectionMethods.uhd_find_devices,
        lsusb: this.detectionMethods.lsusb,
        dev: this.detectionMethods.dev
      },
      cache: {
        lastUpdate: this.cache.lastUpdate,
        isUpdating: this.cache.isUpdating,
        cacheAge: this.cache.lastUpdate ? Date.now() - this.cache.lastUpdate : null
      },
      timestamp: new Date().toISOString()
    };

    if (hasError) {
      response.error = errorMessage;
    }

    return response;
  }

  /**
   * Get cache statistics
   */
  getCacheStats() {
    return {
      lastUpdate: this.cache.lastUpdate,
      isUpdating: this.cache.isUpdating,
      cacheAge: this.cache.lastUpdate ? Date.now() - this.cache.lastUpdate : null,
      cacheDuration: this.cache.cacheDuration,
      deviceCount: this.cache.devices.length,
      isCacheValid: this.isCacheValid()
    };
  }

  /**
   * Update cache duration
   */
  setCacheDuration(duration) {
    this.cache.cacheDuration = duration;
    logger.debug('USRP cache duration updated', { duration });
  }

  /**
   * Enable/disable detection methods
   */
  setDetectionMethods(methods) {
    this.detectionMethods = { ...this.detectionMethods, ...methods };
    logger.debug('USRP detection methods updated', { methods: this.detectionMethods });
  }
}

module.exports = UsrpDeviceManager;
