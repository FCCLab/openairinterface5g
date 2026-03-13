#!/bin/bash

# Check if cuBB_SDK is defined, if not, use default path
cuBB_Path="${cuBB_SDK:-/opt/nvidia/cuBB}"

#./insmod.sh
cd "$cuBB_Path" || exit 1

# Restart MPS
# Export variables
export CUDA_DEVICE_MAX_CONNECTIONS=8
export CUDA_MPS_PIPE_DIRECTORY=/var
export CUDA_MPS_LOG_DIRECTORY=/var

# Stop existing MPS
echo quit | sudo -E nvidia-cuda-mps-control

# Start MPS
sudo -E nvidia-cuda-mps-control -d
sudo -E echo start_server -uid 0 | sudo -E nvidia-cuda-mps-control

# Start cuphycontroller_scf
# Check if an argument is provided
if [ $# -eq 0 ]; then
# No argument provided, use default value
		serverVendorAndModel=$(cat /sys/devices/virtual/dmi/id/board_vendor)
		serverVendorAndModel+="-"
		serverVendorAndModel+=$(cat /sys/devices/virtual/dmi/id/board_name)
		echo $serverVendorAndModel
		case $serverVendorAndModel in
	"Dell Inc.-06V45N")
		argument="P5G_FXN_R750"
		;;
	"GIGABYTE-MU71-SU0-00")
		argument="P5G_FXN"
		;;
	"Supermicro-G1SMH-G")
		argument="P5G_WNC_GH"
		#argument="P5G_FXN_GH"
		;;
	*)
		echo "Unrecognized server: $serverVendorAndModel"
		exit
		;;
	esac
else
	# Argument provided, use it
	argument="$1"
fi
configFile=${cuBB_SDK}/cuPHY-CP/cuphycontroller/config/cuphycontroller_${argument}.yaml  

#Change this to the MAC address of the ORU
#sudo -E sed -i "s/ dst_mac_addr:.*/ dst_mac_addr: e8:c7:cf:ac:58:32/" ${configFile}
if [[ $argument == "P5G_FXN_GH" ]]; then
	sudo -E sed -i "s/ dst_mac_addr:.*/ dst_mac_addr: 6c:ad:ad:00:04:6c/" ${configFile}
	sudo -E sed -i "s/ vlan:.*/ vlan: 2/" ${configFile}
fi
# Uncomment for below config. Because aerial is mounted as a volume the $configFile needs to be reverted.
#config="UL-Heavy" # Commenting this line won't revert the change
if [[ $config = "UL-Heavy" ]]; then
	echo "Doing UL-heavy"
	sudo -E sed -i "s/shm_log_level: 4/shm_log_level: 5/" ${configFile}
	sudo -E sed -i "s/pusch_aggr_per_ctx:.*/pusch_aggr_per_ctx: 12/" ${configFile}
	sudo -E sed -i "s/prach_aggr_per_ctx.*/prach_aggr_per_ctx: 4/" ${configFile}
	sudo -E sed -i "s/ul_input_buffer_per_cell:.*/ul_input_buffer_per_cell: 20/" ${configFile}
fi

export AERIAL_LOG_PATH=/var/log/aerial
sudo -E "$cuBB_Path"/build.$(uname -m)/cuPHY-CP/cuphycontroller/examples/cuphycontroller_scf $argument
sudo -E ./build.$(uname -m)/cuPHY-CP/gt_common_libs/nvIPC/tests/pcap/pcap_collect nvipc /tmp
sudo -E mv /tmp/nvipc*.pcap $AERIAL_LOG_PATH

#Uncomment this if using multiple nvipc interfaces
#sudo ./build/cuPHY-CP/gt_common_libs/nvIPC/tests/pcap/pcap_collect nvipc1 /tmp
#sudo mv /tmp/nvipc*.pcap /var/log/aerial/
#sleep infinity
