# Quotas Calculation

## Initial metrics
2 Slices: S3 + S4
```
Average CPU Usage of oai-cu-up-1: 48.92641044776119
Average CPU Usage of oai-upf-slice-1: 59.99614179104478
Average CPU Usage of oai-cu-up-4: 75.55089629629629
Average CPU Usage of oai-upf-slice-4: 86.47834074074075
Total 270.951789275843
```

We chose to limit CPU quotas at 300ms

With the notice that the ratio between UPF and CU-UP is constant `UPF/CU-UP ~= 1.3`, we will have  `300/2.3*1=130ms` for 3 slices (S3, S4, S5)
Using priority value, we share this `130ms` for CU-UP of S1, S2, S3.

## Reverse Problem

In order to nicely draw the graph, we choose the following Radio Resource and Compute Resource Ratio

|     |  Radio Resource (%slot/frame)   |  Compute Resource (ms/s)    |
| :---: | :---: | :---: |
| S3 | 15 | 20 |
| S4 | 20 | 30 |
| S5 | 40 | 80 |

```
130 * (15x)/(15x + 20y + 40z) = 20
130 * (20y)/(15x + 20y + 40z) = 30
130 * (40z)/(15x + 20y + 40z) = 80
```

Solution:
```
x = 2k/15
y = 3k/20
z = 8k/40
```

Choose k = 1, we have non-violation weights:
```
x = 0.13
y = 0.15
z = 0.2
```

## Forward Problem
|     |  Radio Resource (%slot/frame)   |  Non-violation weight    | Compute Resource |
| :---: | :---: | :---: | :---: |
| S3 | 15 | 0.13 | 130 * 15 * 0.13 / (15 * 0.13 + 20 * 0.15 + 40 * 0.2) ~= 20 |
| S4 | 20 | 0.15 | 130 * 20 * 0.15 / (15 * 0.13 + 20 * 0.15 + 40 * 0.2) ~= 30 |
| S5 | 40 | 0.2  | 130 * 40 * 0.20 / (15 * 0.13 + 20 * 0.15 + 40 * 0.2) ~= 80 | 
