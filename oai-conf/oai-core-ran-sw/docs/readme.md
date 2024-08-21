# Guide

```
iperf3 -c 172.27.2.21 -B 10.0.1.2 -t 10000 -p 5201 -R
iperf3 -c 172.27.2.21 -B 10.0.2.2 -t 10000 -p 5202 -R
iperf3 -c 172.27.2.21 -B 10.0.3.2 -t 10000 -p 5203 -R
iperf3 -c 172.27.2.21 -B 10.0.4.2 -t 10000 -p 5204 -R
```

```
ping 172.27.2.21 -I oaitun_ue1
```

```
tc qdisc show dev if-region-core
tc qdisc del dev if-region-core root
tc qdisc add dev if-region-core root netem delay 10ms
```

```
tc qdisc del dev if-region-core root
tc qdisc add dev if-region-core root handle 1: netem delay 10ms
tc qdisc add dev if-region-core parent 1:1 handle 10: tbf rate 50mbit burst 32kbit latency 400ms
tc qdisc show dev if-region-core
```

```
tc qdisc del dev if-edge-region root
tc qdisc add dev if-edge-region root handle 1: netem delay 10ms
tc qdisc add dev if-edge-region parent 1:1 handle 10: tbf rate 50mbit burst 32kbit latency 400ms
tc qdisc show dev if-edge-region
```

# Results


