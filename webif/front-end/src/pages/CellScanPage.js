import React, { useState, useEffect } from 'react';
import { useNotification } from '../context/NotificationContext';

function CellScanPage() {
  const { showError, showSuccess } = useNotification();
  const [isScanning, setIsScanning] = useState(false);
  const [scanResults, setScanResults] = useState([]);
  const [scanProgress, setScanProgress] = useState(0);

  // Mock cell data
  const mockCells = [
    { id: 1, mcc: '310', mnc: '260', cellId: '0x1234', frequency: '2600', rsrp: -85, rsrq: -12, sinr: 15, status: 'Available' },
    { id: 2, mcc: '310', mnc: '260', cellId: '0x5678', frequency: '2600', rsrp: -92, rsrq: -15, sinr: 8, status: 'Available' },
    { id: 3, mcc: '310', mnc: '260', cellId: '0x9abc', frequency: '1800', rsrp: -78, rsrq: -8, sinr: 22, status: 'Available' },
    { id: 4, mcc: '310', mnc: '260', cellId: '0xdef0', frequency: '1800', rsrp: -95, rsrq: -18, sinr: 5, status: 'Available' },
  ];

  const startScan = () => {
    setIsScanning(true);
    setScanProgress(0);
    setScanResults([]);

    // Simulate scanning progress
    const interval = setInterval(() => {
      setScanProgress(prev => {
        if (prev >= 100) {
          clearInterval(interval);
          setIsScanning(false);
          setScanResults(mockCells);
          showSuccess(`Scan completed! Found ${mockCells.length} cells.`);
          return 100;
        }
        return prev + 10;
      });
    }, 200);
  };

  const stopScan = () => {
    setIsScanning(false);
    setScanProgress(0);
  };

  return (
    <div className="page-container">
      <div className="page-header">
        <h1>Cell Scan</h1>
        <p>Scan for available 5G cells in your area</p>
      </div>

      <div className="scan-controls">
        <button 
          className={`scan-btn ${isScanning ? 'scanning' : ''}`}
          onClick={isScanning ? stopScan : startScan}
          disabled={isScanning}
        >
          {isScanning ? 'Scanning...' : 'Start Scan'}
        </button>
        
        {isScanning && (
          <div className="progress-container">
            <div className="progress-bar">
              <div 
                className="progress-fill" 
                style={{ width: `${scanProgress}%` }}
              ></div>
            </div>
            <span className="progress-text">{scanProgress}%</span>
          </div>
        )}
      </div>

      {scanResults.length > 0 && (
        <div className="results-container">
          <h2>Scan Results ({scanResults.length} cells found)</h2>
          <div className="cells-grid">
            {scanResults.map(cell => (
              <div key={cell.id} className="cell-card">
                <div className="cell-header">
                  <h3>Cell {cell.cellId}</h3>
                  <span className={`status ${cell.status.toLowerCase()}`}>
                    {cell.status}
                  </span>
                </div>
                <div className="cell-details">
                  <div className="detail-row">
                    <span className="label">MCC/MNC:</span>
                    <span className="value">{cell.mcc}/{cell.mnc}</span>
                  </div>
                  <div className="detail-row">
                    <span className="label">Frequency:</span>
                    <span className="value">{cell.frequency} MHz</span>
                  </div>
                  <div className="detail-row">
                    <span className="label">RSRP:</span>
                    <span className="value">{cell.rsrp} dBm</span>
                  </div>
                  <div className="detail-row">
                    <span className="label">RSRQ:</span>
                    <span className="value">{cell.rsrq} dB</span>
                  </div>
                  <div className="detail-row">
                    <span className="label">SINR:</span>
                    <span className="value">{cell.sinr} dB</span>
                  </div>
                </div>
                <button className="attach-btn">Attach to Cell</button>
              </div>
            ))}
          </div>
        </div>
      )}

      {!isScanning && scanResults.length === 0 && (
        <div className="empty-state">
          <p>No scan results available. Click "Start Scan" to begin scanning for cells.</p>
        </div>
      )}
    </div>
  );
}

export default CellScanPage;
