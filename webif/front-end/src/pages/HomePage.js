import React, { useState, useEffect } from 'react';
import axios from 'axios';

function HomePage() {
  const [currentTime, setCurrentTime] = useState(new Date());
  const [cpuInfo, setCpuInfo] = useState({
    usage: 0,
    temperature: 0,
    cores: 0,
    frequency: 'N/A',
    load: [0, 0, 0],
    model: 'Unknown'
  });
  const [networkInterfaces, setNetworkInterfaces] = useState([]);
  const [memoryInfo, setMemoryInfo] = useState({
    total: 0,
    used: 0,
    free: 0,
    usage: 0
  });
  const [systemInfo, setSystemInfo] = useState({
    uptime: '0d 0h 0m',
    hostname: 'Unknown',
    platform: 'Unknown'
  });
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  // Update time every second
  useEffect(() => {
    const timer = setInterval(() => {
      setCurrentTime(new Date());
    }, 1000);

    return () => clearInterval(timer);
  }, []);

  // Fetch system information
  const fetchSystemInfo = async () => {
    try {
      setError(null);
      const response = await axios.get('/api/system/info');
      const { cpu, network, memory, system } = response.data;
      
      setCpuInfo({
        usage: cpu.usage || 0,
        temperature: cpu.temperature || 0,
        cores: cpu.cores || 0,
        frequency: cpu.frequency || 'N/A',
        load: cpu.load || [0, 0, 0],
        model: cpu.model || 'Unknown'
      });
      
      setNetworkInterfaces(network || []);
      
      setMemoryInfo({
        total: memory?.total || 0,
        used: memory?.used || 0,
        free: memory?.free || 0,
        usage: memory?.usage || 0
      });
      
      setSystemInfo({
        uptime: system?.uptime?.formattedUptime || '0d 0h 0m',
        hostname: system?.hostname || 'Unknown',
        platform: system?.platform || 'Unknown'
      });
      
      setLoading(false);
    } catch (err) {
      console.error('Error fetching system info:', err);
      setError('Failed to fetch system information');
      setLoading(false);
    }
  };

  // Initial fetch and periodic updates
  useEffect(() => {
    fetchSystemInfo();
    
    // Update system info every 5 seconds
    const interval = setInterval(fetchSystemInfo, 5000);
    
    return () => clearInterval(interval);
  }, []);

  const getCpuStatus = (usage) => {
    if (usage < 30) return 'Low';
    if (usage < 60) return 'Normal';
    if (usage < 80) return 'High';
    return 'Critical';
  };

  const getMemoryStatus = (usage) => {
    if (usage < 50) return 'Low';
    if (usage < 80) return 'Normal';
    if (usage < 90) return 'High';
    return 'Critical';
  };

  const getInterfaceStatusColor = (status) => {
    switch (status) {
      case 'up':
        return 'success';
      case 'down':
        return 'error';
      case 'unknown':
        return 'error';
      default:
        return 'error';
    }
  };

  const getInterfaceStatusText = (status) => {
    switch (status) {
      case 'up':
        return 'UP';
      case 'down':
        return 'DOWN';
      case 'unknown':
        return 'UNKNOWN';
      default:
        return status.toUpperCase();
    }
  };

  const getInterfaceTypeColor = (type) => {
    switch (type) {
      case 'Ethernet': return '#4ade80';
      case 'WiFi': return '#f59e0b';
      case 'Loopback': return '#8b5cf6';
      case 'Bridge': return '#06b6d4';
      case 'Virtual': return '#ec4899';
      case 'Tunnel': return '#84cc16';
      default: return '#6b7280';
    }
  };

  if (loading) {
    return (
      <div className="home-container">
        <div className="loading-container">
          <div className="loading-spinner"></div>
          <p>Loading system information...</p>
        </div>
      </div>
    );
  }

  if (error) {
    return (
      <div className="home-container">
        <div className="error-container">
          <p className="error-message">{error}</p>
          <button onClick={fetchSystemInfo} className="retry-btn">
            Retry
          </button>
        </div>
      </div>
    );
  }

  return (
    <div className="home-container">
      <div className="status-grid">
        <div className="status-section">
          <h3>System Status</h3>
          <div className="status-item">
            <span className="label">Time:</span>
            <span className="value">{currentTime.toLocaleTimeString()}</span>
          </div>
          <div className="status-item">
            <span className="label">Date:</span>
            <span className="value">{currentTime.toLocaleDateString()}</span>
          </div>
          <div className="status-item">
            <span className="label">Uptime:</span>
            <span className="value">{systemInfo.uptime}</span>
          </div>
          <div className="status-item">
            <span className="label">Hostname:</span>
            <span className="value">{systemInfo.hostname}</span>
          </div>
          <div className="status-item">
            <span className="label">Platform:</span>
            <span className="value">{systemInfo.platform}</span>
          </div>
          <div className="status-item">
            <span className="label">UP/DOWN Interfaces:</span>
            <span className="value">{networkInterfaces.filter(iface => iface.status === 'up').length}/{networkInterfaces.length}</span>
          </div>
        </div>

        <div className="status-section">
          <h3>CPU Information</h3>
          <div className="status-item">
            <span className="label">Usage:</span>
            <span className={`value ${cpuInfo.usage > 80 ? 'error' : cpuInfo.usage > 60 ? 'warning' : 'success'}`}>
              {cpuInfo.usage.toFixed(1)}%
            </span>
          </div>
          <div className="status-item">
            <span className="label">Temperature:</span>
            <span className={`value ${cpuInfo.temperature > 80 ? 'error' : cpuInfo.temperature > 70 ? 'warning' : 'success'}`}>
              {cpuInfo.temperature > 0 ? `${cpuInfo.temperature.toFixed(1)}°C` : 'N/A'}
            </span>
          </div>
          <div className="status-item">
            <span className="label">Cores:</span>
            <span className="value">{cpuInfo.cores}</span>
          </div>
          <div className="status-item">
            <span className="label">Frequency:</span>
            <span className="value">{cpuInfo.frequency} GHz</span>
          </div>
          <div className="status-item">
            <span className="label">Status:</span>
            <span className={`value ${getCpuStatus(cpuInfo.usage) === 'Critical' ? 'error' : getCpuStatus(cpuInfo.usage) === 'High' ? 'warning' : 'success'}`}>
              {getCpuStatus(cpuInfo.usage)}
            </span>
          </div>
          {cpuInfo.model !== 'Unknown' && (
            <div className="status-item">
              <span className="label">Model:</span>
              <span className="value cpu-model">{cpuInfo.model}</span>
            </div>
          )}
        </div>

        <div className="status-section">
          <h3>System Load</h3>
          <div className="load-info">
            <div className="load-item">
              <span className="label">CPU Load 1 min:</span>
              <span className="value">{cpuInfo.load[0]?.toFixed(2) || '0.00'}</span>
            </div>
            <div className="load-item">
              <span className="label">CPU Load 5 min:</span>
              <span className="value">{cpuInfo.load[1]?.toFixed(2) || '0.00'}</span>
            </div>
            <div className="load-item">
              <span className="label">CPU Load 15 min:</span>
              <span className="value">{cpuInfo.load[2]?.toFixed(2) || '0.00'}</span>
            </div>
            <div className="load-item">
              <span className="label">RAM Usage:</span>
              <span className={`value ${memoryInfo.usage > 90 ? 'error' : memoryInfo.usage > 80 ? 'warning' : 'success'}`}>
                {memoryInfo.usage.toFixed(1)}%
              </span>
            </div>
            <div className="load-item">
              <span className="label">RAM Used:</span>
              <span className="value">{memoryInfo.used.toFixed(1)} GB</span>
            </div>
            <div className="load-item">
              <span className="label">RAM Free:</span>
              <span className="value">{memoryInfo.free.toFixed(1)} GB</span>
            </div>
            <div className="load-item">
              <span className="label">RAM Total:</span>
              <span className="value">{memoryInfo.total.toFixed(1)} GB</span>
            </div>
            <div className="load-item">
              <span className="label">RAM Status:</span>
              <span className={`value ${getMemoryStatus(memoryInfo.usage) === 'Critical' ? 'error' : getMemoryStatus(memoryInfo.usage) === 'High' ? 'warning' : 'success'}`}>
                {getMemoryStatus(memoryInfo.usage)}
              </span>
            </div>
          </div>
        </div>
      </div>

      {networkInterfaces.length > 0 && (
        <div className="interfaces-section">
          <h3>Network Interfaces</h3>
          <div className="interfaces-grid">
            {networkInterfaces.map((iface, index) => (
              <div key={index} className="interface-card">
                <div className="interface-header">
                  <div className="interface-name">
                    <span className="interface-icon" style={{ backgroundColor: getInterfaceTypeColor(iface.type) }}>
                      {iface.name}
                    </span>
                    <span className="interface-type">{iface.type}</span>
                  </div>
                  <span className={`status-badge ${getInterfaceStatusColor(iface.status)}`}>
                    {getInterfaceStatusText(iface.status)}
                  </span>
                </div>
                <div className="interface-details">
                  <div className="detail-row">
                    <span className="label">IP Addresses:</span>
                    <span className="value">
                      {iface.ipAddresses && iface.ipAddresses.length > 0 ? (
                        <div className="ip-addresses">
                          {iface.ipAddresses.map((ip, ipIndex) => (
                            <div key={ipIndex} className="ip-address">
                              <span className={`ip-family ${ip.family.toLowerCase()}`}>
                                {ip.family}
                              </span>
                              <span className="ip-addr">{ip.address}</span>
                              <span className="ip-netmask">/{ip.netmask}</span>
                            </div>
                          ))}
                        </div>
                      ) : (
                        'N/A'
                      )}
                    </span>
                  </div>
                  <div className="detail-row">
                    <span className="label">MAC Address:</span>
                    <span className="value">{iface.mac}</span>
                  </div>
                  <div className="detail-row">
                    <span className="label">Speed:</span>
                    <span className="value">{iface.speed}</span>
                  </div>
                </div>
              </div>
            ))}
          </div>
        </div>
      )}

      {networkInterfaces.length === 0 && (
        <div className="interfaces-section">
          <h3>Network Interfaces</h3>
          <div className="empty-state">
            <p>No network interfaces found or unable to retrieve interface information.</p>
          </div>
        </div>
      )}
    </div>
  );
}

export default HomePage;
