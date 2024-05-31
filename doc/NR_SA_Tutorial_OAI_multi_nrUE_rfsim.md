<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="./images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI 5G NR SA tutorial with multiple OAI nrUE in RFSIM</font></b>
    </td>
  </tr>
</table>

**Table of Contents**

[[_TOC_]]

#  1. Scenario
In this tutorial we describe how to configure and run a 5G end-to-end setup with OAI CN5G, OAI gNB with single/multiple OAI nrUE(s) in the RFsimulator.

Minimum hardware requirements:
- Laptop/Desktop/Server for OAI CN5G and OAI gNB and UE
    - Operating System: [Ubuntu 22.04 LTS](https://releases.ubuntu.com/22.04/ubuntu-22.04.3-desktop-amd64.iso)
    - CPU: 8 cores x86_64 @ 3.5 GHz
    - RAM: 32 GB




# 2. OAI CN5G

## 2.1 OAI CN5G pre-requisites

Please install and configure OAI CN5G as described here:
[OAI CN5G](NR_SA_Tutorial_OAI_CN5G.md)


# 3. OAI gNB and OAI nrUE

## 3.1 Build OAI gNB and OAI nrUE

```bash
# Get openairinterface5g source code
git clone https://gitlab.eurecom.fr/oai/openairinterface5g.git ~/openairinterface5g
cd ~/openairinterface5g
git checkout develop

# Install OAI dependencies
cd ~/openairinterface5g/cmake_targets
./build_oai -I

# nrscope dependencies
sudo apt install -y libforms-dev libforms-bin

# Build OAI gNB
./build_oai -w SIMU --ninja --nrUE --gNB --build-lib "nrscope telnetsrv" -C
```

# 4. Run OAI CN5G and OAI gNB

## 4.1 Run OAI CN5G

```bash
cd ~/oai-cn5g
docker compose up -d
```

## 4.2 Run OAI gNB 


### RFsimulator

Channel models in the context of wireless communication refer to mathematical models that simulate the effects of transmission mediums on signal propagation. These models account for factors such as attenuation, interference, and fading, which can affect the quality of communication between transmitter and receiver. Different channel models represent different real-world scenarios, such as urban environments, indoor spaces, or rural areas. By using these models, researchers and engineers can predict and evaluate the performance of wireless systems under various conditions. Here a simple scenario based on the noise of the chanel is described. The following steps are descreibed below. 

- For the gNB configuration file, follow the link to the configuration files: [Configurations](../ci-scripts/conf_files/gnb.sa.band78.106prb.rfsim.conf)

- Add this line to the bottom of the conf file for including the channel models in your simulations. 

```bash
@include "channelmod_rfsimu.conf"
```

You can check the example run 2(RFSIMULATOR) in the Launch gNB in one window part in this link: [RFSIMULATOR Tutorial](../radio/rfsimulator/README.md)


- For the channel model configuration, follow the link to the configurtion files:  [Configurations](../ci-scripts/conf_files/channelmod_rfsimu.conf) 

- In the rfsimu_channel_enB0 model part, edit the noise power as the following:
```bash
        noise_power_dB                   = -10;
```
-In the rfsimu_channel_ue0 model part, edit the noise power as the following:
```bash
        noise_power_dB                   = -20; 
```
and the rest of the channelmod_rfsimu.conf remains unchanged. 




# 5. OAI  UE 

For the UE configuration file, follow the link to the configuration files: [Configurations](../ci-scripts/conf_files/ue.sa.conf)

- Edit the IMSI as the following
```bash
imsi="0010100000001";
```
- Add this line to the bottom of the conf file for including the channel models in your simulations. 


```bash
@include "channelmod_rfsimu.conf"
```

You can check the example run 1(RFSIMULATOR) in the Launch UE in another window part in this link: [RFSIMULATOR Tutorial](../radio/rfsimulator/README.md)

# 5.2 OAI multiple UE 


## 5.1 Testing OAI nrUE with multiple UEs in RFsimulator
Important notes:
- This should be run on the same host as the OAI gNB
- It only applies when running OAI gNB with RFsimulator
- Use the script (multi-ue.sh) in [RFSIMULATOR Tutorial](../radio/rfsimulator) to make namespaces for 
multiple UEs.  

- For the first UE, create the namespace ue1 (-c1) and then execute bash inside (-e):
```bash
sudo ./multi-ue.sh -c1 -e
sudo ./multi-ue.sh -o1
```
- After entering the bash environment, run the following command to deploy your first UE
```bash
sudo ./nr-uesoftmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.conf -r 106 --numerology 1 --band 78 -C 3619200000 --rfsim --sa --uicc0.imsi 001010000000001 --nokrnmod -E --rfsimulator.options chanmod --rfsimulator.serveraddr 10.201.1.100 --telnetsrv --telnetsrv.listenport 9095
```
- For the second UE, create the namespace ue2 (-c2) and then execute bash inside (-e):
```bash
sudo ./multi-ue.sh -c2 -e
sudo ./multi-ue.sh -o2
```
- After entering the bash environment, run the following command to deploy your second UE
```bash
sudo ./nr-uesoftmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/ue.conf -r 106 --numerology 1 --band 78 -C 3619200000 --rfsim --sa --uicc0.imsi 001010000000001 --nokrnmod -E --rfsimulator.options chanmod --rfsimulator.serveraddr 10.202.1.100 --telnetsrv --telnetsrv.listenport 9096
```


### 5.3 Telnet server

-single UE in RFsimulator

- gNB host
```bash
telnet 127.0.0.1 9099
```
- UE host
```bash
telnet 127.0.0.1 9095
```
-Multiple UEs in RFsimulator

- gNB host
```bash
telnet 127.0.0.1 9099
```
- UE host
```bash
telnet 10.201.1.1 9095 ### For accessing to the first UE
telnet 10.202.1.2 9096 ### For accessing to the second UE
```
After entering to the bash environment you can type help and see the possible options to change the channelmodels and other available options in RFSIM. 

### 5.4 Monitoring by nr-scope 

In order to verify the effects of the changes, the nr-scope constellation tool can be used to track and analyze the modulation constellation points. This tool allows users to visualize the modulation scheme being used and assess the quality of the received signals. By observing the constellation points, users can verify whether the changes made to the system configuration have resulted in.
