#!/bin/bash

# Simple script to set maximum CPU performance for spectrogram processing
# This version can be run without sudo for basic performance settings

echo "Setting maximum CPU performance for spectrogram processing..."

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

# Function to check available governors
check_available_governors() {
    echo "Available CPU governors:"
    if [ -f "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors" ]; then
        cat "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors"
    else
        echo "No governor information available"
    fi
}

# Function to check available frequencies
check_available_frequencies() {
    echo "Available CPU frequencies:"
    if [ -f "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies" ]; then
        cat "/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies"
    else
        echo "No frequency information available"
    fi
}

# Function to provide instructions for manual optimization
provide_instructions() {
    echo ""
    echo "=== Manual CPU Performance Optimization Instructions ==="
    echo ""
    echo "To set maximum CPU performance, you can run these commands:"
    echo ""
    echo "1. Set CPU governor to performance mode (requires sudo):"
    echo "   sudo cpupower frequency-set -g performance"
    echo ""
    echo "2. Set maximum frequency (requires sudo):"
    echo "   sudo cpupower frequency-set -f max"
    echo ""
    echo "3. Or manually set each core (requires sudo):"
    echo "   for i in {0..$(($(nproc)-1))}; do"
    echo "     sudo echo performance > /sys/devices/system/cpu/cpu\$i/cpufreq/scaling_governor"
    echo "   done"
    echo ""
    echo "4. Check if cpupower is available:"
    echo "   which cpupower"
    echo ""
    echo "5. Install cpupower if not available:"
    echo "   sudo apt-get install linux-tools-common linux-tools-generic"
    echo ""
}

# Main execution
main() {
    echo "=== CPU Performance Check for Spectrogram ==="
    echo ""
    
    # Show current status
    show_cpu_status
    echo ""
    
    # Check available options
    check_available_governors
    echo ""
    
    check_available_frequencies
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
        
        if [ "$GOVERNOR" != "performance" ]; then
            echo "  ⚠️  Warning: Core $LAST_CORE is not in performance mode"
            echo "  💡 Tip: Run with sudo for automatic optimization"
        else
            echo "  ✅ Core $LAST_CORE is already in performance mode"
        fi
    fi
    echo ""
    
    # Provide instructions
    provide_instructions
}

# Run main function
main
