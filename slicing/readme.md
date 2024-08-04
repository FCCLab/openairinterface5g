sudo ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band78.fr1.106PRB.2x2.usrpx410.352501.50MHz.conf --sa --usrp-tx-thread-config 1

# Dev
```
docker build -t oai-dev -f slicing/Dockerfile .
```

# CU-CP
```
sudo ./nr-softmodem -O ../../../slicing/gnb-cucp.sa.f1.conf --sa --usrp-tx-thread-config 1
```

sudo ./nr-softmodem -O ../../../slicing/gnb-cucp.sa.f1.conf --gNBs.[0].min_rxtxtime 6 --sa

# CU-UP
```
sudo ./nr-cuup -O ../../../slicing/gnb-cuup.sa.f1.conf --sa
```

## DU-UP
```
sudo ./nr-softmodem -O ../../../slicing/gnb-du.sa.conf --sa
```