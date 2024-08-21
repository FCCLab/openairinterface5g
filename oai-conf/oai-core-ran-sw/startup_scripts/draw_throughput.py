#!/usr/bin/env python3

import matplotlib.pyplot as plt
from datetime import datetime, timezone, timedelta
from influxdb_client import InfluxDBClient, Point, QueryApi
import matplotlib.ticker as ticker
import numpy as np

from oai_config import *

local_tz = timezone(timedelta(hours=8))

# Create an InfluxDB client
client = InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)

# Instantiate the Query API
query_api = client.query_api()

REGION_LABEL_Y_POSE = 5

query_template = f"""
from(bucket: "{INFLUXDB_BUCKET}")
  |> range(start: %s, stop: %s)
  |> filter(fn: (r) => r["_measurement"] == "%s")
  |> filter(fn: (r) => r["_field"] == "bandwidth_bps")
  |> yield(name: "last")
"""

window_size = 3
weights = np.ones(window_size) / window_size
half_window = window_size // 2

fig, ax = plt.subplots()

############################################ SCENARIO 1 ############################################
SCENARIO_START = 0
time_axis_ue13 = []
bandwidth_ue13 = []
SCENARIO = [datetime(2024, 8, 17, 16, 46, 0, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
              datetime(2024, 8, 17, 16, 48, 40, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
            ]
query = query_template % (*SCENARIO, "oai-nr-ue-13")
print(query)
tables = query_api.query(query)
table = tables[0]
for idx, record in enumerate(table.records):
    if idx == 0:
        time_axis_ue13.append(SCENARIO_START + 1)
    else:
        time_axis_ue13.append(time_axis_ue13[-1] + 1)
    bandwidth_ue13.append(record.get_value()/1e6)
print(time_axis_ue13)
print(bandwidth_ue13)

x_start = SCENARIO_START
x_end   = time_axis_ue13[-1]
SCENARIO_END_S1 = x_end
label_x = (x_start + x_end) / 2  # Position the label in the middle of the shaded region
label_y = REGION_LABEL_Y_POSE                 # Position the label above the plot data
ax.text(label_x, label_y, '{S1}', fontsize=8, color='darkred', ha='center', va='center')

########################################## SCENARIO 2 ############################################
SCENARIO_START = time_axis_ue13[-1]
time_axis_ue12 = []
bandwidth_ue12 = []
SCENARIO = [datetime(2024, 8, 17, 16, 49, 0, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
              datetime(2024, 8, 17, 16, 50, 0, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
            ]

query = query_template % (*SCENARIO, "oai-nr-ue-13")
print(query)
tables = query_api.query(query)
table = tables[0]
for idx, record in enumerate(table.records):
    if idx == 0:
        time_axis_ue13.append(SCENARIO_START + 1)
    else:
        time_axis_ue13.append(time_axis_ue13[-1] + 1)
    bandwidth_ue13.append(record.get_value()/1e6)
print(bandwidth_ue13)

query = query_template % (*SCENARIO, "oai-nr-ue-12")
print(query)
tables = query_api.query(query)
table = tables[0]
for idx, record in enumerate(table.records):
    if idx == 0:
        time_axis_ue12.append(SCENARIO_START + 1)
    else:
        time_axis_ue12.append(time_axis_ue12[-1] + 1)
    bandwidth_ue12.append(record.get_value()/1e6)
print(bandwidth_ue12)

x_start = SCENARIO_START
x_end   = max(time_axis_ue13[-1], time_axis_ue12[-1])
SCENARIO_END_S2 = x_end
label_x = (x_start + x_end) / 2  # Position the label in the middle of the shaded region
label_y = REGION_LABEL_Y_POSE                  # Position the label above the plot data
ax.text(label_x, label_y, '{S1, S2}', fontsize=8, color='darkred', ha='center', va='center')

############################################ SCENARIO 3 ############################################
SCENARIO_START = max(time_axis_ue13[-1], time_axis_ue12[-1])
time_axis_ue11 = []
bandwidth_ue11 = []
SCENARIO = [datetime(2024, 8, 17, 17, 4, 20, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
              datetime(2024, 8, 17, 17, 5, 32, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
            ]

query = query_template % (*SCENARIO, "oai-nr-ue-13")
print(query)
tables = query_api.query(query)
table = tables[0]
for idx, record in enumerate(table.records):
    if idx == 0:
        time_axis_ue13.append(SCENARIO_START + 1)
    else:
        time_axis_ue13.append(time_axis_ue13[-1] + 1)
    bandwidth_ue13.append(record.get_value()/1e6)
print(bandwidth_ue13)

query = query_template % (*SCENARIO, "oai-nr-ue-12")
print(query)
tables = query_api.query(query)
table = tables[0]
for idx, record in enumerate(table.records):
    if idx == 0:
        time_axis_ue12.append(SCENARIO_START + 1)
    else:
        time_axis_ue12.append(time_axis_ue12[-1] + 1)
    bandwidth_ue12.append(record.get_value()/1e6)
print(bandwidth_ue12)

query = query_template % (*SCENARIO, "oai-nr-ue-11")
print(query)
tables = query_api.query(query)
table = tables[0]
for idx, record in enumerate(table.records):
    if idx == 0:
        time_axis_ue11.append(SCENARIO_START + 1)
    else:
        time_axis_ue11.append(time_axis_ue11[-1] + 1)
    bandwidth_ue11.append(record.get_value()/1e6)
print(bandwidth_ue11)

x_start = SCENARIO_START
x_end   = max(time_axis_ue13[-1], time_axis_ue12[-1], time_axis_ue11[-1])
SCENARIO_END_S3 = x_end
label_x = (x_start + x_end) / 2  # Position the label in the middle of the shaded region
label_y = REGION_LABEL_Y_POSE             # Position the label above the plot data
ax.text(label_x, label_y, '{S1, S2, S3}', fontsize=8, color='darkred', ha='center', va='center')

############################################ SCENARIO 4 ############################################
SCENARIO_START = max(time_axis_ue13[-1], time_axis_ue12[-1], time_axis_ue11[-1])
time_axis_ue14 = []
bandwidth_ue14 = []
# SCENARIO = [datetime(2024, 8, 17, 17, 6, 0, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
#               datetime(2024, 8, 17, 17, 7, 0, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
#             ]
SCENARIO = [datetime(2024, 8, 20, 12, 49, 0, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
              datetime(2024, 8, 20, 12, 50, 10, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
            ]

scenario_ue13 = []
scenario_ue12 = []
scenario_ue11 = []
scenario_ue14 = []

query = query_template % (*SCENARIO, "oai-nr-ue-13")
print(query)
tables = query_api.query(query)
table = tables[0]
print(table)
for idx, record in enumerate(table.records):
    if idx == 0:
        time_axis_ue13.append(SCENARIO_START + 1)
    else:
        time_axis_ue13.append(time_axis_ue13[-1] + 1)
    value = record.get_value()/1e6
    bandwidth_ue13.append(value)
    scenario_ue13.append(value)
print(bandwidth_ue13)

query = query_template % (*SCENARIO, "oai-nr-ue-12")
print(query)
tables = query_api.query(query)
table = tables[0]
print(table)
for idx, record in enumerate(table.records):
    if idx == 0:
        time_axis_ue12.append(SCENARIO_START + 1)
    else:
        time_axis_ue12.append(time_axis_ue12[-1] + 1)
    value = record.get_value()/1e6
    bandwidth_ue12.append(value)
    scenario_ue12.append(value)
print(bandwidth_ue12)

query = query_template % (*SCENARIO, "oai-nr-ue-11")
print(query)
tables = query_api.query(query)
table = tables[0]
print(table)
for idx, record in enumerate(table.records):
    if idx == 0:
        time_axis_ue11.append(SCENARIO_START + 1)
    else:
        time_axis_ue11.append(time_axis_ue11[-1] + 1)
    value = record.get_value()/1e6
    bandwidth_ue11.append(value)
    scenario_ue11.append(value)
print(bandwidth_ue11)

query = query_template % (*SCENARIO, "oai-nr-ue-14")
print(query)
tables = query_api.query(query)
table = tables[0]
print(table)
for idx, record in enumerate(table.records):
    if idx == 0:
        time_axis_ue14.append(SCENARIO_START + 1)
    else:
        time_axis_ue14.append(time_axis_ue14[-1] + 1)
    value = record.get_value()/1e6
    bandwidth_ue14.append(record.get_value()/1e6)
    scenario_ue14.append(record.get_value()/1e6)
print(bandwidth_ue14)

print("UE 1: " + str(np.average(scenario_ue13)))
print("UE 2: " + str(np.average(scenario_ue12)))
print("UE 3: " + str(np.average(scenario_ue11)))
print("UE 4: " + str(np.average(scenario_ue14)))

x_start = SCENARIO_START
x_end   = max(time_axis_ue13[-1], time_axis_ue12[-1], time_axis_ue11[-1], time_axis_ue14[-1])
SCENARIO_END_S4 = x_end
label_x = (x_start + x_end) / 2  # Position the label in the middle of the shaded region
label_y = REGION_LABEL_Y_POSE                 # Position the label above the plot data
ax.text(label_x, label_y, '{S1, S2, S3, S4}', fontsize=8, color='darkred', ha='center', va='center')

############################################ Plot ############################################

extended_data = np.pad(bandwidth_ue13, pad_width=half_window, mode='reflect')
filtered_data = np.convolve(extended_data, weights, mode='valid')
ax.plot(time_axis_ue13, filtered_data, label="UE1 of S1 (SST=1, SD=1)")

extended_data = np.pad(bandwidth_ue12, pad_width=half_window, mode='reflect')
filtered_data = np.convolve(extended_data, weights, mode='valid')
ax.plot(time_axis_ue12, filtered_data, label="UE2 of S2 (SST=1, SD=2)")

extended_data = np.pad(bandwidth_ue11, pad_width=half_window, mode='reflect')
filtered_data = np.convolve(extended_data, weights, mode='valid')
ax.plot(time_axis_ue11, filtered_data, label="UE3 of S3 (SST=1, SD=3)")

extended_data = np.pad(bandwidth_ue14, pad_width=half_window, mode='reflect')
filtered_data = np.convolve(extended_data, weights, mode='valid')
ax.plot(time_axis_ue14, filtered_data, label="UE4 of S4 (SST=1, SD=4)")

ax.axvline(SCENARIO_END_S1, color='black', linestyle='--')
ax.axvline(SCENARIO_END_S2, color='black', linestyle='--')
ax.axvline(SCENARIO_END_S3, color='black', linestyle='--')

# np.save('BW.npy', ((time_axis_ue13, bandwidth_ue13), (time_axis_ue12, bandwidth_ue12), (time_axis_ue11, bandwidth_ue11), (time_axis_ue14, bandwidth_ue14)))

plt.xlabel('Time (seconds)')
plt.ylabel('Downlink Throughput (Mbps)')
# plt.title("UEs' Throughput over Time")
legend = plt.legend(loc='upper right', fontsize='large', frameon=True)
legend.get_frame().set_alpha(0.95)
plt.ylim(-10, 270)
# legend.get_frame().set_facecolor('black')  # Set the background color of the legend
# for text in legend.get_texts():
#     text.set_color('lightgrey')

# plt.gca().spines['top'].set_visible(False)
# plt.gca().spines['right'].set_visible(False)
# plt.gca().spines['left'].set_visible(False)
# plt.gca().spines['bottom'].set_visible(False)

# Save the plot to a PDF file
plt.tight_layout()
plt.savefig('Exp1_Throughput.pdf', format='pdf')
plt.show()


# Close the client
client.close()