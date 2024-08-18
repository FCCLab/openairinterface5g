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
  |> filter(fn: (r) => r["_field"] == "rtt")
  |> yield(name: "last")
"""


############################################ SCENARIO 1 ############################################
SCENARIO_START = 0
time_axis_ue13 = []
rtt_ue13 = []
SCENARIO = [datetime(2024, 8, 17, 22, 52, 34, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
            #   datetime(2024, 8, 17, 22, 55, 34, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
              datetime(2024, 8, 17, 22, 54, 34, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
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
    rtt_ue13.append(record.get_value())
print(time_axis_ue13)
print(rtt_ue13)

x_start = SCENARIO_START
x_end   = time_axis_ue13[-1]
SCENARIO_END_S1 = x_end
label_x = (x_start + x_end) / 2  # Position the label in the middle of the shaded region
label_y = REGION_LABEL_Y_POSE                 # Position the label above the plot data
plt.text(label_x, label_y, '{S1}', fontsize=8, color='darkred', ha='center', va='center')

########################################## SCENARIO 2 ############################################
SCENARIO_START = time_axis_ue13[-1]
time_axis_ue12 = []
rtt_ue12 = []
SCENARIO = [datetime(2024, 8, 17, 23, 0, 45, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
            #   datetime(2024, 8, 17, 23, 5, 30, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
              datetime(2024, 8, 17, 23, 2, 30, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
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
    rtt_ue13.append(record.get_value())
print(rtt_ue13)

query = query_template % (*SCENARIO, "oai-nr-ue-12")
print(query)
tables = query_api.query(query)
table = tables[0]
for idx, record in enumerate(table.records):
    if idx == 0:
        time_axis_ue12.append(SCENARIO_START + 1)
    else:
        time_axis_ue12.append(time_axis_ue12[-1] + 1)
    rtt_ue12.append(record.get_value())
print(rtt_ue12)

x_start = SCENARIO_START
x_end   = max(time_axis_ue13[-1], time_axis_ue12[-1])
SCENARIO_END_S2 = x_end
label_x = (x_start + x_end) / 2  # Position the label in the middle of the shaded region
label_y = REGION_LABEL_Y_POSE                  # Position the label above the plot data
plt.text(label_x, label_y, '{S1, S2}', fontsize=8, color='darkred', ha='center', va='center')

############################################ SCENARIO 3 ############################################
SCENARIO_START = max(time_axis_ue13[-1], time_axis_ue12[-1])
time_axis_ue11 = []
rtt_ue11 = []
SCENARIO = [datetime(2024, 8, 17, 23, 10, 30, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
            #   datetime(2024, 8, 17, 23, 15, 30, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
              datetime(2024, 8, 17, 23, 12, 30, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
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
    rtt_ue13.append(record.get_value())
print(rtt_ue13)

query = query_template % (*SCENARIO, "oai-nr-ue-12")
print(query)
tables = query_api.query(query)
table = tables[0]
for idx, record in enumerate(table.records):
    if idx == 0:
        time_axis_ue12.append(SCENARIO_START + 1)
    else:
        time_axis_ue12.append(time_axis_ue12[-1] + 1)
    rtt_ue12.append(record.get_value())
print(rtt_ue12)

query = query_template % (*SCENARIO, "oai-nr-ue-11")
print(query)
tables = query_api.query(query)
table = tables[0]
for idx, record in enumerate(table.records):
    if idx == 0:
        time_axis_ue11.append(SCENARIO_START + 1)
    else:
        time_axis_ue11.append(time_axis_ue11[-1] + 1)
    rtt_ue11.append(record.get_value())
print(rtt_ue11)

x_start = SCENARIO_START
x_end   = max(time_axis_ue13[-1], time_axis_ue12[-1], time_axis_ue11[-1])
SCENARIO_END_S3 = x_end
label_x = (x_start + x_end) / 2  # Position the label in the middle of the shaded region
label_y = REGION_LABEL_Y_POSE             # Position the label above the plot data
plt.text(label_x, label_y, '{S1, S2, S3}', fontsize=8, color='darkred', ha='center', va='center')

############################################ SCENARIO 4 ############################################
SCENARIO_START = max(time_axis_ue13[-1], time_axis_ue12[-1], time_axis_ue11[-1])
time_axis_ue14 = []
rtt_ue14 = []
SCENARIO = [datetime(2024, 8, 17, 23, 18, 15, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
            #   datetime(2024, 8, 17, 23, 23, 15, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
              datetime(2024, 8, 17, 23, 20, 15, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
            ]

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
    rtt_ue13.append(record.get_value())
print(rtt_ue13)

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
    rtt_ue12.append(record.get_value())
print(rtt_ue12)

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
    rtt_ue11.append(record.get_value())
print(rtt_ue11)

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
    rtt_ue14.append(record.get_value())
print(rtt_ue14)

x_start = SCENARIO_START
x_end   = max(time_axis_ue13[-1], time_axis_ue12[-1], time_axis_ue11[-1], time_axis_ue14[-1])
SCENARIO_END_S4 = x_end
label_x = (x_start + x_end) / 2  # Position the label in the middle of the shaded region
label_y = REGION_LABEL_Y_POSE                 # Position the label above the plot data
plt.text(label_x, label_y, '{S1, S2, S3, S4}', fontsize=8, color='darkred', ha='center', va='center')

############################################ Plot ############################################
plt.plot(time_axis_ue13, rtt_ue13, label="UE1 of S1 (SST=1, SD=1)")
plt.plot(time_axis_ue12, rtt_ue12, label="UE2 of S2 (SST=1, SD=2)")
plt.plot(time_axis_ue11, rtt_ue11, label="UE3 of S3 (SST=1, SD=3)")
plt.plot(time_axis_ue14, rtt_ue14, label="UE4 of S4 (SST=1, SD=4)")

plt.axvline(SCENARIO_END_S1, color='black', linestyle='--')
plt.axvline(SCENARIO_END_S2, color='black', linestyle='--')
plt.axvline(SCENARIO_END_S3, color='black', linestyle='--')

# np.save('RTT.npy', ((time_axis_ue13, rtt_ue13), (time_axis_ue12, rtt_ue12), (time_axis_ue11, rtt_ue11), (time_axis_ue14, rtt_ue14)))

plt.xlabel('Time (seconds)')
plt.ylabel('Round Trip Time (ms)')
legend = plt.legend(loc='upper right', fontsize='large', frameon=True)
legend.get_frame().set_alpha(0.95)

plt.ylim([-5, 135])

# Save the plot to a PDF file
plt.savefig('RTT.pdf', format='pdf')
plt.show()

# Close the client
client.close()