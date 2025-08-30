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
          colorRangeMin: -80,   // dB minimum - typical RF range
    colorRangeMax: -20,   // dB maximum - typical RF range

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
  const [colorRangeMinInput, setColorRangeMinInput] = useState('');
  const [colorRangeMaxInput, setColorRangeMaxInput] = useState('');

  const [isInitialLoad, setIsInitialLoad] = useState(true);
  const [hasLoadedSettings, setHasLoadedSettings] = useState(false);
  const [isLoadingFromStorage, setIsLoadingFromStorage] = useState(false);
  const [autoUpdatesEnabled, setAutoUpdatesEnabled] = useState(false);
  const [loadedSettings, setLoadedSettings] = useState(null);
  const [usrpDevices, setUsrpDevices] = useState([]);
  const [selectedUsrpDevice, setSelectedUsrpDevice] = useState('');
  const [lastSavedTime, setLastSavedTime] = useState(null);
  
  const canvasRef = useRef(null);
  const animationRef = useRef(null);
  const spectrogramHistory = useRef([]);
  const maxHistoryLengthRef = useRef(1000); // Will be updated to canvas height when canvas is available
  const startTimeRef = useRef(null);
  const eventSourceRef = useRef(null); // SSE connection reference
  
  // Refs for input boxes
  const windowSizeInputRef = useRef(null);
  const hopSizeInputRef = useRef(null);
  const frequencyInputRef = useRef(null);
  const sampleRateInputRef = useRef(null);
  const resolutionInputRef = useRef(null);
  const fftSizeInputRef = useRef(null);
  
  // Refs for conversion display boxes (for status transitions)
  const frequencyConversionRef = useRef(null);
  const sampleRateConversionRef = useRef(null);
  const resolutionConversionRef = useRef(null);
  const fftSizeConversionRef = useRef(null);
  const windowSizeConversionRef = useRef(null);
  const hopSizeConversionRef = useRef(null);
  const colormapConversionRef = useRef(null);
  const colorRangeMinConversionRef = useRef(null);
  const colorRangeMaxConversionRef = useRef(null);
  const displayModeConversionRef = useRef(null);
  const scrollDirectionConversionRef = useRef(null);
  const usrpDeviceConversionRef = useRef(null);
  const windowTypeConversionRef = useRef(null);
  const gainConversionRef = useRef(null);

  // State for custom tooltip
  const [tooltipContent, setTooltipContent] = useState('');
  const [tooltipPosition, setTooltipPosition] = useState({ x: 0, y: 0 });
  const [showTooltip, setShowTooltip] = useState(false);
  const tooltipRef = useRef(null);
  
  // State for status box transitions
  const [statusBoxStates, setStatusBoxStates] = useState({
    frequency: { isTransitioning: false, oldValue: null, newValue: null, transitionText: '', phase: 'normal' },
    sampleRate: { isTransitioning: false, oldValue: null, newValue: null, transitionText: '', phase: 'normal' },
    resolution: { isTransitioning: false, oldValue: null, newValue: null, transitionText: '', phase: 'normal' },
    fftSize: { isTransitioning: false, oldValue: null, newValue: null, transitionText: '', phase: 'normal' },
    windowSize: { isTransitioning: false, oldValue: null, newValue: null, transitionText: '', phase: 'normal' },
          hopSize: { isTransitioning: false, oldValue: null, newValue: null, transitionText: '', phase: 'normal' },
      colormap: { isTransitioning: false, oldValue: null, newValue: null, transitionText: '', phase: 'normal' },
      colorRangeMin: { isTransitioning: false, oldValue: null, newValue: null, transitionText: '', phase: 'normal' },
      colorRangeMax: { isTransitioning: false, oldValue: null, newValue: null, transitionText: '', phase: 'normal' },
      displayMode: { isTransitioning: false, oldValue: null, newValue: null, transitionText: '', phase: 'normal' },
    scrollDirection: { isTransitioning: false, oldValue: null, newValue: null, transitionText: '', phase: 'normal' },
    usrpDevice: { isTransitioning: false, oldValue: null, newValue: null, transitionText: '', phase: 'normal' },
    windowType: { isTransitioning: false, oldValue: null, newValue: null, transitionText: '', phase: 'normal' },
    gain: { isTransitioning: false, oldValue: null, newValue: null, transitionText: '', phase: 'normal' }
  });

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

  // Convert amplitude to color using colormap - optimized
  const amplitudeToColor = (amplitude) => {
    const colormap = colormaps[settings.colormap] || colormaps.viridis;

    // Use color range to distribute colors across the colormap
    const rangeMin = settings.colorRangeMin;
    const rangeMax = settings.colorRangeMax;
    

    const clampedAmp = Math.min(Math.max(amplitude, rangeMin), rangeMax);
    
    // Use amplitude only for color mapping
    const colorIndex = Math.floor((clampedAmp - rangeMin) / (rangeMax - rangeMin) * (colormap.length - 1));
    const color = colormap[colorIndex];

    // Use template literal for better performance
    return `rgba(${color[0]},${color[1]},${color[2]},1)`;
  };

  // Auto-adjust color range based on current data
  const autoAdjustColorRange = () => {
    if (spectrogramHistory.current.length === 0) {
      console.log('❌ No data available for auto-adjustment');
      return;
    }

    const latestData = spectrogramHistory.current[spectrogramHistory.current.length - 1];
    if (!latestData || !latestData.data || latestData.data.length === 0) {
      console.log('❌ No valid data for auto-adjustment');
      return;
    }

    const amplitudes = latestData.data;
    const minAmp = Math.min(...amplitudes);
    const maxAmp = Math.max(...amplitudes);
    
    // Add some padding to the range
    const newMin = Math.floor(minAmp - 2);
    const newMax = Math.ceil(maxAmp + 2);
    
    console.log(`🎨 Auto-adjusting color range: ${newMin} to ${newMax} dB`);
    
    // Update settings
    updateSettings('colorRangeMin', newMin);
    updateSettings('colorRangeMax', newMax);
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
    console.log(`🔧 updateSettings called: ${key} = ${value}, isInitialLoad: ${isInitialLoad}`);
    
    // Get the old value before updating
    const oldValue = settings[key];
    
    setSettings(prev => ({
      ...prev,
      [key]: value
    }));
    
    // Show notification for any setting change (not just auto-updates)
    if (!isInitialLoad && oldValue !== value) {
      // Show status box transition for any setting change
      if (!isInitialLoad) {
        showStatusBoxTransition(key, oldValue, value);
      }
    }
  };


  
  // Show status box transition
  const showStatusBoxTransition = (settingKey, oldValue, newValue) => {
    const oldFormatted = oldValue?.toLocaleString() || 'N/A';
    const newFormatted = newValue?.toLocaleString() || 'N/A';
    const transitionText = `${oldFormatted} → ${newFormatted}`;
    
    // Start transition
    setStatusBoxStates(prev => ({
      ...prev,
      [settingKey]: {
        isTransitioning: true,
        oldValue,
        newValue,
        transitionText,
        phase: 'transition'
      }
    }));
    
    // After 2 seconds, return to normal
    setTimeout(() => {
      setStatusBoxStates(prev => ({
        ...prev,
        [settingKey]: {
          isTransitioning: false,
          oldValue: null,
          newValue: null,
          transitionText: '',
          phase: 'normal'
        }
      }));
    }, 2000);
  };

  // Convert and update frequency settings
  const convertAndUpdateFrequency = (key, value) => {
    console.log(`🔍 convertAndUpdateFrequency called with key: ${key}, value: ${value}, isInitialLoad: ${isInitialLoad}`);
    const frequencyHz = convertFrequencyToHz(value);
    if (frequencyHz > 0) {
      updateSettings(key, frequencyHz);
      
      // Only auto-update FFT size if not during initial load
      if (!isInitialLoad) {
        // If resolution is being updated, automatically update FFT size
        if (key === 'resolution' && frequencyHz > 0) {
          updateFFTSizeFromResolution(frequencyHz);
        }
        
        // If sample rate is being updated and resolution exists, update FFT size
        if (key === 'sampleRate' && settings.resolution > 0) {
          updateFFTSizeFromResolution(settings.resolution);
        }
      }
    }
  };

  // Update FFT size automatically based on resolution
  const updateFFTSizeFromResolution = (resolutionHz) => {
    console.log(`🔍 updateFFTSizeFromResolution called with resolutionHz: ${resolutionHz}, isInitialLoad: ${isInitialLoad}`);
    
    // Calculate FFT size based on resolution and current sample rate
    // Formula: FFT Size = Sample Rate / Resolution
    if (settings.sampleRate > 0) {
      const calculatedFFTSize = Math.pow(2, Math.ceil(Math.log2(settings.sampleRate / resolutionHz)));
      
      // Check if calculated FFT size is out of range
      const isOutOfRange = calculatedFFTSize < 16 || calculatedFFTSize > 65536;
      
      // Limit FFT size to reasonable range (16 to 65536)
      const limitedFFTSize = Math.max(16, Math.min(65536, calculatedFFTSize));
      
      // Update FFT size setting
      updateSettings('fftSize', limitedFFTSize);
      
      // Auto-update Window Size to match FFT size
      updateSettings('windowSize', limitedFFTSize);
      setWindowSizeInput(''); // Clear the input to show the auto-updated value
      
      // Auto-update Hop Size to half of Window Size
      const newHopSize = Math.floor(limitedFFTSize / 2);
      updateSettings('hopSize', newHopSize);
      setHopSizeInput(''); // Clear the input to show the auto-updated value
      
      // Store out-of-range status for display
      setSettings(prev => ({
        ...prev,
        fftSizeOutOfRange: isOutOfRange,
        originalCalculatedFFTSize: calculatedFFTSize
      }));
      
      console.log(`📊 Auto-updated FFT size to ${limitedFFTSize}, Window Size to ${limitedFFTSize}, and Hop Size to ${newHopSize} based on resolution ${resolutionHz.toLocaleString()} Hz and sample rate ${settings.sampleRate.toLocaleString()} Hz${isOutOfRange ? ' (OUT OF RANGE - original: ' + calculatedFFTSize + ')' : ''}`);
    }
  };

  // Optimized tooltip positioning helper
  const calculateTooltipPosition = (event, content) => {
    const rect = event.target.getBoundingClientRect();
    const containerRect = event.target.closest('.spectrogram-container').getBoundingClientRect();
    
    // Estimate tooltip dimensions
    const estimatedWidth = Math.min(500, Math.max(200, content.length * 7));
    const estimatedHeight = Math.max(80, (content.split('\n').length + 1) * 20);
    
    // Calculate initial position (centered on element, below it)
    let x = rect.left + rect.width / 2;
    let y = rect.bottom + 10;
    
    // Adjust horizontal position to keep tooltip within container
    if (x + estimatedWidth / 2 > containerRect.right - 20) {
      x = containerRect.right - estimatedWidth / 2 - 20;
    } else if (x - estimatedWidth / 2 < containerRect.left + 20) {
      x = containerRect.left + estimatedWidth / 2 + 20;
    }
    
    // Adjust vertical position if tooltip would go below container
    const showAbove = y + estimatedHeight > containerRect.bottom - 20;
    if (showAbove) {
      y = rect.top - estimatedHeight - 10;
    }
    
    // Convert to container-relative coordinates
    x = x - containerRect.left;
    y = y - containerRect.top;
    
    // Calculate arrow offset (where arrow should point relative to tooltip)
    const elementCenter = rect.left + rect.width / 2 - containerRect.left;
    const arrowOffset = elementCenter - x;
    
    // Ensure minimum values
    x = Math.max(0, x);
    y = Math.max(0, y);
    
    return {
      x,
      y,
      showAbove,
      arrowOffset
    };
  };

  // Optimized tooltip handler
  const handleTooltipShow = (event, content) => {
    const position = calculateTooltipPosition(event, content);
    setTooltipContent(content);
    setTooltipPosition(position);
    setShowTooltip(true);
  };

  const handleTooltipHide = () => {
    setShowTooltip(false);
  };

  // Helper functions for validation tooltips
  const getColorRangeValidationContent = (isValid) => {
    if (isValid) {
      return `Color range validation passed!\n\nCurrent values:\n• Min: ${settings.colorRangeMin} dB\n• Max: ${settings.colorRangeMax} dB\n\n✅ Valid configuration:\n• Min (${settings.colorRangeMin}) < Max (${settings.colorRangeMax})\n• Range: ${settings.colorRangeMax - settings.colorRangeMin} dB\n\nTypical ranges:\n• Weak signals: -80 dB to -60 dB\n• Medium signals: -60 dB to -40 dB\n• Strong signals: -40 dB to -20 dB\n\nOptimal for spectrogram visualization!`;
    } else {
      return `Color range validation failed!\n\nCurrent values:\n• Min: ${settings.colorRangeMin} dB\n• Max: ${settings.colorRangeMax} dB\n\nIssue: Minimum value (${settings.colorRangeMin}) is greater than or equal to maximum value (${settings.colorRangeMax})\n\nFor proper spectrogram visualization:\n• Min should be less than Max\n• Typical range: -80 dB to -20 dB\n• Min: -80 dB (weak signals)\n• Max: -20 dB (strong signals)`;
    }
  };

  // Optimized validation icon renderer
  const renderValidationIcon = (isValid) => {
    const content = getColorRangeValidationContent(isValid);
    const iconClass = isValid ? 'success-icon' : 'warning-icon';
    const iconSymbol = isValid ? '✅' : '⚠️';
    
    return (
      <span 
        className={iconClass} 
        onMouseEnter={(e) => handleTooltipShow(e, content)}
        onMouseLeave={handleTooltipHide}
      >
        {iconSymbol}
      </span>
    );
  };

  // Helper functions for FFT Size tooltips
  const getFFTSizeWarningContent = () => {
    const resolutionHz = settings.sampleRate / settings.originalCalculatedFFTSize;
    return `FFT size limited to ${settings.fftSize.toLocaleString()} (calculated: ${settings.originalCalculatedFFTSize?.toLocaleString()})\n\nCalculation:\n• Formula: FFT Size = Sample Rate ÷ Resolution\n• ${settings.sampleRate.toLocaleString()} Hz ÷ ${resolutionHz.toLocaleString()} Hz = ${settings.originalCalculatedFFTSize?.toLocaleString()}\n• Rounded up to power of 2: ${settings.originalCalculatedFFTSize?.toLocaleString()}\n• Limited to range: 16 - 65,536`;
  };

  const getFFTSizeSuccessContent = () => {
    const exactDivision = settings.sampleRate / settings.resolution;
    const log2Exact = Math.log2(exactDivision);
    const roundedLog2 = Math.ceil(log2Exact);
    return `FFT size calculated: ${settings.fftSize.toLocaleString()}\n\nCalculation:\n• Formula: FFT Size = Sample Rate ÷ Resolution\n• ${settings.sampleRate.toLocaleString()} Hz ÷ ${settings.resolution.toLocaleString()} Hz = ${exactDivision.toFixed(2)}\n• log₂(${exactDivision.toFixed(2)}) = ${log2Exact.toFixed(2)}\n• Rounded up: ${log2Exact.toFixed(2)} → ${roundedLog2}\n• FFT Size: 2^${roundedLog2} = ${settings.fftSize.toLocaleString()}\n• Status: Within valid range (16 - 65,536)`;
  };

  // Optimized FFT Size icon renderer
  const renderFFTSizeIcon = () => {
    if (settings.fftSizeOutOfRange) {
      return (
        <span 
          className="warning-icon" 
          onMouseEnter={(e) => handleTooltipShow(e, getFFTSizeWarningContent())}
          onMouseLeave={handleTooltipHide}
        >
          ⚠️
        </span>
      );
    } else if (settings.resolution > 0) {
      return (
        <span 
          className="success-icon" 
          onMouseEnter={(e) => handleTooltipShow(e, getFFTSizeSuccessContent())}
          onMouseLeave={handleTooltipHide}
        >
          ✅
        </span>
      );
    }
    return null;
  };

  // Helper functions for input validation
  const validateAndUpdateSetting = (value, settingKey, min = -200, max = 200) => {
    const parsedValue = parseFloat(value);
    if (!isNaN(parsedValue) && parsedValue >= min && parsedValue <= max) {
      updateSettings(settingKey, parsedValue);
      return true;
    }
    return false;
  };

  const handleInputKeyDown = (e, settingKey, inputSetter, min = -200, max = 200) => {
    if (e.key === 'Enter') {
      const isValid = validateAndUpdateSetting(e.target.value, settingKey, min, max);
      inputSetter('');
    }
  };

  const handleInputBlur = (e, settingKey, inputSetter, min = -200, max = 200) => {
    validateAndUpdateSetting(e.target.value, settingKey, min, max);
    inputSetter('');
  };

  // Render help icon with tooltip
  const renderHelpIcon = (content) => (
    <span
      className="help-icon"
      onMouseEnter={(e) => handleTooltipShow(e, content)}
      onMouseLeave={handleTooltipHide}
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
    console.log('💾 Auto-save triggered');
    console.log('💾 Current settings:', settings);
    console.log('💾 Current inputs:', {
      frequencyInput,
      sampleRateInput,
      resolutionInput,
      windowSizeInput,
      hopSizeInput
    });
    
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
    console.log('💾 Auto-save completed at:', currentTime);
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
            if (spectrogramHistory.current.length > maxHistoryLengthRef.current) {
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
  // Draw waterfall spectrogram on canvas - fixed pixel height
  const drawSpectrogram = () => {
    const canvas = canvasRef.current;
    if (!canvas) return;
    
    const ctx = canvas.getContext('2d');
    const width = canvas.width;
    const height = canvas.height;
    
    // Check if canvas has proper dimensions
    if (width <= 0 || height <= 0) return;
    
    if (spectrogramHistory.current.length === 0) return;
    
    // Waterfall parameters - fill entire canvas
    const pixelHeight = 2; // Fixed height for each pixel
    
    // Calculate frequency width per bin - use actual data length
    const actualDataLength = spectrogramHistory.current[0]?.data?.length || settings.fftSize / 2;
    const freqWidth = width / actualDataLength;
    
    // Clear canvas
    ctx.clearRect(0, 0, width, height);
    
    // Draw waterfall based on scroll direction - fill entire canvas
    const dataToDraw = spectrogramHistory.current.slice(-maxHistoryLengthRef.current); // Use history length to fill canvas
    
    dataToDraw.forEach((data, timeIndex) => {
      if (!data.data || data.data.length === 0) return;
      
      // Calculate Y position based on scroll direction
      const timeY = settings.scrollDirection === 'up' 
        ? height - (dataToDraw.length - timeIndex) * pixelHeight  // Newest at bottom
        : (dataToDraw.length - 1 - timeIndex) * pixelHeight; // Newest at top
      
      // Draw each frequency bin as a single pixel line
      for (let freqIndex = 0; freqIndex < data.data.length; freqIndex++) {
        const amplitude = data.data[freqIndex];
        const freqX = freqIndex * freqWidth;
        const color = amplitudeToColor(amplitude);
        
        ctx.fillStyle = color;
        ctx.fillRect(
          freqX, 
          timeY, 
          freqWidth, 
          pixelHeight
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
    
    // Update maxHistoryLength to match canvas height
    const canvasHeight = rect.height;
    maxHistoryLengthRef.current = canvasHeight;
    console.log('📏 Updated maxHistoryLength to canvas height:', canvasHeight);
    
    // Clear the canvas after resize
    ctx.clearRect(0, 0, canvas.width, canvas.height);
  };

  // useEffect hooks
  useEffect(() => {
    console.log('🔍 Initial load useEffect triggered');
    // Load latest configuration on mount
    const configs = JSON.parse(localStorage.getItem('spectrogramConfigs') || '{}');
    const latestConfig = configs['latest'];
    
    if (latestConfig) {
      console.log('🔍 Loading saved config:', latestConfig.settings);
      setIsLoadingFromStorage(true); // Mark that we're loading from storage
      
      // Store the loaded settings for comparison
      setLoadedSettings(latestConfig.settings);
      
      // Load all settings exactly as saved, without triggering auto-updates
      setSettings(latestConfig.settings);
      setFrequencyInput(latestConfig.frequencyInput || '');
      setSampleRateInput(latestConfig.sampleRateInput || '');
      setResolutionInput(latestConfig.resolutionInput || '');
      setWindowSizeInput(latestConfig.windowSizeInput || '');
      setHopSizeInput(latestConfig.hopSizeInput || '');
      setHasLoadedSettings(true); // Mark that we've loaded settings
      
      // Set the last saved time from the loaded configuration
      if (latestConfig.savedAt) {
        setLastSavedTime(latestConfig.savedAt);
      }
      
      // Only fill in completely missing values, don't override existing ones
      if (latestConfig.settings) {
        const updatedSettings = { ...latestConfig.settings };
        let hasChanges = false;
        
        // Only set window size if it's completely missing
        if (latestConfig.settings.fftSize && !latestConfig.settings.windowSize) {
          updatedSettings.windowSize = latestConfig.settings.fftSize;
          hasChanges = true;
        }
        
        // Only set hop size if it's completely missing
        if (updatedSettings.windowSize && !latestConfig.settings.hopSize) {
          updatedSettings.hopSize = Math.floor(updatedSettings.windowSize / 2);
          hasChanges = true;
        }
        
        // Update settings only if we made changes
        if (hasChanges) {
          setSettings(updatedSettings);
        }
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
      // Delay setting isInitialLoad to false to prevent auto-updates during initial load
      const timeoutId = setTimeout(() => {
        console.log('🔍 Setting isInitialLoad to false (delayed)');
        setIsInitialLoad(false);
        setIsLoadingFromStorage(false); // Also mark that we're no longer loading from storage
        
        // Enable auto-updates after a longer delay to ensure all initial effects have completed
        const autoUpdateTimeoutId = setTimeout(() => {
          console.log('🔍 Enabling auto-updates');
          setAutoUpdatesEnabled(true);
        }, 500); // Longer delay to ensure all initial effects have run
        
        return () => clearTimeout(autoUpdateTimeoutId);
      }, 100); // Small delay to ensure all initial effects have run
      
      return () => clearTimeout(timeoutId);
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

  // Ensure window size matches FFT size whenever FFT size changes (but not on initial load)
  useEffect(() => {
    // Only run if auto-updates are enabled and we have a valid FFT size, window size is different
    if (autoUpdatesEnabled && settings.fftSize && settings.windowSize !== settings.fftSize) {
      // Check if this is a user-initiated change (not from loading from storage)
      const isUserChange = !loadedSettings || settings.fftSize !== loadedSettings.fftSize;
      
              if (isUserChange) {
          updateSettings('windowSize', settings.fftSize);
          setWindowSizeInput(''); // Clear the input to show the auto-updated value
          
          // Auto-update Hop Size to half of Window Size
          const newHopSize = Math.floor(settings.fftSize / 2);
          updateSettings('hopSize', newHopSize);
          setHopSizeInput(''); // Clear the input to show the auto-updated value
          
          console.log(`🔄 Auto-updated Window Size to ${settings.fftSize} and Hop Size to ${newHopSize} to match FFT Size (user change)`);
        } else {
          console.log(`🔍 Skipping auto-update - FFT size matches loaded settings (${settings.fftSize})`);
        }
    }
  }, [settings.fftSize, autoUpdatesEnabled, loadedSettings]);

  // Ensure hop size is half of window size whenever window size changes (but not on initial load)
  useEffect(() => {
    if (autoUpdatesEnabled && settings.windowSize && settings.hopSize !== Math.floor(settings.windowSize / 2)) {
      // Check if this is a user-initiated change (not from loading from storage)
      const isUserChange = !loadedSettings || settings.windowSize !== loadedSettings.windowSize;
      
      if (isUserChange) {
        const newHopSize = Math.floor(settings.windowSize / 2);
        
        updateSettings('hopSize', newHopSize);
        setHopSizeInput(''); // Clear the input to show the auto-updated value
        
        console.log(`🔄 Auto-updated Hop Size to ${newHopSize} (half of Window Size ${settings.windowSize}) (user change)`);
      } else {
        console.log(`🔍 Skipping auto-update - Window size matches loaded settings (${settings.windowSize})`);
      }
    }
  }, [settings.windowSize, autoUpdatesEnabled, loadedSettings]);

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
      { label: 'Max History:', value: Math.round(maxHistoryLengthRef.current), active: null },
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
                             <div className="setting-status" ref={colormapConversionRef}>
                 <span className={`status-transition ${statusBoxStates.colormap.isTransitioning ? 'transitioning' : 'normal'}`}>
                   {statusBoxStates.colormap.isTransitioning ? statusBoxStates.colormap.transitionText : settings.colormap}
                 </span>
               </div>
             </div>
             
             <div className="setting-item">
               <label>
                 Color Range Min (dB)
                 {renderHelpIcon("Minimum value for the color scale in decibels. Lower values show more detail in weak signals, higher values focus on stronger signals.")}
                 {renderValidationIcon(settings.colorRangeMin < settings.colorRangeMax)}
               </label>
               <input
                 type="text"
                 value={colorRangeMinInput !== '' ? colorRangeMinInput : settings.colorRangeMin}
                 onInput={(e) => {
                   console.log('Color Range Min input:', e.target.value);
                   setColorRangeMinInput(e.target.value);
                 }}
                 onKeyDown={(e) => handleInputKeyDown(e, 'colorRangeMin', setColorRangeMinInput)}
                 onBlur={(e) => handleInputBlur(e, 'colorRangeMin', setColorRangeMinInput)}
                 placeholder="-50"
                 className={settings.colorRangeMin >= settings.colorRangeMax ? 'input-error' : 'input-valid'}
               />
               <div className="setting-status" ref={colorRangeMinConversionRef}>
                 <span className={`status-transition ${statusBoxStates.colorRangeMin.isTransitioning ? 'transitioning' : 'normal'}`}>
                   {statusBoxStates.colorRangeMin.isTransitioning ? statusBoxStates.colorRangeMin.transitionText : `${settings.colorRangeMin} dB`}
                 </span>
               </div>
             </div>
             
             <div className="setting-item">
               <label>
                 Color Range Max (dB)
                 {renderHelpIcon("Maximum value for the color scale in decibels. Higher values show more detail in strong signals, lower values focus on weaker signals.")}
                 {renderValidationIcon(settings.colorRangeMin < settings.colorRangeMax)}
               </label>
               <input
                 type="text"
                 value={colorRangeMaxInput !== '' ? colorRangeMaxInput : settings.colorRangeMax}
                 onInput={(e) => {
                   console.log('Color Range Max input:', e.target.value);
                   setColorRangeMaxInput(e.target.value);
                 }}
                 onKeyDown={(e) => handleInputKeyDown(e, 'colorRangeMax', setColorRangeMaxInput)}
                 onBlur={(e) => handleInputBlur(e, 'colorRangeMax', setColorRangeMaxInput)}
                 placeholder="0"
                 className={settings.colorRangeMin >= settings.colorRangeMax ? 'input-error' : 'input-valid'}
               />
               <div className="setting-status" ref={colorRangeMaxConversionRef}>
                 <span className={`status-transition ${statusBoxStates.colorRangeMax.isTransitioning ? 'transitioning' : 'normal'}`}>
                   {statusBoxStates.colorRangeMax.isTransitioning ? statusBoxStates.colorRangeMax.transitionText : `${settings.colorRangeMax} dB`}
                 </span>
               </div>
             </div>
             
             <div className="setting-item">
               <label>
                 Auto-Adjust Color Range
                 {renderHelpIcon("Automatically adjust color range based on current signal amplitudes for optimal visualization.")}
               </label>
               <button
                 className="control-btn config"
                 onClick={autoAdjustColorRange}
                 style={{ width: '100%', marginTop: '0.5rem' }}
               >
                 Auto-Adjust Range
               </button>
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
              <div className="setting-status" ref={displayModeConversionRef}>
                <span className={`status-transition ${statusBoxStates.displayMode.isTransitioning ? 'transitioning' : 'normal'}`}>
                  {statusBoxStates.displayMode.isTransitioning ? statusBoxStates.displayMode.transitionText : settings.displayMode}
                </span>
              </div>
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
              <div className="setting-status" ref={scrollDirectionConversionRef}>
                <span className={`status-transition ${statusBoxStates.scrollDirection.isTransitioning ? 'transitioning' : 'normal'}`}>
                  {statusBoxStates.scrollDirection.isTransitioning ? statusBoxStates.scrollDirection.transitionText : settings.scrollDirection}
                </span>
              </div>
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
              <div className="setting-status" ref={usrpDeviceConversionRef}>
                <span className={`status-transition ${statusBoxStates.usrpDevice.isTransitioning ? 'transitioning' : 'normal'}`}>
                  {statusBoxStates.usrpDevice.isTransitioning ? statusBoxStates.usrpDevice.transitionText : (selectedUsrpDevice || 'No device selected')}
                </span>
              </div>
            </div>
            
            <div className="setting-item">
              <label>
                Center Frequency
                {renderHelpIcon("The center frequency of the signal to analyze. This is the frequency around which the USRP will be tuned.\n\nSupports units:\n• G (GHz) - e.g., 2.4G = 2.4 GHz\n• M (MHz) - e.g., 100M = 100 MHz\n• K (kHz) - e.g., 20K = 20 kHz\n\nCommon frequencies:\n• WiFi: 2.4G, 5G\n• Cellular: 900M, 1800M, 2100M\n• GPS: 1.575G")}
              </label>
              <div className="frequency-input-container">
                <input
                  ref={frequencyInputRef}
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
              <div className="setting-status" ref={frequencyConversionRef}>
                <span className={`status-transition ${statusBoxStates.frequency.isTransitioning ? 'transitioning' : 'normal'}`}>
                  {statusBoxStates.frequency.isTransitioning ? statusBoxStates.frequency.transitionText : `${settings.frequency.toLocaleString()} Hz`}
                </span>
              </div>
            </div>
            
            <div className="setting-item">
              <label>
                Sample Rate (Bandwidth)
                {renderHelpIcon("The sampling rate (bandwidth) of the signal capture. This determines how much of the frequency spectrum you can observe.\n\nHigher rates provide:\n• Wider frequency coverage\n• Better time resolution\n• More data to process\n\nSupports units:\n• G (GHz) - e.g., 1G = 1 GHz\n• M (MHz) - e.g., 10M = 10 MHz\n• K (kHz) - e.g., 44.1K = 44.1 kHz\n\nTypical values:\n• Audio: 44.1K, 48K\n• RF: 1M, 10M, 30.72M")}
              </label>
              <div className="frequency-input-container">
                <input
                  ref={sampleRateInputRef}
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
              <div className="setting-status" ref={sampleRateConversionRef}>
                <span className={`status-transition ${statusBoxStates.sampleRate.isTransitioning ? 'transitioning' : 'normal'}`}>
                  {statusBoxStates.sampleRate.isTransitioning ? statusBoxStates.sampleRate.transitionText : `${settings.sampleRate.toLocaleString()} Hz`}
                </span>
              </div>
            </div>
            
            <div className="setting-item">
              <label>
                Resolution
                {renderHelpIcon("Desired frequency resolution. This determines the frequency precision of the analysis.\n\nHigher resolution provides:\n• Better frequency precision\n• Ability to distinguish close frequencies\n• Slower update rates\n• Larger FFT size required\n\nLower resolution provides:\n• Faster updates\n• Less computational load\n• Coarser frequency detail\n• Smaller FFT size\n\nSupports units:\n• G (GHz) - e.g., 1G = 1 GHz\n• M (MHz) - e.g., 1M = 1 MHz\n• K (kHz) - e.g., 1K = 1 kHz\n\nFormula: FFT Size = Sample Rate / Resolution\n\nTypical values:\n• High precision: 1K, 10K\n• Medium precision: 100K, 1M\n• Low precision: 10M, 100M\n\n🔄 Auto-Update: When you set resolution, the FFT size is automatically calculated for optimal performance.")}
              </label>
              <div className="frequency-input-container">
                <input
                  ref={resolutionInputRef}
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
              <div className="setting-status" ref={resolutionConversionRef}>
                <span className={`status-transition ${statusBoxStates.resolution.isTransitioning ? 'transitioning' : 'normal'}`}>
                  {statusBoxStates.resolution.isTransitioning ? statusBoxStates.resolution.transitionText : (settings.resolution ? `${settings.resolution.toLocaleString()} Hz` : 'Auto (calculated from FFT size)')}
                </span>
              </div>
            </div>
            
            <div className="setting-item">
              <label>
                FFT Size
                {renderHelpIcon("Number of frequency bins in the FFT (Fast Fourier Transform).\n\nLarger FFT sizes provide:\n• Better frequency resolution\n• More precise frequency measurements\n• More computational time required\n\nSmaller FFT sizes provide:\n• Faster processing\n• Less memory usage\n• Coarser frequency resolution\n\nMust be a power of 2:\n• 256, 512, 1024, 2048, 4096, 8192\n\nRelationship:\n• Frequency Resolution = Sample Rate / FFT Size\n• Time Resolution = FFT Size / Sample Rate\n\n🔄 Auto-Calculated: Automatically calculated from resolution setting for optimal performance.")}
                {renderFFTSizeIcon()}
              </label>
              <input
                ref={fftSizeInputRef}
                type="number"
                value={settings.fftSize}
                readOnly
                min="16"
                max="65536"
                step="16"
                placeholder="Auto-calculated"
                style={{
                  border: settings.fftSizeOutOfRange ? '2px solid #ef4444' : '1px solid #d1d5db',
                  backgroundColor: settings.fftSizeOutOfRange ? '#fef2f2' : '#f9fafb',
                  color: '#6b7280',
                  cursor: 'not-allowed'
                }}
              />
              <div className="setting-status" ref={fftSizeConversionRef}>
                <span className={`status-transition ${statusBoxStates.fftSize.isTransitioning ? 'transitioning' : 'normal'}`}>
                  {statusBoxStates.fftSize.isTransitioning ? statusBoxStates.fftSize.transitionText : `log₂(${settings.fftSize}) = ${Math.log2(settings.fftSize).toFixed(1)}`}
                </span>
              </div>
            </div>
            
            <div className="setting-item">
              <label>
                Window Size
                {renderHelpIcon("Number of samples per STFT (Short-Time Fourier Transform) frame.\n\nShould be ≤ FFT Size for optimal performance.\n\nSmaller windows provide:\n• Better time resolution\n• Faster response to signal changes\n• Less frequency resolution\n\nLarger windows provide:\n• Better frequency resolution\n• Slower response to changes\n• More stable frequency measurements\n\nWarning: Values > FFT Size may cause issues")}
                {settings.windowSize > settings.fftSize && (
                  <span
                    className="warning-icon"
                    onMouseEnter={(e) => {
                      const rect = e.target.getBoundingClientRect();
                      const containerRect = e.target.closest('.spectrogram-container').getBoundingClientRect();
                      
                      const content = `Window Size (${settings.windowSize.toLocaleString()}) is larger than FFT Size (${settings.fftSize.toLocaleString()})\n\nThis may cause:\n• Performance issues\n• Incorrect frequency analysis\n• Memory problems\n• Unexpected behavior\n\nRecommendation: Use Window Size ≤ FFT Size`;
                      const estimatedWidth = Math.min(500, Math.max(300, content.length * 7));
                      const estimatedHeight = 120;
                      
                      let x = rect.left + rect.width / 2;
                      let y = rect.bottom + 10;
                      
                      if (x + estimatedWidth / 2 > containerRect.right - 20) {
                        x = containerRect.right - estimatedWidth / 2 - 20;
                      } else if (x - estimatedWidth / 2 < containerRect.left + 20) {
                        x = containerRect.left + estimatedWidth / 2 + 20;
                      }
                      
                      if (y + estimatedHeight > containerRect.bottom - 20) {
                        y = rect.top - estimatedHeight - 10;
                      }
                      
                      x = x - containerRect.left;
                      y = y - containerRect.top;
                      
                      const questionMarkCenter = rect.left + rect.width / 2 - containerRect.left;
                      const arrowOffset = questionMarkCenter - x;
                      
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
                  >
                    ⚠️
                  </span>
                )}
                {settings.windowSize <= settings.fftSize && settings.windowSize > 0 && (
                  <span 
                    className="success-icon" 
                    onMouseEnter={(e) => {
                      const rect = e.target.getBoundingClientRect();
                      const containerRect = e.target.closest('.spectrogram-container').getBoundingClientRect();
                      
                      const content = `Window Size (${settings.windowSize.toLocaleString()}) is properly configured\n\n✅ Valid range: ≤ FFT Size (${settings.fftSize.toLocaleString()})\n✅ Optimal performance\n✅ Correct STFT analysis\n\nWindow Size: ${settings.windowSize.toLocaleString()} samples\nFFT Size: ${settings.fftSize.toLocaleString()} samples\nStatus: Valid configuration`;
                      const estimatedWidth = Math.min(500, Math.max(300, content.length * 7));
                      const estimatedHeight = 120;
                      
                      let x = rect.left + rect.width / 2;
                      let y = rect.bottom + 10;
                      
                      if (x + estimatedWidth / 2 > containerRect.right - 20) {
                        x = containerRect.right - estimatedWidth / 2 - 20;
                      } else if (x - estimatedWidth / 2 < containerRect.left + 20) {
                        x = containerRect.left + estimatedWidth / 2 + 20;
                      }
                      
                      if (y + estimatedHeight > containerRect.bottom - 20) {
                        y = rect.top - estimatedHeight - 10;
                      }
                      
                      x = x - containerRect.left;
                      y = y - containerRect.top;
                      
                      const questionMarkCenter = rect.left + rect.width / 2 - containerRect.left;
                      const arrowOffset = questionMarkCenter - x;
                      
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
                  >
                    ✅
                  </span>
                )}
              </label>
              <input
                ref={windowSizeInputRef}
                type="number"
                placeholder="Auto (matches FFT size)"
                value={windowSizeInput || settings.windowSize || settings.fftSize}
                onChange={(e) => setWindowSizeInput(e.target.value)}
                onBlur={() => {
                  const value = parseInt(windowSizeInput);
                  updateSettings('windowSize', value || settings.fftSize);
                  setWindowSizeInput(value || '');
                }}
                onKeyPress={(e) => {
                  if (e.key === 'Enter') {
                    const value = parseInt(windowSizeInput);
                    updateSettings('windowSize', value || settings.fftSize);
                    setWindowSizeInput(value || '');
                    e.target.blur();
                  }
                }}
                min="16"
                max="65536"
                step="16"
                style={{
                  border: settings.windowSize > settings.fftSize ? '2px solid #ef4444' : '1px solid #d1d5db',
                  backgroundColor: settings.windowSize > settings.fftSize ? '#fef2f2' : '#ffffff',
                  color: settings.windowSize > settings.fftSize ? '#dc2626' : '#1f2937'
                }}
              />
              <div className="setting-status" ref={windowSizeConversionRef}>
                <span className={`status-transition ${statusBoxStates.windowSize.isTransitioning ? 'transitioning' : 'normal'}`}>
                  {statusBoxStates.windowSize.isTransitioning ? statusBoxStates.windowSize.transitionText : `${settings.windowSize?.toLocaleString() || settings.fftSize?.toLocaleString() || 'Auto'}`}
                </span>
              </div>
            </div>
            
            <div className="setting-item">
              <label>
                Hop Size
                {renderHelpIcon("Number of samples to advance between consecutive STFT frames.\n\nIf not specified, defaults to window_size/2.\n\nSmaller hop sizes provide:\n• Smoother time resolution\n• More overlapping frames\n• Higher computational overhead\n• Better time-domain detail\n\nLarger hop sizes provide:\n• Faster processing\n• Less overlap between frames\n• Lower computational cost\n• Coarser time resolution\n\nOverlap = Window Size - Hop Size\nTypical overlap: 50% (hop = window/2)")}
                {settings.hopSize > settings.windowSize && (
                  <span
                    className="warning-icon"
                    onMouseEnter={(e) => {
                      const rect = e.target.getBoundingClientRect();
                      const containerRect = e.target.closest('.spectrogram-container').getBoundingClientRect();
                      
                      const content = `Hop Size (${settings.hopSize.toLocaleString()}) is larger than Window Size (${settings.windowSize.toLocaleString()})\n\nThis may cause:\n• Incorrect STFT analysis\n• Missing data points\n• Performance issues\n• Unexpected behavior\n\nRecommendation: Use Hop Size ≤ Window Size`;
                      const estimatedWidth = Math.min(500, Math.max(300, content.length * 7));
                      const estimatedHeight = 120;
                      
                      let x = rect.left + rect.width / 2;
                      let y = rect.bottom + 10;
                      
                      if (x + estimatedWidth / 2 > containerRect.right - 20) {
                        x = containerRect.right - estimatedWidth / 2 - 20;
                      } else if (x - estimatedWidth / 2 < containerRect.left + 20) {
                        x = containerRect.left + estimatedWidth / 2 + 20;
                      }
                      
                      if (y + estimatedHeight > containerRect.bottom - 20) {
                        y = rect.top - estimatedHeight - 10;
                      }
                      
                      x = x - containerRect.left;
                      y = y - containerRect.top;
                      
                      const questionMarkCenter = rect.left + rect.width / 2 - containerRect.left;
                      const arrowOffset = questionMarkCenter - x;
                      
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
                  >
                    ⚠️
                  </span>
                )}
                {settings.hopSize <= settings.windowSize && settings.hopSize > 0 && (
                  <span 
                    className="success-icon" 
                    onMouseEnter={(e) => {
                      const rect = e.target.getBoundingClientRect();
                      const containerRect = e.target.closest('.spectrogram-container').getBoundingClientRect();
                      
                      const content = `Hop Size (${settings.hopSize.toLocaleString()}) is properly configured\n\n✅ Valid range: ≤ Window Size (${settings.windowSize.toLocaleString()})\n✅ Optimal STFT analysis\n✅ Correct frame overlap\n\nHop Size: ${settings.hopSize.toLocaleString()} samples\nWindow Size: ${settings.windowSize.toLocaleString()} samples\nOverlap: ${(settings.windowSize - settings.hopSize).toLocaleString()} samples\nStatus: Valid configuration`;
                      const estimatedWidth = Math.min(500, Math.max(300, content.length * 7));
                      const estimatedHeight = 120;
                      
                      let x = rect.left + rect.width / 2;
                      let y = rect.bottom + 10;
                      
                      if (x + estimatedWidth / 2 > containerRect.right - 20) {
                        x = containerRect.right - estimatedWidth / 2 - 20;
                      } else if (x - estimatedWidth / 2 < containerRect.left + 20) {
                        x = containerRect.left + estimatedWidth / 2 + 20;
                      }
                      
                      if (y + estimatedHeight > containerRect.bottom - 20) {
                        y = rect.top - estimatedHeight - 10;
                      }
                      
                      x = x - containerRect.left;
                      y = y - containerRect.top;
                      
                      const questionMarkCenter = rect.left + rect.width / 2 - containerRect.left;
                      const arrowOffset = questionMarkCenter - x;
                      
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
                  >
                    ✅
                  </span>
                )}
              </label>
              <input
                ref={hopSizeInputRef}
                type="number"
                placeholder="Auto (window_size/2)"
                value={hopSizeInput || settings.hopSize || Math.floor((settings.windowSize || settings.fftSize) / 2)}
                onChange={(e) => setHopSizeInput(e.target.value)}
                onBlur={() => {
                  const value = parseInt(hopSizeInput);
                  updateSettings('hopSize', value || Math.floor((settings.windowSize || settings.fftSize) / 2));
                  setHopSizeInput(value || '');
                }}
                onKeyPress={(e) => {
                  if (e.key === 'Enter') {
                    const value = parseInt(hopSizeInput);
                    updateSettings('hopSize', value || Math.floor((settings.windowSize || settings.fftSize) / 2));
                    setHopSizeInput(value || '');
                    e.target.blur();
                  }
                }}
                min="1"
                max="65536"
                step="1"
                style={{
                  border: settings.hopSize > settings.windowSize ? '2px solid #ef4444' : '1px solid #d1d5db',
                  backgroundColor: settings.hopSize > settings.windowSize ? '#fef2f2' : '#ffffff',
                  color: settings.hopSize > settings.windowSize ? '#dc2626' : '#1f2937'
                }}
              />
              <div className="setting-status" ref={hopSizeConversionRef}>
                <span className={`status-transition ${statusBoxStates.hopSize.isTransitioning ? 'transitioning' : 'normal'}`}>
                  {statusBoxStates.hopSize.isTransitioning ? statusBoxStates.hopSize.transitionText : `${settings.hopSize?.toLocaleString() || Math.floor((settings.windowSize || settings.fftSize) / 2)?.toLocaleString() || 'Auto'}`}
                </span>
              </div>
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
              <div className="setting-status" ref={windowTypeConversionRef}>
                <span className={`status-transition ${statusBoxStates.windowType.isTransitioning ? 'transitioning' : 'normal'}`}>
                  {statusBoxStates.windowType.isTransitioning ? statusBoxStates.windowType.transitionText : settings.windowType}
                </span>
              </div>
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
              <div className="setting-status" ref={gainConversionRef}>
                <span className={`status-transition ${statusBoxStates.gain.isTransitioning ? 'transitioning' : 'normal'}`}>
                  {statusBoxStates.gain.isTransitioning ? statusBoxStates.gain.transitionText : `${settings.gain} dB`}
                </span>
              </div>
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
          <div className="spectrogram-main">
            {/* Spectrogram canvas with frequency axis */}
            <div className="spectrogram-canvas" style={{ position: 'relative' }}>
              <canvas
                ref={canvasRef}
                width={800}
                height={600}
                style={{
                  width: '100%',
                  height: '100%',
                  display: 'block'
                }}
              />
              {/* Frequency axis overlay */}
              <div className="frequency-axis-overlay">
                {Array.from({length: 9}, (_, i) => {
                  // Calculate frequency based on FFT size and sample rate
                  const freqBinIndex = Math.floor((i * settings.fftSize / 2) / 8);
                  const freq = (freqBinIndex * settings.sampleRate) / settings.fftSize;
                  const xPosition = (i / 8) * 100; // Percentage position
                  
                  return (
                    <div 
                      key={i} 
                      className="frequency-tick"
                      style={{ 
                        position: 'absolute',
                        left: `${xPosition}%`,
                        bottom: '0',
                        transform: 'translateX(-50%)'
                      }}
                    >
                      <div className="tick-line" style={{
                        width: '1px',
                        height: '10px',
                        background: '#666',
                        margin: '0 auto'
                      }}></div>
                      <div className="tick-label" style={{
                        fontSize: '10px',
                        color: '#666',
                        textAlign: 'center',
                        marginTop: '2px'
                      }}>
                        {freq >= 1000 ? `${(freq / 1000).toFixed(1)}k` : freq.toFixed(0)}
                      </div>
                    </div>
                  );
                })}
              </div>
            </div>
            
            {/* Color bar (right) */}
            <div className="color-bar-container">
              <div className="color-bar-title">Amplitude (dB)</div>
              <div className="color-bar">
                <div 
                  className="color-bar-gradient"
                  style={{
                    background: `linear-gradient(to top, ${getColormapGradient(settings.colormap)})`
                  }}
                ></div>
                <div className="color-bar-labels">
                  <span className="color-label max">{settings.colorRangeMax} dB</span>
                  <span className="color-label mid">{Math.round((settings.colorRangeMax + settings.colorRangeMin) / 2)} dB</span>
                  <span className="color-label min">{settings.colorRangeMin} dB</span>
                </div>
              </div>
            </div>
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
