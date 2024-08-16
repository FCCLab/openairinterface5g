import subprocess
import re
from influxdb_client import InfluxDBClient, Point, WritePrecision

import influxdb_client, os, time
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

# InfluxDB setup
INFLUXDB_TOKEN = "vZx9j5ONbI6ylC7pj_jVYHsbUqlF4KqeKM_tMtQzAvnOwh-GGf-peaUidDhfiV-_1wynQx8AxpHq0yP0agVOpw=="
INFLUXDB_URL = "http://172.27.2.21:8086"
INFLUXDB_ORG = "FCCLab"
INFLUXDB_BUCKET = "OAI"

# iperf3 setup
IPERF3_SERVER = "172.27.2.21"
IPERF3_PORTS = ["5201", "5202", "5203", "5204", "5205", "5206", "5207"]

# Function to run iperf3 and stream output line by line
def run_iperf3(bind):
    while True:
        for port in IPERF3_PORTS:
            print("Try with port " + port)
            command = f"iperf3 -c {IPERF3_SERVER} -B {bind} -p {port} -t 0 -i 1 --forceflush"
            print(command)
            process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            while process.poll() is None:
                line = process.stdout.readline()
                print(line, end="")
                if "connected to" in line:
                    print("Found iperf3 on port " + port)
                    return process
        time.sleep(1)

# Function to parse iperf3 output and send to InfluxDB
def parse_line(line):
    # Example output to match: "[  4]   0.00-10.00  sec  1.02 GBytes  876 Mbits/sec"
    match = re.search(r"\[.*?\]\s+\d+\.\d+-\d+\.\d+\s+sec\s+([\d\.]+)\s+(\w+)\s+([\d\.]+)\s+(\w+)/sec", line)
    
    if match:
        data_transferred = float(match.group(1))
        data_unit = match.group(2)
        bandwidth = float(match.group(3))
        bandwidth_unit = match.group(4)

        # Convert units if necessary
        if data_unit == "KBytes":
            data_transferred *= 1024
        elif data_unit == "MBytes":
            data_transferred *= 1024 ** 2
        elif data_unit == "GBytes":
            data_transferred *= 1024 ** 3

        if bandwidth_unit == "Kbits":
            bandwidth *= 1e3
        elif bandwidth_unit == "Mbits":
            bandwidth *= 1e6
        elif bandwidth_unit == "Gbits":
            bandwidth *= 1e9
        return (data_transferred, bandwidth)
    
    return (None, None)

# Main loop to run iperf3 and publish results in real-time
def main():
    interface_ip = None
    while True:
        result = subprocess.run(['ip', 'addr', 'show', 'oaitun_ue1'], capture_output=True, text=True)
        print("Getting IP address of oaitun_ue1")
        if result.returncode == 0:
            matches = re.findall(r'inet (\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})', result.stdout)
            if len(matches) > 0:
                interface_ip = matches[0]
                break
        time.sleep(1)
    
    HOSTNAME = os.getenv("HOSTNAME")
    print(f"{HOSTNAME} oaitun_ue1 IP address: {interface_ip}")

    try:
        write_client = influxdb_client.InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
        write_api = write_client.write_api(write_options=SYNCHRONOUS)
        
        print("Starting iperf3")
        process = run_iperf3(interface_ip)
        
        while process.poll() is None:
            line = process.stdout.readline()
            print(line, end="")	
            (data_transferred, bandwidth) = parse_line(line)
            if (data_transferred is None) or (bandwidth is None):
                continue

            point = (
                Point(HOSTNAME)
                .field("data_transferred_bytes", data_transferred)
                .field("bandwidth_bps", bandwidth)
            )
            write_api.write(bucket=INFLUXDB_BUCKET, org=INFLUXDB_ORG, record=point)
            print(f"Sent data: {data_transferred} bytes, {bandwidth} bps")
    except KeyboardInterrupt:
        print("Stopped by the user")
    finally:
        write_client.close()

if __name__ == "__main__":
    main()
