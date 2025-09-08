const winston = require('winston');
const DailyRotateFile = require('winston-daily-rotate-file');
const path = require('path');
const fs = require('fs');

// Create logs directory if it doesn't exist
const logsDir = path.join(__dirname, 'logs');
if (!fs.existsSync(logsDir)) {
  fs.mkdirSync(logsDir, { recursive: true });
}

// Custom format for console output (strongSwan style)
const consoleFormat = winston.format.combine(
  winston.format.timestamp({ format: 'YYYY-MM-DD HH:mm:ss' }),
  winston.format.errors({ stack: true }),
  winston.format.printf(({ timestamp, level, message, stack, ...meta }) => {
    const metaStr = Object.keys(meta).length ? JSON.stringify(meta, null, 2) : '';
    const stackStr = stack ? `\n${stack}` : '';
    return `[${timestamp}] [${level.toUpperCase()}] ${message}${metaStr}${stackStr}`;
  })
);

// Custom format for file output (JSON format for better parsing)
const fileFormat = winston.format.combine(
  winston.format.timestamp(),
  winston.format.errors({ stack: true }),
  winston.format.json()
);

// Create the main logger
const logger = winston.createLogger({
  level: process.env.LOG_LEVEL || 'debug',
  format: fileFormat,
  defaultMeta: { 
    service: 'openairinterface5g-backend',
    version: '1.0.0'
  },
  transports: [
    // Error log file (daily rotation)
    new DailyRotateFile({
      filename: path.join(logsDir, 'error-%DATE%.log'),
      datePattern: 'YYYY-MM-DD',
      level: 'error',
      maxSize: '20m',
      maxFiles: '14d', // Keep 14 days of error logs
      zippedArchive: true,
      format: fileFormat
    }),
    
    // Combined log file (daily rotation)
    new DailyRotateFile({
      filename: path.join(logsDir, 'combined-%DATE%.log'),
      datePattern: 'YYYY-MM-DD',
      maxSize: '20m',
      maxFiles: '30d', // Keep 30 days of combined logs
      zippedArchive: true,
      format: fileFormat
    }),
    
    // API access log file (daily rotation)
    new DailyRotateFile({
      filename: path.join(logsDir, 'access-%DATE%.log'),
      datePattern: 'YYYY-MM-DD',
      maxSize: '20m',
      maxFiles: '30d',
      zippedArchive: true,
      format: fileFormat,
      level: 'info'
    })
  ]
});

// Create a separate spectrogram logger
const spectrogramLogger = winston.createLogger({
  level: process.env.LOG_LEVEL || 'debug',
  format: fileFormat,
  defaultMeta: { 
    service: 'openairinterface5g-backend',
    version: '1.0.0'
  },
  transports: [
    // Spectrogram log file (daily rotation) - only spectrogram functionality
    new DailyRotateFile({
      filename: path.join(logsDir, 'spectrogram-%DATE%.log'),
      datePattern: 'YYYY-MM-DD',
      maxSize: '20m',
      maxFiles: '30d',
      zippedArchive: true,
      format: fileFormat,
      level: 'debug'
    })
  ]
});

// Create a separate UE logger for real UE manager
const ueLogger = winston.createLogger({
  level: process.env.LOG_LEVEL || 'debug',
  format: fileFormat,
  defaultMeta: { 
    service: 'openairinterface5g-backend',
    version: '1.0.0',
    component: 'ue-manager'
  },
  transports: [
    // UE log file (daily rotation) - only UE functionality
    new DailyRotateFile({
      filename: path.join(logsDir, 'ue-%DATE%.log'),
      datePattern: 'YYYY-MM-DD',
      maxSize: '20m',
      maxFiles: '30d',
      zippedArchive: true,
      format: fileFormat,
      level: 'debug'
    }),
    // Console transport for UE logs
    new winston.transports.Console({
      format: winston.format.combine(
        winston.format.colorize(),
        winston.format.timestamp({ format: 'YYYY-MM-DD HH:mm:ss' }),
        winston.format.printf(({ timestamp, level, message, ...meta }) => {
          const metaStr = Object.keys(meta).length ? JSON.stringify(meta, null, 2) : '';
          return `[UE] ${timestamp} ${level}: ${message} ${metaStr}`;
        })
      ),
      level: 'debug'
    })
  ]
});

// Add console transport in development
if (process.env.NODE_ENV !== 'production') {
  logger.add(new winston.transports.Console({
    format: consoleFormat,
    level: 'debug'
  }));
}

// Create a stream for Morgan HTTP logging
logger.stream = {
  write: (message) => {
    logger.info(message.trim(), { type: 'http' });
  }
};

// Helper functions for different log types
logger.api = (message, meta = {}) => {
  logger.info(message, { ...meta, type: 'api' });
};

logger.system = (message, meta = {}) => {
  logger.info(message, { ...meta, type: 'system' });
};

logger.security = (message, meta = {}) => {
  logger.warn(message, { ...meta, type: 'security' });
};

logger.performance = (message, meta = {}) => {
  logger.info(message, { ...meta, type: 'performance' });
};

logger.debug = (message, meta = {}) => {
  logger.log('debug', message, { ...meta, type: 'debug' });
};

logger.spectrogram = (message, meta = {}) => {
  spectrogramLogger.info(message, { ...meta, type: 'spectrogram' });
};

// Helper function for spectrogram logs with custom level
logger.spectrogramLevel = (level, message, meta = {}) => {
  spectrogramLogger.log(level, message, { ...meta, type: 'spectrogram' });
};

// UE logger helper functions
logger.ue = (message, meta = {}) => {
  ueLogger.info(message, { ...meta, type: 'ue' });
};

logger.ueLevel = (level, message, meta = {}) => {
  ueLogger.log(level, message, { ...meta, type: 'ue' });
};

// Export the UE logger for direct access if needed
logger.ueLogger = ueLogger;

// Log uncaught exceptions and unhandled rejections
logger.exceptions.handle(
  new DailyRotateFile({
    filename: path.join(logsDir, 'exceptions-%DATE%.log'),
    datePattern: 'YYYY-MM-DD',
    maxSize: '20m',
    maxFiles: '14d',
    zippedArchive: true,
    format: fileFormat
  })
);

logger.rejections.handle(
  new DailyRotateFile({
    filename: path.join(logsDir, 'rejections-%DATE%.log'),
    datePattern: 'YYYY-MM-DD',
    maxSize: '20m',
    maxFiles: '14d',
    zippedArchive: true,
    format: fileFormat
  })
);

module.exports = logger;
