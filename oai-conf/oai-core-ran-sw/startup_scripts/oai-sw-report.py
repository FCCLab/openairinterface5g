#!/usr/bin/env python3

from oai_config import *

import psutil, os, time
import influxdb_client
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS
from threading import Thread

def get_network_throughput(interface, interval=1):
    net_io = psutil.net_io_counters(pernic=True)
    
    if interface not in net_io:
        print(f"Interface {interface} not found.")
        return (0, 0)

    initial_stats = net_io[interface]
    initial_bytes_recv = initial_stats.bytes_recv
    initial_bytes_sent = initial_stats.bytes_sent

    time.sleep(interval)

    net_io = psutil.net_io_counters(pernic=True)
    final_stats = net_io[interface]
    final_bytes_recv = final_stats.bytes_recv
    final_bytes_sent = final_stats.bytes_sent

    downlink_throughput = (final_bytes_recv - initial_bytes_recv) / interval
    if downlink_throughput < 0:
        downlink_throughput = 0
        
    uplink_throughput = (final_bytes_sent - initial_bytes_sent) / interval
    if uplink_throughput < 0:
        uplink_throughput = 0

    return downlink_throughput, uplink_throughput

def collect_and_send_throughput(write_api):
    net_io_prev = psutil.net_io_counters(pernic=True)
    time.sleep(1)
    net_io_now = psutil.net_io_counters(pernic=True)
    
    for interface in net_io_now:
        # if interface != "if-du-f1uc":
        #     continue

        if interface not in net_io_prev:
            continue
        
        bytes_send = 8. * (net_io_now[interface].bytes_sent - net_io_prev[interface].bytes_sent)/1.0
        bytes_recv = 8. * (net_io_now[interface].bytes_recv - net_io_prev[interface].bytes_recv)/1.0

        point = (
            Point(HOSTNAME)
            .tag("interface", interface)
            .field("downlink", bytes_send)
            .field("uplink", bytes_recv)
        )
        write_api.write(bucket=INFLUXDB_BUCKET, org=INFLUXDB_ORG, record=point)       
        print(f"Interface {interface}: Sent data: {bytes_send} bits down, {bytes_recv} bits up")

    # time.sleep(1)

def main():
    write_client = influxdb_client.InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
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
