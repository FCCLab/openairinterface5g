import React, { useState, useEffect, useRef } from 'react';
import axios from 'axios';

function LogsPage() {
  const [logFiles, setLogFiles] = useState([]);
  const [selectedFile, setSelectedFile] = useState('');
  const [logContent, setLogContent] = useState([]);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [autoRefresh, setAutoRefresh] = useState(true);
  const [refreshInterval, setRefreshInterval] = useState(5000); // Default 5 seconds
  const [logLevel, setLogLevel] = useState('all');
  const [searchTerm, setSearchTerm] = useState('');
  const [excludeTerm, setExcludeTerm] = useState('');
  const [useRegex, setUseRegex] = useState(false);
  const [lines, setLines] = useState(20);
  const [refreshing, setRefreshing] = useState(false);
  const [searchHistory, setSearchHistory] = useState([]);
  const [excludeHistory, setExcludeHistory] = useState([]);
  const [showSearchHistory, setShowSearchHistory] = useState(false);
  const [showExcludeHistory, setShowExcludeHistory] = useState(false);
  
  // Ref to track the current interval
  const intervalRef = useRef(null);

  // Helper function to safely test regex patterns
  const testRegex = (pattern, text) => {
    if (!pattern) return true;
    
    try {
      if (useRegex) {
        const regex = new RegExp(pattern, 'i'); // Case-insensitive
        return regex.test(text);
      } else {
        return text.toLowerCase().includes(pattern.toLowerCase());
      }
    } catch (error) {
      // If regex is invalid, fall back to string search
      console.warn('Invalid regex pattern:', pattern, error);
      return text.toLowerCase().includes(pattern.toLowerCase());
    }
  };

  // Fetch available log files
  const fetchLogFiles = async () => {
    try {
      setRefreshing(true);
      setError(null);
      const response = await axios.get('/api/logs/files');
      setLogFiles(response.data.files || []);
    } catch (err) {
      console.error('Error fetching log files:', err);
      setError('Failed to fetch log files');
    } finally {
      setRefreshing(false);
    }
  };

  // Fetch log content
  const fetchLogContent = async (filename, lineCount = 20) => {
    if (!filename) return;
    
    try {
      setLoading(true);
      setRefreshing(true);
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
      setRefreshing(false);
    }
  };

  // Handle scroll to top refresh and auto-refresh management
  const handleScroll = (e) => {
    const scrollTop = e.target.scrollTop;
    const scrollHeight = e.target.scrollHeight;
    const clientHeight = e.target.clientHeight;
    const isAtTop = scrollTop === 0;
    const isAtBottom = scrollTop + clientHeight >= scrollHeight - 10; // 10px tolerance
    
    // Auto-enable refresh when at top
    if (isAtTop && !autoRefresh) {
      setAutoRefresh(true);
      fetchLogContent(selectedFile, lines);
    }
    
    // Auto-disable refresh when scrolling down (not at top)
    if (!isAtTop && autoRefresh) {
      setAutoRefresh(false);
    }
    
    // Refresh when at top and auto-refresh is enabled
    if (isAtTop && autoRefresh && selectedFile) {
      fetchLogContent(selectedFile, lines);
      // Reset the timer when scrolling to top
      resetAutoRefreshTimer();
    }
  };
  
  // Function to reset the auto-refresh timer
  const resetAutoRefreshTimer = () => {
    if (intervalRef.current) {
      clearInterval(intervalRef.current);
      intervalRef.current = null;
    }
    
    if (autoRefresh && selectedFile) {
      intervalRef.current = setInterval(() => {
        fetchLogContent(selectedFile, lines);
      }, refreshInterval);
    }
  };

  // Auto-refresh log content with interval
  useEffect(() => {
    // Clear existing interval
    if (intervalRef.current) {
      clearInterval(intervalRef.current);
      intervalRef.current = null;
    }
    
    if (autoRefresh && selectedFile) {
      intervalRef.current = setInterval(() => {
        fetchLogContent(selectedFile, lines);
      }, refreshInterval);
    }
    
    // Cleanup on unmount or dependency change
    return () => {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
        intervalRef.current = null;
      }
    };
  }, [autoRefresh, selectedFile, lines, refreshInterval]);

  // Initial load
  useEffect(() => {
    fetchLogFiles();
  }, []);

  // Close dropdowns when clicking outside
  useEffect(() => {
    const handleClickOutside = (event) => {
      if (!event.target.closest('.input-container')) {
        setShowSearchHistory(false);
        setShowExcludeHistory(false);
      }
    };

    document.addEventListener('click', handleClickOutside);
    return () => {
      document.removeEventListener('click', handleClickOutside);
    };
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
    const data = logEntry.data || '';
    
    // Auto-detect all fields except the main ones
    const mainFields = ['timestamp', 'level', 'message', 'type', 'ip', 'data'];
    const allFields = {};
    
    Object.keys(logEntry).forEach(key => {
      if (!mainFields.includes(key)) {
        allFields[key] = logEntry[key];
      }
    });
    
    return {
      timestamp,
      level,
      message,
      type,
      meta,
      data,
      allFields,
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
      
      // Filter by search term (include)
      if (searchTerm && !testRegex(searchTerm, log.message)) {
        return false;
      }
      
      // Filter by exclude term (exclude)
      if (excludeTerm && testRegex(excludeTerm, log.message)) {
        return false;
      }
      
      return true;
    })
    .reverse(); // Show newest logs first

  const handleFileSelect = (filename) => {
    setSelectedFile(filename);
    setLogContent([]);
  };

  const saveSearchToHistory = (term) => {
    if (!term.trim()) return;
    
    setSearchHistory(prev => {
      const updated = [term.trim(), ...prev.filter(t => t !== term.trim())];
      return updated.slice(0, 10); // Keep only the last 10 searches
    });
  };

  const saveExcludeToHistory = (term) => {
    if (!term.trim()) return;
    
    setExcludeHistory(prev => {
      const updated = [term.trim(), ...prev.filter(t => t !== term.trim())];
      return updated.slice(0, 10); // Keep only the last 10 excludes
    });
  };

  const handleRefresh = () => {
    if (selectedFile) {
      fetchLogContent(selectedFile, lines);
    }
  };

  const handleSearchChange = (e) => {
    const value = e.target.value;
    setSearchTerm(value);
  };

  const handleExcludeChange = (e) => {
    const value = e.target.value;
    setExcludeTerm(value);
  };

  const handleSearchKeyDown = (e) => {
    if (e.key === 'Enter') {
      if (e.target.value.trim()) {
        saveSearchToHistory(e.target.value);
      }
    }
  };

  const handleExcludeKeyDown = (e) => {
    if (e.key === 'Enter') {
      if (e.target.value.trim()) {
        saveExcludeToHistory(e.target.value);
      }
    }
  };

  const handleSearchBlur = (e) => {
    if (e.target.value.trim()) {
      saveSearchToHistory(e.target.value);
    }
  };

  const handleExcludeBlur = (e) => {
    if (e.target.value.trim()) {
      saveExcludeToHistory(e.target.value);
    }
  };

  const handleSearchClick = () => {
    setShowSearchHistory(true);
    setShowExcludeHistory(false);
  };

  const handleExcludeClick = () => {
    setShowExcludeHistory(true);
    setShowSearchHistory(false);
  };

  const handleSearchSelect = (term) => {
    setSearchTerm(term);
    setShowSearchHistory(false);
  };

  const handleExcludeSelect = (term) => {
    setExcludeTerm(term);
    setShowExcludeHistory(false);
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
              disabled={refreshing}
            >
              <span className={`refresh-icon ${refreshing ? 'rotating' : ''}`}>🔄</span>
              Refresh
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
                    <option value={10}>10</option>
                    <option value={20}>20</option>
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
                  <div className="input-container">
                    <input
                      type="text"
                      placeholder="Search logs..."
                      value={searchTerm}
                      onChange={handleSearchChange}
                      onKeyDown={handleSearchKeyDown}
                      onBlur={handleSearchBlur}
                      onClick={handleSearchClick}
                      className="search-input"
                    />
                    {showSearchHistory && searchHistory.length > 0 && (
                      <div className="search-history-dropdown">
                        {searchHistory.map((term, index) => (
                          <div 
                            key={index} 
                            className="history-item"
                            onClick={() => handleSearchSelect(term)}
                          >
                            🔍 {term}
                          </div>
                        ))}
                      </div>
                    )}
                  </div>
                </div>

                <div className="control-group">
                  <div className="input-container">
                    <input
                      type="text"
                      placeholder="Exclude logs containing..."
                      value={excludeTerm}
                      onChange={handleExcludeChange}
                      onKeyDown={handleExcludeKeyDown}
                      onBlur={handleExcludeBlur}
                      onClick={handleExcludeClick}
                      className="search-input"
                    />
                    {showExcludeHistory && excludeHistory.length > 0 && (
                      <div className="search-history-dropdown">
                        {excludeHistory.map((term, index) => (
                          <div 
                            key={index} 
                            className="history-item"
                            onClick={() => handleExcludeSelect(term)}
                          >
                            ❌ {term}
                          </div>
                        ))}
                      </div>
                    )}
                  </div>
                </div>

                <div className="control-group">
                  <label className="checkbox-label">
                    <input
                      type="checkbox"
                      checked={useRegex}
                      onChange={(e) => setUseRegex(e.target.checked)}
                    />
                    Use Regular Expressions
                  </label>
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

                <div className="control-group">
                  <label>Refresh Interval:</label>
                  <select 
                    value={refreshInterval} 
                    onChange={(e) => setRefreshInterval(Number(e.target.value))}
                    className="control-select"
                    disabled={!autoRefresh}
                  >
                    <option value={1000}>1 second</option>
                    <option value={2000}>2 seconds</option>
                    <option value={5000}>5 seconds</option>
                    <option value={10000}>10 seconds</option>
                    <option value={30000}>30 seconds</option>
                    <option value={60000}>1 minute</option>
                  </select>
                </div>

                <div className="control-actions">
                  <button onClick={handleRefresh} className="action-btn" disabled={refreshing}>
                    <span className={`refresh-icon ${refreshing ? 'rotating' : ''}`}>🔄</span>
                    Refresh
                  </button>
                  <button onClick={handleDownload} className="action-btn">
                    📥 Download
                  </button>
                </div>
              </div>

              {/* Log Content */}
              <div className="logs-viewer" onScroll={handleScroll}>
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
                            <span className="log-message">{log.message}</span>
                          </div>
                          
                          {/* Auto-detected fields */}
                          {Object.keys(log.allFields).length > 0 && (
                            <>
                              {Object.entries(log.allFields).map(([key, value]) => (
                                <div key={key} className="log-field-item">
                                  <span className="log-field-label">{key}:</span>
                                  <span className="log-field-content">
                                    {typeof value === 'object' ? JSON.stringify(value, null, 2) : String(value)}
                                  </span>
                                </div>
                              ))}
                            </>
                          )}
                          
                          {log.data && (
                            <div className="log-data">
                              <span className="log-data-content">{log.data}</span>
                            </div>
                          )}
                          
                          {log.stack && (
                            <div className="log-stack">
                              <span className="log-stack-content">{log.stack}</span>
                            </div>
                          )}
                          
                          {log.error && (
                            <div className="log-error">{log.error}</div>
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
