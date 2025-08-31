const express = require('express');
const path = require('path');
const fs = require('fs');
const fsPromises = require('fs').promises;
const logger = require('../logger');

const router = express.Router();

// Get list of log files
router.get('/files', async (req, res) => {
  try {
    logger.api('Log files list requested', { ip: req.ip });
    const logsDir = path.join(__dirname, '../logs');
    
    logger.info('Debug: Checking logs directory', { logsDir, exists: fs.existsSync(logsDir) });
    
    if (!fs.existsSync(logsDir)) {
      logger.warn('Debug: Logs directory does not exist', { logsDir });
      return res.json({ files: [] });
    }
    
    const files = await fsPromises.readdir(logsDir);
    logger.info('Debug: Found files in logs directory', { files });
    const logFiles = [];
    
    for (const file of files) {
      // Include all .log files and rotated log files
      // Handle both regular .log files and rotated files (e.g., combined-2024-12-01.log, combined-2024-12-01.log.1, etc.)
      if (file.endsWith('.log') || file.match(/\.log\.\d+$/)) {
        const filePath = path.join(logsDir, file);
        const stats = await fsPromises.stat(filePath);
        const size = (stats.size / 1024).toFixed(2);
        logFiles.push({
          name: file,
          size: `${size} KB`,
          modified: stats.mtime.toISOString()
        });
        logger.info('Debug: Added log file', { file, size: `${size} KB` });
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

// View log file content
router.get('/view/:filename', async (req, res) => {
  try {
    const { filename } = req.params;
    const lines = parseInt(req.query.lines) || 100;
    
    logger.api('Log file content requested', { filename, lines, ip: req.ip });
    
    // Security: prevent directory traversal
    if (filename.includes('..') || (!filename.endsWith('.log') && !filename.match(/\.log\.\d+$/))) {
      return res.status(400).json({ error: 'Invalid filename' });
    }
    
    const filePath = path.join(__dirname, '../logs', filename);
    
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

// Download log file
router.get('/download/:filename', async (req, res) => {
  try {
    const { filename } = req.params;
    
    logger.api('Log file download requested', { filename, ip: req.ip });
    
    // Security: prevent directory traversal
    if (filename.includes('..') || (!filename.endsWith('.log') && !filename.match(/\.log\.\d+$/))) {
      return res.status(400).json({ error: 'Invalid filename' });
    }
    
    const filePath = path.join(__dirname, '../logs', filename);
    
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

module.exports = router;
