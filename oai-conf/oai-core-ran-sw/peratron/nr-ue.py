import subprocess
import re
from influxdb_client import InfluxDBClient, Point, WritePrecision

import influxdb_client, os, time
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

import sys

# InfluxDB setup

INFLUXDB_TOKEN = "nwkbR7yXpi9JuaFbsMRS5cAB0rIJGJ8c02sjIbLZNRGMV5t-npUM4iri72frr4Gm8TOLyT9aS5Vpe02Eb0s41g=="
INFLUXDB_URL = "http://172.27.2.21:8086"
INFLUXDB_ORG = "FCCLab"
INFLUXDB_BUCKET = "OAI"

# iperf3 setup
IPERF3_SERVER = "172.27.2.21"
IPERF3_PORTS = ["5201", "5202", "5203", "5204", "5205", "5206", "5207"]

if len(sys.argv) > 2:
    INTERFACE_NAME = sys.argv[1]
    ACTION = sys.argv[2]
else:
    print("Usage: python3 nr-ue.py <interface_name> <ping|iperf>")
    sys.exit(1)

# Function to run iperf3 and stream output line by line
def run_iperf3(bind, bind_dev):
    while True:
        for port in IPERF3_PORTS:
            print("Try with port " + port)
            command = f"iperf3 -c {IPERF3_SERVER} -B {bind} -p {port} -t 0 -i 1 --forceflush -R -P 10 --bind-dev {bind_dev} "
            print(command)
            process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            while process.poll() is None:
                line = process.stdout.readline()
                print(line, end="")
                if "connected to" in line:
                    print("Found iperf3 on port " + port)
                    return process
        time.sleep(1)

def run_ping(bind_dev):
    command = f"ping -I {bind_dev} {IPERF3_SERVER}"
    print(command)
    process = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    while process.poll() is None:
        line = process.stdout.readline()
        print(line, end="")
        if "bytes from" in line:
            return process

# Function to parse iperf3 output and send to InfluxDB
def parse_iperf3_line(line):
    # Example output to match: "[SUM]   0.00-10.00  sec  1.02 GBytes  876 Mbits/sec"
    match = re.search(r"\[SUM\]\s+\d+\.\d+-\d+\.\d+\s+sec\s+([\d\.]+)\s+(\w+)\s+([\d\.]+)\s+(\w+)/sec", line)
    
    if match:
        print(line, end="")
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

def parse_ping_line(line):
    match = re.search(r"icmp_seq=(\d+) ttl=(\d+) time=([\d\.]+) ms", line)
    if match:
        icmp_seq = int(match.group(1))
        ttl = int(match.group(2))
        rtt = float(match.group(3))
        return (icmp_seq, ttl, rtt)
    return (None, None, None)

# Main loop to run iperf3 and publish results in real-time
def main():
    interface_ip = None
    while True:
        result = subprocess.run(['ip', 'addr', 'show', INTERFACE_NAME], capture_output=True, text=True)
        print(f"Getting IP address of {INTERFACE_NAME}")
        # print(result.stdout)
        if result.returncode == 0:
            matches = re.findall(r'inet (\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})', result.stdout)
            if len(matches) > 0:
                interface_ip = matches[0]
                HOSTNAME = "oai-nr-ue-" + matches[0].split('.')[2]
                break
        time.sleep(1)

    print(f"{HOSTNAME} {INTERFACE_NAME} IP address: {interface_ip}")

    while True:
        try:
            write_client = influxdb_client.InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
            write_api = write_client.write_api(write_options=SYNCHRONOUS)
            
            if ACTION == "ping":
                print("Starting ping")
                process = run_ping(INTERFACE_NAME)
                while process.poll() is None:
                    line = process.stdout.readline()
                    print(line, end="")
                    (icmp_seq, ttl, rtt) = parse_ping_line(line)
                    if (icmp_seq is None) or (ttl is None) or (rtt is None):
                        continue

                    point = (
                        Point(HOSTNAME)
                        .field("ttl", ttl)
                        .field("rtt", rtt)
                    )
                    write_api.write(bucket=INFLUXDB_BUCKET, org=INFLUXDB_ORG, record=point)
                    print(f"Sent ping: {icmp_seq}, {ttl}, {rtt}")
            else:
                print("Starting iperf3")
                process = run_iperf3(interface_ip, INTERFACE_NAME)

                while process.poll() is None:
                    line = process.stdout.readline()
                    # print(line, end="")	
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
            exit()
        except Exception as e:
            print("-------------------------------------------------")
            print(e)
            print("-------------------------------------------------")
        finally:
            write_client.close()
            time.sleep(1)

if __name__ == "__main__":
    main()
