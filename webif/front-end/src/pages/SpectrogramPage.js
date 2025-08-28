import React, { useState, useEffect, useRef } from 'react';
import axios from 'axios';

function SpectrogramPage() {
  const [isCapturing, setIsCapturing] = useState(false);
  const [spectrogramData, setSpectrogramData] = useState(null);
  const [settings, setSettings] = useState({
    frequency: 2400,
    sampleRate: 44100,
    fftSize: 1024,
    windowType: 'hann',
    bufferSize: 2048,
    colormap: 'viridis',
    minFreq: 0,
    maxFreq: 22050,
    loudnessSensibility: 0.5,
    scrollDirection: 'up',
    scrollSpeed: 1,
    pauseScrolling: false,
    displayMode: 'linear' // 'linear' or 'mel'
  });
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [maxFrequency, setMaxFrequency] = useState({ freq: 0, amplitude: 0 });
  const [currentTime, setCurrentTime] = useState(0);
  const [streamConnected, setStreamConnected] = useState(false); // SSE connection status
  
  const [frequencyInput, setFrequencyInput] = useState('1G');
  const [sampleRateInput, setSampleRateInput] = useState('44.1K');
  const [lastAutoSave, setLastAutoSave] = useState(null);
  const [isInitialLoad, setIsInitialLoad] = useState(true);
  
  const canvasRef = useRef(null);
  const animationRef = useRef(null);
  const spectrogramHistory = useRef([]);
  const maxHistoryLength = 200; // Number of time slices to keep
  const startTimeRef = useRef(null);
  const eventSourceRef = useRef(null); // SSE connection reference

  // Colormap definitions (inspired by js-colormaps)
  const colormaps = {
    viridis: [
      [68, 1, 84], [72, 35, 116], [64, 67, 135], [52, 94, 141], [41, 120, 142],
      [31, 144, 139], [37, 167, 133], [92, 185, 125], [152, 202, 113], [213, 218, 103],
      [253, 231, 37], [254, 240, 65], [254, 249, 93], [254, 254, 121]
    ],
    plasma: [
      [13, 8, 135], [75, 3, 161], [125, 3, 168], [168, 34, 150], [203, 70, 121],
      [229, 107, 93], [248, 148, 65], [253, 195, 40], [240, 249, 33]
    ],
    inferno: [
      [0, 0, 4], [31, 12, 9], [77, 15, 8], [103, 23, 7], [130, 44, 12],
      [179, 70, 8], [217, 95, 14], [244, 142, 49], [254, 204, 97], [254, 254, 164]
    ],
    magma: [
      [0, 0, 4], [28, 16, 68], [79, 18, 123], [129, 37, 129], [181, 54, 122],
      [229, 80, 100], [251, 135, 97], [254, 194, 135], [254, 253, 204]
    ],
    cool: [
      [0, 255, 255], [0, 200, 255], [0, 150, 255], [0, 100, 255], [0, 50, 255],
      [0, 0, 255], [50, 0, 255], [100, 0, 255], [150, 0, 255], [200, 0, 255]
    ],
    hot: [
      [0, 0, 0], [128, 0, 0], [255, 0, 0], [255, 128, 0], [255, 255, 0],
      [255, 255, 128], [255, 255, 255]
    ]
  };

  // Convert frequency to color using colormap
  const frequencyToColor = (freq, amplitude) => {
    const colormap = colormaps[settings.colormap] || colormaps.viridis;
    const normalizedFreq = Math.min(Math.max(freq / settings.maxFreq, 0), 1);
    const normalizedAmp = Math.min(Math.max(amplitude / 100, 0), 1);
    
    const colorIndex = Math.floor(normalizedFreq * (colormap.length - 1));
    const color = colormap[colorIndex];
    
    // Apply amplitude scaling with better contrast
    const alpha = Math.max(0.1, normalizedAmp);
    
    return `rgba(${color[0]}, ${color[1]}, ${color[2]}, ${alpha})`;
  };

  // Get colormap gradient for CSS
  const getColormapGradient = (colormapName) => {
    const colormap = colormaps[colormapName] || colormaps.viridis;
    return colormap.map(color => `rgb(${color[0]}, ${color[1]}, ${color[2]})`).join(', ');
  };

  // Draw spectrogram on canvas
  const drawSpectrogram = () => {
    const canvas = canvasRef.current;
    if (!canvas) {
      console.log('Canvas not available');
      return;
    }
    
    const ctx = canvas.getContext('2d');
    const width = canvas.width;
    const height = canvas.height;
    
    console.log('🎨 Drawing spectrogram:', {
      canvasWidth: width,
      canvasHeight: height,
      historyLength: spectrogramHistory.current.length,
      isCapturing,
      hasData: spectrogramHistory.current.length > 0
    });
    
    // Check if canvas has proper dimensions
    if (width <= 0 || height <= 0) {
      console.log('Canvas has invalid dimensions, skipping draw');
      return;
    }
    
    // Clear canvas
    ctx.fillStyle = 'rgba(0, 0, 0, 0.1)';
    ctx.fillRect(0, 0, width, height);
    
    if (spectrogramHistory.current.length === 0) {
      console.log('⚠️ No spectrogram data to draw');
      return;
    }
    
    // Draw spectrogram with time on y-axis and frequency on x-axis (full canvas)
    const spectrogramWidth = width;
    const spectrogramHeight = height;
    
    // Calculate proper scaling for frequency and time
    const freqWidth = spectrogramWidth / (settings.fftSize / 2);
    const timeHeight = spectrogramHeight / maxHistoryLength;
    
    // Clear the spectrogram area
    ctx.fillStyle = 'rgba(0, 0, 0, 0.8)';
    ctx.fillRect(0, 0, spectrogramWidth, spectrogramHeight);
    
    // Draw spectrogram data with proper alignment
    console.log('🎯 Drawing spectrogram data:', {
      slices: spectrogramHistory.current.length,
      pointsPerSlice: spectrogramHistory.current[0]?.length || 0,
      freqWidth,
      timeHeight
    });
    
    // Apply scroll direction
    const dataToDraw = settings.scrollDirection === 'down' 
      ? [...spectrogramHistory.current].reverse() 
      : spectrogramHistory.current;
    
    dataToDraw.forEach((slice, timeIndex) => {
      const y = timeIndex * timeHeight;
      
      slice.forEach((amplitude, freqIndex) => {
        const x = freqIndex * freqWidth;
        const color = frequencyToColor(freqIndex * (settings.maxFreq / (settings.fftSize / 2)), amplitude);
        
        ctx.fillStyle = color;
        ctx.fillRect(x, y, freqWidth, timeHeight);
      });
    });
    
    console.log('✅ Finished drawing spectrogram data');
  };





  // Animation loop
  const animate = () => {
    console.log('🎬 Animation frame called');
    drawSpectrogram();
    animationRef.current = requestAnimationFrame(animate);
  };

  // Start spectrogram capture
  const startCapture = async () => {
    try {
      console.log('🚀 Starting spectrogram capture...');
      setLoading(true);
      setError(null);
      
      console.log('📤 Sending start request with settings:', settings);
      const response = await axios.post('/api/spectrogram/start', settings);
      
      if (response.data.success) {
        console.log('✅ Start request successful:', response.data);
        
        console.log('✅ Polling started, beginning capture...');
        setIsCapturing(true);
        setLoading(false);
        startTimeRef.current = Date.now();
        setCurrentTime(0);
        
        console.log('🔌 Connecting to WebSocket...');
        // Connect to WebSocket for true server-push
        connectToWebSocket();
        
        console.log('🎬 Starting animation loop...');
        // Start animation
        animate();
      } else {
        console.error('❌ Start request failed:', response.data);
        setError('Failed to start spectrogram capture');
        setLoading(false);
      }
    } catch (err) {
      console.error('Error starting spectrogram:', err);
      setError('Failed to start spectrogram capture');
      setLoading(false);
    }
  };

  // Stop spectrogram capture
  const stopCapture = async () => {
    try {
      setLoading(true);
      setError(null);
      
      const response = await axios.post('/api/spectrogram/stop');
      
      if (response.data.success) {
        setIsCapturing(false);
        setLoading(false);
        
        // Disconnect from SSE stream
        disconnectFromStream();
        
        // Stop animation
        if (animationRef.current) {
          cancelAnimationFrame(animationRef.current);
        }
      } else {
        setError('Failed to stop spectrogram capture');
        setLoading(false);
      }
    } catch (err) {
      console.error('Error stopping spectrogram:', err);
      setError('Failed to stop spectrogram capture');
      setLoading(false);
    }
  };

    // Connect to WebSocket for true server-push
  const connectToWebSocket = () => {
    try {
      console.log('🔌 Connecting to WebSocket for server-push...');
      
      const serverIP = window.location.hostname;
      const serverPort = '40000';
      const wsUrl = `ws://${serverIP}:${serverPort}`;
      
      console.log('🌐 WebSocket URL:', wsUrl);
      const ws = new WebSocket(wsUrl);
      
      ws.onopen = () => {
        console.log('✅ WebSocket connected');
        setStreamConnected(true);
      };
      
      ws.onmessage = (event) => {
        try {
          const message = JSON.parse(event.data);
          
          if (message.type === 'connected') {
            console.log('🔗 WebSocket connected:', message.message);
            return;
          }
          
          if (message.type === 'spectrogram_data' && message.data) {
            const data = message.data;
            console.log('📊 Received WebSocket spectrogram data:', {
              dataLength: data.data.length,
              timestamp: data.timestamp,
              historyLength: spectrogramHistory.current.length
            });
            
            // Update current time
            if (startTimeRef.current) {
              const elapsed = (Date.now() - startTimeRef.current) / 1000;
              setCurrentTime(elapsed);
            }
            
            // Add new data at the beginning (top) and remove oldest (bottom)
            spectrogramHistory.current.unshift(data.data);
            
            // Keep only the window size
            if (spectrogramHistory.current.length > maxHistoryLength) {
              spectrogramHistory.current.pop();
            }
            
            // Find max frequency
            const maxIndex = data.data.indexOf(Math.max(...data.data));
            const maxFreq = maxIndex * (settings.maxFreq / (settings.fftSize / 2));
            setMaxFrequency({ freq: maxFreq, amplitude: data.data[maxIndex] });
            
            setSpectrogramData(data);
          }
        } catch (error) {
          console.error('❌ Error parsing WebSocket message:', error);
        }
      };
      
      ws.onerror = (error) => {
        console.error('❌ WebSocket error:', error);
        setStreamConnected(false);
        setError('WebSocket connection error');
      };
      
      ws.onclose = () => {
        console.log('🔌 WebSocket disconnected');
        setStreamConnected(false);
      };
      
      // Store WebSocket reference for cleanup
      eventSourceRef.current = { 
        ws,
        close: () => {
          console.log('🛑 Closing WebSocket connection');
          if (ws.readyState === WebSocket.OPEN) {
            ws.close();
          }
          eventSourceRef.current = null;
        }
      };
      
    } catch (error) {
      console.error('❌ Error connecting to WebSocket:', error);
      setError('Failed to connect to WebSocket');
    }
  };
  
  // Stop polling
  const disconnectFromStream = () => {
    if (eventSourceRef.current) {
      eventSourceRef.current.close();
      eventSourceRef.current = null;
      setStreamConnected(false);
      console.log('Polling stopped');
    }
  };

  // Convert frequency string to Hz
  const convertFrequencyToHz = (frequencyStr) => {
    if (typeof frequencyStr === 'number') {
      return frequencyStr;
    }
    
    const str = frequencyStr.toString().trim().toUpperCase();
    
    // Extract number and unit - improved regex for floating point
    const match = str.match(/^([0-9]+\.?[0-9]*|[0-9]*\.[0-9]+)\s*([GMK]?)$/);
    if (!match) {
      return 0; // Invalid format
    }
    
    const number = parseFloat(match[1]);
    const unit = match[2];
    
    // Handle invalid numbers
    if (isNaN(number)) {
      return 0;
    }
    
    switch (unit) {
      case 'G':
        return number * 1000000000; // GHz to Hz
      case 'M':
        return number * 1000000; // MHz to Hz
      case 'K':
        return number * 1000; // KHz to Hz
      default:
        return number; // Assume Hz if no unit
    }
  };

  // Convert Hz to readable format
  const convertHzToReadable = (hz) => {
    if (hz >= 1000000000) {
      return `${(hz / 1000000000).toFixed(1)}G`;
    } else if (hz >= 1000000) {
      return `${(hz / 1000000).toFixed(1)}M`;
    } else if (hz >= 1000) {
      return `${(hz / 1000).toFixed(1)}K`;
    } else {
      return hz.toString();
    }
  };

  // Update settings
  const updateSettings = (key, value) => {
    setSettings(prev => ({
      ...prev,
      [key]: value
    }));
  };

  // Convert and update frequency settings
  const convertAndUpdateFrequency = (key, value) => {
    const frequencyHz = convertFrequencyToHz(value);
    setSettings(prev => ({
      ...prev,
      [key]: frequencyHz
    }));
  };

  // Save configuration
  const saveConfig = () => {
    try {
      const configName = prompt('Enter configuration name:');
      if (!configName) return;
      
      const configs = JSON.parse(localStorage.getItem('spectrogramConfigs') || '{}');
      configs[configName] = {
        ...settings,
        savedAt: new Date().toISOString()
      };
      
      localStorage.setItem('spectrogramConfigs', JSON.stringify(configs));
      alert(`Configuration "${configName}" saved successfully!`);
    } catch (error) {
      console.error('Error saving configuration:', error);
      alert('Failed to save configuration');
    }
  };

  // Load configuration
  const loadConfig = () => {
    try {
      const configs = JSON.parse(localStorage.getItem('spectrogramConfigs') || '{}');
      const configNames = Object.keys(configs);
      
      if (configNames.length === 0) {
        alert('No saved configurations found');
        return;
      }
      
      const configName = prompt(`Enter configuration name to load:\n\nAvailable: ${configNames.join(', ')}`);
      if (!configName) return;
      
      if (!configs[configName]) {
        alert(`Configuration "${configName}" not found`);
        return;
      }
      
      const config = configs[configName];
      delete config.savedAt; // Remove metadata
      setSettings(config);
      alert(`Configuration "${configName}" loaded successfully!`);
    } catch (error) {
      console.error('Error loading configuration:', error);
      alert('Failed to load configuration');
    }
  };

  // List saved configurations
  const listConfigs = () => {
    try {
      const configs = JSON.parse(localStorage.getItem('spectrogramConfigs') || '{}');
      const configNames = Object.keys(configs);
      
      if (configNames.length === 0) {
        alert('No saved configurations found');
        return;
      }
      
      const configList = configNames.map(name => {
        const config = configs[name];
        const savedAt = new Date(config.savedAt).toLocaleString();
        return `${name} (saved: ${savedAt})`;
      }).join('\n');
      
      alert(`Saved configurations:\n\n${configList}`);
    } catch (error) {
      console.error('Error listing configurations:', error);
      alert('Failed to list configurations');
    }
  };

  // Delete configuration
  const deleteConfig = () => {
    try {
      const configs = JSON.parse(localStorage.getItem('spectrogramConfigs') || '{}');
      const configNames = Object.keys(configs);
      
      if (configNames.length === 0) {
        alert('No saved configurations found');
        return;
      }
      
      const configName = prompt(`Enter configuration name to delete:\n\nAvailable: ${configNames.join(', ')}`);
      if (!configName) return;
      
      if (!configs[configName]) {
        alert(`Configuration "${configName}" not found`);
        return;
      }
      
      const confirmDelete = window.confirm(`Are you sure you want to delete configuration "${configName}"?`);
      if (!confirmDelete) return;
      
      delete configs[configName];
      localStorage.setItem('spectrogramConfigs', JSON.stringify(configs));
      alert(`Configuration "${configName}" deleted successfully!`);
    } catch (error) {
      console.error('Error deleting configuration:', error);
      alert('Failed to delete configuration');
    }
  };

  // Auto-save configuration as latest.conf
  const autoSaveConfig = () => {
    try {
      const configs = JSON.parse(localStorage.getItem('spectrogramConfigs') || '{}');
      configs['latest.conf'] = {
        ...settings,
        savedAt: new Date().toISOString(),
        autoSaved: true
      };
      
      localStorage.setItem('spectrogramConfigs', JSON.stringify(configs));
      setLastAutoSave(new Date());
      console.log('🔄 Auto-saved configuration as latest.conf');
    } catch (error) {
      console.error('Error auto-saving configuration:', error);
    }
  };

  // Load latest configuration on mount
  const loadLatestConfig = () => {
    try {
      const configs = JSON.parse(localStorage.getItem('spectrogramConfigs') || '{}');
      if (configs['latest.conf']) {
        const latestConfig = configs['latest.conf'];
        delete latestConfig.savedAt;
        delete latestConfig.autoSaved;
        
        // Set the settings first
        setSettings(latestConfig);
        
        // Update input values to match loaded settings
        setFrequencyInput(convertHzToReadable(latestConfig.frequency));
        setSampleRateInput(convertHzToReadable(latestConfig.sampleRate));
        
        // Set last auto-save time from loaded config
        if (latestConfig.savedAt) {
          setLastAutoSave(new Date(latestConfig.savedAt));
        }
        
        console.log('📂 Loaded latest configuration:', {
          frequency: latestConfig.frequency,
          sampleRate: latestConfig.sampleRate,
          frequencyInput: convertHzToReadable(latestConfig.frequency),
          sampleRateInput: convertHzToReadable(latestConfig.sampleRate)
        });
      } else {
        console.log('📂 No latest configuration found');
      }
      
      // Mark initial load as complete
      setIsInitialLoad(false);
    } catch (error) {
      console.error('Error loading latest configuration:', error);
      setIsInitialLoad(false);
    }
  };

  // Initialize canvas
  useEffect(() => {
    console.log('🎨 Initializing canvas...');
    const canvas = canvasRef.current;
    if (canvas) {
      const updateCanvasSize = () => {
        const rect = canvas.getBoundingClientRect();
        canvas.width = rect.width;
        canvas.height = rect.height;
        console.log('📏 Canvas size updated:', { width: canvas.width, height: canvas.height });
      };
      
      updateCanvasSize();
      
      // Add resize listener
      window.addEventListener('resize', updateCanvasSize);
      
      return () => {
        window.removeEventListener('resize', updateCanvasSize);
      };
    }
  }, []);

  // Cleanup SSE connection on unmount
  useEffect(() => {
    return () => {
      disconnectFromStream();
    };
  }, []);

  // Cleanup animation on unmount
  useEffect(() => {
    return () => {
      if (animationRef.current) {
        cancelAnimationFrame(animationRef.current);
      }
    };
  }, []);

  // Load latest configuration on mount
  useEffect(() => {
    console.log('🔄 Component mounted, loading latest configuration...');
    // Add a small delay to ensure component is fully mounted
    setTimeout(() => {
      loadLatestConfig();
    }, 100);
  }, []); // Run only on mount

  // Auto-save configuration when settings change
  useEffect(() => {
    // Don't auto-save on initial load
    if (!isInitialLoad && (settings.frequency !== 1000000000 || settings.sampleRate !== 44100)) {
      autoSaveConfig();
    }
  }, [settings, isInitialLoad]); // Re-run when settings change

  // Auto-save configuration when input values change (for frequency inputs)
  useEffect(() => {
    // Don't auto-save on initial load
    if (!isInitialLoad) {
      // Only save if the input values are valid and different from current settings
      const convertedFreq = convertFrequencyToHz(frequencyInput);
      const convertedSampleRate = convertFrequencyToHz(sampleRateInput);
      
      if (convertedFreq > 0 && convertedSampleRate > 0) {
        // Create a temporary config with current input values
        const tempConfig = {
          ...settings,
          frequency: convertedFreq,
          sampleRate: convertedSampleRate
        };
        
        // Save the temporary config
        try {
          const configs = JSON.parse(localStorage.getItem('spectrogramConfigs') || '{}');
          configs['latest.conf'] = {
            ...tempConfig,
            savedAt: new Date().toISOString(),
            autoSaved: true
          };
          
          localStorage.setItem('spectrogramConfigs', JSON.stringify(configs));
          setLastAutoSave(new Date());
          console.log('🔄 Auto-saved configuration with input values:', {
            frequencyInput,
            sampleRateInput,
            convertedFreq,
            convertedSampleRate
          });
        } catch (error) {
          console.error('Error auto-saving configuration:', error);
        }
      }
    }
  }, [frequencyInput, sampleRateInput, isInitialLoad]); // Re-run when input values change

  return (
    <div className="page-container">
      <div className="page-header">
        <h1>Spectrogram Analysis</h1>
        <p>Real-time frequency domain analysis and visualization</p>
      </div>

      <div className="spectrogram-container">
        {/* Control Panel */}
        <div className="control-panel">
          <div className="control-section">
            <h3>Controls and Configuration</h3>
            <div className="control-buttons">
              <button 
                className={`control-btn ${isCapturing ? 'stop' : 'start'}`}
                onClick={isCapturing ? stopCapture : startCapture}
                disabled={loading}
              >
                {loading ? 'Processing...' : (isCapturing ? 'Stop Capture' : 'Start Capture')}
              </button>
              <button 
                className="control-btn config"
                onClick={saveConfig}
              >
                Save Config
              </button>
              <button 
                className="control-btn config"
                onClick={loadConfig}
              >
                Load Config
              </button>
              <button 
                className="control-btn config"
                onClick={listConfigs}
              >
                List Configs
              </button>
              <button 
                className="control-btn config"
                onClick={deleteConfig}
              >
                Delete Config
              </button>
              <button 
                className="control-btn config"
                onClick={loadLatestConfig}
              >
                Load Latest
              </button>
            </div>
          </div>

          <div className="control-section">
            <h3>Display Settings</h3>
            <div className="settings-grid">
              <div className="setting-item">
                <label>Colormap:</label>
                <select
                  value={settings.colormap}
                  onChange={(e) => updateSettings('colormap', e.target.value)}
                >
                  <option value="viridis">Viridis</option>
                  <option value="plasma">Plasma</option>
                  <option value="inferno">Inferno</option>
                  <option value="magma">Magma</option>
                  <option value="cool">Cool</option>
                  <option value="hot">Hot</option>
                </select>
              </div>
              
              <div className="setting-item">
                <label>Display Mode:</label>
                <select
                  value={settings.displayMode}
                  onChange={(e) => updateSettings('displayMode', e.target.value)}
                >
                  <option value="linear">Linear</option>
                  <option value="mel">Mel Scale</option>
                </select>
              </div>
              
              <div className="setting-item">
                <label>Scroll Direction:</label>
                <select
                  value={settings.scrollDirection}
                  onChange={(e) => updateSettings('scrollDirection', e.target.value)}
                >
                  <option value="up">Up</option>
                  <option value="down">Down</option>
                </select>
              </div>
              
              <div className="setting-item">
                <label>Scroll Speed:</label>
                <input
                  type="range"
                  min="0.1"
                  max="3"
                  step="0.1"
                  value={settings.scrollSpeed}
                  onChange={(e) => updateSettings('scrollSpeed', parseFloat(e.target.value))}
                />
                <span>{settings.scrollSpeed}x</span>
              </div>
            </div>
          </div>

          <div className="control-section">
            <h3>Analysis Settings</h3>
            <div className="settings-grid">
              <div className="setting-item">
                <label>Center Frequency:</label>
                <div className="frequency-input-container">
                  <input
                    type="text"
                    placeholder="e.g., 10G, 100M, 20K"
                    value={frequencyInput}
                    onChange={(e) => setFrequencyInput(e.target.value)}
                    onBlur={() => convertAndUpdateFrequency('frequency', frequencyInput)}
                    onKeyPress={(e) => {
                      if (e.key === 'Enter') {
                        convertAndUpdateFrequency('frequency', frequencyInput);
                        e.target.blur();
                      }
                    }}
                    className="frequency-input"
                  />
                  <span className="frequency-hint">
                    ({settings.frequency.toLocaleString()} Hz)
                  </span>
                </div>
              </div>
              
              <div className="setting-item">
                <label>Sample Rate:</label>
                <div className="frequency-input-container">
                  <input
                    type="text"
                    placeholder="e.g., 44.1K, 48K, 96K"
                    value={sampleRateInput}
                    onChange={(e) => setSampleRateInput(e.target.value)}
                    onBlur={() => convertAndUpdateFrequency('sampleRate', sampleRateInput)}
                    onKeyPress={(e) => {
                      if (e.key === 'Enter') {
                        convertAndUpdateFrequency('sampleRate', sampleRateInput);
                        e.target.blur();
                      }
                    }}
                    className="frequency-input"
                  />
                  <span className="frequency-hint">
                    ({settings.sampleRate.toLocaleString()} Hz)
                  </span>
                </div>
              </div>
              
              <div className="setting-item">
                <label>FFT Size:</label>
                <select
                  value={settings.fftSize}
                  onChange={(e) => updateSettings('fftSize', parseInt(e.target.value))}
                >
                  <option value={512}>512</option>
                  <option value={1024}>1024</option>
                  <option value={2048}>2048</option>
                  <option value={4096}>4096</option>
                </select>
              </div>
              
              <div className="setting-item">
                <label>Window Type:</label>
                <select
                  value={settings.windowType}
                  onChange={(e) => updateSettings('windowType', e.target.value)}
                >
                  <option value="hann">Hann</option>
                  <option value="hamming">Hamming</option>
                  <option value="blackman">Blackman</option>
                  <option value="rectangular">Rectangular</option>
                </select>
              </div>
              
              <div className="setting-item">
                <label>Max Frequency (Hz):</label>
                <input
                  type="number"
                  value={settings.maxFreq}
                  onChange={(e) => updateSettings('maxFreq', parseInt(e.target.value))}
                  min="1000"
                  max="96000"
                  step="1000"
                />
              </div>
              
              <div className="setting-item">
                <label>Loudness Sensitivity:</label>
                <input
                  type="range"
                  min="0.1"
                  max="2"
                  step="0.1"
                  value={settings.loudnessSensibility}
                  onChange={(e) => updateSettings('loudnessSensibility', parseFloat(e.target.value))}
                />
                <span>{settings.loudnessSensibility}</span>
              </div>
            </div>
          </div>
        </div>

        {/* Spectrogram Display */}
        <div className="spectrogram-display">
          <div className="display-header">
            <h3>Spectrogram Visualization</h3>
            <div className="display-info">
              <span className={`status-indicator ${isCapturing ? 'active' : 'inactive'}`}>
                {isCapturing ? 'Capturing' : 'Stopped'}
              </span>
              <span className={`status-indicator ${streamConnected ? 'active' : 'inactive'}`}>
                {streamConnected ? 'Stream Connected' : 'Stream Disconnected'}
              </span>
              {maxFrequency.freq > 0 && (
                <span className="max-frequency">
                  Max: {maxFrequency.freq.toFixed(1)} Hz ({maxFrequency.amplitude.toFixed(1)} dB)
                </span>
              )}
              {isCapturing && (
                <span className="current-time">
                  Time: {currentTime.toFixed(1)}s
                </span>
              )}
            </div>
          </div>
          
          <div className="spectrogram-visualization">
            {/* Frequency labels (top) */}
            <div className="frequency-labels-top">
              {Array.from({length: 9}, (_, i) => {
                const freq = (settings.maxFreq * i) / 8;
                return (
                  <span key={i} className="freq-label">
                    {freq >= 1000 ? `${(freq / 1000).toFixed(1)}k` : freq.toFixed(0)}
                  </span>
                );
              })}
            </div>
            
            <div className="spectrogram-main">
              {/* Time labels (left) */}
              <div className="time-labels-left">
                {Array.from({length: 6}, (_, i) => {
                  const time = (maxHistoryLength * 0.05 * (5 - i)) / 5;
                  return (
                    <span key={i} className="time-label">
                      {time.toFixed(1)}s
                    </span>
                  );
                })}
              </div>
              
              {/* Spectrogram canvas */}
              <div className="spectrogram-canvas">
                <canvas
                  ref={canvasRef}
                  className="spectrogram-canvas-element"
                  style={{
                    width: '100%',
                    height: '100%',
                    borderRadius: '8px',
                    background: 'rgba(0, 0, 0, 0.8)'
                  }}
                />
              </div>
              
              {/* Color bar (right) */}
              <div className="color-bar-container">
                <div className="color-bar-title">Amplitude</div>
                <div className="color-bar">
                  <div 
                    className="color-bar-gradient"
                    style={{
                      background: `linear-gradient(to bottom, ${getColormapGradient(settings.colormap)})`
                    }}
                  ></div>
                  <div className="color-bar-labels">
                    <span className="color-label max">100 dB</span>
                    <span className="color-label mid">50 dB</span>
                    <span className="color-label min">0 dB</span>
                  </div>
                </div>
              </div>
            </div>

            {/* Frequency labels (bottom) */}
            <div className="frequency-labels-bottom">
              {Array.from({length: 9}, (_, i) => {
                const freq = (settings.maxFreq * i) / 8;
                return (
                  <span key={i} className="freq-label">
                    {freq >= 1000 ? `${(freq / 1000).toFixed(1)}k` : freq.toFixed(0)}
                  </span>
                );
              })}
            </div>
          </div>
        </div>

        {/* Error Display */}
        {error && (
          <div className="error-message">
            <p>{error}</p>
            <button onClick={() => setError(null)}>Dismiss</button>
          </div>
        )}

        {/* Status Bar */}
        <div className="status-bar">
          <div className="status-item">
            <span className="status-label">Capture:</span>
            <span className={`status-value ${isCapturing ? 'active' : 'inactive'}`}>
              {isCapturing ? 'Running' : 'Stopped'}
            </span>
          </div>
          <div className="status-item">
            <span className="status-label">Connection:</span>
            <span className={`status-value ${streamConnected ? 'active' : 'inactive'}`}>
              {streamConnected ? 'Connected' : 'Disconnected'}
            </span>
          </div>
          <div className="status-item">
            <span className="status-label">Data Points:</span>
            <span className="status-value">
              {spectrogramHistory.current.length}
            </span>
          </div>
          <div className="status-item">
            <span className="status-label">Max Frequency:</span>
            <span className="status-value">
              {maxFrequency.freq > 0 ? `${maxFrequency.freq.toFixed(1)} Hz` : 'N/A'}
            </span>
          </div>
          <div className="status-item">
            <span className="status-label">Current Time:</span>
            <span className="status-value">
              {isCapturing ? `${currentTime.toFixed(1)}s` : '0.0s'}
            </span>
          </div>
          <div className="status-item">
            <span className="status-label">Last Auto-Save:</span>
            <span className="status-value">
              {lastAutoSave ? lastAutoSave.toLocaleTimeString() : 'Never'}
            </span>
          </div>
        </div>
      </div>
    </div>
  );
}

export default SpectrogramPage;
