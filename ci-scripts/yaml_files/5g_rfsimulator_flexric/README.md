# How to build necessary docker images for oai-gnb and oai-nr-ue (optional)
building gNB images
```bash
docker build --target ran-base --tag ran-base:latest --file docker/Dockerfile.base.ubuntu .
docker build -t 10.1.110.84:5000/ran-build-e2-kpmv301-e2apv2:0.0.1 -f docker/Dockerfile.build.ubuntu .
docker build -t 10.1.110.84:5000/oai-gnb-e2-kpmv301-e2apv2:0.0.1 --file docker/Dockerfile.gNB.ubuntu.rfsim .
```
building ue images
```bash
docker build --tag 10.1.110.84:5000/oai-ue-e2-kpmv301-e2apv2:0.0.1 --file docker/Dockerfile.nrUE.ubuntu.rfsim .
```
# How to connect the E2 termination point of OSC near-rt-ric
1. Find out what is the kubernetes Node IP of the kubernetes node you use for the near-rt-ric.
2. To adjust the conf file for the gNB to match your e2 termination point, run the command:
```bash
export HOST_IP_FOR_E2_TERM=<YOUR_NODE_IP>
envsubst < ci-scripts/conf_files/gnb.sa.band78.106prb.rfsim.flexric.conf.template > ci-scripts/conf_files/gnb.sa.band78.106prb.rfsim.flexric.conf
```
3. Ensure your OSC near-rt-ric is running, check pods in the `ricplt` namespace.
```bash
kubectl get pods -n ricplt
```
4. Bring up the rfsim5g-oai-cn core network containers:
```bash
docker compose up -d mysql oai-amf oai-smf oai-upf oai-ext-dn
```
5. Bring up the rfsim5g-oai-gnb container:
```bash
docker-compose up -d oai-gnb
```
6. Bring up the rfsim5g-oai-nr-ue container:
```bash
docker compose up -d oai-nr-ue
```
7. Check that the UE managed to register to the core
```bash
docker logs rfsim5g-oai-nr-ue | grep FGS_REGISTRATION_ACCEPT
```
Output should be:
```log
[NAS]    I [UE 0] Received NAS_DOWNLINK_DATA_IND type FGS_REGISTRATION_ACCEPT with length 59
```
8. Now you can start the xApps, refer to the 