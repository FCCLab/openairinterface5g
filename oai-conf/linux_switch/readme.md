# Guide

```
brctl addbr br0
brctl addif br0 eth0
brctl addif br0 eth1
ifconfig br0 up
brctl show

ifconfig eth0 192.168.0.100/24
ifconfig eth0 192.168.0.200/24
ping 192.168.0.100

iperf3 -c 192.168.0.100

tc qdisc add dev eth0 root netem delay 100ms
```