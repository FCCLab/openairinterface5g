FROM ubuntu:jammy

RUN apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get upgrade --yes && \
    DEBIAN_FRONTEND=noninteractive apt-get install --yes \
        iputils-ping \
        net-tools \
        bridge-utils \
        iperf3 \
        iproute2 \
        python3 pip

RUN pip3 install influxdb_client psutil
