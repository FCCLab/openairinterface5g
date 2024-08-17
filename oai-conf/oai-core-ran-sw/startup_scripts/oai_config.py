import os
import subprocess

# InfluxDB setup
INFLUXDB_TOKEN = "vZx9j5ONbI6ylC7pj_jVYHsbUqlF4KqeKM_tMtQzAvnOwh-GGf-peaUidDhfiV-_1wynQx8AxpHq0yP0agVOpw=="
INFLUXDB_URL = "http://172.27.2.21:8086"
INFLUXDB_ORG = "FCCLab"
INFLUXDB_BUCKET = "OAI"

HOSTNAME = subprocess.check_output(["hostname"]).decode('utf-8').strip()
if HOSTNAME is "":
    HOSTNAME = "localhost"
    
print("-"*20)
print("INFLUXDB_TOKEN", INFLUXDB_TOKEN)
print("INFLUXDB_URL", INFLUXDB_URL)
print("INFLUXDB_ORG", INFLUXDB_ORG)
print("INFLUXDB_BUCKET", INFLUXDB_BUCKET)
print("HOSTNAME", HOSTNAME)
print("-"*20)
