#!/bin/bash

# ORAN FHI 7.2 Interface Setup Script for Physical Function (PF)
# This script configures network interfaces and DPDK Physical Functions for O-RAN Fronthaul Interface
# Based on the ORAN FHI 7.2 Tutorial requirements
# Updated to use Physical Function instead of Virtual Functions due to Intel X710 VF queue issues

set -e  # Exit on any error

# Default configuration variables
DEFAULT_IF_NAME="enp1s0f0"
DEFAULT_MAX_RING_BUFFER_SIZE=4096
DEFAULT_MTU=9600
DEFAULT_DU_U_PLANE_MAC_ADD="00:1b:21:b9:45:e8"  # PF MAC address
DEFAULT_DU_C_PLANE_MAC_ADD="00:1b:21:b9:45:e8"  # Same PF for both planes
DEFAULT_VLAN=3
DEFAULT_DRIVER="vfio_pci"
DEFAULT_USE_PF=true  # Use Physical Function instead of VFs

# Initialize configuration variables with defaults
IF_NAME="$DEFAULT_IF_NAME"
MAX_RING_BUFFER_SIZE="$DEFAULT_MAX_RING_BUFFER_SIZE"
MTU="$DEFAULT_MTU"
DU_U_PLANE_MAC_ADD="$DEFAULT_DU_U_PLANE_MAC_ADD"
DU_C_PLANE_MAC_ADD="$DEFAULT_DU_C_PLANE_MAC_ADD"
VLAN="$DEFAULT_VLAN"
DRIVER="$DEFAULT_DRIVER"
USE_PF="$DEFAULT_USE_PF"

# CPU Configuration based on your setup:
# XRAN DPDK usage: CPUs 1,2,3
# OAI ru_thread: CPU 4
# OAI L1_rx_thread: CPU 5
# OAI L1_tx_thread: CPU 6
# OAI nr-softmodem: CPUs 8,9,10,11
# Kernel: CPUs 12,13,14,15

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Log file setup
LOG_FILE="interface_setup.log"

# Function to initialize log file
init_log_file() {
    # Create/clear log file with header
    cat > "$LOG_FILE" << EOF
========================================
ORAN FHI 7.2 Interface Setup Log
Started: $(date)
Command: $0 $*
========================================

EOF
    log "Log file initialized: $LOG_FILE"
}

# Logging function
log() {
    local message="[$(date +'%Y-%m-%d %H:%M:%S')] $1"
    echo -e "${BLUE}${message}${NC}"
    echo "$message" >> "$LOG_FILE"
}

log_success() {
    local message="[$(date +'%Y-%m-%d %H:%M:%S')] SUCCESS: $1"
    echo -e "${GREEN}${message}${NC}"
    echo "$message" >> "$LOG_FILE"
}

log_warning() {
    local message="[$(date +'%Y-%m-%d %H:%M:%S')] WARNING: $1"
    echo -e "${YELLOW}${message}${NC}"
    echo "$message" >> "$LOG_FILE"
}

log_error() {
    local message="[$(date +'%Y-%m-%d %H:%M:%S')] ERROR: $1"
    echo -e "${RED}${message}${NC}"
    echo "$message" >> "$LOG_FILE"
}

# Function to display usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "ORAN FHI 7.2 Interface Setup Script"
    echo "Configures network interfaces and DPDK Physical Functions for O-RAN Fronthaul Interface"
    echo ""
    echo "OPTIONS:"
    echo "  -i, --interface INTERFACE    Network interface name (default: $DEFAULT_IF_NAME)"
    echo "  -m, --mtu MTU               MTU size (default: $DEFAULT_MTU)"
    echo "  -r, --ring-buffer SIZE      Ring buffer size (default: $DEFAULT_MAX_RING_BUFFER_SIZE)"
    echo "  -v, --vlan VLAN_ID          VLAN ID (default: $DEFAULT_VLAN)"
    echo "  -u, --uplane-mac MAC        U-plane MAC address (default: $DEFAULT_DU_U_PLANE_MAC_ADD)"
    echo "  -c, --cplane-mac MAC        C-plane MAC address (default: $DEFAULT_DU_C_PLANE_MAC_ADD)"
    echo "  -d, --driver DRIVER         DPDK driver (default: $DEFAULT_DRIVER)"
    echo "  --use-pf                    Use Physical Function instead of VFs (default: true)"
    echo "  -l, --log-file FILE         Log file path (default: interface_setup.log)"
    echo "  -h, --help                  Show this help message"
    echo "  --restore                   Restore interface to default configuration"
    echo ""
    echo "EXAMPLES:"
    echo "  $0                                    # Use all default values"
    echo "  $0 -i enp1s0f0 -m 9600               # Custom interface and MTU"
    echo "  $0 --interface enp1s0f0 --use-pf     # Custom interface with PF"
    echo "  $0 -l /var/log/interface_setup.log   # Custom log file location"
    echo "  $0 --restore                          # Restore interface to defaults"
    echo ""
    echo "DEFAULT VALUES:"
    echo "  Interface: $DEFAULT_IF_NAME"
    echo "  MTU: $DEFAULT_MTU"
    echo "  Ring Buffer: $DEFAULT_MAX_RING_BUFFER_SIZE"
    echo "  VLAN: $DEFAULT_VLAN"
    echo "  U-plane MAC: $DEFAULT_DU_U_PLANE_MAC_ADD"
    echo "  C-plane MAC: $DEFAULT_DU_C_PLANE_MAC_ADD"
    echo "  Driver: $DEFAULT_DRIVER"
    echo "  Use Physical Function: $DEFAULT_USE_PF"
}

# Function to parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -i|--interface)
                IF_NAME="$2"
                shift 2
                ;;
            -m|--mtu)
                MTU="$2"
                shift 2
                ;;
            -r|--ring-buffer)
                MAX_RING_BUFFER_SIZE="$2"
                shift 2
                ;;
            -v|--vlan)
                VLAN="$2"
                shift 2
                ;;
            -u|--uplane-mac)
                DU_U_PLANE_MAC_ADD="$2"
                shift 2
                ;;
            -c|--cplane-mac)
                DU_C_PLANE_MAC_ADD="$2"
                shift 2
                ;;
            -d|--driver)
                DRIVER="$2"
                shift 2
                ;;
            --use-pf)
                USE_PF=true
                shift
                ;;
            -l|--log-file)
                LOG_FILE="$2"
                shift 2
                ;;
            -h|--help)
                show_usage
                exit 0
                ;;
            --restore)
                restore_script="/tmp/restore_interface_${IF_NAME}.sh"
                if [ -f "$restore_script" ]; then
                    log "Running restore script..."
                    bash "$restore_script"
                else
                    log_error "Restore script not found: $restore_script"
                    exit 1
                fi
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
}

# Function to check if running as root
check_root() {
    if [[ $EUID -ne 0 ]]; then
        log_error "This script must be run as root (use sudo)"
        exit 1
    fi
}

# Function to check if interface exists
check_interface() {
    if ! ip link show "$IF_NAME" >/dev/null 2>&1; then
        log_error "Interface $IF_NAME does not exist"
        log "Available interfaces:"
        ip link show | grep -E "^[0-9]+:" | awk -F': ' '{print $2}' | grep -v lo
        exit 1
    fi
    log_success "Interface $IF_NAME found"
}

# Function to get current interface status
get_interface_info() {
    log "Current interface information for $IF_NAME:"
    echo "  - Status: $(ip link show $IF_NAME | grep -o 'state [A-Z]*' | cut -d' ' -f2)"
    echo "  - MTU: $(ip link show $IF_NAME | grep -o 'mtu [0-9]*' | cut -d' ' -f2)"
    echo "  - MAC: $(ip link show $IF_NAME | grep -o 'link/ether [a-f0-9:]*' | cut -d' ' -f2)"
    
    # Get ring buffer info
    if command -v ethtool >/dev/null 2>&1; then
        echo "  - Ring buffers:"
        ethtool -g "$IF_NAME" 2>/dev/null | grep -E "(RX:|TX:)" | head -4
    fi
}

# Function to set maximum ring buffers
set_ring_buffers() {
    log "Setting maximum ring buffers for $IF_NAME to $MAX_RING_BUFFER_SIZE"
    
    if ! command -v ethtool >/dev/null 2>&1; then
        log_error "ethtool is not installed. Please install it first."
        exit 1
    fi
    
    # Get current ring buffer sizes
    current_rx=$(ethtool -g "$IF_NAME" 2>/dev/null | grep "RX:" | head -1 | awk '{print $2}')
    current_tx=$(ethtool -g "$IF_NAME" 2>/dev/null | grep "TX:" | head -1 | awk '{print $2}')
    
    log "Current RX ring buffer: $current_rx, TX ring buffer: $current_tx"
    
    # Set ring buffers
    if ethtool -G "$IF_NAME" rx "$MAX_RING_BUFFER_SIZE" tx "$MAX_RING_BUFFER_SIZE" 2>/dev/null; then
        log_success "Ring buffers set to $MAX_RING_BUFFER_SIZE"
    else
        log_warning "Failed to set ring buffers. This might be normal if already at maximum."
    fi
}

# Function to set MTU
set_mtu() {
    log "Setting MTU for $IF_NAME to $MTU"
    
    current_mtu=$(ip link show "$IF_NAME" | grep -o 'mtu [0-9]*' | cut -d' ' -f2)
    log "Current MTU: $current_mtu"
    
    if ip link set "$IF_NAME" mtu "$MTU"; then
        log_success "MTU set to $MTU"
    else
        log_error "Failed to set MTU to $MTU"
        exit 1
    fi
}

# Function to configure Physical Function
configure_pf() {
    log "Configuring Physical Function (PF) for $IF_NAME"
    
    # Get the current MAC address of the PF
    current_mac=$(ip link show "$IF_NAME" | grep -o 'link/ether [a-f0-9:]*' | cut -d' ' -f2)
    log "Current PF MAC address: $current_mac"
    
    # Update the MAC address variables with the actual PF MAC
    DU_U_PLANE_MAC_ADD="$current_mac"
    DU_C_PLANE_MAC_ADD="$current_mac"
    
    log_success "PF configured with MAC: $current_mac"
    log "Note: Using Physical Function instead of Virtual Functions to avoid Intel X710 VF queue issues"
}

# Function to get PF PCI address
get_pf_pci_address() {
    log "Getting Physical Function PCI address"
    
    # Get the PCI address of the physical interface
    PF_PCI_ADDRESS=$(readlink -f "/sys/class/net/$IF_NAME/device" | sed 's/.*\///')
    log "Physical Function PCI address: $PF_PCI_ADDRESS"
    
    # Set both U-plane and C-plane to use the same PF
    U_PLANE_PCI_BUS_ADD="$PF_PCI_ADDRESS"
    C_PLANE_PCI_BUS_ADD="$PF_PCI_ADDRESS"
    
    log_success "PF PCI address found: $PF_PCI_ADDRESS"
}

# Function to bind PF to DPDK
bind_pf_to_dpdk() {
    log "Binding Physical Function to DPDK driver $DRIVER"
    
    # Check if dpdk-devbind.py exists
    if [ ! -f "/usr/local/bin/dpdk-devbind.py" ]; then
        log_error "dpdk-devbind.py not found at /usr/local/bin/dpdk-devbind.py"
        log "Please ensure DPDK is properly installed"
        exit 1
    fi
    
    # Load the driver
    log "Loading driver $DRIVER"
    if ! modprobe "$DRIVER"; then
        log_error "Failed to load driver $DRIVER"
        exit 1
    fi
    
    # Get the device ID for the PF
    device_id=$(lspci -n -s "$PF_PCI_ADDRESS" | cut -d' ' -f3)
    log "PF device ID: $device_id"
    
    # Add device ID to vfio-pci if needed
    if [ "$DRIVER" = "vfio-pci" ]; then
        log "Adding device ID $device_id to vfio-pci driver"
        echo "$device_id" | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id >/dev/null 2>&1 || true
    fi
    
    # Unbind PF from current driver
    log "Unbinding PF from current driver"
    /usr/local/bin/dpdk-devbind.py --unbind "$PF_PCI_ADDRESS" 2>/dev/null || true
    
    # Bind PF to DPDK driver
    log "Binding PF $PF_PCI_ADDRESS to $DRIVER"
    if /usr/local/bin/dpdk-devbind.py --bind "$DRIVER" "$PF_PCI_ADDRESS"; then
        log_success "PF bound to $DRIVER"
    else
        log_error "Failed to bind PF to $DRIVER"
        exit 1
    fi
}

# Function to verify DPDK binding
verify_dpdk_binding() {
    log "Verifying DPDK binding"
    
    # Check DPDK device status
    /usr/local/bin/dpdk-devbind.py --status | grep -E "($PF_PCI_ADDRESS)"
    
    log_success "DPDK binding verification complete"
}

# Function to display configuration summary
display_summary() {
    log "Configuration Summary:"
    log "  - Interface: $IF_NAME"
    log "  - MTU: $MTU"
    log "  - Ring Buffer Size: $MAX_RING_BUFFER_SIZE"
    log "  - VLAN: $VLAN"
    log "  - Use Physical Function: $USE_PF"
    log "  - CU-plane MAC: $DU_U_PLANE_MAC_ADD"
    log "  - CU-plane PCI: $U_PLANE_PCI_BUS_ADD"
    log "  - DPDK Driver: $DRIVER"
    log ""
    log "Use these parameters in your OAI configuration file:"
    log "  dpdk_devices = (\"$U_PLANE_PCI_BUS_ADD\");"
    log "  ru_addr = (\"$DU_U_PLANE_MAC_ADD\");"
}

# Function to create a restore script
create_restore_script() {
    local restore_script="/tmp/restore_interface_${IF_NAME}.sh"
    
    cat > "$restore_script" << EOF
#!/bin/bash
# Restore script for $IF_NAME
# Generated by interface_setup_ws1.sh

set -e

echo "Restoring interface $IF_NAME configuration..."

# Unbind PF from DPDK
if [ -f "/usr/local/bin/dpdk-devbind.py" ]; then
    /usr/local/bin/dpdk-devbind.py --unbind "$PF_PCI_ADDRESS" 2>/dev/null || true
    echo "PF unbound from DPDK"
fi

# Rebind PF to original driver (i40e for Intel X710)
if [ -f "/usr/local/bin/dpdk-devbind.py" ]; then
    /usr/local/bin/dpdk-devbind.py --bind i40e "$PF_PCI_ADDRESS" 2>/dev/null || true
    echo "PF rebound to i40e driver"
fi

# Reset MTU to default
ip link set "$IF_NAME" mtu 1500 2>/dev/null || true
echo "MTU reset to default"

echo "Interface $IF_NAME restored to default configuration"
EOF
    
    chmod +x "$restore_script"
    log_success "Restore script created: $restore_script"
}

# Main execution
main() {
    # Parse command line arguments
    parse_arguments "$@"
    
    # Initialize log file
    init_log_file
    
    log "Starting ORAN FHI 7.2 Interface Setup for $IF_NAME"
    log "=================================================="
    
    # Check prerequisites
    check_root
    check_interface
    
    # Display current interface information
    get_interface_info
    echo ""
    
    # Configure interface
    set_ring_buffers
    set_mtu
    
    # Configure Physical Function
    configure_pf
    get_pf_pci_address
    
    # Bind PF to DPDK
    bind_pf_to_dpdk
    verify_dpdk_binding
    
    # Create restore script
    create_restore_script
    
    # Display summary
    echo ""
    display_summary
    
    log_success "Interface setup completed successfully!"
    log "You can now use the configuration in your OAI gNB setup."
    log "To restore the interface to default settings, run: $restore_script"
}

# Execute main function with all arguments
main "$@"
