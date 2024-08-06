
# User Guide

```
cd ~/openairinterface5g
docker build -t ran-base:ubuntu22 -f docker/Dockerfile.base.ubuntu22 .
docker build -t ran-build:ubuntu22 -f docker/Dockerfile.build.ubuntu22 .
docker build -t ran-build-flexric:ubuntu22 -f docker/Dockerfile.build.flexric.ubuntu22 .

docker build -t oai-nr-softmodem:ubuntu22 -f docker/Dockerfile.gNB.ubuntu22 .
docker build -t oai-nr-cuup:ubuntu22 -f docker/Dockerfile.nr-cuup.ubuntu22 .
docker build -t oai-nr-flexric:ubuntu22 -f docker/Dockerfile.flexric.ubuntu22 .
```

```
docker run -d --name oai-router --network oai-cucp-cuup-du --cap-add=NET_ADMIN  --privileged alpine sleep infinity
docker network connect demo-oai-public-net oai-router
docker exec -it oai-router sh -c "echo 1 > /proc/sys/net/ipv4/ip_forward"
```

```
sysctl: cannot stat /proc/sys/net/core/rmem_max: No such file or directory
sysctl: cannot stat /proc/sys/net/core/wmem_max: No such file or directory
[HW]   Can't set kernel parameters for X4x0
[INFO] [MPMD] Initializing 1 device(s) in parallel with args: mgmt_addr=192.168.10.2,type=x4xx,product=x410,serial=32DAFAA,name=ni-x4xx-32DAFAA,fpga=X4_200,claimed=False,addr=192.168.10.2,clock_source=internal,time_source=internal,master_clock_rate=245760000.000000
[WARNING] [MPM.RPCServer] A timeout event occured!
[ERROR] [MPMD] MPM major compat number mismatch. Expected: 4.4 Actual: 5.3. Please update the version of MPM on your USRP device.
terminate called after throwing an instance of 'uhd::runtime_error'
  what():  RuntimeError: MPM major compat number mismatch. Expected: 4.4 Actual: 5.3. Please update the version of MPM on your USRP device.
[INFO  tini (1)] Main child exited with signal (with signal 'Aborted')
```
==> change UHD_VERSION in Dockerfile.base and build again

```
uhd_images_downloader -l
```

## Build
```
./build_oai  -w USRP --gNB --build-e2 --ninja
```

## IP
```
sudo ip address add  192.168.71.101/24 dev lo
sudo ip address add  192.168.71.102/24 dev lo
sudo ip address add  192.168.71.103/24 dev lo
```

## CU-CP
```
sudo ./nr-softmodem -O ../../../oai-conf/slicing/gnb-cucp.sa.f1.conf --sa
```

## CU-UP
```
sudo ./nr-cuup -O ../../../oai-conf/slicing/gnb-cuup.sa.f1.slice1.conf --sa
sudo ./nr-cuup -O ../../../oai-conf/slicing/gnb-cuup.sa.f1.slice2.conf --sa
sudo ./nr-cuup -O ../../../oai-conf/slicing/gnb-cuup.sa.f1.slice3.conf --sa
```

## DU-UP
```
sudo ./nr-softmodem -O /home/vantuan_ngo/oai-conf/slicing/gnb-du.sa.conf --sa
```

## Connection between subnets'
```
iptables -I FORWARD -i oai-cu-cp-up-du -o demo-oai -j ACCEPT
iptables -I FORWARD -i demo-oai -o oai-cu-cp-up-du -j ACCEPT
```

```
sudo ip route add 192.168.70.0/24 dev demo-oai proto kernel scope link src 192.168.70.1 
sudo ip route add 192.168.71.0/24 dev oai-cu-cp-up-du proto kernel scope link src 192.168.71.1 

sudo ip route delete 192.168.70.0/24 dev oai-cu-cp-up-du
sudo ip route delete 192.168.71.0/24 dev demo-oai 
```

```
sudo iptables -S
sudo iptables -t nat -S

sudo iptables -D DOCKER-ISOLATION-STAGE-1 -i demo-oai ! -o demo-oai -j DOCKER-ISOLATION-STAGE-2
sudo iptables -D DOCKER-ISOLATION-STAGE-1 -i oai-cu-cp-up-du ! -o oai-cu-cp-up-du -j DOCKER-ISOLATION-STAGE-2

sudo iptables -t nat -D POSTROUTING -s 192.168.70.0/24 ! -o demo-oai -j MASQUERADE
sudo iptables -t nat -D POSTROUTING -s 192.168.71.0/24 ! -o oai-cu-cp-up-du -j MASQUERADE
```

```
/etc/sudoers.d/nephio.conf

vantuan_ngo ALL=(ALL) NOPASSWD: ALL
```

