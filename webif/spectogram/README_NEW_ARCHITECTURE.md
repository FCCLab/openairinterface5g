# Spectrogram Service - New Modular Architecture

## 🏗️ Architecture Overview

The spectrogram service has been refactored from a single monolithic file into a modular, class-based architecture for better maintainability, debugging, and process isolation.

## 📁 File Structure

```
spectogram/
├── spectrogram_main.py      # Main orchestrator (replaces spectrogram.py)
├── tx_rx_process.py         # Self-contained TX/RX process
├── stft_process.py          # Self-contained STFT processing process
├── websocket_process.py     # Self-contained WebSocket server process
├── cpu_pe.py               # CPU P-core/E-core detection module
├── run_spectrogram.py      # Simple launcher script
├── requirements.txt         # Python dependencies
└── README_NEW_ARCHITECTURE.md  # This file
```

## 🔧 Key Improvements

### 1. **Process Isolation**
- Each process is now a **self-contained class** with its own logging, signal handlers, and CPU affinity
- **No shared state issues** between processes
- **Independent error handling** for each process

### 2. **Fixed Stop Event Issue**
- **Root cause resolved**: The `stop_event` was being shared incorrectly between processes
- Each process now has **proper signal handling** and **independent stop logic**
- **No more immediate process crashes** due to stop event contamination

### 3. **Better Error Handling**
- **Comprehensive logging** at each stage of process startup
- **Detailed exception information** with type and details
- **Graceful degradation** when individual processes fail

### 4. **Modular Design**
- **Easy to test** individual components
- **Simple to modify** specific functionality
- **Clear separation of concerns** between processes

## 🚀 Usage

### **Option 1: Use the new launcher (recommended)**
```bash
python3 run_spectrogram.py --device "" --freq 2.4e9 --sample-rate 1e6 --gain 37.5
```

### **Option 2: Use the main orchestrator directly**
```bash
python3 spectrogram_main.py --device "" --freq 2.4e9 --sample-rate 1e6 --gain 37.5
```

### **Option 3: Test individual processes**
```bash
# Test TX/RX process
python3 tx_rx_process.py --cpu-core 1

# Test STFT process
python3 stft_process.py --sample-rate 1e6 --fft-size 1024 --cpu-core 2

# Test WebSocket process
python3 websocket_process.py --port 40001 --cpu-core 3
```

## 🔍 Process Details

### **TX/RX Process (`tx_rx_process.py`)**
- **Purpose**: USRP communication (transmit/receive IQ samples)
- **CPU Core**: P-core (performance optimized)
- **Features**: 
  - Continuous TX/RX streaming
  - Thread-based TX and RX operations
  - USRP device management
  - Queue-based data output

### **STFT Process (`stft_process.py`)**
- **Purpose**: Signal processing (Short-Time Fourier Transform)
- **CPU Core**: P-core (performance optimized)
- **Features**:
  - Real-time STFT calculation
  - Frequency domain analysis
  - Magnitude calculation in dB
  - Queue-based input/output

### **WebSocket Process (`websocket_process.py`)**
- **Purpose**: Real-time data broadcasting
- **CPU Core**: P-core (performance optimized)
- **Features**:
  - WebSocket server management
  - Client connection handling
  - Real-time data broadcasting
  - JSON data formatting

### **Main Orchestrator (`spectrogram_main.py`)**
- **Purpose**: Process management and coordination
- **CPU Core**: E-core (efficiency optimized)
- **Features**:
  - Process lifecycle management
  - CPU core assignment
  - Health monitoring
  - Automatic restart mechanism
  - Statistics logging

## 🧠 CPU Core Assignment

The system automatically detects P-cores and E-cores and assigns them optimally:

- **Main Process**: First available E-core (efficiency)
- **TX/RX Process**: First available P-core (performance)
- **STFT Process**: Second available P-core (performance)
- **WebSocket Process**: Third available P-core (performance)

## 🔄 Restart Mechanism

- **Automatic restart** of failed processes (up to 10 attempts)
- **Time-windowed restarts** (1 minute window)
- **Process health monitoring** every second
- **Graceful degradation** when restart limits are exceeded

## 📊 Monitoring

- **Statistics logging** every 5 seconds
- **Queue status monitoring**
- **Memory usage tracking**
- **Process health status**
- **Restart attempt counters**

## 🐛 Troubleshooting

### **Process Not Starting**
1. Check individual process logs in `logs/` directory
2. Verify CPU core assignments
3. Check for import errors in individual process files

### **Process Dying Immediately**
1. Check signal handler setup
2. Verify CPU affinity settings
3. Check for missing dependencies

### **Queue Issues**
1. Monitor queue sizes in statistics
2. Check process communication
3. Verify data flow between processes

## 🔄 Migration from Old Architecture

The new architecture is **fully backward compatible** with the same command-line interface. Simply replace:

```bash
# Old way
python3 spectrogram.py [args]

# New way
python3 run_spectrogram.py [args]
```

## ✨ Benefits of New Architecture

1. **🔧 Easier Debugging**: Each process can be tested independently
2. **🚀 Better Performance**: Proper process isolation and CPU affinity
3. **🛡️ More Robust**: Independent error handling and restart mechanisms
4. **📝 Cleaner Code**: Modular, maintainable design
5. **🧪 Easier Testing**: Individual components can be unit tested
6. **📊 Better Monitoring**: Comprehensive health monitoring and statistics

## 🎯 Next Steps

1. **Test the new architecture** with your existing setup
2. **Monitor the logs** for any issues
3. **Customize individual processes** as needed
4. **Extend functionality** by adding new process classes

The new architecture resolves the critical `stop_event` issue and provides a solid foundation for future enhancements! 🚀
