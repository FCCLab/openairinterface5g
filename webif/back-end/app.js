const express = require('express');
const path = require('path');
const { exec } = require('child_process');
const os = require('os');
const fs = require('fs');
const fsPromises = require('fs').promises;
const cors = require('cors');
const helmet = require('helmet');
const morgan = require('morgan');
const compression = require('compression');
const rateLimit = require('express-rate-limit');
const WebSocket = require('ws');
const logger = require('./logger');
const UsrpDeviceManager = require('./UsrpDeviceManager');

const app = express();
const port = process.env.PORT || 40000;

// Initialize USRP Device Manager
const usrpDeviceManager = new UsrpDeviceManager();

// Create HTTP server for WebSocket
const server = require('http').createServer(app);

// Create WebSocket server
const wss = new WebSocket.Server({ server });

// Store WebSocket connections
let wsConnections = new Set();

// WebSocket connection handling
wss.on('connection', (ws, req) => {
  const clientId = `${req.socket.remoteAddress}-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;
  
  logger.info('WebSocket client connected', { 
    clientId, 
    ip: req.socket.remoteAddress,
    totalConnections: wsConnections.size + 1
  });
  
  // Add to connections set
  wsConnections.add(ws);
  
  // Send welcome message
  ws.send(JSON.stringify({
    type: 'connected',
    clientId,
    message: 'WebSocket connected for spectrogram data'
  }));
  
  // Handle client disconnect
  ws.on('close', () => {
    wsConnections.delete(ws);
    logger.info('WebSocket client disconnected', { 
      clientId, 
      totalConnections: wsConnections.size 
    });
  });
  
  // Handle client errors
  ws.on('error', (error) => {
    logger.warn('WebSocket client error', { 
      clientId, 
      error: error.message 
    });
    wsConnections.delete(ws);
  });
});

// Security middleware
app.use(helmet({
  contentSecurityPolicy: {
    directives: {
      defaultSrc: ["'self'"],
      styleSrc: ["'self'", "'unsafe-inline'"],
      scriptSrc: ["'self'"],
      imgSrc: ["'self'", "data:", "https:"],
    },
  },
}));

// Rate limiting
const limiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 1000, // limit each IP to 100 requests per windowMs
  message: 'Too many requests from this IP, please try again later.'
});
app.use('/api/', limiter);

// Stricter rate limiting for spectrogram APIs
const spectrogramLimiter = rateLimit({
  windowMs: 1 * 60 * 1000, // 1 minute
  max: 60, // limit each IP to 60 requests per minute (1 per second)
  message: 'Spectrogram API rate limit exceeded. Please reduce request frequency.',
  standardHeaders: true,
  legacyHeaders: false,
  handler: (req, res) => {
    logger.warn('Spectrogram rate limit exceeded', {
      ip: req.ip,
      userAgent: req.get('User-Agent'),
      path: req.path
    });
    res.status(429).json({
      error: 'Spectrogram API rate limit exceeded',
      message: 'Too many spectrogram requests. Please wait before trying again.',
      retryAfter: Math.ceil(req.rateLimit.resetTime / 1000)
    });
  }
});

// Compression middleware
app.use(compression());

// Logging middleware
app.use(morgan('combined', { stream: logger.stream }));

// CORS configuration
const corsOptions = {
  origin: process.env.NODE_ENV === 'production' 
    ? ['http://localhost:41000', 'https://yourdomain.com'] 
    : ['http://localhost:41000', 'http://localhost:40000'],
  credentials: true,
  optionsSuccessStatus: 200
};
app.use(cors(corsOptions));

// Body parsing middleware
app.use(express.json({ limit: '10mb' }));
app.use(express.urlencoded({ extended: true, limit: '10mb' }));

// Request logging middleware
app.use((req, res, next) => {
  logger.api(`${req.method} ${req.path}`, {
    ip: req.ip,
    userAgent: req.get('User-Agent'),
    referer: req.get('Referer')
  });
  
  // Debug logging for request details
  logger.debug('Request details', {
    method: req.method,
    url: req.url,
    path: req.path,
    query: req.query,
    headers: {
      'content-type': req.get('Content-Type'),
      'accept': req.get('Accept'),
      'user-agent': req.get('User-Agent')
    },
    ip: req.ip,
    timestamp: new Date().toISOString()
  });
  
  next();
});

// Security middleware for spectrogram endpoints
const validateSpectrogramRequest = (req, res, next) => {
  // For SSE connections, be more lenient with origin checking
  const isSSE = req.path.includes('/stream');
  
  if (!isSSE) {
    // Check if request is from allowed origins (only for non-SSE requests)
    const origin = req.get('Origin');
    const referer = req.get('Referer');
    const allowedOrigins = ['http://localhost:41000', 'http://localhost:40000'];
    
    if (!allowedOrigins.includes(origin) && (!referer || !allowedOrigins.some(allowed => referer.includes(allowed)))) {
      logger.warn('Unauthorized spectrogram request', {
        ip: req.ip,
        origin,
        referer,
        userAgent: req.get('User-Agent'),
        path: req.path
      });
      return res.status(403).json({ error: 'Unauthorized access to spectrogram API' });
    }
  } else {
    // For SSE connections, be more permissive - allow any origin
    logger.debug('SSE connection request', {
      ip: req.ip,
      origin: req.get('Origin'),
      userAgent: req.get('User-Agent'),
      path: req.path
    });
  }
  
  // Check for suspicious patterns
  const userAgent = req.get('User-Agent') || '';
  if (userAgent.includes('bot') || userAgent.includes('crawler') || userAgent.includes('scraper')) {
    logger.warn('Bot detected accessing spectrogram API', {
      ip: req.ip,
      userAgent,
      path: req.path
    });
    return res.status(403).json({ error: 'Bot access not allowed' });
  }
  
  next();
};

// Store connected clients for server-push
let connectedClients = new Map();

// Generate spectrogram data and push to all connected clients
const generateSpectrogramData = () => {
  try {
    logger.info('Generating spectrogram data', { 
      activeStreams: spectrogramStreams.size,
      connectedClients: connectedClients.size,
      hasInterval: !!spectrogramInterval 
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
      data: data
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
    
    // Send data to all WebSocket clients (true server-push)
    let wsSentCount = 0;
    wsConnections.forEach((ws) => {
      try {
        if (ws.readyState === WebSocket.OPEN) {
          ws.send(JSON.stringify({
            type: 'spectrogram_data',
            data: spectrogramData
          }));
          wsSentCount++;
        }
      } catch (error) {
        logger.warn('Failed to send data to WebSocket client', { error: error.message });
        wsConnections.delete(ws);
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
      wsConnections: wsConnections.size,
      sentToClients: sentCount,
      wsSentCount,
      dataPoints: data.length 
    });
  } catch (error) {
    logger.error('Error generating spectrogram data', { error: error.message });
  }
};

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

// API Routes
app.get('/api/system/cpu', async (req, res) => {
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

app.get('/api/system/memory', async (req, res) => {
  try {
    const memoryInfo = await getMemoryInfo();
    res.json(memoryInfo);
  } catch (error) {
    console.error('Error in Memory API:', error);
    res.status(500).json({ error: 'Failed to get memory information' });
  }
});

app.get('/api/system/disk', async (req, res) => {
  try {
    const diskInfo = await getDiskInfo();
    res.json(diskInfo);
  } catch (error) {
    console.error('Error in Disk API:', error);
    res.status(500).json({ error: 'Failed to get disk information' });
  }
});

app.get('/api/system/network', async (req, res) => {
  try {
    const interfaces = await getNetworkInterfaces();
    res.json(interfaces);
  } catch (error) {
    console.error('Error in Network API:', error);
    res.status(500).json({ error: 'Failed to get network information' });
  }
});

app.get('/api/system/uptime', (req, res) => {
  try {
    const uptimeInfo = getSystemUptime();
    res.json(uptimeInfo);
  } catch (error) {
    console.error('Error in Uptime API:', error);
    res.status(500).json({ error: 'Failed to get uptime information' });
  }
});

// USRP Device Management APIs
app.get('/api/usrp/devices', async (req, res) => {
  try {
    logger.api('USRP devices requested', { ip: req.ip });
    const usrpInfo = await usrpDeviceManager.getUsrpDevices();
    res.json(usrpInfo);
  } catch (error) {
    logger.error('Error in USRP devices API', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ error: 'Failed to get USRP devices' });
  }
});

app.post('/api/usrp/refresh', async (req, res) => {
  try {
    logger.api('USRP cache refresh requested', { ip: req.ip });
    const usrpInfo = await usrpDeviceManager.forceUpdate();
    res.json(usrpInfo);
  } catch (error) {
    logger.error('Error in USRP refresh API', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ error: 'Failed to refresh USRP devices' });
  }
});

app.get('/api/usrp/cache/stats', (req, res) => {
  try {
    logger.api('USRP cache stats requested', { ip: req.ip });
    const stats = usrpDeviceManager.getCacheStats();
    res.json(stats);
  } catch (error) {
    logger.error('Error in USRP cache stats API', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ error: 'Failed to get USRP cache stats' });
  }
});

app.get('/api/usrp/device/details', async (req, res) => {
  try {
    const { deviceArgs } = req.query;
    if (!deviceArgs) {
      return res.status(400).json({ error: 'deviceArgs parameter is required' });
    }

    logger.api('USRP device details requested', { ip: req.ip, deviceArgs });
    const details = await usrpDeviceManager.getDeviceDetails(deviceArgs);
    res.json(details);
  } catch (error) {
    logger.error('Error in USRP device details API', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ error: 'Failed to get USRP device details' });
  }
});

app.get('/api/system/info', async (req, res) => {
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

// Health check endpoint
app.get('/health', (req, res) => {
  res.json({ 
    status: 'OK', 
    uptime: process.uptime(),
    timestamp: new Date().toISOString(),
    version: '1.0.0'
  });
});

// Spectrogram streaming state
let spectrogramStreams = new Map(); // Map to track active streams
let spectrogramInterval = null; // Interval for generating data

// Spectrogram API endpoints with strict rate limiting and security
app.post('/api/spectrogram/start', validateSpectrogramRequest, spectrogramLimiter, async (req, res) => {
  try {
    logger.api('Spectrogram start requested', { ip: req.ip, settings: req.body });
    
    // Start spectrogram data generation if not already running
    if (!spectrogramInterval) {
      spectrogramInterval = setInterval(() => {
        generateSpectrogramData();
      }, 100); // Generate data every second
      logger.debug('Spectrogram data generation started');
    }
    
    const response = {
      success: true,
      message: 'Spectrogram capture started',
      settings: req.body
    };
    
    logger.api('Spectrogram start successful', { ip: req.ip });
    res.json(response);
  } catch (error) {
    logger.error('Error starting spectrogram', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ success: false, error: 'Failed to start spectrogram capture' });
  }
});

app.post('/api/spectrogram/stop', validateSpectrogramRequest, spectrogramLimiter, async (req, res) => {
  try {
    logger.api('Spectrogram stop requested', { ip: req.ip });
    
    // TODO: Implement actual spectrogram stop logic
    // For now, return success response
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
app.options('/api/spectrogram/stream', (req, res) => {
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
app.get('/api/spectrogram/stream', async (req, res) => {
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
app.post('/api/spectrogram/register', async (req, res) => {
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
app.get('/api/spectrogram/data/:clientId', async (req, res) => {
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
app.get('/api/spectrogram/data', spectrogramLimiter, async (req, res) => {
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

// Logs API endpoints
app.get('/api/logs/files', async (req, res) => {
  try {
    logger.api('Log files list requested', { ip: req.ip });
    const logsDir = path.join(__dirname, 'logs');
    
    if (!fs.existsSync(logsDir)) {
      return res.json({ files: [] });
    }
    
    const files = await fsPromises.readdir(logsDir);
    const logFiles = [];
    
    for (const file of files) {
      if (file.endsWith('.log')) {
        const filePath = path.join(logsDir, file);
        const stats = await fsPromises.stat(filePath);
        const size = (stats.size / 1024).toFixed(2);
        logFiles.push({
          name: file,
          size: `${size} KB`,
          modified: stats.mtime.toISOString()
        });
      }
    }
    
    // Sort by modification time (newest first)
    logFiles.sort((a, b) => new Date(b.modified) - new Date(a.modified));
    
    logger.api('Log files list provided', { count: logFiles.length, ip: req.ip });
    res.json({ files: logFiles });
  } catch (error) {
    logger.error('Error getting log files list', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ error: 'Failed to get log files list' });
  }
});

app.get('/api/logs/view/:filename', async (req, res) => {
  try {
    const { filename } = req.params;
    const lines = parseInt(req.query.lines) || 100;
    
    logger.api('Log file content requested', { filename, lines, ip: req.ip });
    
    // Security: prevent directory traversal
    if (filename.includes('..') || !filename.endsWith('.log')) {
      return res.status(400).json({ error: 'Invalid filename' });
    }
    
    const filePath = path.join(__dirname, 'logs', filename);
    
    if (!fs.existsSync(filePath)) {
      return res.status(404).json({ error: 'Log file not found' });
    }
    
    const content = await fsPromises.readFile(filePath, 'utf8');
    const logLines = content.split('\n').filter(line => line.trim());
    const lastLines = logLines.slice(-lines);
    
    const logs = lastLines.map(line => {
      try {
        return JSON.parse(line);
      } catch (e) {
        return { message: line, timestamp: new Date().toISOString(), level: 'info' };
      }
    });
    
    logger.api('Log file content provided', { filename, lines: logs.length, ip: req.ip });
    res.json({ logs });
  } catch (error) {
    logger.error('Error reading log file', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ error: 'Failed to read log file' });
  }
});

app.get('/api/logs/download/:filename', async (req, res) => {
  try {
    const { filename } = req.params;
    
    logger.api('Log file download requested', { filename, ip: req.ip });
    
    // Security: prevent directory traversal
    if (filename.includes('..') || !filename.endsWith('.log')) {
      return res.status(400).json({ error: 'Invalid filename' });
    }
    
    const filePath = path.join(__dirname, 'logs', filename);
    
    if (!fs.existsSync(filePath)) {
      return res.status(404).json({ error: 'Log file not found' });
    }
    
    res.download(filePath, filename);
    logger.api('Log file download completed', { filename, ip: req.ip });
  } catch (error) {
    logger.error('Error downloading log file', { error: error.message, stack: error.stack, ip: req.ip });
    res.status(500).json({ error: 'Failed to download log file' });
  }
});

// API documentation endpoint
app.get('/api', (req, res) => {
  res.json({
    name: 'OpenAirInterface5G API',
    version: '1.0.0',
    description: 'System monitoring API for OpenAirInterface5G',
    endpoints: {
      '/api/system/cpu': 'Get CPU information and temperature',
      '/api/system/memory': 'Get memory usage information',
      '/api/system/disk': 'Get disk usage information',
      '/api/system/network': 'Get network interfaces information',
      '/api/system/uptime': 'Get system uptime information',
      '/api/system/info': 'Get comprehensive system information',
      '/api/logs/files': 'Get list of available log files',
      '/api/logs/view/:filename': 'Get log file content',
      '/api/logs/download/:filename': 'Download log file',
      '/health': 'Health check endpoint'
    }
  });
});

// Serve static files in production
if (process.env.NODE_ENV === 'production') {
  app.use(express.static(path.join(__dirname, '../front-end/build')));
  
  app.get('*', (req, res) => {
    res.sendFile(path.join(__dirname, '../front-end/build', 'index.html'));
  });
} else {
  // 404 handler for development - only for undefined API routes
  app.use('/api/*', (req, res) => {
    logger.warn('API endpoint not found', { 
      url: req.url, 
      method: req.method, 
      ip: req.ip 
    });
    res.status(404).json({ error: 'API endpoint not found' });
  });
}

// Error handling middleware
app.use((err, req, res, next) => {
  logger.error('Unhandled error', {
    error: err.message,
    stack: err.stack,
    url: req.url,
    method: req.method,
    ip: req.ip,
    userAgent: req.get('User-Agent')
  });
  
  res.status(500).json({ 
    error: 'Something went wrong!',
    message: process.env.NODE_ENV === 'development' ? err.message : 'Internal server error'
  });
});

// Graceful shutdown
process.on('SIGTERM', () => {
  logger.system('SIGTERM received, shutting down gracefully');
  process.exit(0);
});

process.on('SIGINT', () => {
  logger.system('SIGINT received, shutting down gracefully');
  process.exit(0);
});

server.listen(port, () => {
  logger.system(`🚀 OpenAirInterface5G Backend Server started`, {
    port,
    environment: process.env.NODE_ENV || 'development',
    nodeVersion: process.version,
    pid: process.pid,
    websocket: 'enabled'
  });
  
  console.log(`🚀 OpenAirInterface5G Backend Server listening on port ${port}`);
  console.log(`🔌 WebSocket server enabled on port ${port}`);
  console.log(`📊 Health check: http://localhost:${port}/health`);
  console.log(`📚 API docs: http://localhost:${port}/api`);
  console.log(`🌍 Environment: ${process.env.NODE_ENV || 'development'}`);
  console.log(`📝 Logs: ./logs/`);
});
