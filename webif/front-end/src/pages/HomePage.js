import React, { useState, useEffect } from 'react';
import axios from 'axios';
import { useNotification } from '../context/NotificationContext';
import NotificationDemo from '../components/NotificationDemo';

function HomePage() {
  const { showError, showSuccess } = useNotification();
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
  const [usrpInfo, setUsrpInfo] = useState({
    detected: false,
    count: 0,
    devices: []
  });
  const [expandedDevices, setExpandedDevices] = useState(new Set());
  const [deviceDetails, setDeviceDetails] = useState({});
  const [loadingDetails, setLoadingDetails] = useState(new Set());
  const [loading, setLoading] = useState(true);

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
      
      // Debug USRP data
      console.log('USRP Info received:', response.data.usrp);
      if (response.data.usrp && response.data.usrp.devices) {
        console.log('USRP Devices:', response.data.usrp.devices);
        console.log('USRP Device count:', response.data.usrp.devices.length);
      }
      
      setUsrpInfo({
        detected: response.data.usrp?.detected || false,
        count: response.data.usrp?.count || 0,
        devices: response.data.usrp?.devices || []
      });
      
      setLoading(false);
    } catch (err) {
      console.error('Error fetching system info:', err);
      showError({
        message: 'Failed to fetch system information. Please check your connection and try again.',
        onRetry: fetchSystemInfo
      });
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

  const toggleDeviceDetails = async (deviceIndex, device) => {
    const deviceKey = `${deviceIndex}-${device.name}`;
    
    if (expandedDevices.has(deviceKey)) {
      // Collapse
      setExpandedDevices(prev => {
        const newSet = new Set(prev);
        newSet.delete(deviceKey);
        return newSet;
      });
    } else {
      // Expand and load details
      setExpandedDevices(prev => new Set(prev).add(deviceKey));
      
      if (!deviceDetails[deviceKey]) {
        setLoadingDetails(prev => new Set(prev).add(deviceKey));
        
        try {
          // Construct device args for uhd_usrp_probe
          let deviceArgs = '';
          if (device.details['Serial Number']) {
            deviceArgs = `serial=${device.details['Serial Number']}`;
          } else if (device.details['IP Address']) {
            deviceArgs = `addr=${device.details['IP Address']}`;
          } else {
            deviceArgs = `type=${device.details['Type'] || 'x300'}`;
          }
          
          const response = await fetch(`/api/usrp/device/details?deviceArgs=${encodeURIComponent(deviceArgs)}`);
          if (response.ok) {
            const details = await response.json();
            setDeviceDetails(prev => ({
              ...prev,
              [deviceKey]: details
            }));
          } else {
            console.error('Failed to fetch device details');
            showError({
              message: 'Failed to fetch device details. Please try again.',
              onRetry: () => toggleDeviceDetails(deviceIndex, device)
            });
          }
        } catch (error) {
          console.error('Error fetching device details:', error);
          showError({
            message: 'Error fetching device details. Please check your connection and try again.',
            onRetry: () => toggleDeviceDetails(deviceIndex, device)
          });
        } finally {
          setLoadingDetails(prev => {
            const newSet = new Set(prev);
            newSet.delete(deviceKey);
            return newSet;
          });
        }
      }
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

      <div className="status-grid">
        <div className="status-section">
          <div className="section-header">
            <h3>USRP Devices</h3>
            <button 
              className="refresh-btn" 
              onClick={async () => {
                console.log('Manual USRP refresh triggered');
                try {
                  const response = await fetch('/api/usrp/refresh', { method: 'POST' });
                  if (response.ok) {
                    const usrpData = await response.json();
                    setUsrpInfo(usrpData);
                    console.log('USRP cache refreshed:', usrpData);
                  }
                } catch (error) {
                  console.error('Error refreshing USRP cache:', error);
                }
              }}
              disabled={loading}
              title="Refresh USRP devices cache"
            >
              {loading ? '⏳' : '🔄'} Refresh USRP Devices
            </button>
          </div>
          {loading ? (
            <div className="status-item">
              <span className="label">Status:</span>
              <span className="value">Loading...</span>
            </div>
          ) : (
            <>
              <div className="status-item">
                <span className="label">Status:</span>
                <span className={`value ${usrpInfo.detected ? 'success' : 'error'}`}>
                  {usrpInfo.detected ? 'Detected' : 'Not Detected'}
                </span>
              </div>
            </>
          )}
          <div className="status-item">
            <span className="label">Count:</span>
            <span className="value">{usrpInfo.count}</span>
          </div>
          {usrpInfo.detectionMethods && (
            <div className="status-item">
              <span className="label">Detection Methods:</span>
              <span className="value">
                {Object.entries(usrpInfo.detectionMethods)
                  .filter(([method, available]) => available)
                  .map(([method]) => method)
                  .join(', ') || 'None'}
              </span>
            </div>
          )}
          {usrpInfo.cache && (
            <div className="status-item">
              <span className="label">Cache Age:</span>
              <span className="value">
                {usrpInfo.cache.cacheAge ? `${Math.round(usrpInfo.cache.cacheAge / 1000)}s` : 'N/A'}
              </span>
            </div>
          )}
          {usrpInfo.devices && usrpInfo.devices.length > 0 ? (
            <div className="usrp-devices">
              <div className="usrp-devices-header">
                <h4>Detected Devices ({usrpInfo.devices.length})</h4>
              </div>
              {usrpInfo.devices.map((device, index) => {
                const deviceKey = `${index}-${device.name}`;
                const isExpanded = expandedDevices.has(deviceKey);
                const isLoading = loadingDetails.has(deviceKey);
                const details = deviceDetails[deviceKey];
                
                return (
                  <div key={index} className="usrp-device">
                    <div className="device-header">
                      <span className="device-name">{device.name}</span>
                      <div className="device-controls">
                        <span className={`device-method ${device.method.replace(/[^a-zA-Z0-9]/g, '')}`}>{device.method}</span>
                      </div>
                    </div>
                    {device.details && Object.keys(device.details).length > 0 && (
                      <div className="device-details">
                        {Object.entries(device.details).map(([key, value]) => (
                          <div key={key} className="detail-row">
                            <span className="label">{key}:</span>
                            <span className="value">{value}</span>
                          </div>
                        ))}
                      </div>
                    )}
                    
                    {/* Details button under the device details */}
                    <div className="details-button-container">
                      <button 
                        className={`details-btn ${isExpanded ? 'expanded' : ''}`}
                        onClick={() => toggleDeviceDetails(index, device)}
                        disabled={isLoading}
                      >
                        {isLoading ? '⏳ Loading...' : isExpanded ? '▼ Hide Details' : '▶ Show Details'}
                      </button>
                    </div>
                    
                    {isExpanded && (
                      <div className="device-expanded-details">
                        {isLoading ? (
                          <div className="loading-details">
                            <span>Loading detailed device information...</span>
                          </div>
                        ) : details && details.success ? (
                          <div className="probe-details">
                            {details.details.mboard && Object.keys(details.details.mboard).length > 0 && (
                              <div className="detail-section">
                                <h4>Mainboard</h4>
                                <div className="detail-grid">
                                  {Object.entries(details.details.mboard).map(([key, value]) => (
                                    <div key={key} className="detail-item">
                                      <span className="detail-label">{key}:</span>
                                      <span className="detail-value">{value}</span>
                                    </div>
                                  ))}
                                </div>
                              </div>
                            )}
                            
                            {details.details.rfBlocks && details.details.rfBlocks.length > 0 && (
                              <div className="detail-section">
                                <h4>RFNoC Blocks</h4>
                                <div className="rf-blocks">
                                  {details.details.rfBlocks.map((block, idx) => (
                                    <span key={idx} className="rf-block">{block}</span>
                                  ))}
                                </div>
                              </div>
                            )}
                            
                            {details.details.dboards && details.details.dboards.length > 0 && (
                              <div className="detail-section">
                                <h4>Daughterboards</h4>
                                <div className="dboards-grid">
                                  {details.details.dboards.map((dboard, idx) => (
                                    <div key={idx} className="dboard-item">
                                      <h5>{dboard.type} Dboard</h5>
                                      <div className="dboard-details">
                                        {dboard.id && <div><span>ID:</span> {dboard.id}</div>}
                                        {dboard.serial && <div><span>Serial:</span> {dboard.serial}</div>}
                                        {dboard.frontend.name && <div><span>Frontend:</span> {dboard.frontend.name}</div>}
                                        {dboard.frontend.freqRange && <div><span>Freq Range:</span> {dboard.frontend.freqRange}</div>}
                                        {dboard.frontend.gainRange && <div><span>Gain Range:</span> {dboard.frontend.gainRange}</div>}
                                      </div>
                                    </div>
                                  ))}
                                </div>
                              </div>
                            )}
                          </div>
                        ) : details && !details.success ? (
                          <div className="error-details">
                            <span>Failed to load device details: {details.error}</span>
                          </div>
                        ) : null}
                      </div>
                    )}
                    

                  </div>
                );
              })}
            </div>
          ) : (
            <div className="usrp-no-devices">
              <p>No USRP devices detected</p>
              <p className="usrp-help-text">Make sure your USRP device is connected and UHD drivers are installed</p>
            </div>
          )}
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

      {/* Notification Demo - Remove this section after testing */}
      <div style={{ marginTop: '2rem', padding: '2rem', border: '1px solid rgba(255, 255, 255, 0.2)', borderRadius: '8px' }}>
        <NotificationDemo />
      </div>
    </div>
  );
}

export default HomePage;
