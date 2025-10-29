#!/bin/bash

# PTP Time Alignment Fix Script
# This script fixes the phc2sys "Invalid argument" error by properly aligning
# PHC and system time before starting PTP services

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
INTERFACE="enp1s0f0np0"
PTP_DEVICE="/dev/ptp0"  # Default, will be detected if different

# Logging functions
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

# Function to detect PTP device
detect_ptp_device() {
    log "Detecting PTP device for interface $INTERFACE"
    
    # Find PTP device associated with the interface
    for ptp_dev in /dev/ptp*; do
        if [ -e "$ptp_dev" ]; then
            # Check if this PTP device is associated with our interface
            ptp_num=$(basename "$ptp_dev" | sed 's/ptp//')
            if [ -d "/sys/class/ptp/ptp$ptp_num" ]; then
                # Check if this PTP device is linked to our interface
                if [ -L "/sys/class/ptp/ptp$ptp_num/device" ]; then
                    device_path=$(readlink "/sys/class/ptp/ptp$ptp_num/device")
                    if echo "$device_path" | grep -q "$INTERFACE"; then
                        PTP_DEVICE="$ptp_dev"
                        log_success "Found PTP device: $PTP_DEVICE for interface $INTERFACE"
                        return 0
                    fi
                fi
            fi
        fi
    done
    
    # Fallback: use first available PTP device
    if [ -e "/dev/ptp0" ]; then
        PTP_DEVICE="/dev/ptp0"
        log_warning "Using default PTP device: $PTP_DEVICE"
        return 0
    fi
    
    log_error "No PTP device found for interface $INTERFACE"
    return 1
}

# Function to stop conflicting time services
stop_conflicting_services() {
    log "Stopping conflicting time synchronization services"
    
    # Stop PTP services first
    systemctl stop phc2sys.service 2>/dev/null || true
    systemctl stop ptp4l.service 2>/dev/null || true
    
    # Stop other time sync services
    systemctl stop chronyd 2>/dev/null || true
    systemctl stop systemd-timesyncd 2>/dev/null || true
    systemctl stop ntpd 2>/dev/null || true
    
    log_success "Conflicting services stopped"
}

# Function to align PHC and system time
align_time() {
    log "Aligning PHC and system time"
    
    # Get current system time
    current_time=$(date '+%Y-%m-%d %H:%M:%S')
    log "Current system time: $current_time"
    
    # Set PHC to current system time
    log "Setting PHC ($PTP_DEVICE) to system time"
    if phc_ctl "$PTP_DEVICE" set; then
        log_success "PHC set to system time"
    else
        log_error "Failed to set PHC to system time"
        return 1
    fi
    
    # Verify the alignment
    sleep 1
    phc_time=$(phc_ctl "$PTP_DEVICE" get)
    log "PHC time after alignment: $phc_time"
    
    log_success "Time alignment completed"
}

# Function to copy updated configuration
copy_updated_config() {
    log "Copying updated phc2sys configuration"
    
    # Get the directory where this script is located
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    
    # Copy the updated sysconfig
    if [ -f "$SCRIPT_DIR/phc2sys_sysconfig" ]; then
        cp "$SCRIPT_DIR/phc2sys_sysconfig" /etc/sysconfig/phc2sys
        log_success "Updated phc2sys configuration copied to /etc/sysconfig/phc2sys"
    else
        log_error "phc2sys_sysconfig file not found in $SCRIPT_DIR"
        return 1
    fi
    
    # Reload systemd
    systemctl daemon-reload
    log_success "Systemd configuration reloaded"
}

# Function to start PTP services in correct order
start_ptp_services() {
    log "Starting PTP services in correct order"
    
    # Start ptp4l first
    log "Starting ptp4l service"
    systemctl start ptp4l.service
    
    # Wait for ptp4l to stabilize
    log "Waiting for ptp4l to stabilize..."
    sleep 5
    
    # Check ptp4l status
    if systemctl is-active --quiet ptp4l.service; then
        log_success "ptp4l service started successfully"
    else
        log_error "ptp4l service failed to start"
        return 1
    fi
    
    # Start phc2sys
    log "Starting phc2sys service"
    systemctl start phc2sys.service
    
    # Wait a moment and check status
    sleep 2
    if systemctl is-active --quiet phc2sys.service; then
        log_success "phc2sys service started successfully"
    else
        log_error "phc2sys service failed to start"
        return 1
    fi
}

# Function to display status
display_status() {
    log "PTP Services Status:"
    echo ""
    
    echo "=== Service Status ==="
    systemctl status ptp4l.service --no-pager -l | head -5
    echo ""
    systemctl status phc2sys.service --no-pager -l | head -5
    
    echo ""
    echo "=== Recent Logs ==="
    echo "ptp4l logs:"
    journalctl -u ptp4l.service -n 5 --no-pager
    echo ""
    echo "phc2sys logs:"
    journalctl -u phc2sys.service -n 5 --no-pager
    
    echo ""
    echo "=== PHC Status ==="
    phc_ctl "$PTP_DEVICE" get
}

# Main execution
main() {
    log "Starting PTP Time Alignment Fix"
    log "==============================="
    
    # Check prerequisites
    check_root
    
    # Detect PTP device
    detect_ptp_device
    
    # Stop conflicting services
    stop_conflicting_services
    
    # Align time
    align_time
    
    # Copy updated configuration
    copy_updated_config
    
    # Start PTP services
    start_ptp_services
    
    # Display status
    display_status
    
    log_success "PTP time alignment fix completed!"
    log "Monitor with: journalctl -u ptp4l -u phc2sys -f"
}

# Handle script arguments
case "${1:-}" in
    --help|-h)
        echo "Usage: $0 [options]"
        echo ""
        echo "Options:"
        echo "  --help, -h     Show this help message"
        echo "  --status       Show PTP status without making changes"
        echo "  --align-only   Only align time, don't restart services"
        echo ""
        echo "This script fixes phc2sys 'Invalid argument' errors by:"
        echo "  1. Stopping conflicting time services"
        echo "  2. Aligning PHC and system time"
        echo "  3. Updating phc2sys configuration"
        echo "  4. Starting PTP services in correct order"
        exit 0
        ;;
    --status)
        check_root
        detect_ptp_device
        display_status
        exit 0
        ;;
    --align-only)
        check_root
        detect_ptp_device
        stop_conflicting_services
        align_time
        log_success "Time alignment completed. Services not restarted."
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
