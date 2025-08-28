import React, { useState, useEffect } from 'react';
import axios from 'axios';

function LogsPage() {
  const [logFiles, setLogFiles] = useState([]);
  const [selectedFile, setSelectedFile] = useState('');
  const [logContent, setLogContent] = useState([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [autoRefresh, setAutoRefresh] = useState(false);
  const [logLevel, setLogLevel] = useState('all');
  const [searchTerm, setSearchTerm] = useState('');
  const [lines, setLines] = useState(100);

  // Fetch available log files
  const fetchLogFiles = async () => {
    try {
      setError(null);
      const response = await axios.get('/api/logs/files');
      setLogFiles(response.data.files || []);
    } catch (err) {
      console.error('Error fetching log files:', err);
      setError('Failed to fetch log files');
    }
  };

  // Fetch log content
  const fetchLogContent = async (filename, lineCount = 100) => {
    if (!filename) return;
    
    try {
      setLoading(true);
      setError(null);
      const response = await axios.get(`/api/logs/view/${filename}`, {
        params: { lines: lineCount }
      });
      setLogContent(response.data.logs || []);
    } catch (err) {
      console.error('Error fetching log content:', err);
      setError('Failed to fetch log content');
    } finally {
      setLoading(false);
    }
  };

  // Auto-refresh log content
  useEffect(() => {
    if (autoRefresh && selectedFile) {
      const interval = setInterval(() => {
        fetchLogContent(selectedFile, lines);
      }, 5000); // Refresh every 5 seconds
      
      return () => clearInterval(interval);
    }
  }, [autoRefresh, selectedFile, lines]);

  // Initial load
  useEffect(() => {
    fetchLogFiles();
  }, []);

  // Load log content when file is selected
  useEffect(() => {
    if (selectedFile) {
      fetchLogContent(selectedFile, lines);
    }
  }, [selectedFile, lines]);

  const formatLogEntry = (logEntry) => {
    const timestamp = new Date(logEntry.timestamp).toLocaleString();
    const level = logEntry.level.toUpperCase();
    const message = logEntry.message;
    const type = logEntry.type || '';
    const meta = logEntry.ip ? `[${logEntry.ip}]` : '';
    
    return {
      timestamp,
      level,
      message,
      type,
      meta,
      raw: logEntry
    };
  };

  const getLevelColor = (level) => {
    switch (level.toLowerCase()) {
      case 'error': return 'error';
      case 'warn': return 'warning';
      case 'info': return 'info';
      case 'debug': return 'debug';
      default: return 'default';
    }
  };

  const getTypeColor = (type) => {
    switch (type) {
      case 'api': return '#4ade80';
      case 'system': return '#06b6d4';
      case 'security': return '#f59e0b';
      case 'performance': return '#8b5cf6';
      case 'http': return '#ec4899';
      default: return '#6b7280';
    }
  };

  const filteredLogs = logContent
    .map(formatLogEntry)
    .filter(log => {
      // Filter by log level
      if (logLevel !== 'all' && log.raw.level !== logLevel) {
        return false;
      }
      
      // Filter by search term
      if (searchTerm && !log.message.toLowerCase().includes(searchTerm.toLowerCase())) {
        return false;
      }
      
      return true;
    });

  const handleFileSelect = (filename) => {
    setSelectedFile(filename);
    setLogContent([]);
  };

  const handleRefresh = () => {
    if (selectedFile) {
      fetchLogContent(selectedFile, lines);
    }
  };

  const handleDownload = async () => {
    if (!selectedFile) return;
    
    try {
      const response = await axios.get(`/api/logs/download/${selectedFile}`, {
        responseType: 'blob'
      });
      
      const url = window.URL.createObjectURL(new Blob([response.data]));
      const link = document.createElement('a');
      link.href = url;
      link.setAttribute('download', selectedFile);
      document.body.appendChild(link);
      link.click();
      link.remove();
      window.URL.revokeObjectURL(url);
    } catch (err) {
      console.error('Error downloading log file:', err);
      setError('Failed to download log file');
    }
  };

  return (
    <div className="page-container">
      <div className="page-header">
        <h1>System Logs</h1>
        <p>View and monitor backend system logs</p>
      </div>

      <div className="logs-container">
        {/* Log Files Panel */}
        <div className="logs-sidebar">
          <div className="logs-section">
            <h3>Log Files</h3>
            <button 
              onClick={fetchLogFiles} 
              className="refresh-btn"
              disabled={loading}
            >
              {loading ? 'Refreshing...' : '🔄 Refresh'}
            </button>
          </div>
          
          <div className="log-files-list">
            {logFiles.map((file, index) => (
              <div
                key={index}
                className={`log-file-item ${selectedFile === file.name ? 'active' : ''}`}
                onClick={() => handleFileSelect(file.name)}
              >
                <div className="log-file-name">{file.name}</div>
                <div className="log-file-size">{file.size}</div>
              </div>
            ))}
          </div>
        </div>

        {/* Log Content Panel */}
        <div className="logs-content">
          {selectedFile ? (
            <>
              {/* Log Controls */}
              <div className="logs-controls">
                <div className="control-group">
                  <label>Lines:</label>
                  <select 
                    value={lines} 
                    onChange={(e) => setLines(parseInt(e.target.value))}
                    className="control-select"
                  >
                    <option value={50}>50</option>
                    <option value={100}>100</option>
                    <option value={200}>200</option>
                    <option value={500}>500</option>
                    <option value={1000}>1000</option>
                  </select>
                </div>

                <div className="control-group">
                  <label>Level:</label>
                  <select 
                    value={logLevel} 
                    onChange={(e) => setLogLevel(e.target.value)}
                    className="control-select"
                  >
                    <option value="all">All</option>
                    <option value="error">Error</option>
                    <option value="warn">Warning</option>
                    <option value="info">Info</option>
                    <option value="debug">Debug</option>
                  </select>
                </div>

                <div className="control-group">
                  <input
                    type="text"
                    placeholder="Search logs..."
                    value={searchTerm}
                    onChange={(e) => setSearchTerm(e.target.value)}
                    className="search-input"
                  />
                </div>

                <div className="control-group">
                  <label className="checkbox-label">
                    <input
                      type="checkbox"
                      checked={autoRefresh}
                      onChange={(e) => setAutoRefresh(e.target.checked)}
                    />
                    Auto-refresh
                  </label>
                </div>

                <div className="control-actions">
                  <button onClick={handleRefresh} className="action-btn">
                    🔄 Refresh
                  </button>
                  <button onClick={handleDownload} className="action-btn">
                    📥 Download
                  </button>
                </div>
              </div>

              {/* Log Content */}
              <div className="logs-viewer">
                {loading ? (
                  <div className="loading-container">
                    <div className="loading-spinner"></div>
                    <p>Loading logs...</p>
                  </div>
                ) : error ? (
                  <div className="error-container">
                    <p className="error-message">{error}</p>
                    <button onClick={handleRefresh} className="retry-btn">
                      Retry
                    </button>
                  </div>
                ) : (
                  <div className="log-entries">
                    {filteredLogs.length === 0 ? (
                      <div className="empty-state">
                        <p>No logs found matching the current filters.</p>
                      </div>
                    ) : (
                      filteredLogs.map((log, index) => (
                        <div key={index} className={`log-entry log-${getLevelColor(log.level)}`}>
                          <div className="log-header">
                            <span className="log-timestamp">{log.timestamp}</span>
                            <span className={`log-level log-level-${getLevelColor(log.level)}`}>
                              {log.level}
                            </span>
                            {log.type && (
                              <span 
                                className="log-type"
                                style={{ backgroundColor: getTypeColor(log.type) }}
                              >
                                {log.type}
                              </span>
                            )}
                            {log.meta && <span className="log-meta">{log.meta}</span>}
                          </div>
                          <div className="log-message">{log.message}</div>
                          {log.raw.error && (
                            <div className="log-error">{log.raw.error}</div>
                          )}
                        </div>
                      ))
                    )}
                  </div>
                )}
              </div>
            </>
          ) : (
            <div className="empty-state">
              <p>Select a log file to view its contents.</p>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

export default LogsPage;
