# Guide

```
iperf3 -c 172.27.2.21 -B 10.0.1.2 -t 10000 -p 5201 -R
iperf3 -c 172.27.2.21 -B 10.0.2.2 -t 10000 -p 5202 -R
iperf3 -c 172.27.2.21 -B 10.0.3.2 -t 10000 -p 5203 -R
```

```
ping 172.27.2.21 -I oaitun_ue1
```

```
tc qdisc show dev if-region-core
tc qdisc del dev if-region-core root
tc qdisc add dev if-region-core root netem delay 10ms
```
