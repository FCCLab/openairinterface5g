#!/bin/bash

# CPU Configuration Script for ORAN FHI 7.2
# Sets up realtime profile and CPU power management
# Based on delta-oru CPU allocation

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging function
log() {
    local message="[$(date +'%Y-%m-%d %H:%M:%S')] $1"
    echo -e "${BLUE}${message}${NC}"
}

log_success() {
    local message="[$(date +'%Y-%m-%d %H:%M:%S')] SUCCESS: $1"
    echo -e "${GREEN}${message}${NC}"
}

log_warning() {
    local message="[$(date +'%Y-%m-%d %H:%M:%S')] WARNING: $1"
    echo -e "${YELLOW}${message}${NC}"
}

log_error() {
    local message="[$(date +'%Y-%m-%d %H:%M:%S')] ERROR: $1"
    echo -e "${RED}${message}${NC}"
}

# Function to check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        log_error "This script must be run as root (use sudo)"
        exit 1
    fi
}

# Function to check if tuned is installed
check_tuned() {
    if ! command -v tuned-adm >/dev/null 2>&1; then
        log_error "tuned-adm is not installed. Please install it first:"
        log "sudo apt install tuned"
        exit 1
    fi
}

# Function to check if cpupower is installed
check_cpupower() {
    if ! command -v cpupower >/dev/null 2>&1; then
        log_error "cpupower is not installed. Please install it first:"
        log "sudo apt install linux-tools-common linux-tools-generic"
        exit 1
    fi
}

# Function to setup realtime variables
setup_realtime_variables() {
    log "Setting up realtime variables configuration"
    
    local script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    local realtime_vars="$script_dir/realtime-variables.conf"
    local system_vars="/etc/tuned/realtime-variables.conf"
    
    # Check if realtime-variables.conf exists in script directory
    if [ ! -f "$realtime_vars" ]; then
        log_error "realtime-variables.conf not found in script directory: $realtime_vars"
        log_error "This file is required for the realtime profile to work"
        exit 1
    fi
    
    # Copy the file - fail if copy fails
    log "Copying realtime-variables.conf to system location"
    if ! cp "$realtime_vars" "$system_vars"; then
        log_error "Failed to copy realtime-variables.conf to $system_vars"
        log_error "Make sure you have write permissions to /etc/tuned/"
        exit 1
    fi
    
    # Verify the file was copied successfully
    if [ ! -f "$system_vars" ]; then
        log_error "File copy verification failed: $system_vars does not exist"
        exit 1
    fi
    
    log_success "Realtime variables configuration installed successfully"
}

# Function to apply realtime profile
apply_realtime_profile() {
    log "Applying realtime profile"
    
    # Stop any existing tuned profile
    log "Stopping current tuned profile"
    tuned-adm off 2>/dev/null || true
    
    # Apply realtime profile
    log "Applying realtime profile"
    if tuned-adm profile realtime; then
        log_success "Realtime profile applied successfully"
    else
        log_error "Failed to apply realtime profile"
        log "Make sure realtime-variables.conf is properly configured"
        exit 1
    fi
}

# Function to configure CPU power management
configure_cpu_power() {
    log "Configuring CPU power management"
    
    # Disable CPU idle states
    log "Disabling CPU idle states"
    if cpupower idle-set -D 0; then
        log_success "CPU idle states disabled"
    else
        log_warning "Failed to disable CPU idle states"
    fi
    
    # Set CPU governor to performance
    log "Setting CPU governor to performance"
    if cpupower frequency-set -g performance; then
        log_success "CPU governor set to performance"
    else
        log_warning "Failed to set CPU governor to performance"
    fi
}

# Function to display current CPU configuration
display_cpu_config() {
    log "Current CPU configuration:"
    echo ""
    
    # Show current tuned profile
    log "Current tuned profile:"
    tuned-adm active 2>/dev/null || echo "No active profile"
    echo ""
    
    # Show CPU frequency info
    log "CPU frequency information:"
    cpupower frequency-info 2>/dev/null | grep -E "(current|governor)" || echo "Frequency info not available"
    echo ""
    
    # Show CPU idle info
    log "CPU idle information:"
    cpupower idle-info 2>/dev/null | head -10 || echo "Idle info not available"
    echo ""
    
    # Show isolated cores
    log "Isolated cores:"
    if [ -f "/sys/devices/system/cpu/isolated" ]; then
        cat /sys/devices/system/cpu/isolated 2>/dev/null || echo "No isolated cores"
    else
        echo "Isolated cores not available"
    fi
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "CPU Configuration Script for ORAN FHI 7.2"
    echo "Sets up realtime profile and CPU power management"
    echo ""
    echo "OPTIONS:"
    echo "  -h, --help          Show this help message"
    echo "  -s, --status        Show current CPU configuration status"
    echo "  -r, --reset         Reset to default configuration"
    echo ""
    echo "EXAMPLES:"
    echo "  $0                  # Apply realtime configuration"
    echo "  $0 --status         # Show current status"
    echo "  $0 --reset          # Reset to default"
}

# Function to reset configuration
reset_configuration() {
    log "Resetting CPU configuration to default"
    
    # Stop tuned profile
    log "Stopping tuned profile"
    if ! tuned-adm off 2>/dev/null; then
        log_warning "Failed to stop tuned profile (may not be active)"
    fi
    
    # Remove realtime-variables.conf if it exists
    local system_vars="/etc/tuned/realtime-variables.conf"
    if [ -f "$system_vars" ]; then
        log "Removing realtime-variables.conf"
        if ! rm "$system_vars"; then
            log_error "Failed to remove $system_vars"
            exit 1
        fi
        log_success "Realtime variables configuration removed"
    fi
    
    # Reset CPU power settings
    log "Resetting CPU power settings"
    if ! cpupower idle-set -E 2>/dev/null; then
        log_warning "Failed to reset CPU idle settings"
    fi
    
    if ! cpupower frequency-set -g ondemand 2>/dev/null; then
        log_warning "Failed to reset CPU governor to ondemand"
    fi
    
    log_success "CPU configuration reset to default"
}

# Main execution
main() {
    local show_status=false
    local reset_config=false
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_usage
                exit 0
                ;;
            -s|--status)
                show_status=true
                shift
                ;;
            -r|--reset)
                reset_config=true
                shift
                ;;
            *)
                log_error "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
    
    # Check if status is requested
    if [ "$show_status" = true ]; then
        display_cpu_config
        exit 0
    fi
    
    # Check if reset is requested
    if [ "$reset_config" = true ]; then
        check_root
        reset_configuration
        exit 0
    fi
    
    # Main configuration process
    log "Starting CPU configuration for ORAN FHI 7.2"
    log "============================================="
    
    # Check prerequisites
    check_root
    check_tuned
    check_cpupower
    
    # Setup and apply configuration
    setup_realtime_variables
    apply_realtime_profile
    configure_cpu_power
    
    # Display final status
    echo ""
    display_cpu_config
    
    log_success "CPU configuration completed successfully!"
    log "Realtime profile is now active with isolated cores for ORAN FHI 7.2"
}

# Execute main function with all arguments
main "$@"