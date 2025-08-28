#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

const logsDir = path.join(__dirname, 'logs');

function listLogFiles() {
  if (!fs.existsSync(logsDir)) {
    console.log('📁 No logs directory found. Run the server first to generate logs.');
    return;
  }

  const files = fs.readdirSync(logsDir);
  const logFiles = files.filter(file => file.endsWith('.log'));

  if (logFiles.length === 0) {
    console.log('📁 No log files found in logs directory.');
    return;
  }

  console.log('📁 Available log files:');
  logFiles.forEach(file => {
    const filePath = path.join(logsDir, file);
    const stats = fs.statSync(filePath);
    const size = (stats.size / 1024).toFixed(2);
    console.log(`  📄 ${file} (${size} KB)`);
  });
}

function viewLogFile(filename, lines = 50) {
  const filePath = path.join(logsDir, filename);
  
  if (!fs.existsSync(filePath)) {
    console.log(`❌ Log file ${filename} not found.`);
    listLogFiles();
    return;
  }

  const content = fs.readFileSync(filePath, 'utf8');
  const logLines = content.split('\n').filter(line => line.trim());
  
  console.log(`📄 Viewing last ${lines} lines of ${filename}:`);
  console.log('─'.repeat(80));
  
  const lastLines = logLines.slice(-lines);
  lastLines.forEach(line => {
    try {
      const logEntry = JSON.parse(line);
      const timestamp = new Date(logEntry.timestamp).toLocaleString();
      const level = logEntry.level.toUpperCase();
      const message = logEntry.message;
      const meta = logEntry.type ? `[${logEntry.type}]` : '';
      
      console.log(`[${timestamp}] [${level}] ${meta} ${message}`);
      
      if (logEntry.error) {
        console.log(`  Error: ${logEntry.error}`);
      }
    } catch (e) {
      // If not JSON, print as-is
      console.log(line);
    }
  });
}

function tailLogFile(filename) {
  const filePath = path.join(logsDir, filename);
  
  if (!fs.existsSync(filePath)) {
    console.log(`❌ Log file ${filename} not found.`);
    listLogFiles();
    return;
  }

  console.log(`📄 Tailing ${filename} (Press Ctrl+C to stop):`);
  console.log('─'.repeat(80));
  
  const tail = require('child_process').spawn('tail', ['-f', filePath]);
  
  tail.stdout.on('data', (data) => {
    const lines = data.toString().split('\n');
    lines.forEach(line => {
      if (line.trim()) {
        try {
          const logEntry = JSON.parse(line);
          const timestamp = new Date(logEntry.timestamp).toLocaleString();
          const level = logEntry.level.toUpperCase();
          const message = logEntry.message;
          const meta = logEntry.type ? `[${logEntry.type}]` : '';
          
          console.log(`[${timestamp}] [${level}] ${meta} ${message}`);
        } catch (e) {
          console.log(line);
        }
      }
    });
  });
  
  tail.stderr.on('data', (data) => {
    console.error(`Tail error: ${data}`);
  });
  
  process.on('SIGINT', () => {
    tail.kill();
    process.exit(0);
  });
}

// Parse command line arguments
const args = process.argv.slice(2);
const command = args[0];
const filename = args[1];
const lines = parseInt(args[2]) || 50;

switch (command) {
  case 'list':
  case 'ls':
    listLogFiles();
    break;
    
  case 'view':
  case 'cat':
    if (!filename) {
      console.log('Usage: node view-logs.js view <filename> [lines]');
      console.log('Example: node view-logs.js view combined-2024-01-15.log 100');
      listLogFiles();
    } else {
      viewLogFile(filename, lines);
    }
    break;
    
  case 'tail':
  case 'follow':
    if (!filename) {
      console.log('Usage: node view-logs.js tail <filename>');
      console.log('Example: node view-logs.js tail combined-2024-01-15.log');
      listLogFiles();
    } else {
      tailLogFile(filename);
    }
    break;
    
  default:
    console.log('📝 OpenAirInterface5G Backend Log Viewer');
    console.log('');
    console.log('Usage:');
    console.log('  node view-logs.js list                    - List available log files');
    console.log('  node view-logs.js view <filename> [lines] - View log file (default: 50 lines)');
    console.log('  node view-logs.js tail <filename>         - Follow log file in real-time');
    console.log('');
    console.log('Examples:');
    console.log('  node view-logs.js list');
    console.log('  node view-logs.js view combined-2024-01-15.log');
    console.log('  node view-logs.js view error-2024-01-15.log 100');
    console.log('  node view-logs.js tail combined-2024-01-15.log');
    console.log('');
    listLogFiles();
}
