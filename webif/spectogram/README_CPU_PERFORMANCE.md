# CPU Performance Optimization for Spectrogram Processing

This directory contains scripts to optimize CPU performance for real-time spectrogram processing.

## Available Scripts

### 1. `max_cpu_performance.sh` (Recommended)
**Simple one-liner using cpupower**
```bash
./max_cpu_performance.sh
```
- Uses `cpupower` command (most reliable)
- Sets performance governor and maximum frequency
- Requires sudo privileges
- Shows before/after status

### 2. `set_max_cpu_performance.sh`
**Comprehensive CPU optimization**
```bash
sudo ./set_max_cpu_performance.sh
```
- Sets performance governor for all cores
- Sets maximum frequency for all cores
- Disables CPU idle states
- Requires sudo privileges
- Shows detailed status for all cores

### 3. `set_max_cpu_performance_simple.sh`
**Diagnostic and instruction script**
```bash
./set_max_cpu_performance_simple.sh
```
- No sudo required
- Shows current CPU status
- Provides manual optimization instructions
- Good for checking current settings

## Automatic CPU Optimization

The `spectrogram.sh` script now automatically sets CPU performance mode when starting the spectrogram service:

```bash
./spectrogram.sh [options]
```

This will:
1. Set CPU governor to performance mode
2. Set maximum frequency
3. Run spectrogram on dedicated core 13 (last core)
4. Apply CPU affinity for optimal performance

## Manual Commands

If you prefer to set CPU performance manually:

```bash
# Set performance governor
sudo cpupower frequency-set -g performance

# Set maximum frequency
sudo cpupower frequency-set -f max

# Check current status
cpupower frequency-info
```

## Performance Benefits

- **Reduced Latency**: CPU runs at maximum frequency
- **Consistent Performance**: No frequency scaling delays
- **Dedicated Core**: Spectrogram runs on isolated core 13
- **Real-time Processing**: Optimized for continuous data processing

## Requirements

- `cpupower` command (usually pre-installed)
- sudo privileges for performance settings
- Linux kernel with CPU frequency scaling support

## Installation

If `cpupower` is not available:
```bash
sudo apt-get install linux-tools-common linux-tools-generic
```

## Notes

- CPU performance settings reset after reboot
- Performance mode increases power consumption
- Settings apply to all CPU cores
- Spectrogram specifically uses core 13 (last core)

## Troubleshooting

If scripts fail:
1. Check if `cpupower` is installed
2. Ensure sudo privileges
3. Verify CPU frequency scaling is enabled
4. Check kernel support for performance governor

## Current Status

The spectrogram service now automatically:
- ✅ Sets CPU to performance mode
- ✅ Runs on dedicated core 13
- ✅ Uses maximum CPU frequency
- ✅ Applies CPU affinity optimization
