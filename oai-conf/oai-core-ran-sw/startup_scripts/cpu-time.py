import subprocess
import time
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS

from oai_config import *

# Initial CPU quota and decrement step
initial_cpu_quota = 250000
decrement_step = 10000  # Amount to decrease the quota by each second
cpu_period = 1000000
container_name = 'oai-cu-up-2'

# Function to update Docker container CPU quota
def update_cpu_quota(cpu_quota):
    command = [
        'docker', 'update',
        f'--cpu-quota={cpu_quota}',
        f'--cpu-period={cpu_period}',
        container_name
    ]
    subprocess.run(command)

def main():
    write_client = InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
    write_api = write_client.write_api(write_options=SYNCHRONOUS)
    current_cpu_quota = initial_cpu_quota
    while True:
        if current_cpu_quota < 5000:
            break
        point = (
            Point(container_name)
            .field("cpu_quota", current_cpu_quota)
        )
        write_api.write(bucket=INFLUXDB_BUCKET, org=INFLUXDB_ORG, record=point)       
        update_cpu_quota(current_cpu_quota)
        print(f'Updated CPU quota to {current_cpu_quota}')
        current_cpu_quota -= decrement_step
        time.sleep(40)

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