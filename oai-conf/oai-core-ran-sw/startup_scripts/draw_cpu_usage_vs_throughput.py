#!/usr/bin/env python3

import matplotlib.pyplot as plt
from datetime import datetime, timezone, timedelta
from influxdb_client import InfluxDBClient, Point, QueryApi
import numpy as np

from oai_config import *

REGION_LABEL_Y_POSE = -0

local_tz = timezone(timedelta(hours=8))

# Create an InfluxDB client
client = InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)

# Instantiate the Query API
query_api = client.query_api()


query_template = f"""
from(bucket: "{INFLUXDB_BUCKET}")
  |> range(start: %s, stop: %s)
  |> filter(fn: (r) => r["_measurement"] == "%s")
  |> filter(fn: (r) => r["_field"] == "%s")
  |> yield(name: "last")
"""

# SCENARIO = [datetime(2024, 8, 19, 12, 56, 30, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
#               datetime(2024, 8, 19, 13, 14, 0, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
#             ]

SCENARIO = [datetime(2024, 8, 19, 22, 2, 20, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
              datetime(2024, 8, 19, 22, 27, 40, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
            ]

query = query_template % (*SCENARIO, "oai-cu-up-2", "cpu_quota")
print(query)
tables = query_api.query(query)
table = tables[0]
time_axis_cpu_quota = []
cpu_quota = []
for idx, record in enumerate(table.records):
  time_axis_cpu_quota.append(record.get_time())
  cpu_quota.append(record.get_value() / 1e3)
# plt.plot(time_axis_cpu_quota, cpu_quota)

def get_average_cpu_usage(time, measurement="oai-cu-up-2"):
  query = query_template % (time.isoformat(), (time + timedelta(seconds=40)).isoformat(), measurement, "cpu_usage_usec")
  print(query)
  tables = query_api.query(query)
  table = tables[0]
  time_axis_cpu_usage_usec = []
  cpu_usage_usec = []
  for idx, record in enumerate(table.records):
    time_axis_cpu_usage_usec.append(record.get_time())
    cpu_usage_usec.append(record.get_value())
  return np.mean(cpu_usage_usec) / 1e3

def get_average_throughput(time):
  query = query_template % (time.isoformat(), (time + timedelta(seconds=40)).isoformat(), "oai-nr-ue-12", "bandwidth_bps")
  print(query)
  tables = query_api.query(query)
  if len(tables) == 0:
    return 0
  table = tables[0]
  time_axis_throughput = []
  throughput = []
  for idx, record in enumerate(table.records):
    time_axis_throughput.append(record.get_time())
    throughput.append(record.get_value())
  return np.mean(throughput) / 1e6

average_throughput = []
average_cpu_usage = []
average_cpu_usage_upf = []
for time in time_axis_cpu_quota:
  throughput = get_average_throughput(time)
  cpu_usage = get_average_cpu_usage(time)
  cpu_usage_upf = get_average_cpu_usage(time, measurement="oai-upf-slice-2")
  print(time, throughput, cpu_usage)
  average_throughput.append(throughput)
  average_cpu_usage.append(cpu_usage)
  average_cpu_usage_upf.append(cpu_usage_upf)

# plt.plot(time_axis_cpu_quota, cpu_quota)
# plt.plot(time_axis_cpu_quota, average_cpu_usage)
# plt.plot(time_axis_cpu_quota, average_throughput)

# Create the figure and the first y-axis
fig, (ax1) = plt.subplots(1,1)

time_axis = [i for i in range(len(time_axis_cpu_quota))]

# Plot the data for the first y-axis
line_cpu_quotas, = ax1.plot(time_axis, cpu_quota, 'r-', label='Conf. CPU Quotas for CU-UP')  # 'g-' indicates a green line
line_cpu_usage, = ax1.plot(time_axis, average_cpu_usage, 'g-', label='Avg. CPU Usage of CU-UP')  # 'g-' indicates a green line
line_cpu_usage_upf, = ax1.plot(time_axis, average_cpu_usage_upf, 'orange', label='Avg. CPU Usage of UPF')  # 'g-' indicates a green line
ax1.set_xlabel('Time (x60 seconds)')
ax1.set_ylabel('CPU Usage (ms/s)')
# ax1.tick_params(axis='y', labelcolor='g')

# Create the second y-axis sharing the same x-axis
ax2 = ax1.twinx()

# Plot the data for the second y-axis
line_throughput, = ax2.plot(time_axis, average_throughput, 'b--', label="Avg. UE's DL Throughput")  # 'b--' indicates a blue dashed line
ax2.set_ylabel("Downlink Throughput (Mbps)", color='b')
ax2.tick_params(axis='y', labelcolor='b')

# Combine legends from both axes
lines = [line_cpu_quotas, line_cpu_usage, line_cpu_usage_upf, line_throughput]
labels = [line.get_label() for line in lines]

# Place the combined legend in the upper right corner
ax2.legend(lines, labels, loc='upper right')

plt.tight_layout()
plt.savefig('CU-UP_UPF_CPU_Usage_vs_Throughput-a.pdf', format='pdf')
plt.show()

ax3 = plt.subplot()

#### Plot CPU usage vs Throughput ####
ax3.plot(cpu_quota, average_throughput, label="Average UE's DL Throughput")
ax3.set_ylabel("Downlink Throughput (Mbps)", color='b')

slope, intercept = np.polyfit(cpu_quota, average_throughput, 1)  # 1 is the degree of the polynomial (linear)
average_throughput_regression = slope * np.array(cpu_quota) + intercept

ax3.plot(cpu_quota, average_throughput_regression, '--', label="Estimated UE's DL Throughput")

sample_points_x = []
sample_points_y = []

for x, y in zip(cpu_quota, average_throughput_regression):
    print(x, y)
    if x == 20 or x == 40 or x == 80 or x == 160:
      sample_points_x.append(x)
      sample_points_y.append(y)
    
ax3.scatter(sample_points_x, sample_points_y, color='red')  # Plot points
for i in range(len(sample_points_x)):
    ax3.annotate(f'({sample_points_x[i]:.0f}, {sample_points_y[i]:.1f})', (sample_points_x[i], sample_points_y[i]), textcoords="offset points", xytext=(40, -5), ha='center')

ax3.tick_params(axis='y', labelcolor='b')
ax3.set_xlabel('Configured CPU Quotas of CU-UP (ms/s)')
ax3.legend(loc='upper left')

plt.tight_layout()
plt.savefig('CU-UP_UPF_CPU_Usage_vs_Throughput-b.pdf', format='pdf')
plt.show()