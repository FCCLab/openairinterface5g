
# User Guide

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