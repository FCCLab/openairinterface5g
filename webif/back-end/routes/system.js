const express = require('express');
const { exec } = require('child_process');
const os = require('os');
const fs = require('fs');
const fsPromises = require('fs').promises;
const logger = require('../logger');

const router = express.Router();

// Helper function to execute shell commands
const execCommand = (command) => {
  return new Promise((resolve, reject) => {
    logger.debug(`Executing command: ${command}`);
    exec(command, { timeout: 5000 }, (error, stdout, stderr) => {
      if (error) {
        logger.error(`Command failed: ${command}`, { error: error.message, stderr });
        reject(error);
        return;
      }
      logger.debug(`Command succeeded: ${command}`, { stdout: stdout.trim() });
      resolve(stdout.trim());
    });
  });
};

// Get CPU information
const getCpuInfo = async () => {
  try {
    logger.debug('Getting CPU information');
    logger.debug('CPU cores count', { cores: os.cpus().length });
    const cpus = os.cpus();
    const loadAvg = os.loadavg();
    
    // Get CPU usage from /proc/stat
    const statContent = await fsPromises.readFile('/proc/stat', 'utf8');
    const cpuLine = statContent.split('\n')[0];
    const cpuValues = cpuLine.split(' ').filter(val => val !== '');
    
    const user = parseInt(cpuValues[1]);
    const nice = parseInt(cpuValues[2]);
    const system = parseInt(cpuValues[3]);
    const idle = parseInt(cpuValues[4]);
    const iowait = parseInt(cpuValues[5]);
    const irq = parseInt(cpuValues[6]);
    const softirq = parseInt(cpuValues[7]);
    
    const total = user + nice + system + idle + iowait + irq + softirq;
    const used = total - idle - iowait;
    const usage = Math.round((used / total) * 100);
    
    const cpuInfo = {
      usage: Math.min(100, Math.max(0, usage)),
      cores: cpus.length,
      frequency: (cpus[0].speed / 1000).toFixed(1),
      load: loadAvg,
      model: cpus[0].model
    };
    
    logger.debug('CPU information retrieved', { cpuInfo });
    logger.debug('CPU usage calculation', { 
      usage: cpuInfo.usage,
      temperature: cpuInfo.temperature,
      load: cpuInfo.load
    });
    return cpuInfo;
  } catch (error) {
    logger.error('Error getting CPU info', { error: error.message, stack: error.stack });
    return {
      usage: 0,
      cores: os.cpus().length,
      frequency: 'N/A',
      load: [0, 0, 0],
      model: 'Unknown'
    };
  }
};

// Get CPU temperature
const getCpuTemperature = async () => {
  try {
    // Try different temperature file locations
    const tempPaths = [
      '/sys/class/thermal/thermal_zone0/temp',
      '/sys/class/hwmon/hwmon0/temp1_input',
      '/sys/class/hwmon/hwmon1/temp1_input'
    ];
    
    for (const tempPath of tempPaths) {
      try {
        const temp = await fs.readFile(tempPath, 'utf8');
        return Math.round(parseInt(temp) / 1000);
      } catch (err) {
        continue;
      }
    }
    
    // Fallback: try using sensors command
    try {
      const sensorsOutput = await execCommand('sensors');
      const tempMatch = sensorsOutput.match(/Core 0:\s+\+(\d+\.\d+)°C/);
      if (tempMatch) {
        return Math.round(parseFloat(tempMatch[1]));
      }
    } catch (err) {
      // Ignore sensors command errors
    }
    
    return null;
  } catch (error) {
    console.error('Error getting CPU temperature:', error);
    return null;
  }
};

// Get memory information
const getMemoryInfo = async () => {
  try {
    const totalMem = os.totalmem();
    const freeMem = os.freemem();
    const usedMem = totalMem - freeMem;
    const memUsage = Math.round((usedMem / totalMem) * 100);
    
    return {
      total: Math.round(totalMem / (1024 * 1024 * 1024) * 100) / 100, // GB
      used: Math.round(usedMem / (1024 * 1024 * 1024) * 100) / 100, // GB
      free: Math.round(freeMem / (1024 * 1024 * 1024) * 100) / 100, // GB
      usage: memUsage
    };
  } catch (error) {
    console.error('Error getting memory info:', error);
    return {
      total: 0,
      used: 0,
      free: 0,
      usage: 0
    };
  }
};

// Get disk information
const getDiskInfo = async () => {
  try {
    const dfOutput = await execCommand('df -h /');
    const lines = dfOutput.split('\n');
    const diskLine = lines[1];
    const parts = diskLine.split(/\s+/);
    
    return {
      total: parts[1],
      used: parts[2],
      available: parts[3],
      usage: parseInt(parts[4].replace('%', ''))
    };
  } catch (error) {
    console.error('Error getting disk info:', error);
    return {
      total: 'N/A',
      used: 'N/A',
      available: 'N/A',
      usage: 0
    };
  }
};

// Get network interfaces
const getNetworkInterfaces = async () => {
  try {
    logger.debug('Getting network interfaces information');
    
    // Get all network interface names using command line
    let interfaceNames = [];
    try {
      const cmdOutput = await execCommand("ls /sys/class/net");
      interfaceNames = cmdOutput.split('\n').map(x => x.trim()).filter(x => x);
    } catch (err) {
      logger.error('Failed to get network interface names from command line', { error: err.message });
      interfaceNames = Object.keys(os.networkInterfaces());
    }
    logger.debug('Found interface names', { interfaces: interfaceNames });
  
    const result = [];
    
    // Get detailed information for each interface
    for (const name of interfaceNames) {
      logger.debug('Processing interface', { name });
      
      try {
        // Get detailed information for this specific interface
        const interfaceInfo = await execCommand(`ip addr show ${name}`);
        logger.debug('Interface detailed info', { interface: name, info: interfaceInfo });
        
        // Get interface status (up/down)
          let status = 'unknown';
          try {
          const operstate = await execCommand(`cat /sys/class/net/${name}/operstate 2>/dev/null || echo "unknown"`);
          if (operstate === 'up') {
            status = 'up';
          } else if (operstate === 'down') {
            status = 'down';
          } else {
            status = 'unknown';
          }
        } catch (err) {
          logger.debug('Error getting interface status', { interface: name, error: err.message });
          status = 'unknown';
        }
        
        // Extract MAC address
        let mac = 'N/A';
        try {
          const macFromSys = await execCommand(`cat /sys/class/net/${name}/address 2>/dev/null || echo "N/A"`);
          if (macFromSys && macFromSys !== 'N/A') {
            mac = macFromSys.trim();
            logger.debug('MAC address from sys', { interface: name, mac: mac });
          }
        } catch (err) {
          logger.debug('Error getting MAC from sys', { interface: name, error: err.message });
        }
        
        // Extract IP addresses from interface info
        const ipAddresses = [];
        const infoLines = interfaceInfo.split('\n');
        for (const line of infoLines) {
          // Match IPv4 addresses
          const ipv4Match = line.match(/inet\s+(\d+\.\d+\.\d+\.\d+)\/(\d+)/);
          if (ipv4Match) {
            ipAddresses.push({
              address: ipv4Match[1],
              netmask: ipv4Match[2],
              family: 'IPv4'
            });
          }
          
          // Match IPv6 addresses
          const ipv6Match = line.match(/inet6\s+([0-9a-fA-F:]+)\/(\d+)/);
          if (ipv6Match) {
            ipAddresses.push({
              address: ipv6Match[1],
              netmask: ipv6Match[2],
              family: 'IPv6'
            });
          }
        }
        
        logger.debug('Interface IP addresses', { interface: name, ipCount: ipAddresses.length, ips: ipAddresses });
          
          // Get interface speed if possible
          let speed = 'N/A';
          try {
            const speedOutput = await execCommand(`cat /sys/class/net/${name}/speed 2>/dev/null || echo "N/A"`);
            if (speedOutput !== 'N/A') {
              speed = `${speedOutput} Mbps`;
            }
          } catch (err) {
            // Ignore speed reading errors
          }
          
          // Determine interface type
          let type = 'Unknown';
          if (name.startsWith('eth') || name.startsWith('en')) {
            type = 'Ethernet';
          } else if (name.startsWith('wlan') || name.startsWith('wl')) {
            type = 'WiFi';
          } else if (name === 'lo') {
            type = 'Loopback';
          } else if (name.startsWith('docker')) {
            type = 'Bridge';
          } else if (name.startsWith('veth')) {
            type = 'Virtual';
          } else if (name.startsWith('tun') || name.startsWith('tap')) {
            type = 'Tunnel';
          }
          
        // Create interface object with all IP addresses
        const interfaceObj = {
            name,
          status: status,
          mac,
            type,
          speed,
          ipAddresses: ipAddresses,
          primaryIp: ipAddresses.find(ip => ip.family === 'IPv4')?.address || ipAddresses[0]?.address || 'N/A'
        };
        
        result.push(interfaceObj);
        
      } catch (err) {
        logger.error('Error processing interface', { interface: name, error: err.message });
      }
    }
    
    logger.debug('Network interfaces retrieved', { count: result.length, interfaces: result.map(i => ({ name: i.name, ipCount: i.ipAddresses.length })) });
    logger.debug('Network interface details', {
      totalInterfaces: result.length,
      upInterfaces: result.filter(iface => iface.status === 'up').length,
      downInterfaces: result.filter(iface => iface.status === 'down').length,
      unknownInterfaces: result.filter(iface => iface.status === 'unknown').length,
      totalIpAddresses: result.reduce((sum, iface) => sum + iface.ipAddresses.length, 0)
    });
    
    return result;
  } catch (error) {
    logger.error('Error getting network interfaces', { error: error.message, stack: error.stack });
    return [];
  }
};

// Get system uptime and boot time
const getSystemUptime = () => {
  const uptime = os.uptime();
  const bootTime = new Date(Date.now() - uptime * 1000);
  
  return {
    uptime: uptime,
    bootTime: bootTime.toISOString(),
    formattedUptime: formatUptime(uptime)
  };
};

// Format uptime to human readable
const formatUptime = (seconds) => {
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  
  if (days > 0) {
    return `${days}d ${hours}h ${minutes}m`;
  } else if (hours > 0) {
    return `${hours}h ${minutes}m`;
  } else {
    return `${minutes}m`;
  }
};

// Get USRP device information using the dedicated manager
const getUsrpDevices = async () => {
  try {
    const usrpDeviceManager = require('../UsrpDeviceManager');
    return await usrpDeviceManager.getUsrpDevices();
  } catch (error) {
    logger.error('Error getting USRP devices', { error: error.message, stack: error.stack });
    return {
      detected: false,
      count: 0,
      devices: [],
      error: error.message
    };
  }
};

// CPU endpoint
router.get('/cpu', async (req, res) => {
  try {
    logger.api('CPU information requested', { ip: req.ip });
    const cpuInfo = await getCpuInfo();
    const temperature = await getCpuTemperature();
    
    const response = {
      ...cpuInfo,
      temperature: temperature || 0
    };
    
    logger.api('CPU information provided', { cpuInfo: response });
    res.json(response);
  } catch (error) {
    logger.error('Error in CPU API', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ error: 'Failed to get CPU information' });
  }
});

// Memory endpoint
router.get('/memory', async (req, res) => {
  try {
    const memoryInfo = await getMemoryInfo();
    res.json(memoryInfo);
  } catch (error) {
    console.error('Error in Memory API:', error);
    res.status(500).json({ error: 'Failed to get memory information' });
  }
});

// Disk endpoint
router.get('/disk', async (req, res) => {
  try {
    const diskInfo = await getDiskInfo();
    res.json(diskInfo);
  } catch (error) {
    console.error('Error in Disk API:', error);
    res.status(500).json({ error: 'Failed to get disk information' });
  }
});

// Network endpoint
router.get('/network', async (req, res) => {
  try {
    const interfaces = await getNetworkInterfaces();
    res.json(interfaces);
  } catch (error) {
    console.error('Error in Network API:', error);
    res.status(500).json({ error: 'Failed to get network information' });
  }
});

// Uptime endpoint
router.get('/uptime', (req, res) => {
  try {
    const uptimeInfo = getSystemUptime();
    res.json(uptimeInfo);
  } catch (error) {
    console.error('Error in Uptime API:', error);
    res.status(500).json({ error: 'Failed to get uptime information' });
  }
});

// System info endpoint
router.get('/info', async (req, res) => {
  try {
    logger.api('System information requested', { ip: req.ip });
    logger.debug('System info request started', { 
      ip: req.ip, 
      userAgent: req.get('User-Agent'),
      timestamp: new Date().toISOString()
    });
    const startTime = Date.now();
    
    const [cpuInfo, interfaces, memoryInfo, diskInfo, usrpInfo] = await Promise.all([
      getCpuInfo(),
      getNetworkInterfaces(),
      getMemoryInfo(),
      getDiskInfo(),
      getUsrpDevices()
    ]);
    
    const temperature = await getCpuTemperature();
    const uptimeInfo = getSystemUptime();
    
    const response = {
      cpu: {
        ...cpuInfo,
        temperature: temperature || 0
      },
      memory: memoryInfo,
      disk: diskInfo,
      network: interfaces,
      usrp: usrpInfo,
      system: {
        hostname: os.hostname(),
        platform: os.platform(),
        arch: os.arch(),
        uptime: uptimeInfo,
        nodeVersion: process.version,
        pid: process.pid
      }
    };
    
    const responseTime = Date.now() - startTime;
    logger.api('System information provided', { 
      responseTime: `${responseTime}ms`,
      networkInterfaces: interfaces.length,
      ip: req.ip 
    });
    logger.debug('System info response details', {
      responseTime: `${responseTime}ms`,
      cpuInfo: !!cpuInfo,
      memoryInfo: !!memoryInfo,
      diskInfo: !!diskInfo,
      networkInterfaces: interfaces.length,
      usrpDevices: usrpInfo.count,
      temperature: temperature,
      uptimeInfo: !!uptimeInfo
    });
    
    res.json(response);
  } catch (error) {
    logger.error('Error in System Info API', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ error: 'Failed to get system information' });
  }
});

module.exports = router;
