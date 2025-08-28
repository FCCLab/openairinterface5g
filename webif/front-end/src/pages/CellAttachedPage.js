import React, { useState, useEffect } from 'react';

function CellAttachedPage() {
  const [attachedCell, setAttachedCell] = useState(null);
  const [connectionStatus, setConnectionStatus] = useState('Disconnected');
  const [signalMetrics, setSignalMetrics] = useState({
    rsrp: -85,
    rsrq: -12,
    sinr: 15,
    rssi: -65
  });

  // Simulate connection status
  useEffect(() => {
    const interval = setInterval(() => {
      // Simulate signal fluctuations
      setSignalMetrics(prev => ({
        rsrp: prev.rsrp + (Math.random() - 0.5) * 2,
        rsrq: prev.rsrq + (Math.random() - 0.5) * 1,
        sinr: prev.sinr + (Math.random() - 0.5) * 3,
        rssi: prev.rssi + (Math.random() - 0.5) * 2
      }));
    }, 2000);

    return () => clearInterval(interval);
  }, []);

  const connectToCell = () => {
    setConnectionStatus('Connecting...');
    
    // Simulate connection process
    setTimeout(() => {
      setAttachedCell({
        id: '0x1234',
        mcc: '310',
        mnc: '260',
        frequency: '2600',
        bandwidth: '20 MHz',
        tac: '0x1234',
        pci: 123
      });
      setConnectionStatus('Connected');
    }, 2000);
  };

  const disconnectFromCell = () => {
    setConnectionStatus('Disconnecting...');
    
    setTimeout(() => {
      setAttachedCell(null);
      setConnectionStatus('Disconnected');
    }, 1000);
  };

  const getSignalQuality = (rsrp) => {
    if (rsrp >= -80) return 'Excellent';
    if (rsrp >= -90) return 'Good';
    if (rsrp >= -100) return 'Fair';
    return 'Poor';
  };

  return (
    <div className="page-container">
      <div className="page-header">
        <h1>Cell Attached</h1>
        <p>Monitor your current cell connection and signal quality</p>
      </div>

      <div className="connection-status">
        <div className={`status-indicator ${connectionStatus.toLowerCase()}`}>
          <span className="status-dot"></span>
          <span className="status-text">{connectionStatus}</span>
        </div>
        
        {!attachedCell && (
          <button className="connect-btn" onClick={connectToCell}>
            Connect to Cell
          </button>
        )}
        
        {attachedCell && (
          <button className="disconnect-btn" onClick={disconnectFromCell}>
            Disconnect
          </button>
        )}
      </div>

      {attachedCell && (
        <div className="cell-info-container">
          <div className="cell-info-card">
            <h2>Attached Cell Information</h2>
            <div className="info-grid">
              <div className="info-item">
                <span className="label">Cell ID:</span>
                <span className="value">{attachedCell.id}</span>
              </div>
              <div className="info-item">
                <span className="label">MCC/MNC:</span>
                <span className="value">{attachedCell.mcc}/{attachedCell.mnc}</span>
              </div>
              <div className="info-item">
                <span className="label">Frequency:</span>
                <span className="value">{attachedCell.frequency} MHz</span>
              </div>
              <div className="info-item">
                <span className="label">Bandwidth:</span>
                <span className="value">{attachedCell.bandwidth}</span>
              </div>
              <div className="info-item">
                <span className="label">TAC:</span>
                <span className="value">{attachedCell.tac}</span>
              </div>
              <div className="info-item">
                <span className="label">PCI:</span>
                <span className="value">{attachedCell.pci}</span>
              </div>
            </div>
          </div>

          <div className="signal-metrics-card">
            <h2>Signal Metrics</h2>
            <div className="metrics-grid">
              <div className="metric-item">
                <span className="metric-label">RSRP</span>
                <span className="metric-value">{signalMetrics.rsrp.toFixed(1)} dBm</span>
                <span className="metric-quality">{getSignalQuality(signalMetrics.rsrp)}</span>
              </div>
              <div className="metric-item">
                <span className="metric-label">RSRQ</span>
                <span className="metric-value">{signalMetrics.rsrq.toFixed(1)} dB</span>
              </div>
              <div className="metric-item">
                <span className="metric-label">SINR</span>
                <span className="metric-value">{signalMetrics.sinr.toFixed(1)} dB</span>
              </div>
              <div className="metric-item">
                <span className="metric-label">RSSI</span>
                <span className="metric-value">{signalMetrics.rssi.toFixed(1)} dBm</span>
              </div>
            </div>
          </div>
        </div>
      )}

      {!attachedCell && connectionStatus === 'Disconnected' && (
        <div className="empty-state">
          <p>No cell attached. Click "Connect to Cell" to establish a connection.</p>
        </div>
      )}
    </div>
  );
}

export default CellAttachedPage;
