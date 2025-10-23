# CPU Allocation

|Applicative Threads|Allocated CPUs    |
|-------------------|------------------|
|XRAN DPDK usage    |1,2,3             |
|OAI `ru_thread`    |4                 |
|OAI `L1_rx_thread` |5                 |
|OAI `L1_tx_thread` |6                 |
|OAI `nr-softmodem` |8,9,10,11         |
|kernel             |12,13,14,15       |

# Virtual Function
[2025-10-23 15:21:42] Use these parameters in your OAI configuration file:
[2025-10-23 15:21:42]   dpdk_devices = ("0000:01:02.0");
[2025-10-23 15:21:42]   ru_addr = ("00:11:22:33:44:66");

# RU
```
tail -f /var/log/slabtimingptp2.log
```
