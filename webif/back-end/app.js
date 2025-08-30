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
const usrpDeviceManager = require('./UsrpDeviceManager');
const systemRoutes = require('./routes/system');
const usrpRoutes = require('./routes/usrp');
const logsRoutes = require('./routes/logs');
const spectrogramRoutes = require('./routes/spectrogram');

const app = express();
const port = process.env.PORT || 40000;

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

// API Routes
app.use('/api/system', systemRoutes);
app.use('/api/usrp', usrpRoutes);
app.use('/api/logs', logsRoutes);
app.use('/api/spectrogram', spectrogramRoutes);

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

// Health check endpoint
app.get('/health', (req, res) => {
  res.json({ 
    status: 'OK', 
    uptime: process.uptime(),
    timestamp: new Date().toISOString(),
    version: '1.0.0'
  });
});

// Spectrogram Service Management
let spectrogramServiceProcess = null;
let spectrogramServiceStatus = 'stopped';

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
      '/api/spectrogram/service/start': 'Start the spectrogram Python service',
      '/api/spectrogram/service/stop': 'Stop the spectrogram Python service',
      '/api/spectrogram/service/status': 'Get spectrogram service status',
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
