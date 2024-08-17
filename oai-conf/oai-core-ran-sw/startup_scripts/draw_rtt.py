#!/usr/bin/env python3

import matplotlib.pyplot as plt
from datetime import datetime, timezone, timedelta
from influxdb_client import InfluxDBClient, Point, QueryApi
import numpy as np

from oai_config import *

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
SCENARIO = [datetime(2024, 8, 17, 22, 50, 34, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
              datetime(2024, 8, 17, 22, 55, 34, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
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

########################################## SCENARIO 2 ############################################
SCENARIO_START = time_axis_ue13[-1]
time_axis_ue12 = []
rtt_ue12 = []
SCENARIO = [datetime(2024, 8, 17, 23, 0, 45, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
              datetime(2024, 8, 17, 23, 5, 30, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
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


############################################ SCENARIO 3 ############################################
SCENARIO_START = max(time_axis_ue13[-1], time_axis_ue12[-1])
time_axis_ue11 = []
rtt_ue11 = []
SCENARIO = [datetime(2024, 8, 17, 23, 10, 30, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
              datetime(2024, 8, 17, 23, 15, 30, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
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

############################################ SCENARIO 4 ############################################
SCENARIO_START = max(time_axis_ue13[-1], time_axis_ue12[-1], time_axis_ue11[-1])
time_axis_ue14 = []
rtt_ue14 = []
SCENARIO = [datetime(2024, 8, 17, 23, 18, 15, tzinfo=local_tz).astimezone(timezone.utc).isoformat(),
              datetime(2024, 8, 17, 23, 23, 15, tzinfo=local_tz).astimezone(timezone.utc).isoformat()
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


############################################ Plot ############################################
plt.plot(time_axis_ue13, rtt_ue13, label="UE3 Slice: SST=1, SD=3")
plt.plot(time_axis_ue12, rtt_ue12, label="UE2 Slice: SST=1, SD=2")
plt.plot(time_axis_ue11, rtt_ue11, label="UE1 Slice: SST=1, SD=1")
plt.plot(time_axis_ue14, rtt_ue14, label="UE4 Slice: SST=1, SD=4")

# np.save('RTT.npy', ((time_axis_ue13, rtt_ue13), (time_axis_ue12, rtt_ue12), (time_axis_ue11, rtt_ue11), (time_axis_ue14, rtt_ue14)))

plt.xlabel('Time')
plt.ylabel('RTT ms')
plt.title("UEs' RTT to N6 Interface")
plt.legend(loc='upper right', frameon=False, fontsize='large')
plt.show()

# Close the client
client.close()