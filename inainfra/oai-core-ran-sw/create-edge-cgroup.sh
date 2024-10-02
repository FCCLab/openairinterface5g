sudo cgcreate -g cpu,memory:/oai_cgroup_edge.slice

sudo cgset -r cpu.max='300000 1000000' /oai_cgroup_edge.slice

cgget -r cpu.max /oai_cgroup_edge.slice

# Average CPU Usage of oai-cu-up-1: 48.92641044776119
# Average CPU Usage of oai-upf-slice-1: 59.99614179104478
# Average CPU Usage of oai-cu-up-4: 75.55089629629629
# Average CPU Usage of oai-upf-slice-4: 86.47834074074075
# 270.951789275843

docker update --cpu-quota 20000 --cpu-period 1000000 oai-cu-up-1
docker update --cpu-quota 30000 --cpu-period 1000000 oai-cu-up-4
docker update --cpu-quota 80000 --cpu-period 1000000 oai-cu-up-5
