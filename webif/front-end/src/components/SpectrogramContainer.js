import React, { useState, useEffect, useRef } from 'react';
import axios from 'axios';

// Template for the spectrogram container
const SpectrogramContainerTemplate = () => {
  // State management
  const [isCapturing, setIsCapturing] = useState(false);
  const [spectrogramData, setSpectrogramData] = useState(null);
  const [settings, setSettings] = useState({
    // FFT Parameters
    frequency: 2400,
    sampleRate: 1000000, // 1 MHz default
    fftSize: 512, // Reduced from 1024 to make it lighter
    resolution: null, // Optional, overrides fftSize if provided
    windowSize: null, // Optional, defaults to fftSize
    hopSize: null, // Optional, defaults to windowSize/2
    windowType: 'hann',
    gain: 20,
    
    // Display Settings
    colormap: 'viridis',
    scrollDirection: 'up',

    pauseScrolling: false,
    displayMode: 'linear', // 'linear' or 'mel'

  });
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState(null);
  const [maxFrequency, setMaxFrequency] = useState({ freq: 0, amplitude: 0 });
  const [currentTime, setCurrentTime] = useState(0);
  const [streamConnected, setStreamConnected] = useState(false); // SSE connection status
  
  const [frequencyInput, setFrequencyInput] = useState('2.4G');
  const [sampleRateInput, setSampleRateInput] = useState('1M');
  const [resolutionInput, setResolutionInput] = useState('');
  const [windowSizeInput, setWindowSizeInput] = useState('');
  const [hopSizeInput, setHopSizeInput] = useState('');

  const [isInitialLoad, setIsInitialLoad] = useState(true);
  const [usrpDevices, setUsrpDevices] = useState([]);
  const [selectedUsrpDevice, setSelectedUsrpDevice] = useState('');
  const [lastSavedTime, setLastSavedTime] = useState(null);
  
  const canvasRef = useRef(null);
  const animationRef = useRef(null);
  const spectrogramHistory = useRef([]);
  const maxHistoryLength = 100; // Reduced from 200 to make it lighter
  const startTimeRef = useRef(null);
  const eventSourceRef = useRef(null); // SSE connection reference

  // State for custom tooltip
  const [tooltipContent, setTooltipContent] = useState('');
  const [tooltipPosition, setTooltipPosition] = useState({ x: 0, y: 0 });
  const [showTooltip, setShowTooltip] = useState(false);
  const tooltipRef = useRef(null);

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

  // Convert frequency to color using colormap - optimized
  const frequencyToColor = (freq, amplitude) => {
    const colormap = colormaps[settings.colormap] || colormaps.viridis;
    const maxFreq = settings.sampleRate / 2; // Nyquist frequency
    const normalizedFreq = Math.min(Math.max(freq / maxFreq, 0), 1);
    const normalizedAmp = Math.min(Math.max(amplitude / 100, 0), 1);
    
    const colorIndex = Math.floor(normalizedFreq * (colormap.length - 1));
    const color = colormap[colorIndex];
    
    // Apply amplitude scaling with better contrast
    const alpha = Math.max(0.1, normalizedAmp);
    
    // Use template literal for better performance
    return `rgba(${color[0]},${color[1]},${color[2]},${alpha})`;
  };

  // Get colormap gradient for CSS
  const getColormapGradient = (colormapName) => {
    const colormap = colormaps[colormapName] || colormaps.viridis;
    return colormap.map(color => `rgb(${color[0]}, ${color[1]}, ${color[2]})`).join(', ');
  };

  // Convert frequency string to Hz
  const convertFrequencyToHz = (frequencyStr) => {
    if (!frequencyStr || frequencyStr === '') return 0;
    
    const str = frequencyStr.toString().trim().toUpperCase();
    
    // Match pattern: number followed by optional unit (G, M, K)
    const match = str.match(/^([0-9]+\.?[0-9]*|[0-9]*\.[0-9]+)\s*([GMK]?)$/);
    
    if (!match) {
      console.warn('Invalid frequency format:', frequencyStr);
      return 0;
    }
    
    const number = parseFloat(match[1]);
    const unit = match[2];
    
    switch (unit) {
      case 'G':
        return number * 1000000000; // GHz
      case 'M':
        return number * 1000000; // MHz
      case 'K':
        return number * 1000; // kHz
      default:
        return number; // Hz
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
    if (frequencyHz > 0) {
      updateSettings(key, frequencyHz);
    }
  };

  // Render help icon with tooltip
  const renderHelpIcon = (content) => (
    <span
      className="help-icon"
      onMouseEnter={(e) => {
        const rect = e.target.getBoundingClientRect();
        const containerRect = e.target.closest('.spectrogram-container').getBoundingClientRect();
        
        // Estimate tooltip dimensions
        const estimatedWidth = Math.min(500, Math.max(200, content.length * 7));
        const estimatedHeight = Math.max(80, (content.split('\n').length + 1) * 20);
        
        // Calculate initial position (centered on help icon, below it)
        let x = rect.left + rect.width / 2;
        let y = rect.bottom + 10;
        
        // Adjust horizontal position to keep tooltip within spectrogram container
        if (x + estimatedWidth / 2 > containerRect.right - 20) {
          x = containerRect.right - estimatedWidth / 2 - 20;
        } else if (x - estimatedWidth / 2 < containerRect.left + 20) {
          x = containerRect.left + estimatedWidth / 2 + 20;
        }
        
        // Adjust vertical position if tooltip would go below container
        if (y + estimatedHeight > containerRect.bottom - 20) {
          y = rect.top - estimatedHeight - 10; // Show above the icon
        }
        
        // Convert to container-relative coordinates
        x = x - containerRect.left;
        y = y - containerRect.top;
        
        // Calculate arrow offset (where arrow should point relative to tooltip)
        const questionMarkCenter = rect.left + rect.width / 2 - containerRect.left;
        const arrowOffset = questionMarkCenter - x;
        
        // Ensure minimum values
        x = Math.max(0, x);
        y = Math.max(0, y);
        
        setTooltipContent(content);
        setTooltipPosition({ 
          x, 
          y, 
          showAbove: y < (rect.top - containerRect.top),
          arrowOffset: arrowOffset
        });
        setShowTooltip(true);
      }}
      onMouseLeave={() => setShowTooltip(false)}
      style={{
        cursor: 'help',
        marginLeft: '0.5rem'
      }}
    >
      ?
    </span>
  );

  // Configuration management functions
  const saveConfig = () => {
    const configName = prompt('Enter configuration name:');
    if (!configName) return;
    
    const configs = JSON.parse(localStorage.getItem('spectrogramConfigs') || '{}');
    configs[configName] = {
      settings,
      frequencyInput,
      sampleRateInput,
      resolutionInput,
      windowSizeInput,
      hopSizeInput,
      savedAt: new Date().toISOString()
    };
    
    localStorage.setItem('spectrogramConfigs', JSON.stringify(configs));
    alert(`Configuration "${configName}" saved successfully!`);
  };

  const loadConfig = () => {
    const configs = JSON.parse(localStorage.getItem('spectrogramConfigs') || '{}');
    const configNames = Object.keys(configs);
    
    if (configNames.length === 0) {
      alert('No saved configurations found.');
      return;
    }
    
    const configName = prompt(`Enter configuration name to load:\n\nAvailable: ${configNames.join(', ')}`);
    if (!configName || !configs[configName]) {
      alert('Configuration not found.');
      return;
    }
    
    const config = configs[configName];
    setSettings(config.settings);
    setFrequencyInput(config.frequencyInput || '');
    setSampleRateInput(config.sampleRateInput || '');
    setResolutionInput(config.resolutionInput || '');
    setWindowSizeInput(config.windowSizeInput || '');
    setHopSizeInput(config.hopSizeInput || '');
    
    alert(`Configuration "${configName}" loaded successfully!`);
  };

  const listConfigs = () => {
    const configs = JSON.parse(localStorage.getItem('spectrogramConfigs') || '{}');
    const configNames = Object.keys(configs);
    
    if (configNames.length === 0) {
      alert('No saved configurations found.');
      return;
    }
    
    const configList = configNames.map(name => {
      const config = configs[name];
      const savedAt = new Date(config.savedAt).toLocaleString();
      return `${name} (saved: ${savedAt})`;
    }).join('\n');
    
    alert(`Saved configurations:\n\n${configList}`);
  };

  const deleteConfig = () => {
    const configs = JSON.parse(localStorage.getItem('spectrogramConfigs') || '{}');
    const configNames = Object.keys(configs);
    
    if (configNames.length === 0) {
      alert('No saved configurations found.');
      return;
    }
    
    const configName = prompt(`Enter configuration name to delete:\n\nAvailable: ${configNames.join(', ')}`);
    if (!configName || !configs[configName]) {
      alert('Configuration not found.');
      return;
    }
    
    const confirmDelete = window.confirm(`Are you sure you want to delete configuration "${configName}"?`);
    if (!confirmDelete) return;
    
    delete configs[configName];
    localStorage.setItem('spectrogramConfigs', JSON.stringify(configs));
    alert(`Configuration "${configName}" deleted successfully!`);
  };

  const autoSaveConfig = () => {
    const configs = JSON.parse(localStorage.getItem('spectrogramConfigs') || '{}');
    const currentTime = new Date().toISOString();
    configs['latest'] = {
      settings,
      frequencyInput,
      sampleRateInput,
      resolutionInput,
      windowSizeInput,
      hopSizeInput,
      savedAt: currentTime
    };
    
    localStorage.setItem('spectrogramConfigs', JSON.stringify(configs));
    setLastSavedTime(currentTime);
  };

  const loadLatestConfig = () => {
    const configs = JSON.parse(localStorage.getItem('spectrogramConfigs') || '{}');
    const latestConfig = configs['latest'];
    
    if (!latestConfig) {
      alert('No latest configuration found.');
      return;
    }
    
    setSettings(latestConfig.settings);
    setFrequencyInput(latestConfig.frequencyInput || '');
    setSampleRateInput(latestConfig.sampleRateInput || '');
    setResolutionInput(latestConfig.resolutionInput || '');
    setWindowSizeInput(latestConfig.windowSizeInput || '');
    setHopSizeInput(latestConfig.hopSizeInput || '');
    
    alert('Latest configuration loaded successfully!');
  };

  // USRP device management
  const fetchUsrpDevices = async () => {
    console.log('📡 Fetching USRP devices...');
    try {
      const response = await axios.get('/api/usrp/devices');
      console.log('📡 USRP devices response:', response.data);
      const devices = response.data.devices || [];
      setUsrpDevices(devices);
      console.log('📡 Set USRP devices:', devices);
      
      // Auto-select first device if no device is currently selected
      if (devices.length > 0 && !selectedUsrpDevice) {
        const firstDevice = devices[0];
        setSelectedUsrpDevice(firstDevice.name);
        console.log('📡 Auto-selected first USRP device:', firstDevice.name);
      }
    } catch (error) {
      console.error('❌ Error fetching USRP devices:', error);
      setError('Failed to fetch USRP devices');
    }
  };

  // Spectrogram capture functions
  const startCapture = async () => {
    console.log('🚀 Start Capture button clicked');
    console.log('Selected USRP device:', selectedUsrpDevice);
    console.log('Available USRP devices:', usrpDevices);
    
    if (!selectedUsrpDevice) {
      console.log('❌ No USRP device selected');
      setError('Please select a USRP device first');
      return;
    }

    setLoading(true);
    setError(null);

    try {
      const selectedDevice = usrpDevices.find(device => device.name === selectedUsrpDevice);
      if (!selectedDevice) {
        throw new Error('Selected device not found');
      }

      // Convert frequency inputs
      const convertedFreq = convertFrequencyToHz(frequencyInput);
      const convertedSampleRate = convertFrequencyToHz(sampleRateInput);
      const convertedResolution = resolutionInput ? convertFrequencyToHz(resolutionInput) : null;

      // Calculate FFT size if resolution is provided
      let fftSize = settings.fftSize;
      if (convertedResolution && convertedSampleRate > 0) {
        fftSize = Math.pow(2, Math.ceil(Math.log2(convertedSampleRate / convertedResolution)));
      }

      const requestPayload = {
        deviceSerialNumber: selectedDevice.details['Serial Number'],
        centerFreq: convertedFreq,
        bandwidth: convertedSampleRate,
        resolution: convertedResolution,
        windowLen: windowSizeInput ? parseInt(windowSizeInput) : null,
        overlap: hopSizeInput ? parseInt(hopSizeInput) : null,
        fftSize: fftSize,
        windowType: settings.windowType,
        gain: settings.gain
      };

      const response = await axios.post('/api/spectrogram/start', requestPayload);
      
      if (response.data.success) {
        setIsCapturing(true);
        setStreamConnected(true);
        startTimeRef.current = Date.now();
        setCurrentTime(0);
        spectrogramHistory.current = [];
        setMaxFrequency({ freq: 0, amplitude: 0 });
        
        // Start WebSocket connection
        connectToWebSocket();
        
        // Start auto-save timer
        const autoSaveInterval = setInterval(autoSaveConfig, 30000); // Auto-save every 30 seconds (reduced frequency)
        
        // Store interval reference for cleanup
        if (window.autoSaveInterval) {
          clearInterval(window.autoSaveInterval);
        }
        window.autoSaveInterval = autoSaveInterval;
      } else {
        throw new Error(response.data.error || 'Failed to start capture');
      }
    } catch (error) {
      console.error('Error starting capture:', error);
      setError(error.message || 'Failed to start capture');
    } finally {
      setLoading(false);
    }
  };

  const stopCapture = async () => {
    try {
      const response = await axios.post('/api/spectrogram/stop');
      
      if (response.data.success) {
        setIsCapturing(false);
        setStreamConnected(false);
        disconnectFromStream();
        
        // Auto-save final configuration
        autoSaveConfig();
      } else {
        throw new Error(response.data.error || 'Failed to stop capture');
      }
    } catch (error) {
      console.error('Error stopping capture:', error);
      setError(error.message || 'Failed to stop capture');
    }
  };

  // WebSocket connection for real-time data
  const connectToWebSocket = () => {
    const protocol = 'http:';
    const wsUrl = `${protocol}//${window.location.hostname}:40000/api/spectrogram/stream`;
    
    console.log('🔌 Connecting to WebSocket:', wsUrl);
    
    const ws = new WebSocket(wsUrl);
    
    ws.onopen = () => {
      console.log('✅ WebSocket connected');
      setStreamConnected(true);
    };
    
    ws.onmessage = (event) => {
      try {
        const message = JSON.parse(event.data);
        
        if (message.type === 'spectrogram_data') {
          const data = message.data;
          
          // Add timestamp
          const elapsed = (Date.now() - startTimeRef.current) / 1000;
          data.timestamp = elapsed;
          
          // Add to history
          spectrogramHistory.current.push(data);
          
          // Keep only recent data
          if (spectrogramHistory.current.length > maxHistoryLength) {
            spectrogramHistory.current.shift();
          }
          
          // Update current time (throttled)
          if (elapsed - currentTime > 0.1) { // Update every 100ms
            setCurrentTime(elapsed);
          }
          
          // Find maximum frequency (throttled)
          if (data.data && data.data.length > 0 && elapsed % 0.5 < 0.1) { // Update every 500ms
            const maxIndex = data.data.indexOf(Math.max(...data.data));
            const maxFreq = maxIndex * (settings.sampleRate / 2) / (settings.fftSize / 2);
            const maxAmplitude = Math.max(...data.data);
            
            setMaxFrequency({ freq: maxFreq, amplitude: maxAmplitude });
          }
          
          // Trigger canvas redraw (throttled)
          if (!animationRef.current) {
            animationRef.current = requestAnimationFrame(() => {
              drawSpectrogram();
              animationRef.current = null;
            });
          }
        }
      } catch (error) {
        console.error('Error parsing WebSocket message:', error);
      }
    };
    
    ws.onerror = (error) => {
      console.error('❌ WebSocket error:', error);
      setStreamConnected(false);
    };
    
    ws.onclose = () => {
      console.log('🔌 WebSocket disconnected');
      setStreamConnected(false);
    };
    
    // Store WebSocket reference
    eventSourceRef.current = ws;
  };

  const disconnectFromStream = () => {
    if (eventSourceRef.current) {
      eventSourceRef.current.close();
      eventSourceRef.current = null;
    }
    
    if (animationRef.current) {
      cancelAnimationFrame(animationRef.current);
      animationRef.current = null;
    }
  };

  // Canvas drawing functions
  // Draw spectrogram on canvas - optimized for performance
  const drawSpectrogram = () => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    
    const ctx = canvas.getContext('2d');
    const width = canvas.width;
    const height = canvas.height;
    
    // Check if canvas has proper dimensions
    if (width <= 0 || height <= 0) return;
    
    // Clear canvas with lighter operation
    ctx.clearRect(0, 0, width, height);
    
    if (spectrogramHistory.current.length === 0) return;
    
    // Calculate spectrogram dimensions - use full canvas
    const spectrogramWidth = width;
    const spectrogramHeight = height;
    const startX = 0;
    const startY = 0;
    
    // Calculate frequency width per bin
    const freqWidth = spectrogramWidth / (settings.fftSize / 2);
    const timeHeight = spectrogramHeight / spectrogramHistory.current.length;
    
    // Optimize: Only draw every other frequency bin for better performance
    const freqStep = Math.max(1, Math.floor((settings.fftSize / 2) / 200));
    
    // Draw each time slice with reduced frequency resolution
    spectrogramHistory.current.forEach((data, timeIndex) => {
      if (!data.data || data.data.length === 0) return;
      
      // Calculate time position (scrolling effect)
      const timeY = settings.scrollDirection === 'up' 
        ? startY + (spectrogramHistory.current.length - 1 - timeIndex) * timeHeight
        : startY + timeIndex * timeHeight;
      
      // Draw frequency bins with reduced resolution
      for (let freqIndex = 0; freqIndex < data.data.length; freqIndex += freqStep) {
        const amplitude = data.data[freqIndex];
        const freqX = startX + freqIndex * freqWidth;
        const color = frequencyToColor(freqIndex * (settings.sampleRate / 2) / (settings.fftSize / 2), amplitude);
        
        ctx.fillStyle = color;
        ctx.fillRect(
          freqX, 
          timeY, 
          freqWidth * freqStep, 
          timeHeight
        );
      }
    });
  };

  const updateCanvasSize = () => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    
    const rect = canvas.getBoundingClientRect();
    canvas.width = rect.width * window.devicePixelRatio;
    canvas.height = rect.height * window.devicePixelRatio;
    
    const ctx = canvas.getContext('2d');
    ctx.scale(window.devicePixelRatio, window.devicePixelRatio);
    
    // Clear the canvas after resize
    ctx.clearRect(0, 0, canvas.width, canvas.height);
  };

  // useEffect hooks
  useEffect(() => {
    // Load latest configuration on mount
    const configs = JSON.parse(localStorage.getItem('spectrogramConfigs') || '{}');
    const latestConfig = configs['latest'];
    
    if (latestConfig) {
      setSettings(latestConfig.settings);
      setFrequencyInput(latestConfig.frequencyInput || '');
      setSampleRateInput(latestConfig.sampleRateInput || '');
      setResolutionInput(latestConfig.resolutionInput || '');
      setWindowSizeInput(latestConfig.windowSizeInput || '');
      setHopSizeInput(latestConfig.hopSizeInput || '');
      
      // Set the last saved time from the loaded configuration
      if (latestConfig.savedAt) {
        setLastSavedTime(latestConfig.savedAt);
      }
    }
    
    // Fetch USRP devices on mount
    fetchUsrpDevices();
    
    // Auto-select first device after a short delay to ensure devices are loaded
    const autoSelectTimeout = setTimeout(() => {
      if (usrpDevices.length > 0 && !selectedUsrpDevice) {
        const firstDevice = usrpDevices[0];
        setSelectedUsrpDevice(firstDevice.name);
        console.log('📡 Auto-selected first USRP device on mount:', firstDevice.name);
      }
    }, 1000);
    
    return () => {
      clearTimeout(autoSelectTimeout);
      disconnectFromStream();
    };
    
    // Set up canvas
    updateCanvasSize();
  }, []);

  useEffect(() => {
    // Update canvas size on window resize (throttled)
    let resizeTimeout;
    const handleResize = () => {
      clearTimeout(resizeTimeout);
      resizeTimeout = setTimeout(() => {
        updateCanvasSize();
        if (spectrogramHistory.current.length > 0) {
          drawSpectrogram();
        }
      }, 250); // Throttle to 250ms
    };
    
    window.addEventListener('resize', handleResize);
    return () => {
      window.removeEventListener('resize', handleResize);
      clearTimeout(resizeTimeout);
    };
  }, [settings]);

  useEffect(() => {
    // Auto-save when settings change (debounced)
    if (!isInitialLoad) {
      const timeoutId = setTimeout(() => {
        autoSaveConfig();
      }, 1000); // Save 1 second after last change
      
      return () => clearTimeout(timeoutId);
    } else {
      setIsInitialLoad(false);
    }
  }, [settings, frequencyInput, sampleRateInput, resolutionInput, windowSizeInput, hopSizeInput]);

  // Auto-select first USRP device when devices are loaded
  useEffect(() => {
    if (usrpDevices.length > 0 && !selectedUsrpDevice) {
      const firstDevice = usrpDevices[0];
      setSelectedUsrpDevice(firstDevice.name);
      console.log('📡 Auto-selected first USRP device:', firstDevice.name);
    }
  }, [usrpDevices, selectedUsrpDevice]);

  // Template for control buttons (not used - buttons are rendered directly in JSX)

  // Template for colormap options
  const colormapOptionsTemplate = [
    { value: 'viridis', label: 'Viridis' },
    { value: 'plasma', label: 'Plasma' },
    { value: 'inferno', label: 'Inferno' },
    { value: 'magma', label: 'Magma' },
    { value: 'cool', label: 'Cool' },
    { value: 'hot', label: 'Hot' }
  ];

  // Template for FFT size options (lighter options)
  const fftSizeOptionsTemplate = [256, 512, 1024, 2048];

  // Template for window type options
  const windowTypeOptionsTemplate = [
    { value: 'hann', label: 'Hann' },
    { value: 'hamming', label: 'Hamming' },
    { value: 'blackman', label: 'Blackman' },
    { value: 'rectangular', label: 'Rectangular' }
  ];



  // Template for frequency labels
  const frequencyLabelsTemplate = Array.from({length: 9}, (_, i) => {
    const freq = (settings.sampleRate * i) / 16;
    return {
      key: i,
      label: freq >= 1000 ? `${(freq / 1000).toFixed(1)}k` : freq.toFixed(0)
    };
  });

  // Template for time labels
  const timeLabelsTemplate = Array.from({length: 6}, (_, i) => {
    const time = (200 * 0.05 * (5 - i)) / 5;
    return {
      key: i,
      label: `${time.toFixed(1)}s`
    };
  });

  // Template for USRP device options
  const usrpDeviceOptionsTemplate = usrpDevices.map((device, index) => {
    const deviceType = device.details && device.details['Type'] ? device.details['Type'] : 'Unknown';
    const serialNumber = device.details && device.details['Serial Number'] ? device.details['Serial Number'] : 'Unknown';
    const displayName = `${deviceType} (${serialNumber})`;
    
    return {
      key: index,
      value: device.name,
      label: displayName
    };
  });

  // Template for status items (optimized for performance)
  const statusItemsTemplate = [
    { label: 'Capture:', value: isCapturing ? 'Running' : 'Stopped', active: isCapturing },
    { label: 'Connection:', value: streamConnected ? 'Connected' : 'Disconnected', active: streamConnected },
    { label: 'Data Points:', value: spectrogramHistory.current.length, active: null },
    { label: 'Max Frequency:', value: maxFrequency.freq > 0 ? `${Math.round(maxFrequency.freq)} Hz` : 'N/A', active: null },
    { label: 'Last Saved:', value: lastSavedTime ? new Date(lastSavedTime).toLocaleTimeString() : 'Never', active: null }
  ];

  // Top to Spectrogram scroll effect
  useEffect(() => {
    let scrollTimeout;
    
    const handleWheel = (e) => {
      // Clear any existing timeout
      clearTimeout(scrollTimeout);
      
      // Set a timeout to trigger alignment after scrolling stops
      scrollTimeout = setTimeout(() => {
        const spectrogramContainer = document.querySelector('.spectrogram-container');
        if (!spectrogramContainer) return;
        
        const scrollY = window.pageYOffset;
        
        // Only when at top and scrolling down, go to spectrogram
        if (scrollY < 500 && e.deltaY > 0) {
          spectrogramContainer.scrollIntoView({ behavior: 'smooth', block: 'start' });
        }
      }, 100); // Wait 100ms after scrolling stops
    };

    // Add event listener to the document
    document.addEventListener('wheel', handleWheel, { passive: true });

    // Cleanup
    return () => {
      document.removeEventListener('wheel', handleWheel);
      clearTimeout(scrollTimeout);
    };
  }, []);

  return (
    <div className="spectrogram-container" style={{ 
      minHeight: '40vh',
      overflowY: 'auto', 
      padding: '20px',
      border: '2px solid rgba(139, 92, 246, 0.4)',
      borderRadius: '16px',
      background: 'linear-gradient(135deg, rgba(255, 255, 255, 0.95) 0%, rgba(248, 250, 252, 0.95) 100%)',
      backdropFilter: 'blur(15px)',
      boxShadow: '0 12px 40px rgba(139, 92, 246, 0.15), inset 0 1px 0 rgba(255, 255, 255, 0.8)'
    }}>
      {/* Page Header */}
      <div className="page-header">
        <h1>Spectrogram Analysis</h1>
        <p>Real-time frequency domain analysis and visualization</p>
      </div>

      {/* Control Panel */}
      <div className="control-panel">
        <div className="control-section">
          <h3>Controls and Configuration</h3>
          
          <div className="control-buttons">
            <button 
              className={`control-btn ${isCapturing ? 'stop' : 'start'}`}
              onClick={() => {
                console.log('🔘 Button clicked, isCapturing:', isCapturing);
                if (isCapturing) {
                  stopCapture();
                } else {
                  startCapture();
                }
              }}
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
              <label>
                Colormap
                {renderHelpIcon("Color scheme used to display signal amplitude.\n\nViridis:\n• Default choice\n• Good for colorblind users\n• Perceptually uniform\n• Blue to yellow progression\n\nPlasma:\n• Purple to yellow\n• High contrast\n• Good for detailed analysis\n\nInferno:\n• Black to yellow\n• High contrast\n• Good for weak signals\n\nMagma:\n• Black to pink\n• Smooth transitions\n• Good for general use\n\nCool:\n• Cyan to magenta\n• Good for temperature-like data\n\nHot:\n• Black to white\n• Classic heat map\n• High contrast")}
              </label>
              <select
                value={settings.colormap}
                onChange={(e) => updateSettings('colormap', e.target.value)}
              >
                {colormapOptionsTemplate.map(option => (
                  <option key={option.value} value={option.value}>
                    {option.label}
                  </option>
                ))}
              </select>
            </div>
            
            <div className="setting-item">
              <label>
                Display Mode
                {renderHelpIcon("Frequency scale for display. Linear shows actual frequencies, Mel Scale uses a perceptual frequency scale based on human hearing.")}
              </label>
              <select
                value={settings.displayMode}
                onChange={(e) => updateSettings('displayMode', e.target.value)}
              >
                <option value="linear">Linear</option>
                <option value="mel">Mel Scale</option>
              </select>
            </div>
            
            <div className="setting-item">
              <label>
                Scroll Direction
                {renderHelpIcon("Direction of time progression in the spectrogram display. Up shows newest data at the top, Down shows newest data at the bottom.")}
              </label>
              <select
                value={settings.scrollDirection}
                onChange={(e) => updateSettings('scrollDirection', e.target.value)}
              >
                <option value="up">Up</option>
                <option value="down">Down</option>
              </select>
            </div>
            

            

          </div>
        </div>

        <div className="control-section">
          <h3>Analysis Settings</h3>
          <div className="settings-grid">
            {/* USRP Device Selection */}
            <div className="setting-item">
              <label>
                Select USRP Device
                {renderHelpIcon("Choose the USRP device to use for signal capture. The device must be connected and detected by the system. Available devices are automatically detected and listed in the dropdown.")}
              </label>
              <div className="device-selection-container">
                <select
                  value={selectedUsrpDevice}
                  onChange={(e) => setSelectedUsrpDevice(e.target.value)}
                  disabled={usrpDevices.length === 0}
                >
                  <option value="">{usrpDevices.length === 0 ? 'No USRP devices found' : 'Select a device...'}</option>
                  {usrpDeviceOptionsTemplate.map(device => (
                    <option key={device.key} value={device.value}>
                      {device.label}
                    </option>
                  ))}
                </select>
                <button 
                  className="refresh-devices-btn"
                  onClick={fetchUsrpDevices}
                  title="Refresh USRP devices"
                >
                  🔄
                </button>
              </div>
            </div>
            
            <div className="setting-item">
              <label>
                Center Frequency
                {renderHelpIcon("The center frequency of the signal to analyze. This is the frequency around which the USRP will be tuned.\n\nSupports units:\n• G (GHz) - e.g., 2.4G = 2.4 GHz\n• M (MHz) - e.g., 100M = 100 MHz\n• K (kHz) - e.g., 20K = 20 kHz\n\nCommon frequencies:\n• WiFi: 2.4G, 5G\n• Cellular: 900M, 1800M, 2100M\n• GPS: 1.575G")}
              </label>
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
              </div>
              <div className="frequency-conversion">
                {settings.frequency.toLocaleString()} Hz
              </div>
            </div>
            
            <div className="setting-item">
              <label>
                Sample Rate (Bandwidth)
                {renderHelpIcon("The sampling rate (bandwidth) of the signal capture. This determines how much of the frequency spectrum you can observe.\n\nHigher rates provide:\n• Wider frequency coverage\n• Better time resolution\n• More data to process\n\nSupports units:\n• G (GHz) - e.g., 1G = 1 GHz\n• M (MHz) - e.g., 10M = 10 MHz\n• K (kHz) - e.g., 44.1K = 44.1 kHz\n\nTypical values:\n• Audio: 44.1K, 48K\n• RF: 1M, 10M, 30.72M")}
              </label>
              <div className="frequency-input-container">
                <input
                  type="text"
                  placeholder="e.g., 1M, 10M, 30.72M"
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
              </div>
              <div className="frequency-conversion">
                {settings.sampleRate.toLocaleString()} Hz
              </div>
            </div>
            
            <div className="setting-item">
              <label>
                Resolution (Optional)
                {renderHelpIcon("Desired frequency resolution. This determines the frequency precision of the analysis.\n\nHigher resolution provides:\n• Better frequency precision\n• Ability to distinguish close frequencies\n• Slower update rates\n• Larger FFT size required\n\nLower resolution provides:\n• Faster updates\n• Less computational load\n• Coarser frequency detail\n• Smaller FFT size\n\nSupports units:\n• G (GHz) - e.g., 1G = 1 GHz\n• M (MHz) - e.g., 1M = 1 MHz\n• K (kHz) - e.g., 1K = 1 kHz\n\nFormula: FFT Size = Sample Rate / Resolution\n\nTypical values:\n• High precision: 1K, 10K\n• Medium precision: 100K, 1M\n• Low precision: 10M, 100M")}
              </label>
              <div className="frequency-input-container">
                <input
                  type="text"
                  placeholder="e.g., 1K, 10K, 100K"
                  value={resolutionInput}
                  onChange={(e) => setResolutionInput(e.target.value)}
                  onBlur={() => convertAndUpdateFrequency('resolution', resolutionInput)}
                  onKeyPress={(e) => {
                    if (e.key === 'Enter') {
                      convertAndUpdateFrequency('resolution', resolutionInput);
                      e.target.blur();
                    }
                  }}
                  className="frequency-input"
                />
              </div>
              <div className="frequency-conversion">
                {settings.resolution ? `${settings.resolution.toLocaleString()} Hz` : 'Auto (calculated from FFT size)'}
              </div>
            </div>
            
            <div className="setting-item">
              <label>
                FFT Size
                {renderHelpIcon("Number of frequency bins in the FFT (Fast Fourier Transform).\n\nLarger FFT sizes provide:\n• Better frequency resolution\n• More precise frequency measurements\n• More computational time required\n\nSmaller FFT sizes provide:\n• Faster processing\n• Less memory usage\n• Coarser frequency resolution\n\nMust be a power of 2:\n• 512, 1024, 2048, 4096, 8192\n\nRelationship:\n• Frequency Resolution = Sample Rate / FFT Size\n• Time Resolution = FFT Size / Sample Rate")}
              </label>
              <select
                value={settings.fftSize}
                onChange={(e) => updateSettings('fftSize', parseInt(e.target.value))}
              >
                {fftSizeOptionsTemplate.map(size => (
                  <option key={size} value={size}>{size}</option>
                ))}
              </select>
            </div>
            
            <div className="setting-item">
              <label>
                Window Size (Optional)
                {renderHelpIcon("Number of samples per STFT (Short-Time Fourier Transform) frame.\n\nIf not specified, uses the FFT size.\n\nSmaller windows provide:\n• Better time resolution\n• Faster response to signal changes\n• Less frequency resolution\n\nLarger windows provide:\n• Better frequency resolution\n• Slower response to changes\n• More stable frequency measurements\n\nTypical values:\n• 64 to 8192 samples\n• Must be ≤ FFT Size\n• Usually power of 2 for efficiency")}
              </label>
              <input
                type="number"
                placeholder="Auto (uses FFT size)"
                value={windowSizeInput}
                onChange={(e) => setWindowSizeInput(e.target.value)}
                onBlur={() => {
                  const value = parseInt(windowSizeInput);
                  updateSettings('windowSize', value || null);
                  setWindowSizeInput(value || '');
                }}
                min="64"
                max="8192"
                step="64"
              />
            </div>
            
            <div className="setting-item">
              <label>
                Hop Size (Optional)
                {renderHelpIcon("Number of samples to advance between consecutive STFT frames.\n\nIf not specified, defaults to window_size/2.\n\nSmaller hop sizes provide:\n• Smoother time resolution\n• More overlapping frames\n• Higher computational overhead\n• Better time-domain detail\n\nLarger hop sizes provide:\n• Faster processing\n• Less overlap between frames\n• Lower computational cost\n• Coarser time resolution\n\nOverlap = Window Size - Hop Size\nTypical overlap: 50% (hop = window/2)")}
              </label>
              <input
                type="number"
                placeholder="Auto (window_size/2)"
                value={hopSizeInput}
                onChange={(e) => setHopSizeInput(e.target.value)}
                onBlur={() => {
                  const value = parseInt(hopSizeInput);
                  updateSettings('hopSize', value || null);
                  setHopSizeInput(value || '');
                }}
                min="1"
                max="4096"
                step="1"
              />
            </div>
            
            <div className="setting-item">
              <label>
                Window Type
                {renderHelpIcon("Type of window function applied to the signal before FFT.\n\nHann (Hanning):\n• Most commonly used\n• Good balance of main lobe width and sidelobe suppression\n• Recommended for general use\n\nHamming:\n• Better sidelobe suppression than Hann\n• Slightly wider main lobe\n• Good for frequency analysis\n\nBlackman:\n• Best sidelobe suppression\n• Widest main lobe\n• Best for detecting weak signals\n\nRectangular:\n• No windowing applied\n• Sharpest main lobe\n• Poor sidelobe suppression\n• Can cause spectral leakage")}
              </label>
              <select
                value={settings.windowType}
                onChange={(e) => updateSettings('windowType', e.target.value)}
              >
                {windowTypeOptionsTemplate.map(option => (
                  <option key={option.value} value={option.value}>
                    {option.label}
                  </option>
                ))}
              </select>
            </div>
            
            <div className="setting-item">
              <label>
                Gain (dB)
                {renderHelpIcon("Receiver gain in decibels (dB).\n\nHigher gain:\n• Amplifies weak signals\n• Improves signal detection\n• May cause saturation with strong signals\n• Increases noise floor\n\nLower gain:\n• Reduces signal strength\n• Prevents saturation\n• Lower noise floor\n• May miss weak signals\n\nTypical ranges:\n• USRP B200: 0-60 dB\n• USRP X300/X310: 0-60 dB\n• USRP N200/N210: 0-60 dB\n\nStart with 20-30 dB and adjust based on signal strength.")}
              </label>
              <input
                type="number"
                value={settings.gain}
                onChange={(e) => updateSettings('gain', parseInt(e.target.value))}
                min="0"
                max="60"
                step="1"
              />
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
            {frequencyLabelsTemplate.map(label => (
              <span key={label.key} className="freq-label">
                {label.label}
              </span>
            ))}
          </div>
          
          <div className="spectrogram-main">
            {/* Time labels (left) */}
            <div className="time-labels-left">
              {timeLabelsTemplate.map(label => (
                <span key={label.key} className="time-label">
                  {label.label}
                </span>
              ))}
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
                  background: '#000000',
                  display: 'block'
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
            {frequencyLabelsTemplate.map(label => (
              <span key={label.key} className="freq-label">
                {label.label}
              </span>
            ))}
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
        {statusItemsTemplate.map((item, index) => (
          <div key={index} className="status-item">
            <span className="status-label">{item.label}</span>
            <span className={`status-value ${item.active !== null ? (item.active ? 'active' : 'inactive') : ''}`}>
              {item.value}
            </span>
          </div>
        ))}
      </div>

      {/* Custom Tooltip */}
      {showTooltip && (
        <div
          className={`custom-tooltip ${tooltipPosition.showAbove ? 'above' : ''}`}
          style={{
            left: tooltipPosition.x,
            top: tooltipPosition.y,
            '--arrow-offset': `${tooltipPosition.arrowOffset}px`
          }}
        >
          {tooltipContent}
        </div>
      )}
    </div>
  );
};

export default SpectrogramContainerTemplate;
