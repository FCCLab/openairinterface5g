#!/bin/bash

# Script to set maximum CPU performance for spectrogram processing
# This script sets CPU governor to performance mode and maximum frequency

echo "Setting maximum CPU performance for spectrogram processing..."

# Function to set CPU governor to performance mode
set_performance_governor() {
    echo "Setting CPU governor to performance mode..."
    
    # Get number of CPU cores
    CPU_COUNT=$(nproc)
    echo "Detected $CPU_COUNT CPU cores"
    
    # Set governor to performance for all cores
    for ((i=0; i<CPU_COUNT; i++)); do
        if [ -f "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor" ]; then
            echo "performance" | sudo tee "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor" > /dev/null
            if [ $? -eq 0 ]; then
                echo "  Core $i: Set to performance mode"
            else
                echo "  Core $i: Failed to set performance mode (may require sudo)"
            fi
        else
            echo "  Core $i: No scaling governor available"
        fi
    done
}

# Function to set maximum frequency
set_max_frequency() {
    echo "Setting CPU frequency to maximum..."
    
    CPU_COUNT=$(nproc)
    
    for ((i=0; i<CPU_COUNT; i++)); do
        if [ -f "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_available_frequencies" ]; then
            # Get the highest available frequency
            MAX_FREQ=$(cat "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_available_frequencies" | tr ' ' '\n' | sort -n | tail -1)
            
            if [ ! -z "$MAX_FREQ" ]; then
                echo "$MAX_FREQ" | sudo tee "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_setspeed" > /dev/null
                if [ $? -eq 0 ]; then
                    echo "  Core $i: Set to ${MAX_FREQ}Hz"
                else
                    echo "  Core $i: Failed to set frequency (may require sudo)"
                fi
            fi
        else
            echo "  Core $i: No frequency scaling available"
        fi
    done
}

# Function to disable CPU power saving features
disable_power_saving() {
    echo "Disabling CPU power saving features..."
    
    # Disable CPU idle states (requires root)
    if [ -d "/sys/devices/system/cpu/intel_idle" ]; then
        echo "Disabling Intel CPU idle states..."
        for state in /sys/devices/system/cpu/intel_idle/state*/disable; do
            if [ -f "$state" ]; then
                echo "1" | sudo tee "$state" > /dev/null 2>&1
            fi
        done
    fi
    
    # Set CPU scaling minimum to maximum for performance
    CPU_COUNT=$(nproc)
    for ((i=0; i<CPU_COUNT; i++)); do
        if [ -f "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_available_frequencies" ]; then
            MAX_FREQ=$(cat "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_available_frequencies" | tr ' ' '\n' | sort -n | tail -1)
            if [ ! -z "$MAX_FREQ" ]; then
                echo "$MAX_FREQ" | sudo tee "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_min_freq" > /dev/null 2>&1
            fi
        fi
    done
}

# Function to show current CPU status
show_cpu_status() {
    echo "Current CPU status:"
    echo "==================="
    
    CPU_COUNT=$(nproc)
    for ((i=0; i<CPU_COUNT; i++)); do
        if [ -f "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor" ]; then
            GOVERNOR=$(cat "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_governor")
            CURRENT_FREQ=$(cat "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_cur_freq" 2>/dev/null || echo "N/A")
            MAX_FREQ=$(cat "/sys/devices/system/cpu/cpu$i/cpufreq/scaling_max_freq" 2>/dev/null || echo "N/A")
            
            echo "  Core $i: Governor=$GOVERNOR, Current=${CURRENT_FREQ}Hz, Max=${MAX_FREQ}Hz"
        else
            echo "  Core $i: No frequency scaling available"
        fi
    done
}

# Function to check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        echo "Warning: Some operations require root privileges (sudo)"
        echo "You may need to run this script with sudo for full functionality"
        echo ""
    fi
}

# Main execution
main() {
    check_root
    
    echo "=== CPU Performance Optimization for Spectrogram ==="
    echo ""
    
    # Show initial status
    echo "Initial CPU status:"
    show_cpu_status
    echo ""
    
    # Apply performance settings
    set_performance_governor
    echo ""
    
    set_max_frequency
    echo ""
    
    disable_power_saving
    echo ""
    
    # Show final status
    echo "Final CPU status:"
    show_cpu_status
    echo ""
    
    echo "=== Performance Optimization Complete ==="
    echo "CPU is now configured for maximum performance"
    echo "Note: These settings will reset after reboot"
    echo ""
    
    # Show the last core that will be used for spectrogram
    LAST_CORE=$(($(nproc) - 1))
    echo "Spectrogram will run on dedicated core: $LAST_CORE"
    echo "Core $LAST_CORE status:"
    if [ -f "/sys/devices/system/cpu/cpu$LAST_CORE/cpufreq/scaling_governor" ]; then
        GOVERNOR=$(cat "/sys/devices/system/cpu/cpu$LAST_CORE/cpufreq/scaling_governor")
        CURRENT_FREQ=$(cat "/sys/devices/system/cpu/cpu$LAST_CORE/cpufreq/scaling_cur_freq" 2>/dev/null || echo "N/A")
        echo "  Governor: $GOVERNOR"
        echo "  Current Frequency: ${CURRENT_FREQ}Hz"
    fi
}

# Run main function
main
