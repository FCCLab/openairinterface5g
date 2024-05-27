<table style="border-collapse: collapse; border: none;">
  <tr style="border-collapse: collapse; border: none;">
    <td style="border-collapse: collapse; border: none;">
      <a href="http://www.openairinterface.org/">
         <img src="../../../doc/images/oai_final_logo.png" alt="" border=3 height=50 width=150>
         </img>
      </a>
    </td>
    <td style="border-collapse: collapse; border: none; vertical-align: center;">
      <b><font size = "5">OAI O-RAN 7.2 Front-haul Docker Compose</font></b>
    </td>
  </tr>
</table>

![Docker deploy 7.2](../../../doc/images/docker-deploy-oai-7-2.png)

This docker-compose is designed to use `OAI-gNB` with a 7.2 compatible Radio Unit. Before using this docker compose you have to configure the host machine as per the [ORAN_FHI7.2_Tutorial](../../../doc/ORAN_FHI7.2_Tutorial.md). The container image used by the docker compose file is tested only on `Ubuntu 22.04` docker host. The image is present in our official docker hub account `docker.io/oaisoftwarealliance/oai-gnb-7.2:develop`. 


## Build Image (Optional)

If you wish to make your own image after modification or from a specific branch then you can build it manually, 

```bash
cd ../../../
docker build -f docker/Dockerfile.base.ubuntu22 -t ran:base:latest .
docker build -f docker/Dockerfile.fhi72.build.ubuntu22 -t ran:build:latest .
docker build -f docker/Dockerfile.fhi72.gNB.ubuntu22 -t oai-gnb-7.2:<your-image-tag> .
```

The new image will be `oai-gnb-7.2:<your-image-tag>`. You will have to replace this new image with `docker.io/oaisoftwarealliance/oai-gnb-7.2:develop`. 

## Configure Networking 

### SR-IOV Virtual Functions (VFs)

In docker-compose environment there is no automated method to configure the VFs on the fly. The user will have to manually configure C/U plane VFs before starting the container `OAI-gNB`. You can follow the step [configure-network-interfaces-and-dpdk-vfs](../../../doc/ORAN_FHI7.2_Tutorial.md#configure-network-interfaces-and-dpdk-vfs).

### Interface towards AMF (N2)

For N2 interface we are using `macvlan` driver of docker. 

You can use the `bridge` driver, in situation 

- When the core network is running on the same machine 
- or different machine but you have configured needed `ip route` and forwarding to access the core network from RAN host.  

To configure docker `macvlan` network you need to choose `ipam.config` and `driver_opts.parent` are per your environment

```
    oai-net:
        driver: macvlan
        name: oai-net
        ipam:
            config:
                - subnet: "172.21.16.0/22"
                  ip_range: "172.21.18.20/32"
                  gateway: "172.21.19.254"
        driver_opts:
            com.docker.network.bridge.name: "oai-net"
            parent: enp193s0f0
```

To configure `bridge` network you need to choose `ipam.config.subnet` as per your environment. 

```
    oai-net:
        driver: bridge
        name: oai-net
        ipam:
            config:
                - subnet: 192.168.72.128/26
        driver_opts:
            com.docker.network.bridge.name: "oai-net"
```

## Deploy OAI-gNB Container

The [configuration file](../../../targets/PROJECTS/GENERIC-NR-5GC/CONF/gnb.sa.band77.273prb.fhi72.4x4-vvdn.conf) used in by this docker compose is configured for VVDN GEN 3 RU (firmware version 03-v3.0.5). 

```bash
docker-compose up -d
```

