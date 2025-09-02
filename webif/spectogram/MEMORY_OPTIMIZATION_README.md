# Spectrogram Service Memory Optimization

This document describes the memory optimizations implemented in the spectrogram service to prevent memory leaks and improve performance.

## Overview

The spectrogram service consists of three main processes:
1. **TX/RX Process** - Handles USRP device communication
2. **STFT Process** - Performs Short-Time Fourier Transform on IQ data
3. **WebSocket Process** - Broadcasts processed data to frontend clients

## Memory Problems Identified and Fixed

### 1. STFT Process Memory Issues

**Problems Fixed:**
- Window functions were recreated for each frame instead of being cached
- Large intermediate arrays in STFT calculations were not properly cleaned up
- No memory monitoring or automatic cleanup triggers

**Solutions Implemented:**
- Cache window function in `__init__` to avoid recreation
- Explicit cleanup of intermediate arrays with `del` statements
- Periodic memory monitoring with configurable thresholds
- Automatic garbage collection when memory usage exceeds limits

**Key Changes:**
```python
# Cache window function instead of recreating
self.window_cache = self._create_window(self.window_size, self.window_type)

# Use cached window in STFT
f, t, Zxx = scipy_signal.stft(
    samples, 
    window=self.window_cache,  # Use cached window
    # ... other parameters
)

# Explicit cleanup
del Zxx
```

### 2. WebSocket Process Memory Issues

**Problems Fixed:**
- New dictionary objects created every frame for spectrogram data
- New list copies of numpy arrays created every frame
- No memory monitoring or cleanup mechanisms

**Solutions Implemented:**
- Reusable data template to avoid creating new objects every frame
- Memory monitoring with configurable thresholds
- Automatic garbage collection triggers

**Key Changes:**
```python
# Reusable template instead of new dict every frame
self.spectrogram_data_template = {
    'f': 0, 'freq': [], 't': [], 'mag': [], 'ts': 0
}

# Update template instead of creating new dict
self.spectrogram_data_template['f'] = frame_num
self.spectrogram_data_template['freq'] = freq_list
# ... etc

# Create copy for sending
spectrogram_data = self.spectrogram_data_template.copy()
```

### 3. Queue Memory Management

**Problems Fixed:**
- No monitoring of queue memory usage
- Potential memory buildup in large queues
- No automatic cleanup triggers

**Solutions Implemented:**
- Queue memory monitoring in main process
- Automatic cleanup when queue memory exceeds thresholds
- Memory usage reporting in statistics

**Key Changes:**
```python
def _check_queue_memory(self):
    """Check memory usage of queues and trigger cleanup if needed"""
    total_memory = 0
    # Sample queue items to estimate memory usage
    rx_memory = sum(sys.getsizeof(item) for item in list(self.rx_queue._queue)[:10])
    stft_memory = sum(sys.getsizeof(item) for item in list(self.stft_queue._queue)[:10])
    
    if total_memory_mb > 100:  # 100MB threshold
        self._cleanup_queues()
```

### 4. Process Restart Memory Leaks

**Problems Fixed:**
- Old process objects not properly cleaned up during restarts
- Potential accumulation of zombie processes
- No explicit memory cleanup after process termination

**Solutions Implemented:**
- Explicit cleanup of old process references during restart
- Force garbage collection after process cleanup
- Better process lifecycle management

**Key Changes:**
```python
# Clean up old process reference to help garbage collection
old_process = self.tx_rx_process
if old_process and old_process != new_process:
    try:
        if old_process.is_alive():
            old_process.terminate()
            old_process.join(timeout=1.0)
        del old_process
    except Exception as e:
        self.logger.debug(f"Error cleaning up old {process_name} process: {e}")
```

## New Base Class: ProcessBase

A new base class has been created to consolidate common functionality and improve maintainability.

### Features

- **Unified Memory Management**: All processes inherit the same memory monitoring and cleanup logic
- **Common Logging**: Standardized logging setup across all processes
- **Signal Handling**: Unified signal handling for graceful shutdown
- **CPU Affinity**: Consistent CPU core binding
- **Parent Process Monitoring**: Automatic detection of orphaned processes
- **Statistics Collection**: Standardized process statistics

### Usage

```python
from process_base import ProcessBase

class STFTProcess(ProcessBase):
    def __init__(self, rx_queue, sample_rate, window_size, fft_size, hop_size, 
                 window_type, stop_event, stft_queue, timestamp, cpu_core=1, log_dir=None):
        # Call parent constructor
        super().__init__("STFT", stop_event, timestamp, cpu_core, log_dir)
        
        # Set process-specific memory threshold
        self.set_memory_threshold(500)  # 500MB for STFT process
        
        # Process-specific initialization
        self.rx_queue = rx_queue
        self.stft_queue = stft_queue
        # ... other initialization
    
    def run(self):
        """Main processing loop - override from base class"""
        while not self.stop_event.is_set():
            current_time = time.time()
            
            # Use base class memory monitoring
            self._check_memory_periodic(current_time)
            
            # Use base class FPS logging
            self._log_fps(current_time)
            
            # Process-specific logic
            # ...
```

### Memory Thresholds

Each process type has different memory thresholds based on their typical usage:

- **TX/RX Process**: 600MB (handles USRP buffers)
- **STFT Process**: 500MB (processes large arrays)
- **WebSocket Process**: 800MB (manages client connections and data)

### Configuration

Memory thresholds and check intervals can be configured per process:

```python
# Set custom memory threshold
process.set_memory_threshold(1000)  # 1GB

# Set custom check interval
process.set_memory_check_interval(30.0)  # 30 seconds
```

## Performance Improvements

### Before Optimization
- Memory usage could grow indefinitely
- Frequent object creation and destruction
- No automatic cleanup mechanisms
- Potential memory leaks during process restarts

### After Optimization
- Memory usage is monitored and controlled
- Objects are reused where possible
- Automatic garbage collection triggers
- Clean process lifecycle management
- Reduced memory fragmentation

## Monitoring and Debugging

### Memory Usage Logs
```
[STFT] High memory usage: 523.4 MB, triggering cleanup
[STFT] Garbage collection freed 45 objects
[STFT] Memory after cleanup: 498.2 MB
```

### Queue Memory Monitoring
```
Queue Memory: RX: 45 items, ~2.34MB, STFT: 67 items, ~8.91MB, Total: ~11.25MB
```

### Process Statistics
```
STFT FPS: 45.67 frames/sec (Total: 12345 frames in 270.3s)
Current memory usage: 234.5 MB
```

## Best Practices

1. **Always use the base class** for new processes to inherit memory management
2. **Set appropriate memory thresholds** based on process requirements
3. **Monitor memory usage** in production to adjust thresholds
4. **Use explicit cleanup** for large objects and arrays
5. **Test memory behavior** during long-running operations

## Testing

To test the memory optimizations:

1. Run the spectrogram service for extended periods
2. Monitor memory usage with system tools (htop, ps)
3. Check logs for memory cleanup messages
4. Verify no memory leaks during process restarts
5. Test with multiple WebSocket clients

## Troubleshooting

### High Memory Usage
- Check if memory thresholds are appropriate for your system
- Verify that garbage collection is working
- Look for memory leaks in process-specific code

### Process Restart Issues
- Ensure old process references are properly cleaned up
- Check for zombie processes with `ps aux | grep defunct`
- Verify signal handling is working correctly

### Queue Memory Issues
- Monitor queue sizes and memory usage
- Check if data is being processed fast enough
- Verify queue cleanup is working

## Future Improvements

1. **Memory Pooling**: Implement object pools for frequently allocated objects
2. **Compression**: Add data compression for large arrays
3. **Streaming**: Implement streaming processing to reduce memory footprint
4. **Metrics**: Add Prometheus metrics for memory monitoring
5. **Alerts**: Implement memory usage alerts for production systems
