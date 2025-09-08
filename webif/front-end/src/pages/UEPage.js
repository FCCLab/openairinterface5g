import React, { useState, useEffect, useRef } from 'react';
import './UEPage.css';

function UEPage() {
  const [ueStatus, setUeStatus] = useState({
    connected: false,
    state: 'disconnected',
    signalStrength: 0,
    cellId: null,
    frequency: null,
    bandwidth: null,
    plmn: null
  });

  const [ueConfig, setUeConfig] = useState({
    imsi: '001010000000003',
    key: 'fec86ba6eb707ed08905757b1bb44b8f',
    opc: 'c42449363bbad02b66d16bc975d77cc1',
    dnn: 'oai',
    nssai_sst: 1,
    nssai_sd: 1,
    frequency: '3425010000',
    bandwidth: '100',
    numerology: '1'
  });

  const [isConfigEditing, setIsConfigEditing] = useState(false);
  const [logs, setLogs] = useState([]);
  const [detectedCells, setDetectedCells] = useState([]);
  const [isScanning, setIsScanning] = useState(false);
  const [isSimulatedUE, setIsSimulatedUE] = useState(false);
  const [centerFrequency, setCenterFrequency] = useState('3425010000'); // Default frequency in Hz
  const [expandedCells, setExpandedCells] = useState(new Set());
  const [notification, setNotification] = useState({
    show: false,
    type: 'success', // 'success' or 'error'
    message: ''
  });
  const [realTimeStatus, setRealTimeStatus] = useState({
    isRunning: false,
    processId: null,
    connectionState: 'disconnected',
    grpcPort: 50051,
    uptime: 0,
    status: 'Inactive',
    currentMode: 'simulated',
    timestamp: null
  });
  const signalQualityIntervalRef = useRef(null);

  // Notification helper functions
  const showNotification = (type, message) => {
    setNotification({
      show: true,
      type,
      message
    });
    
    // Auto-hide notification after 5 seconds
    setTimeout(() => {
      setNotification(prev => ({ ...prev, show: false }));
    }, 5000);
  };

  const hideNotification = () => {
    setNotification(prev => ({ ...prev, show: false }));
  };

  // Poll UE status every second
  const pollUEStatus = async () => {
    try {
      const response = await fetch('http://10.1.100.143:40000/api/ue/status');
      if (response.ok) {
        const statusData = await response.json();
        setRealTimeStatus(statusData);
        // Update local state based on real-time status
        setIsSimulatedUE(statusData.isRunning);
      }
    } catch (error) {
      console.error('Error fetching UE status:', error);
    }
  };

  // Poll for detected cells
  const pollCells = async () => {
    try {
      const response = await fetch('http://10.1.100.143:40000/api/ue/scan/cells');
      if (response.ok) {
        const data = await response.json();
        if (data.success && data.cells) {
          setDetectedCells(data.cells);
        }
      }
    } catch (error) {
      console.error('Error fetching cells:', error);
    }
  };

  // Start cell polling
  const startCellPolling = () => {
    // Clear any existing interval
    if (signalQualityIntervalRef.current) {
      clearInterval(signalQualityIntervalRef.current);
    }
    
    // Start polling for cells every 2 seconds
    signalQualityIntervalRef.current = setInterval(pollCells, 2000);
  };

  // Stop cell polling
  const stopCellPolling = () => {
    if (signalQualityIntervalRef.current) {
      clearInterval(signalQualityIntervalRef.current);
      signalQualityIntervalRef.current = null;
    }
  };

  // Simulate UE status updates
  useEffect(() => {
    const interval = setInterval(() => {
      // Simulate status changes
      setUeStatus(prev => ({
        ...prev,
        signalStrength: Math.random() * 100,
        connected: Math.random() > 0.3
      }));
    }, 2000);

    return () => clearInterval(interval);
  }, []);

  // Cleanup intervals on unmount
  useEffect(() => {
    return () => {
      if (signalQualityIntervalRef.current) {
        clearInterval(signalQualityIntervalRef.current);
      }
    };
  }, []);

  // Poll UE status every second
  useEffect(() => {
    // Initial fetch
    pollUEStatus();
    
    // Set up polling interval
    const statusInterval = setInterval(pollUEStatus, 1000);
    
    return () => {
      clearInterval(statusInterval);
    };
  }, []);


  const handleConfigSave = () => {
    console.log('Saving UE configuration...', ueConfig);
    setIsConfigEditing(false);
    // TODO: Implement config save functionality
  };

  const handleConfigCancel = () => {
    setIsConfigEditing(false);
    // TODO: Reset config to original values
  };


  const handleStartSimulatedUE = async () => {
    try {
      console.log('Starting UE process with full configuration...');
      console.log('Current ueConfig:', ueConfig);
      console.log('Current centerFrequency:', centerFrequency);
      
      // Prepare complete configuration
      const configData = {
        radio: {
          freq: parseInt(centerFrequency),
          bw: ueConfig.bandwidth,
          numerology: ueConfig.numerology
        },
        authentication: {
          imsi: ueConfig.imsi,
          key: ueConfig.key,
          opc: ueConfig.opc
        },
        network: {
          dnn: ueConfig.dnn,
          nssai_sst: ueConfig.nssai_sst,
          nssai_sd: ueConfig.nssai_sd
        }
      };
      
      console.log('Sending configuration data:', configData);
      
      // Call backend API to start UE process (unified interface)
      const response = await fetch('http://10.1.100.143:40000/api/ue/start', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify(configData)
      });
      
      if (response.ok) {
        const result = await response.json();
        console.log('UE process started:', result);
        setIsSimulatedUE(true); // Only set to true when API call is successful
        showNotification('success', 'UE process started successfully!');
      } else {
        console.error('Failed to start UE process');
        setIsSimulatedUE(false);
        showNotification('error', 'Failed to start UE process');
      }
    } catch (error) {
      console.error('Error starting UE process:', error);
      setIsSimulatedUE(false);
      showNotification('error', 'Error starting UE process: ' + error.message);
    }
  };

  const handleStopSimulatedUE = async () => {
    try {
      console.log('Stopping UE process...');
      
      // Call backend API to stop UE process (unified interface)
      const response = await fetch('http://10.1.100.143:40000/api/ue/stop', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        }
      });
      
      if (response.ok) {
        const result = await response.json();
        console.log('UE process stopped:', result);
        setIsSimulatedUE(false); // Only set to false when API call is successful
        showNotification('success', 'UE process stopped successfully!');
      } else {
        console.error('Failed to stop UE process');
        showNotification('error', 'Failed to stop UE process');
      }
      
    } catch (error) {
      console.error('Error stopping UE process:', error);
      setIsSimulatedUE(false);
      showNotification('error', 'Error stopping UE process: ' + error.message);
    }
  };

  const handleModeChange = async (event) => {
    const newMode = event.target.value;
    try {
      console.log('Switching UE mode to:', newMode);
      
      // Call backend API to switch mode
      const response = await fetch('http://10.1.100.143:40000/api/ue/mode', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ mode: newMode })
      });
      
      if (response.ok) {
        const result = await response.json();
        console.log('UE mode switched:', result);
        showNotification('success', `UE mode switched to ${newMode}`);
        // Update local state
        setRealTimeStatus(prev => ({ ...prev, currentMode: newMode }));
      } else {
        console.error('Failed to switch UE mode');
        showNotification('error', 'Failed to switch UE mode');
      }
      
    } catch (error) {
      console.error('Error switching UE mode:', error);
      showNotification('error', 'Error switching UE mode: ' + error.message);
    }
  };

  const handleStartScan = async () => {
    try {
      console.log('Starting cell scan...');
      
      // Call backend API to start cell scanning
      const response = await fetch('http://10.1.100.143:40000/api/ue/scan/start', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({})
      });
      
      if (response.ok) {
        const result = await response.json();
        console.log('Cell scan started:', result);
        setIsScanning(true);
        showNotification('success', 'Cell scanning started successfully!');
        
        // Start polling for cells
        startCellPolling();
      } else {
        console.error('Failed to start cell scan');
        setIsScanning(false);
        showNotification('error', 'Failed to start cell scan');
      }
    } catch (error) {
      console.error('Error starting cell scan:', error);
      setIsScanning(false);
      showNotification('error', 'Error starting cell scan: ' + error.message);
    }
  };

  const handleStopScan = async () => {
    try {
      console.log('Stopping cell scan...');
      
      // Call backend API to stop cell scanning
      const response = await fetch('http://10.1.100.143:40000/api/ue/scan/stop', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({})
      });
      
      if (response.ok) {
        const result = await response.json();
        console.log('Cell scan stopped:', result);
        setIsScanning(false);
        showNotification('success', 'Cell scanning stopped successfully!');
        
        // Stop polling for cells
        stopCellPolling();
      } else {
        console.error('Failed to stop cell scan');
        showNotification('error', 'Failed to stop cell scan');
      }
    } catch (error) {
      console.error('Error stopping cell scan:', error);
      showNotification('error', 'Error stopping cell scan: ' + error.message);
    }
  };









  const getSignalQualityColor = (ssRsrp) => {
    if (ssRsrp > -80) return '#4CAF50'; // Excellent
    if (ssRsrp > -90) return '#8BC34A'; // Good
    if (ssRsrp > -100) return '#FFC107'; // Fair
    return '#F44336'; // Poor
  };

  const getSignalQualityText = (ssRsrp) => {
    if (ssRsrp > -80) return 'Excellent';
    if (ssRsrp > -90) return 'Good';
    if (ssRsrp > -100) return 'Fair';
    return 'Poor';
  };

  const formatFrequency = (freqHz) => {
    const freqMHz = freqHz / 1000000;
    return `${freqMHz.toFixed(3)} MHz`;
  };

  const validateFrequency = (freq) => {
    const numFreq = parseFloat(freq);
    // Check if it's a valid number and within reasonable 5G NR frequency range
    return !isNaN(numFreq) && numFreq >= 1000000000 && numFreq <= 6000000000;
  };

  const handleFrequencyChange = (e) => {
    const value = e.target.value;
    // Allow only numbers
    if (/^\d*$/.test(value)) {
      setCenterFrequency(value);
    }
  };

  const toggleCellExpansion = (cellId) => {
    setExpandedCells(prev => {
      const newSet = new Set(prev);
      if (newSet.has(cellId)) {
        newSet.delete(cellId);
      } else {
        newSet.add(cellId);
      }
      return newSet;
    });
  };

  const calculatePCI = (pssValue, sssValue) => {
    // PCI = 3 × N_ID^(1) + N_ID^(2)
    // where N_ID^(1) = SSS value (0-335) and N_ID^(2) = PSS value (0-2)
    return 3 * sssValue + pssValue;
  };


  return (
    <div className="ue-page">
      {/* Notification */}
      {notification.show && (
        <div className={`notification ${notification.type}`}>
          <div className="notification-content">
            <span className="notification-message">{notification.message}</span>
            <button className="notification-close" onClick={hideNotification}>×</button>
          </div>
        </div>
      )}

      <div className="ue-header">
        <h1>UE (User Equipment) Management</h1>
        <p>Monitor and control your 5G UE device</p>
      </div>

      <div className="ue-content">
        {/* Configuration */}
        <div className="ue-section">
          <div className="section-header">
            <h2>Configuration</h2>
            <div className="section-actions">
              {!isConfigEditing ? (
                <button 
                  className="btn btn-primary" 
                  onClick={() => setIsConfigEditing(true)}
                >
                  Edit Configuration
                </button>
              ) : (
                <div className="edit-actions">
                  <button 
                    className="btn btn-save" 
                    onClick={handleConfigSave}
                  >
                    Save
                  </button>
                  <button 
                    className="btn btn-cancel" 
                    onClick={handleConfigCancel}
                  >
                    Cancel
                  </button>
                </div>
              )}
            </div>
          </div>

          <div className="config-grid">
            <div className="config-group">
              <h3>Radio</h3>
              <div className="config-item">
                <label>Center Frequency (Hz):</label>
                <div className="freq-input-group">
                  <input 
                    type="text" 
                    value={centerFrequency} 
                    onChange={handleFrequencyChange}
                    placeholder="3425010000"
                    className={`freq-input ${!validateFrequency(centerFrequency) && centerFrequency ? 'invalid' : ''}`}
                    disabled={!isConfigEditing}
                  />
                  <span className="freq-unit">Hz</span>
                  <span className="freq-display">{formatFrequency(centerFrequency)}</span>
                </div>
                {!validateFrequency(centerFrequency) && centerFrequency && (
                  <div className="freq-error">
                    Please enter a valid frequency (1-6000 MHz range)
                  </div>
                )}
              </div>
              <div className="config-item">
                <label>Bandwidth (MHz):</label>
                <input 
                  type="text" 
                  value={ueConfig.bandwidth} 
                  onChange={(e) => setUeConfig({...ueConfig, bandwidth: e.target.value})}
                  disabled={!isConfigEditing}
                />
              </div>
              <div className="config-item">
                <label>Numerology:</label>
                <input 
                  type="text" 
                  value={ueConfig.numerology} 
                  onChange={(e) => setUeConfig({...ueConfig, numerology: e.target.value})}
                  disabled={!isConfigEditing}
                />
              </div>
            </div>

            <div className="config-group">
              <h3>Authentication</h3>
              <div className="config-item">
                <label>IMSI:</label>
                <input 
                  type="text" 
                  value={ueConfig.imsi} 
                  onChange={(e) => setUeConfig({...ueConfig, imsi: e.target.value})}
                  disabled={!isConfigEditing}
                />
              </div>
              <div className="config-item">
                <label>Key:</label>
                <input 
                  type="text" 
                  value={ueConfig.key} 
                  onChange={(e) => setUeConfig({...ueConfig, key: e.target.value})}
                  disabled={!isConfigEditing}
                />
              </div>
              <div className="config-item">
                <label>OPc:</label>
                <input 
                  type="text" 
                  value={ueConfig.opc} 
                  onChange={(e) => setUeConfig({...ueConfig, opc: e.target.value})}
                  disabled={!isConfigEditing}
                />
              </div>
            </div>

            <div className="config-group">
              <h3>Network</h3>
              <div className="config-item">
                <label>DNN:</label>
                <input 
                  type="text" 
                  value={ueConfig.dnn} 
                  onChange={(e) => setUeConfig({...ueConfig, dnn: e.target.value})}
                  disabled={!isConfigEditing}
                />
              </div>
              <div className="config-item">
                <label>NSSAI SST:</label>
                <input 
                  type="number" 
                  value={ueConfig.nssai_sst} 
                  onChange={(e) => setUeConfig({...ueConfig, nssai_sst: parseInt(e.target.value)})}
                  disabled={!isConfigEditing}
                />
              </div>
              <div className="config-item">
                <label>NSSAI SD:</label>
                <input 
                  type="number" 
                  value={ueConfig.nssai_sd} 
                  onChange={(e) => setUeConfig({...ueConfig, nssai_sd: parseInt(e.target.value)})}
                  disabled={!isConfigEditing}
                />
              </div>
            </div>
          </div>
        </div>

        {/* Cell Scan */}
        <div className="ue-section">
          <div className="section-header scan-header">
            <div className="header-title">
              <h2>Cell Scan</h2>
            </div>
            <div className="header-controls">
              <button 
                className={`btn ${isScanning ? 'btn-stop-scan' : 'btn-scan'}`}
                onClick={isScanning ? handleStopScan : handleStartScan}
                disabled={!isSimulatedUE}
              >
                {isScanning ? 'Stop Scan' : 'Start Scan'}
              </button>
            </div>
            <div className="header-status">
              {isScanning && (
                <div className="acquisition-status">
                  <div className="acquisition-indicator acquiring"></div>
                  <span className="acquisition-text">
                    Scanning for cells...
                  </span>
                </div>
              )}
            </div>
          </div>


          <div className="cell-scan-container">

            <div className="detected-cells">
              <h3>Detected Cells ({detectedCells.length})</h3>
              <div className="cells-list">
                {detectedCells.map((cell) => {
                  const isExpanded = expandedCells.has(cell.id);
                  return (
                    <div key={cell.id} className={`cell-item ${cell.isServing || false ? 'serving' : ''}`}>
                      {/* Cell Summary - Always Visible */}
                      <div 
                        className="cell-summary" 
                        onClick={() => toggleCellExpansion(cell.id)}
                      >
                        <div className="cell-header">
                          <div className="cell-id">
                            <span className="cell-label">PCI:</span>
                            <span className="cell-value">{cell.pci}</span>
                            {cell.isServing && <span className="serving-badge">Serving</span>}
                          </div>
                          <div className="signal-quality">
                            <div 
                              className="signal-bar"
                              style={{ backgroundColor: getSignalQualityColor(cell.ss_rsrp) }}
                            ></div>
                            <span className="quality-text">{getSignalQualityText(cell.ss_rsrp)}</span>
                          </div>
                        </div>
                        
                        <div className="cell-summary-metrics">
                          <div className="summary-metric">
                            <span className="metric-label">SS-RSRP:</span>
                            <span className="metric-value">{cell.ss_rsrp?.toFixed(1) || 'N/A'} dBm</span>
                          </div>
                          <div className="summary-metric">
                            <span className="metric-label">SS-RSRQ:</span>
                            <span className="metric-value">{cell.ss_rsrq?.toFixed(1) || 'N/A'} dB</span>
                          </div>
                          <div className="summary-metric">
                            <span className="metric-label">SS-SINR:</span>
                            <span className="metric-value">{cell.ss_sinr?.toFixed(1) || 'N/A'} dB</span>
                          </div>
                        </div>

                        <div className="expand-indicator">
                          <span className="expand-text">
                            {isExpanded ? 'Hide Details' : 'Show Details'}
                          </span>
                          <span className={`expand-arrow ${isExpanded ? 'expanded' : ''}`}>
                            ▼
                          </span>
                        </div>
                      </div>

                      {/* Cell Details - Expandable */}
                      <div className={`cell-details ${isExpanded ? 'expanded' : 'collapsed'}`}>
                        {/* Row 1: Synchronization Status */}
                        <div className="sync-item">
                          <h4>Synchronization Status</h4>
                          <div className="sync-details">
                            <div className="sync-row">
                              <span className="sync-label">PSS (N<sub>ID</sub><sup>(2)</sup>) (0-2):</span>
                              <span className={`sync-status ${cell.pss !== undefined ? 'detected' : 'not-detected'}`}>
                                {cell.pss !== undefined ? cell.pss : 'Not detected'}
                              </span>
                            </div>
                            <div className="sync-row">
                              <span className="sync-label">SSS (N<sub>ID</sub><sup>(1)</sup>) (0-335):</span>
                              <span className={`sync-status ${cell.sss !== undefined ? 'detected' : 'not-detected'}`}>
                                {cell.sss !== undefined ? cell.sss : 'Not detected'}
                              </span>
                            </div>
                            <div className="sync-row full-width">
                              <span className="sync-label">PCI (0-1007):</span>
                              <span className="sync-status detected">
                                {cell.pss !== undefined && cell.sss !== undefined 
                                  ? (
                                    <span className="formula-display">
                                      <span className="formula">3 × N<sub>ID</sub><sup>(1)</sup> + N<sub>ID</sub><sup>(2)</sup></span>
                                      <span className="equals"> = </span>
                                      <span className="substitution">3 × {cell.sss} + {cell.pss}</span>
                                      <span className="equals"> = </span>
                                      <span className="result">{calculatePCI(cell.pss, cell.sss)}</span>
                                    </span>
                                  )
                                  : 'N/A (PSS or SSS not detected)'
                                }
                              </span>
                            </div>
                            <div className="sync-row">
                              <span className="sync-label">PBCH:</span>
                              <span className={`sync-status ${cell.pbch_decoded ? 'detected' : 'not-detected'}`}>
                                {cell.pbch_decoded ? 'Decoded' : 'Not decoded'}
                              </span>
                            </div>
                            <div className="sync-row">
                              <span className="sync-label">SIB1:</span>
                              <span className={`sync-status ${cell.sib1_detected ? 'detected' : 'not-detected'}`}>
                                {cell.sib1_detected ? 'Detected' : 'Not detected'}
                              </span>
                            </div>
                          </div>
                        </div>

                        {/* Row 2: MIB Information */}
                        {cell.mib && (
                          <div className="sync-item">
                            <h4>MIB Information (Master Information Block)</h4>
                            <div className="mib-details">
                              <div className="mib-row">
                                <span>System Frame Number:</span>
                                <span>{cell.mib.system_frame_number}</span>
                              </div>
                              <div className="mib-row">
                                <span>Subcarrier Spacing:</span>
                                <span>{cell.mib.subcarrier_spacing}</span>
                              </div>
                              <div className="mib-row">
                                <span>SSB Subcarrier Offset:</span>
                                <span>{cell.mib.ssb_subcarrier_offset}</span>
                              </div>
                              <div className="mib-row">
                                <span>DMRS Type A Position:</span>
                                <span>{cell.mib.dmrs_type_a_position}</span>
                              </div>
                              <div className="mib-row">
                                <span>PDCCH Config SIB1:</span>
                                <span>{cell.mib.pdcch_config_sib1}</span>
                              </div>
                              <div className="mib-row">
                                <span>Cell Barred:</span>
                                <span className={cell.mib.cell_barred === 'True' ? 'status-barred' : 'status-available'}>
                                  {cell.mib.cell_barred === 'True' ? 'Yes (Barred)' : 'No (Available)'}
                                </span>
                              </div>
                              <div className="mib-row">
                                <span>Intra-Freq Reselection:</span>
                                <span className={cell.mib.intra_freq_reselection === 'True' ? 'status-allowed' : 'status-restricted'}>
                                  {cell.mib.intra_freq_reselection === 'True' ? 'Allowed' : 'Restricted'}
                                </span>
                              </div>
                              <div className="mib-row">
                                <span>Spare:</span>
                                <span>{cell.mib.spare}</span>
                              </div>
                            </div>
                          </div>
                        )}

                        {/* Row 3: SIB1 Information */}
                        {cell.sib1 && (
                          <div className="sync-item">
                            <h4>SIB1 Information (System Information Block 1)</h4>
                            <div className="sib1-details">
                              <div className="info-section">
                                <h5>Cell Information</h5>
                                <div className="sib1-row">
                                  <span>Cell Identity:</span>
                                  <span>{cell.sib1.cell_access_related_info}</span>
                                </div>
                                <div className="sib1-row">
                                  <span>Cell Selection Info:</span>
                                  <span>{cell.sib1.cell_selection_info}</span>
                                </div>
                                <div className="sib1-row">
                                  <span>P-Max:</span>
                                  <span>{cell.sib1.p_max}</span>
                                </div>
                                <div className="sib1-row">
                                  <span>Frequency Band:</span>
                                  <span>{cell.sib1.frequency_band_list}</span>
                                </div>
                              </div>

                              <div className="info-section">
                                <h5>Additional Information</h5>
                                <div className="sib1-row">
                                  <span>SCS Specific Carrier List:</span>
                                  <span>{cell.sib1.scs_specific_carrier_list}</span>
                                </div>
                                <div className="sib1-row">
                                  <span>TDD UL/DL Configuration:</span>
                                  <span>{cell.sib1.tdd_ul_dl_configuration_common}</span>
                                </div>
                                <div className="sib1-row">
                                  <span>SSB Positions in Burst:</span>
                                  <span>{cell.sib1.ssb_positions_in_burst}</span>
                                </div>
                                <div className="sib1-row">
                                  <span>SSB Periodicity:</span>
                                  <span>{cell.sib1.ssb_periodicity_serving_cell}</span>
                                </div>
                              </div>

                              <div className="info-section">
                                <h5>PDCCH Configuration</h5>
                                <div className="sib1-row">
                                  <span>PDCCH Config SIB1:</span>
                                  <span>{cell.sib1.pdcch_config_sib1}</span>
                                </div>
                                <div className="sib1-row">
                                  <span>DMRS Type A Position:</span>
                                  <span>{cell.sib1.dmrs_type_a_position}</span>
                                </div>
                              </div>
                            </div>
                          </div>
                        )}
                      </div>
                    </div>
                  );
                })}
              </div>
            </div>
          </div>
        </div>




        {/* Recent Logs */}
        <div className="ue-section">
          <h2>Recent Logs</h2>
          <div className="logs-container">
            <div className="log-entry">
              <span className="log-time">[12:34:56]</span>
              <span className="log-level info">[INFO]</span>
              <span className="log-message">UE initialized successfully</span>
            </div>
            <div className="log-entry">
              <span className="log-time">[12:34:55]</span>
              <span className="log-level debug">[DEBUG]</span>
              <span className="log-message">Starting cell search...</span>
            </div>
            <div className="log-entry">
              <span className="log-time">[12:34:54]</span>
              <span className="log-level info">[INFO]</span>
              <span className="log-message">USRP device connected</span>
            </div>
          </div>
        </div>
      </div>

      {/* Floating UE Process Management Panel */}
      <div className="floating-ue-panel">
        <div className="floating-panel-header">
          <h3>UE Process</h3>
        </div>
        
        <div className="floating-panel-content">
          <div className="floating-controls">
            {/* UE Mode Selector */}
            <div className="control-group">
              <select 
                value={realTimeStatus.currentMode} 
                onChange={handleModeChange}
                className="mode-selector"
              >
                <option value="simulated">Simulated</option>
                <option value="real">Real</option>
              </select>
            </div>
            
            {/* UE Process */}
            <div className="control-group">
              <button 
                className={!isSimulatedUE ? "btn btn-simulated" : "btn btn-stop-simulated"}
                onClick={!isSimulatedUE ? handleStartSimulatedUE : handleStopSimulatedUE}
                disabled={!isSimulatedUE && !validateFrequency(centerFrequency)}
              >
                {!isSimulatedUE ? "Start UE Process" : "Stop UE Process"}
              </button>
            </div>

          </div>

          <div className="floating-status">
            <div className="process-status">
              <div className={`process-indicator ${realTimeStatus.isRunning ? 'running' : 'stopped'}`}></div>
              <span className="process-text">
                {realTimeStatus.currentMode === 'simulated' ? 'Simulated' : 'Real'} UE: {realTimeStatus.status}
              </span>
            </div>
          </div>

          <div className="floating-info">
            <div className="info-row">
              <span className="info-label">Frequency:</span>
              <span className="info-value">{centerFrequency} Hz</span>
            </div>
            <div className="info-row">
              <span className="info-label">Mode:</span>
              <span className="info-value">{realTimeStatus.currentMode}</span>
            </div>
            <div className="info-row">
              <span className="info-label">Status:</span>
              <span className={`info-value ${realTimeStatus.isRunning ? 'running' : 'stopped'}`}>
                {realTimeStatus.status}
              </span>
            </div>
            {realTimeStatus.isRunning && (
              <div className="info-row">
                <span className="info-label">Uptime:</span>
                <span className="info-value">{realTimeStatus.uptime}s</span>
              </div>
            )}
            {realTimeStatus.isRunning && realTimeStatus.grpcPort && (
              <div className="info-row">
                <span className="info-label">gRPC Port:</span>
                <span className="info-value">{realTimeStatus.grpcPort}</span>
              </div>
            )}
            {realTimeStatus.timestamp && (
              <div className="info-row">
                <span className="info-label">Last Update:</span>
                <span className="info-value">{new Date(realTimeStatus.timestamp).toLocaleTimeString()}</span>
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}

export default UEPage;

