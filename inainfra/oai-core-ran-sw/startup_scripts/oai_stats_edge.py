import time

from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

from oai_config import *

def get_cpu_usage():
    with open("/sys/fs/cgroup/oai_cgroup_edge.slice/cpu.stat") as f:
        lines = f.readlines()
        for line in lines:
            if line.startswith("nr_periods"):
                nr_periods_start = int(line.split()[1])
            if line.startswith("nr_throttled"):
                nr_throttled_start = int(line.split()[1])
            if line.startswith("throttled_usec"):
                throttled_usec_start = int(line.split()[1])
            if line.startswith("user_usec"):
                user_usec_start = int(line.split()[1])
            if line.startswith("system_usec"):
                system_usec_start = int(line.split()[1])

    time.sleep(1)
    with open("/sys/fs/cgroup/oai_cgroup_edge.slice/cpu.stat") as f:
        lines = f.readlines()
        for line in lines:
            if line.startswith("nr_periods"):
                nr_periods_end = int(line.split()[1])
            if line.startswith("nr_throttled"):
                nr_throttled_end = int(line.split()[1])
            if line.startswith("throttled_usec"):
                throttled_usec_end = int(line.split()[1])
            if line.startswith("user_usec"):
                user_usec_end = int(line.split()[1])
            if line.startswith("system_usec"):
                system_usec_end = int(line.split()[1])

    nr_periods = nr_periods_end - nr_periods_start
    nr_throttled = nr_throttled_end - nr_throttled_start
    throttled_usec = throttled_usec_end - throttled_usec_start
    user_usec = user_usec_end - user_usec_start
    system_usec = system_usec_end - system_usec_start

    cpu_usage_usec = (user_usec + system_usec)
    return cpu_usage_usec

def collect_and_send_throughput(write_api):
    cpu_usage_usec = get_cpu_usage()
    point = (
        Point(f"{HOSTNAME}_edge")
        .field("cpu_usage_usec", cpu_usage_usec)
    )
    write_api.write(bucket=INFLUXDB_BUCKET, org=INFLUXDB_ORG, record=point)       
    print(f"CPU usage: {cpu_usage_usec} usec")

def main():
    write_client = InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
    write_api = write_client.write_api(write_options=SYNCHRONOUS)
    while True:
        collect_and_send_throughput (write_api)

if __name__ == "__main__":
    while True:
        try:
            main()
        except KeyboardInterrupt:
            print("Stopped by the user")
            break
        except Exception as e:
            print(e)
            # time.sleep(1)
            break
