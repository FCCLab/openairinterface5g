# ORAN FHI 7.2 Tutorial Checklist

This checklist provides a step-by-step guide for setting up and running OAI 7.2 Fronthaul Interface 5G SA Tutorial.

## Prerequisites Checklist

### Hardware Requirements
- [ ] **Server Configuration**: Intel Xeon Gold 6354 36-Core, 128GB RAM OR AMD EPYC 9374F 32-Core, 128GB RAM
- [ ] **Operating System**: RHEL 9.2 OR Ubuntu 22.04.3 LTS with realtime kernel
- [ ] **NIC Card**: Intel X710, Intel E810, or Intel XXV710 with hardware PTP timestamping support
- [ ] **CPU Generation**: Intel Ice Lake or newer, AMD 4th generation Genoa or newer
- [ ] **Clock Speed**: Higher than 3.0 GHz with `avx512` capabilities
- [ ] **PTP Enabled Switch**: CISCO C93180YC-FX3, Fibrolan Falcon-RX/812/G, or Qulsar Qg2
- [ ] **Tested RU**: VVDN LPRU, LiteON RU, Benetel 650/550, Foxconn RPQN, or Metanoia RU

### Software Requirements
- [ ] **PTP Software**: `ptp4l` version 3.1.1
- [ ] **PTP Software**: `phc2sys` version 3.1.1
- [ ] **libxran**: `oran_e_maintenance_release_v1.0` OR `oran_f_release_v1.0`
- [ ] **Compiler**: gcc-11 and g++-11 minimum

## Server Configuration Checklist

### BIOS and OS Setup
- [ ] **Disable Hyperthreading (HT)** in BIOS
- [ ] **Fresh OS Installation** (RHEL or Ubuntu)
- [ ] **Install Realtime Kernel** for your OS
- [ ] **Configure Boot Commands** via `tuned` or manually

### CPU Allocation
- [ ] **Determine NUMA Configuration**: Check if one or two NUMA nodes
- [ ] **Isolate CPUs for DPDK**: CPUs 0,2,4 for XRAN DPDK usage
- [ ] **Isolate CPU for RU Thread**: CPU 6 for `ru_thread`
- [ ] **Isolate CPU for L1 RX**: CPU 8 for `L1_rx_thread`
- [ ] **Isolate CPU for L1 TX**: CPU 10 for `L1_tx_thread`
- [ ] **Allocate CPUs for OAI**: CPUs 1,3,5,7,9,11,13,15 for `nr-softmodem`
- [ ] **Reserve CPUs for Kernel**: CPUs 16-31 (or appropriate range)

### Boot Command Configuration
- [ ] **Set `isolcpus`** with list of CPUs to isolate for XRAN
- [ ] **Set `nohz_full`** with list of CPUs to isolate for XRAN
- [ ] **Set `rcu_nocbs`** with list of CPUs to isolate for XRAN
- [ ] **Set `kthread_cpus`** with list of CPUs to isolate for kernel
- [ ] **Configure `tuned` profile** to `realtime`
- [ ] **Set maximum performance mode** in BIOS or OS
- [ ] **Disable CPU sleep state** if needed: `sudo cpupower idle-set -D 0`

## PTP Configuration Checklist

### PTP Installation
- [ ] **Install linuxptp package**: `sudo dnf install linuxptp -y` (RHEL) OR `sudo apt install linuxptp -y` (Ubuntu)

### PTP Configuration Files
- [ ] **Create `/etc/ptp4l.conf`** with correct domain number (24)
- [ ] **Create `/etc/sysconfig/ptp4l`** with options
- [ ] **Create `/etc/sysconfig/phc2sys`** with options
- [ ] **Configure ptp4l service** in `/usr/lib/systemd/system/ptp4l.service`
- [ ] **Configure phc2sys service** in `/usr/lib/systemd/system/phc2sys.service`

### PTP Debugging (if issues occur)
- [ ] **Verify `skew_tick=1`** in `/proc/cmdline`
- [ ] **Set `tx_timestamp_timeout`** to 50 or 100 for Intel E-810 cards
- [ ] **Disable other time sources** (NTP, chrony): `timedatectl set-ntp false`
- [ ] **Verify `kthread_cpus`** is set in `/proc/cmdline`
- [ ] **Pin ptp4l and phc2sys processes** to isolated CPU if needed

## DPDK Setup Checklist

### DPDK Download and Installation
- [ ] **Download DPDK 20.11.9**: `wget http://fast.dpdk.org/rel/dpdk-20.11.9.tar.xz`
- [ ] **Install build dependencies**: `sudo apt install wget xz-utils libnuma-dev` (Debian) OR `sudo dnf install wget xz numactl-devel` (Fedora/RHEL)
- [ ] **Install meson**: `sudo apt install meson` (Debian) OR `sudo dnf install meson` (Fedora/RHEL)

### DPDK Compilation
- [ ] **Extract DPDK**: `tar xvf dpdk-20.11.9.tar.xz && cd dpdk-stable-20.11.9`
- [ ] **Configure with meson**: `meson build`
- [ ] **Build DPDK**: `ninja -C build`
- [ ] **Install DPDK**: `sudo ninja install -C build`

### DPDK Verification
- [ ] **Check LD cache**: `sudo ldconfig -v | grep rte_`
- [ ] **Add to LD_LIBRARY_PATH**: Create `/etc/ld.so.conf.d/local-lib.conf` with `/usr/local/lib` and `/usr/local/lib64`
- [ ] **Update LD cache**: `sudo ldconfig`
- [ ] **Verify PKG-CONFIG**: `pkg-config --libs libdpdk --static`

## OAI-FHI gNB Build Checklist

### Clone OAI Repository
- [ ] **Clone OAI**: `git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git ~/openairinterface5g`
- [ ] **Navigate to directory**: `cd ~/openairinterface5g/`

### Build ORAN Fronthaul Interface Library
- [ ] **Clone PHY repository**: `git clone https://gerrit.o-ran-sc.org/r/o-du/phy.git ~/phy`
- [ ] **Checkout correct version**: 
  - For E release: `git checkout oran_e_maintenance_release_v1.0`
  - For F release: `git checkout oran_f_release_v1.0`
- [ ] **Apply patch**:
  - For E release: `git apply ~/openairinterface5g/cmake_targets/tools/oran_fhi_integration_patches/E/oaioran_E.patch`
  - For F release: `git apply ~/openairinterface5g/cmake_targets/tools/oran_fhi_integration_patches/F/oaioran_F.patch`

### Compile Fronthaul Library
- [ ] **Navigate to lib directory**: `cd ~/phy/fhi_lib/lib`
- [ ] **Clean previous build**: `make clean`
- [ ] **Build shared library**:
  - For E release: `RTE_SDK=~/dpdk-stable-20.11.9/ XRAN_DIR=~/phy/fhi_lib make XRAN_LIB_SO=1`
  - For F release: `WIRELESS_SDK_TOOLCHAIN=gcc RTE_SDK=~/dpdk-stable-20.11.9/ XRAN_DIR=~/phy/fhi_lib make XRAN_LIB_SO=1`
- [ ] **Verify shared library**: Check `~/phy/fhi_lib/lib/build/libxran.so` exists

### Arm Targets (Optional)
- [ ] **Clone ArmRAL**: `git clone https://git.gitlab.arm.com/networking/ral.git ~/ral`
- [ ] **Checkout version**: `git checkout armral-25.01`
- [ ] **Build ArmRAL**: `mkdir build && cd build && cmake -GNinja -DBUILD_SHARED_LIBS=On ../ && ninja`
- [ ] **Install ArmRAL**: `ninja install`

### Build OAI gNB
- [ ] **Navigate to cmake_targets**: `cd ~/openairinterface5g/cmake_targets`
- [ ] **Set PKG_CONFIG_PATH** (if custom DPDK path): `export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib64/pkgconfig/`
- [ ] **Install OAI dependencies** (first time): `./build_oai -I`
- [ ] **Build OAI**: `./build_oai --gNB --ninja -t oran_fhlib_5g --cmake-opt -Dxran_LOCATION=$HOME/phy/fhi_lib/lib`
- [ ] **Verify linking**: `ldd ran_build/build/liboran_fhlib_5g.so`

## Configuration Checklist

### RU Configuration
- [ ] **Contact RU vendor** for configuration manual
- [ ] **Configure RU-specific parameters**:
  - [ ] **Benetel 650**: Edit `/etc/ru_config.cfg` with mimo_mode, compression, etc.
  - [ ] **Benetel 550**: Edit `/etc/ru_config.cfg` with mimo_mode, compression, etc.
  - [ ] **LITEON**: SSH as user `user`, use `enable` command, configure via CLI
  - [ ] **VVDN LPRU**: Edit sysrepocfg database, create XML config file
  - [ ] **Metanoia RU**: Edit `/etc/rumanager.conf`
  - [ ] **Foxconn RPQN**: Run init script, edit `/home/root/test/RRHconfig_xran.xml`

### Network Interface and DPDK VF Configuration
- [ ] **Set maximum ring buffers**: `sudo ethtool -G $IF_NAME rx $MAX_RING_BUFFER_SIZE tx $MAX_RING_BUFFER_SIZE`
- [ ] **Set maximum MTU**: `sudo ip link set $IF_NAME mtu $MTU`
- [ ] **Create VF(s)**:
  - [ ] **One VF**: `sudo sh -c 'echo 1 > /sys/class/net/$IF_NAME/device/sriov_numvfs'`
  - [ ] **Two VFs**: `sudo sh -c 'echo 2 > /sys/class/net/$IF_NAME/device/sriov_numvfs'`
- [ ] **Configure VF MAC addresses and VLAN**: `sudo ip link set $IF_NAME vf 0 mac $MAC_ADD vlan $VLAN mtu $MTU spoofchk off`
- [ ] **Identify PCI addresses**: Use `lspci | grep Virtual` to find VF PCI addresses
- [ ] **Bind VF(s) to DPDK**:
  - [ ] **Unbind existing**: `sudo /usr/local/bin/dpdk-devbind.py --unbind $PCI_ADDR`
  - [ ] **Load driver**: `sudo modprobe vfio_pci` (or `mlx5_core`)
  - [ ] **Bind to DPDK**: `sudo /usr/local/bin/dpdk-devbind.py --bind vfio_pci $PCI_ADDR`

### OAI gNB Configuration
- [ ] **Select appropriate config file** based on RU vendor
- [ ] **Edit gNBs section**:
  - [ ] **PLMN section** matches AMF configuration
  - [ ] **amf_ip_address** is correct
  - [ ] **GNB_IPV4_ADDRESS_FOR_NG_AMF** matches gNB N2 interface
  - [ ] **GNB_IPV4_ADDRESS_FOR_NGU** matches gNB N3 interface
  - [ ] **prach_ConfigurationIndex** is set
  - [ ] **prach_msg1_FrequencyStart** is set
  - [ ] **Frequency, bandwidth, and SSB position** are adjusted
- [ ] **Edit L1s section**:
  - [ ] **L1_rx_thread_core** set to isolated core (e.g., CPU 8)
  - [ ] **L1_tx_thread_core** set to isolated core (e.g., CPU 10)
  - [ ] **phase_compensation** set appropriately (0 for RU, 1 for DU)
  - [ ] **tx_amp_backoff_dB** set according to RU documentation
- [ ] **Edit RUs section**:
  - [ ] **ru_thread_core** set to isolated core (e.g., CPU 6)
- [ ] **Edit fhi_72 section**:
  - [ ] **dpdk_devices** set to VF PCI addresses
  - [ ] **system_core** set to isolated core (e.g., CPU 0)
  - [ ] **io_core** set to isolated core (e.g., CPU 4)
  - [ ] **worker_cores** set to isolated cores (e.g., CPU 2)
  - [ ] **ru_addr** set to RU MAC addresses
  - [ ] **mtu** set to RU MTU (1500 or 9600)
  - [ ] **file_prefix** set if needed
  - [ ] **dpdk_mem_size** set appropriately
  - [ ] **dpdk_iova_mode** set to "PA" or "VA"
  - [ ] **owdm_enable** set if RU supports eCPRI One-Way Delay Measurements
  - [ ] **fh_config** configured with DU delay profiles and RU config

## Operation Checklist

### Start OAI gNB
- [ ] **Navigate to build directory**: `cd ~/openairinterface5g/cmake_targets/ran_build/build`
- [ ] **Run nr-softmodem**: `sudo ./nr-softmodem -O <configuration file> --thread-pool <list of non isolated cpus>`
- [ ] **Verify configuration file** is adapted to your machine's isolated cores

### Monitor Operation
- [ ] **Check RX/TX packet counters** in output
- [ ] **Verify PUSCH/PRACH counters** are equal across antennas
- [ ] **Monitor for timing issues**: Look for "Received time doesn't correspond" messages
- [ ] **Check PTP synchronization**: Verify no frame counter jumps
- [ ] **Monitor RU counters** for both RUs (if multiple RUs)

### Troubleshooting
- [ ] **If RX is almost 0**: Check RU configuration, ethernet addresses, VLAN tags
- [ ] **If timing issues**: Verify ptp4l and phc2sys are working, no jumps
- [ ] **If performance issues**: Try compiling with polling option
- [ ] **Check port mirroring** at switch to capture fronthaul packets

## Multiple RUs Configuration (Optional)

### Multi-RU Setup
- [ ] **Configure 8x8 configuration** for two 4x4 RUs
- [ ] **Set nb_tx and nb_rx** to 8 each in RUs section
- [ ] **Configure antenna ports**:
  - [ ] **pdsch_AntennaPorts_XP** = 2
  - [ ] **pdsch_AntennaPorts_N1** = 2
  - [ ] **pusch_AntennaPorts** = 8
  - [ ] **maxMIMO_layers** = 2
- [ ] **Configure fhi_72 for multiple RUs**:
  - [ ] **dpdk_devices** array with all VF PCI addresses
  - [ ] **ru_addr** array with all RU MAC addresses
  - [ ] **fh_config** array with individual RU configurations

## OAI Management Plane (Optional)

### M-plane Prerequisites
- [ ] **Set up DHCP server** with appropriate configuration
- [ ] **Install mandatory packages**:
  - [ ] **Fedora/RHEL**: `sudo dnf install pcre-devel libssh-devel libxml2-devel libyang2-devel libnetconf2-devel`
  - [ ] **Ubuntu**: `sudo apt-get install libpcre3-dev libssh-dev libxml2-dev`
- [ ] **Install libyang2 and libnetconf2** from source (Ubuntu only)

### Benetel O-RU M-plane Setup
- [ ] **Connect to RU as root**: `ssh root@<ru-ip-address>`
- [ ] **Enable mplane service**: `systemctl enable mplane`
- [ ] **Reboot RU**
- [ ] **Create oranbenetel home directory**: `mkdir /home/oranbenetel && chown oranbenetel:oranbenetel /home/oranbenetel`
- [ ] **Generate SSH keys**: `ssh-keygen` as oranbenetel user
- [ ] **Copy DU public key**: `echo "<DU-pub-key>" >> ~/.ssh/authorized_keys`

### M-plane gNB Configuration
- [ ] **Use M-plane config file** (e.g., `gnb.sa.band78.273prb.fhi72.4x4-benetel550-mplane.conf`)
- [ ] **Configure fhi_72 section** with M-plane parameters:
  - [ ] **du_key_pair** set to SSH key paths
  - [ ] **du_addr** set to DU MAC addresses
  - [ ] **vlan_tag** set to VLAN tags
  - [ ] **ru_username** set to RU username
  - [ ] **ru_ip_addr** set to RU IP addresses

### Build and Run M-plane
- [ ] **Build with M-plane support**: `./build_oai --gNB --ninja -t oran_fhlib_5g_mplane --cmake-opt -Dxran_LOCATION=$HOME/phy/fhi_lib/lib`
- [ ] **Run with M-plane config**: `sudo ./nr-softmodem -O <mplane-configuration file> --thread-pool <list of non isolated cpus>`
- [ ] **Monitor M-plane sequence** in logs

## Final Verification Checklist

### System Health
- [ ] **PTP synchronization** is stable (no jumps in logs)
- [ ] **RU is PTP synced** and RF state is Ready
- [ ] **DPDK VFs are properly bound** and accessible
- [ ] **All isolated CPUs** are properly configured
- [ ] **Huge pages** are allocated correctly

### Performance Verification
- [ ] **RX/TX packet counters** show expected traffic
- [ ] **PUSCH/PRACH counters** are balanced across antennas
- [ ] **No timing mismatch** messages in logs
- [ ] **UE can attach** and achieve good performance
- [ ] **Throughput meets expectations** (e.g., 520 Mbps DL, 40 Mbps UL for Foxconn)

### Documentation and Support
- [ ] **Save configuration files** for future reference
- [ ] **Document any custom modifications** made
- [ ] **Prepare logs and configs** for support requests if needed
- [ ] **Check GitLab issues** for known problems
- [ ] **Join mailing lists** for ongoing support

---

## Notes
- This checklist covers the complete setup process for OAI 7.2 Fronthaul Interface
- Some steps may be optional depending on your specific hardware and requirements
- Always refer to the original tutorial document for detailed explanations and troubleshooting
- Keep your configuration files and logs for future reference and support requests
