#!/bin/bash

# PTP Master Setup Script for O-RAN FHI 7.2
# This script configures PTP hardware timestamping in master mode

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration variables
INTERFACE="enp1s0f0"
PTP_CONFIG_FILE="/etc/ptp4l.conf"
LOCAL_PTP_CONFIG="ptp4l.conf.master"

# Logging function
log() {
    echo -e "${BLUE}[$(date +'%Y-%m-%d %H:%M:%S')]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[$(date +'%Y-%m-%d %H:%M:%S')] SUCCESS:${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[$(date +'%Y-%m-%d %H:%M:%S')] WARNING:${NC} $1"
}

log_error() {
    echo -e "${RED}[$(date +'%Y-%m-%d %H:%M:%S')] ERROR:${NC} $1"
}

# Function to check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        log_error "This script must be run as root (use sudo)"
        exit 1
    fi
}

# Function to install PTP packages
install_ptp_packages() {
    log "Installing PTP packages..."
    
    # Update package list
    apt update
    
    # Install linuxptp package
    if apt install -y linuxptp; then
        log_success "linuxptp package installed"
    else
        log_error "Failed to install linuxptp package"
        exit 1
    fi
    
    # Install additional tools
    apt install -y ethtool chrony
    log_success "Additional PTP tools installed"
}

# Function to check hardware timestamping support
check_hardware_timestamping() {
    log "Checking hardware timestamping support for $INTERFACE"
    
    # Check if interface exists
    if ! ip link show "$INTERFACE" >/dev/null 2>&1; then
        log_error "Interface $INTERFACE does not exist"
        exit 1
    fi
    
    # Check hardware timestamping capabilities
    if ethtool -T "$INTERFACE" 2>/dev/null | grep -q "hardware-transmit"; then
        log_success "Hardware timestamping supported on $INTERFACE"
    else
        log_warning "Hardware timestamping may not be supported on $INTERFACE"
    fi
    
    # Check PTP hardware clock
    if [ -d "/sys/class/ptp" ]; then
        log_success "PTP hardware clock detected"
        ls -la /sys/class/ptp/
    else
        log_warning "No PTP hardware clock detected"
    fi
}

# Function to copy PTP4L master configuration
copy_ptp4l_master_config() {
    log "Copying PTP4L master configuration file"
    
    # Get the directory where this script is located
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    LOCAL_CONFIG_PATH="$SCRIPT_DIR/$LOCAL_PTP_CONFIG"
    
    # Check if local config file exists
    if [ ! -f "$LOCAL_CONFIG_PATH" ]; then
        log_error "Local PTP configuration file not found: $LOCAL_CONFIG_PATH"
        log "Please ensure $LOCAL_PTP_CONFIG exists in the same directory as this script"
        exit 1
    fi
    
    # Copy the configuration file
    cp "$LOCAL_CONFIG_PATH" "$PTP_CONFIG_FILE"
    
    # Replace the placeholder interface name with the actual interface
    sed -i "s/your_PTP_ENABLED_NIC/$INTERFACE/g" "$PTP_CONFIG_FILE"
    
    log_success "PTP4L master configuration copied from $LOCAL_CONFIG_PATH to $PTP_CONFIG_FILE"
    log "Interface placeholder replaced with: $INTERFACE"
}

# Function to setup systemd service files for master
setup_systemd_master_services() {
    log "Setting up systemd service files for PTP master"
    
    # Get the directory where this script is located
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    
    # Create sysconfig directory if it doesn't exist
    mkdir -p /etc/sysconfig
    
    # Copy ptp4l sysconfig file
    if [ -f "$SCRIPT_DIR/ptp4l_sysconfig" ]; then
        log "Copying ptp4l sysconfig file"
        cp "$SCRIPT_DIR/ptp4l_sysconfig" /etc/sysconfig/ptp4l
        log_success "ptp4l sysconfig file created: /etc/sysconfig/ptp4l"
    else
        log_error "Required ptp4l_sysconfig file not found: $SCRIPT_DIR/ptp4l_sysconfig"
        exit 1
    fi
    
    # Copy phc2sys sysconfig file
    if [ -f "$SCRIPT_DIR/phc2sys_sysconfig" ]; then
        log "Copying phc2sys sysconfig file"
        cp "$SCRIPT_DIR/phc2sys_sysconfig" /etc/sysconfig/phc2sys
        log_success "phc2sys sysconfig file created: /etc/sysconfig/phc2sys"
    else
        log_error "Required phc2sys_sysconfig file not found: $SCRIPT_DIR/phc2sys_sysconfig"
        exit 1
    fi
    
    # Copy ptp4l service file
    LOCAL_SERVICE_PATH="$SCRIPT_DIR/ptp4l.service"
    if [ -f "$LOCAL_SERVICE_PATH" ]; then
        log "Using existing ptp4l.service file from $LOCAL_SERVICE_PATH"
        cp "$LOCAL_SERVICE_PATH" /etc/systemd/system/ptp4l.service
    else
        log_error "Required ptp4l.service file not found: $LOCAL_SERVICE_PATH"
        exit 1
    fi
    
    # Copy phc2sys service file
    LOCAL_PHC2SYS_SERVICE_PATH="$SCRIPT_DIR/phc2sys.service"
    if [ -f "$LOCAL_PHC2SYS_SERVICE_PATH" ]; then
        log "Using existing phc2sys.service file from $LOCAL_PHC2SYS_SERVICE_PATH"
        cp "$LOCAL_PHC2SYS_SERVICE_PATH" /etc/systemd/system/phc2sys.service
    else
        log_error "Required phc2sys.service file not found: $LOCAL_PHC2SYS_SERVICE_PATH"
        exit 1
    fi

    # Reload systemd
    systemctl daemon-reload
    
    log_success "Systemd service files and sysconfig files setup completed"
}

# Function to start PTP master services
start_ptp_master_services() {
    log "Starting PTP master services"
    
    # Stop any existing PTP services
    systemctl stop ptp4l.service 2>/dev/null || true
    systemctl stop phc2sys.service 2>/dev/null || true
    
    # Enable and start PTP4L
    systemctl enable ptp4l.service
    systemctl start ptp4l.service
    
    # Enable and start PHC2SYS
    systemctl enable phc2sys.service
    systemctl start phc2sys.service
    
    # Check service status
    sleep 2
    systemctl status ptp4l.service --no-pager -l
    systemctl status phc2sys.service --no-pager -l
    
    log_success "PTP master services started"
}

# Function to display PTP master status
display_ptp_master_status() {
    log "PTP Master Status Information:"
    echo ""
    
    # Check PTP4L master status
    echo "=== PTP4L Service Status ==="
    systemctl status ptp4l.service --no-pager -l | head -10
    
    echo ""
    echo "=== PHC2SYS Service Status ==="
    systemctl status phc2sys.service --no-pager -l | head -10
    
    echo ""
    echo "=== PTP Hardware Clock Status ==="
    if [ -d "/sys/class/ptp" ]; then
        for ptp in /sys/class/ptp/ptp*; do
            if [ -d "$ptp" ]; then
                echo "PTP Device: $(basename $ptp)"
                cat "$ptp/clock_name" 2>/dev/null || echo "Clock name not available"
            fi
        done
    else
        echo "No PTP hardware clocks found"
    fi
    
    echo ""
    echo "=== Interface Timestamping Status ==="
    ethtool -T "$INTERFACE" 2>/dev/null | grep -E "(SOF|SOT|hardware)" || echo "Timestamping info not available"
    
    echo ""
    echo "=== PTP Master Configuration ==="
    echo "Domain Number: 24"
    echo "Priority1: 128"
    echo "Interface: $INTERFACE"
    echo "Master Mode: Enabled"
}

# Main execution
main() {
    log "Starting PTP Master Setup for O-RAN FHI 7.2"
    log "==========================================="
    
    # Check prerequisites
    check_root
    
    # Install packages
    install_ptp_packages
    
    # Check hardware support
    check_hardware_timestamping
    
    # Copy configuration files
    copy_ptp4l_master_config
    
    # Setup systemd services
    setup_systemd_master_services
    
    # Start services
    start_ptp_master_services
    
    # Display status
    display_ptp_master_status
    
    log_success "PTP master setup completed successfully!"
    log "PTP master services are now running and configured for hardware timestamping"
    log "Use 'systemctl status ptp4l' and 'systemctl status phc2sys' to monitor"
}

# Handle script arguments
case "${1:-}" in
    --help|-h)
        echo "Usage: $0 [options]"
        echo ""
        echo "Options:"
        echo "  --help, -h     Show this help message"
        echo "  --status       Show PTP master status without making changes"
        echo ""
        echo "This script sets up PTP hardware timestamping in master mode for O-RAN FHI 7.2"
        echo "Configuration based on ORAN FHI 7.2 Tutorial requirements:"
        echo "  - Domain Number: 24"
        echo "  - Priority1: 128"
        echo "  - Master Mode: Enabled"
        echo "  - Hardware Timestamping: Enabled"
        exit 0
        ;;
    --status)
        check_root
        display_ptp_master_status
        exit 0
        ;;
    "")
        main
        ;;
    *)
        log_error "Unknown option: $1"
        echo "Use --help for usage information"
        exit 1
        ;;
esac
