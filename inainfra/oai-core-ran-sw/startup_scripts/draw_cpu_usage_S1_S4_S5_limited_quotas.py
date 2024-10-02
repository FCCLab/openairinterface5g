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

SCENARIO = [datetime(2024, 8, 19, 19, 57, 00, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
              datetime(2024, 8, 19, 19, 59, 0, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
            ]


def get_cpu_usage(measurement):
  query = query_template % (*SCENARIO, measurement, "cpu_usage_usec")
  print(query)
  tables = query_api.query(query)
  table = tables[0]
  time_axis = []
  cpu_usage = []
  for idx, record in enumerate(table.records):
    time_axis.append(record.get_time())
    cpu_usage.append(record.get_value() / 1e3)
  return (time_axis, cpu_usage)

def get_throughput(measurement):
  query = query_template % (*SCENARIO, measurement, "bandwidth_bps")
  print(query)
  tables = query_api.query(query)
  table = tables[0]
  time_axis = []
  cpu_usage = []
  for idx, record in enumerate(table.records):
    time_axis.append(record.get_time())
    cpu_usage.append(record.get_value() / 1e6)
  return (time_axis, cpu_usage)

window_size = 9
weights = np.ones(window_size) / window_size
half_window = window_size // 2

fig, (ax_cpu_usage, ax2_throughput) = plt.subplots(2, 1, figsize=(6, 7))

time_axis, cpu_usage_cu_up_1 = get_cpu_usage("oai-cu-up-1")
# plt.plot(time_axis, cpu_usage_cu_up_1, label="oai-cu-up-1")
extended_data = np.pad(cpu_usage_cu_up_1, pad_width=half_window, mode='reflect')
filtered_data = np.convolve(extended_data, weights, mode='valid')
x = [i for i in range(len(filtered_data))]
ax_cpu_usage.plot(x, filtered_data, label="S3 CU-UP")

time_axis, cpu_usage_upf_slice_1 = get_cpu_usage("oai-upf-slice-1")
# plt.plot(time_axis, cpu_usage_upf_slice_1, label="oai-upf-slice-1")
extended_data = np.pad(cpu_usage_upf_slice_1, pad_width=half_window, mode='reflect')
filtered_data = np.convolve(extended_data, weights, mode='valid')
x = [i for i in range(len(filtered_data))]
ax_cpu_usage.plot(x, filtered_data, label="S3 UPF")

time_axis, cpu_usage_cu_up_4 = get_cpu_usage("oai-cu-up-4")
# plt.plot(time_axis, cpu_usage_cu_up_4, label="oai-cu-up-4")
extended_data = np.pad(cpu_usage_cu_up_4, pad_width=half_window, mode='reflect')
filtered_data = np.convolve(extended_data, weights, mode='valid')
x = [i for i in range(len(filtered_data))]
ax_cpu_usage.plot(x, filtered_data, label="S4 CU-UP")

time_axis, cpu_usage_upf_slice_4 = get_cpu_usage("oai-upf-slice-4")
# plt.plot(time_axis, cpu_usage_upf_slice_4, label="oai-upf-slice-4")
extended_data = np.pad(cpu_usage_upf_slice_4, pad_width=half_window, mode='reflect')
filtered_data = np.convolve(extended_data, weights, mode='valid')
x = [i for i in range(len(filtered_data))]
ax_cpu_usage.plot(x, filtered_data, label="S4 UPF")

time_axis, cpu_usage_cu_up_5 = get_cpu_usage("oai-cu-up-5")
# plt.plot(time_axis, cpu_usage_cu_up_5, label="oai-cu-up-4")
extended_data = np.pad(cpu_usage_cu_up_5, pad_width=half_window, mode='reflect')
filtered_data = np.convolve(extended_data, weights, mode='valid')
x = [i for i in range(len(filtered_data))]
ax_cpu_usage.plot(x, filtered_data, label="S5 CU-UP")

time_axis, cpu_usage_upf_slice_5 = get_cpu_usage("oai-upf-slice-5")
# plt.plot(time_axis, cpu_usage_upf_slice_5, label="oai-upf-slice-4")
extended_data = np.pad(cpu_usage_upf_slice_5, pad_width=half_window, mode='reflect')
filtered_data = np.convolve(extended_data, weights, mode='valid')
x = [i for i in range(len(filtered_data))]
ax_cpu_usage.plot(x, filtered_data, label="S5 UPF")

ax_cpu_usage.set_xlabel ('Time (seconds)')
ax_cpu_usage.set_ylabel ('CPU Usage (ms/s)')
ax_cpu_usage.legend(loc='upper right')
ax_cpu_usage.set_ylim(10, 110)

average_cu_up_1 = np.average(cpu_usage_cu_up_1)
print("Average CPU Usage of oai-cu-up-1:", average_cu_up_1)

average_upf_slice_1 = np.average(cpu_usage_upf_slice_1)
print("Average CPU Usage of oai-upf-slice-1:", average_upf_slice_1)

average_cu_up_4 = np.average(cpu_usage_cu_up_4)
print("Average CPU Usage of oai-cu-up-4:", average_cu_up_4)

average_upf_slice_4 = np.average(cpu_usage_upf_slice_4)
print("Average CPU Usage of oai-upf-slice-4:", average_upf_slice_4)

average_cu_up_5 = np.average(cpu_usage_cu_up_5)
print("Average CPU Usage of oai-cu-up-5:", average_cu_up_5)

average_upf_slice_5 = np.average(cpu_usage_upf_slice_5)
print("Average CPU Usage of oai-upf-slice-4:", average_upf_slice_5)

total_cpu_usage = average_cu_up_1 + average_upf_slice_1 + average_cu_up_4 + average_upf_slice_4 + average_cu_up_5 + average_upf_slice_5
print(total_cpu_usage)

window_size = 9
weights = np.ones(window_size) / window_size
half_window = window_size // 2

time_axis, throughput_ue1 = get_throughput("oai-nr-ue-11")
extended_data = np.pad(throughput_ue1, pad_width=half_window, mode='reflect')
filtered_data = np.convolve(extended_data, weights, mode='valid')
x = [i for i in range(len(filtered_data))]
ax2_throughput.plot(x, filtered_data, label="UE3")

time_axis, throughput_ue4 = get_throughput("oai-nr-ue-14")
extended_data = np.pad(throughput_ue4, pad_width=half_window, mode='reflect')
filtered_data = np.convolve(extended_data, weights, mode='valid')
x = [i for i in range(len(filtered_data))]
ax2_throughput.plot(x, filtered_data, label="UE4")

time_axis, throughput_ue5 = get_throughput("oai-nr-ue-15")
extended_data = np.pad(throughput_ue5, pad_width=half_window, mode='reflect')
filtered_data = np.convolve(extended_data, weights, mode='valid')
x = [i for i in range(len(filtered_data))]
ax2_throughput.plot(x, filtered_data, label="UE5")

ax2_throughput.legend(loc='upper right')
ax2_throughput.set_xlabel('Time (seconds)')
ax2_throughput.set_ylabel('DL Throughput (Mbps)')
ax2_throughput.set_ylim(10, 90)

print(np.average(throughput_ue1))
print(np.average(throughput_ue4))
print(np.average(throughput_ue5))


plt.tight_layout()
plt.savefig('CPU_Usage_and_Throughput_with_S5.pdf', format='pdf')

plt.show()
