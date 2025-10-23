#!/bin/bash

# Script to update GRUB configuration using the cmdline file
# This script reads the cmdline file and updates GRUB accordingly

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

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

# Check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        log_error "This script must be run as root (use sudo)"
        exit 1
    fi
}

# Function to backup current GRUB configuration
backup_grub() {
    log "Creating backup of current GRUB configuration"
    if [ -f "/etc/default/grub" ]; then
        cp /etc/default/grub /etc/default/grub.backup.$(date +%Y%m%d_%H%M%S)
        log_success "GRUB configuration backed up"
    else
        log_error "GRUB configuration file not found"
        exit 1
    fi
}

# Function to read cmdline file
read_cmdline() {
    local cmdline_file="/home/fcp/openairinterface5g/delta-oru/cmdline"
    
    if [ ! -f "$cmdline_file" ]; then
        log_error "cmdline file not found: $cmdline_file"
        exit 1
    fi
    
    log "Reading cmdline from: $cmdline_file"
    CMDLINE_CONTENT=$(cat "$cmdline_file")
    log_success "cmdline content loaded"
}

# Function to update GRUB configuration
update_grub_config() {
    log "Updating GRUB configuration"
    
    # Read current GRUB configuration
    local grub_file="/etc/default/grub"
    
    # Create new GRUB configuration
    cat > "$grub_file" << EOF
# If you change this file, run 'update-grub' afterwards to update
# /boot/grub/grub.cfg.
# For full documentation of the options in this file, see:
#   info -f grub -n 'Simple configuration'

GRUB_DEFAULT=saved
GRUB_TIMEOUT_STYLE=menu
GRUB_TIMEOUT=5
GRUB_DISTRIBUTOR=\`lsb_release -i -s 2> /dev/null || echo Debian\`
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash i915.alpha_support=1"
GRUB_CMDLINE_LINUX="$CMDLINE_CONTENT"

# Uncomment to enable BadRAM filtering, modify to suit your needs
# This works with Linux (no patch required) and with any kernel that obtains
# the memory map information from GRUB (GNU Mach, kernel of FreeBSD ...)
#GRUB_BADRAM="0x01234567,0xfefefefe,0x89abcdef,0xefefefef"

# Uncomment to disable graphical terminal (grub-pc only)
#GRUB_TERMINAL=console

# The resolution used on graphical terminal
# note that you can use only modes which your graphic card supports via VBE
# you can see them in real GRUB with the command \`vbeinfo'
GRUB_GFXMODE=1920x1080

# Uncomment if you don't want GRUB to pass "root=UUID=xxx" parameter to Linux
#GRUB_DISABLE_LINUX_UUID=true

# Uncomment to disable generation of recovery mode menu entries
#GRUB_DISABLE_RECOVERY="true"

# Uncomment to get a beep at grub start
#GRUB_INIT_TUNE="480 440 1"
EOF

    log_success "GRUB configuration updated"
}

# Function to update GRUB
update_grub() {
    log "Updating GRUB bootloader"
    
    if command -v update-grub >/dev/null 2>&1; then
        update-grub
        log_success "GRUB updated successfully"
    elif command -v grub2-mkconfig >/dev/null 2>&1; then
        grub2-mkconfig -o /boot/grub2/grub.cfg
        log_success "GRUB2 updated successfully"
    else
        log_error "Neither update-grub nor grub2-mkconfig found"
        exit 1
    fi
}

# Function to set realtime kernel as default
set_realtime_default() {
    log "Setting realtime kernel as default GRUB entry"
    
    local realtime_entry="Advanced options for Ubuntu>Ubuntu, with Linux 5.15.0-1038-realtime"
    
    if command -v grub-set-default >/dev/null 2>&1; then
        if grub-set-default "$realtime_entry"; then
            log_success "Realtime kernel set as default: $realtime_entry"
        else
            log_warning "Failed to set realtime kernel as default"
        fi
    else
        log_warning "grub-set-default command not found, skipping default kernel setting"
    fi
}

# Function to verify GRUB configuration
verify_grub_config() {
    log "Verifying GRUB configuration"
    
    if command -v grub-editenv >/dev/null 2>&1; then
        log "Current GRUB saved entry:"
        grub-editenv list | grep saved_entry || log_warning "No saved entry found"
        log_success "GRUB configuration verified"
    else
        log_warning "grub-editenv command not found, skipping verification"
    fi
}

# Function to display configuration summary
display_summary() {
    log "Configuration Summary:"
    echo "  - System: AMD Ryzen 9 7950X (16 cores)"
    echo "  - Isolated CPUs: 1,2,3,4,5,6,8,9,10,11 (for OAI/XRAN)"
    echo "  - Kernel CPUs: 0,7,12,13,14,15"
    echo "  - IOMMU: AMD IOMMU and Intel IOMMU enabled with pass-through"
    echo "  - Huge Pages: 20 x 1GB pages (20GB total)"
    echo "  - SR-IOV: Enabled for network interface"
    echo ""
    log "CPU Allocation:"
    echo "  - XRAN DPDK usage: CPUs 1,2,3"
    echo "  - OAI ru_thread: CPU 4"
    echo "  - OAI L1_rx_thread: CPU 5"
    echo "  - OAI L1_tx_thread: CPU 6"
    echo "  - OAI nr-softmodem: CPUs 8,9,10,11"
    echo "  - Kernel: CPUs 0,7,12,13,14,15"
    echo ""
    log "Key parameters added:"
    echo "  - amd_iommu=on intel_iommu=on iommu=pt (AMD and Intel IOMMU support)"
    echo "  - isolcpus=1,2,3,4,5,6,8,9,10,11 (CPU isolation)"
    echo "  - hugepages=20 hugepagesz=1G (Huge pages)"
    echo "  - mitigations=off (Performance optimization)"
    echo "  - skew_tick=1 (Time synchronization)"
    echo ""
    log "GRUB Configuration:"
    echo "  - GRUB_DEFAULT=saved (Remember last boot choice)"
    echo "  - GRUB_TIMEOUT_STYLE=menu (Show boot menu)"
    echo "  - GRUB_TIMEOUT=5 (5 second timeout)"
}

# Main execution
main() {
    log "Starting GRUB update from cmdline file"
    log "======================================"
    
    # Check prerequisites
    check_root
    
    # Read cmdline file
    read_cmdline
    
    # Backup current configuration
    backup_grub
    
    # Update GRUB configuration
    update_grub_config
    
    # Update GRUB
    update_grub
    
    # Set realtime kernel as default
    set_realtime_default
    
    # Verify GRUB configuration
    verify_grub_config
    
    # Display summary
    echo ""
    display_summary
    
    log_success "GRUB update completed successfully!"
    log_warning "REBOOT REQUIRED: Please reboot the system for changes to take effect"
    log "After reboot, you can run the interface setup script:"
    log "  sudo ./interface_setup_ws1.sh"
}

# Handle script arguments
case "${1:-}" in
    --help|-h)
        echo "Usage: $0 [options]"
        echo ""
        echo "Options:"
        echo "  --help, -h     Show this help message"
        echo "  --dry-run      Show what would be done without making changes"
        echo ""
        echo "This script updates GRUB configuration using the cmdline file"
        echo "for AMD Ryzen 9 7950X system with ORAN FHI 7.2 optimization"
        echo "CPU allocation: XRAN(1,2,3), RU(4), L1_RX(5), L1_TX(6), OAI(8,9,10,11), Kernel(0,7,12,13,14,15)"
        exit 0
        ;;
    --dry-run)
        log "DRY RUN MODE - No changes will be made"
        read_cmdline
        echo "Would update GRUB_CMDLINE_LINUX with:"
        echo "$CMDLINE_CONTENT"
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
