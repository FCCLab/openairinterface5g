# CPU Configuration for AMD Ryzen 9 7950X - ORAN FHI 7.2

## System Overview
- **CPU**: AMD Ryzen 9 7950X (16 cores, 1 thread per core)
- **Total CPUs**: 16 (0-15)
- **NUMA**: Single NUMA node
- **Socket**: Single socket

## CPU Allocation Strategy

| Component | CPUs | Purpose |
|-----------|------|---------|
| **XRAN DPDK usage** | 1,2,3 | DPDK control threads and XRAN library |
| **OAI ru_thread** | 4 | Radio Unit thread |
| **OAI L1_rx_thread** | 5 | L1 receive thread |
| **OAI L1_tx_thread** | 6 | L1 transmit thread |
| **OAI nr-softmodem** | 8,9,10,11 | Main OAI processing threads |
| **Kernel** | 0,7,12,13,14,15 | System kernel threads |

## Isolated vs Non-Isolated CPUs

### Isolated CPUs (for real-time applications)
- **CPUs**: 1,2,3,4,5,6,8,9,10,11
- **Purpose**: Dedicated to OAI and XRAN processing
- **Configuration**: `isolcpus=managed_irq,domain,1,2,3,4,5,6,8,9,10,11`

### Non-Isolated CPUs (for kernel)
- **CPUs**: 0,7,12,13,14,15
- **Purpose**: System kernel threads and general processing
- **Configuration**: `kthread_cpus=0,7,12,13,14,15`

## Kernel Parameters

### Core Parameters
```bash
isolcpus=managed_irq,domain,1,2,3,4,5,6,8,9,10,11
nohz_full=1,2,3,4,5,6,8,9,10,11
rcu_nocbs=1,2,3,4,5,6,8,9,10,11
kthread_cpus=0,7,12,13,14,15
```

### IOMMU Parameters
```bash
amd_iommu=on
iommu=pt
```

### Memory Parameters
```bash
hugepagesz=1G
hugepages=20
default_hugepagesz=1G
```

### Performance Parameters
```bash
mitigations=off
skew_tick=1
intel_pstate=disable
nosoftlockup
```

## OAI Configuration Mapping

When configuring OAI gNB, use these CPU assignments:

```bash
# In OAI configuration file
L1s = {
    L1_rx_thread_core = 5;
    L1_tx_thread_core = 6;
};

RUs = {
    ru_thread_core = 4;
};

fhi_72 = {
    system_core = 1;        # DPDK control threads
    io_core = 2;            # XRAN library
    worker_cores = (3);     # XRAN worker threads
};

# Thread pool for nr-softmodem
--thread-pool 8,9,10,11
```

## Files Updated

1. **`cmdline`**: Kernel boot parameters
2. **`interface_setup_ws1.sh`**: Network interface setup script
3. **`update_grub_from_cmdline.sh`**: GRUB update script

## Next Steps

1. **Update GRUB**:
   ```bash
   sudo ./update_grub_from_cmdline.sh
   ```

2. **Reboot system**:
   ```bash
   sudo reboot
   ```

3. **Verify configuration**:
   ```bash
   cat /proc/cmdline
   cat /proc/meminfo | grep -i huge
   dmesg | grep -i "iommu.*enabled"
   ```

4. **Run interface setup**:
   ```bash
   sudo ./interface_setup_ws1.sh
   ```

## Expected Results

After reboot and configuration:
- ✅ IOMMU enabled for SR-IOV support
- ✅ 20GB huge pages allocated for DPDK
- ✅ CPU isolation working properly
- ✅ VF creation successful
- ✅ DPDK binding working
- ✅ OAI ready for ORAN FHI 7.2 deployment
