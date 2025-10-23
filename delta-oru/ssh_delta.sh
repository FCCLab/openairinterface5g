sudo ifconfig enp1s0f0 10.101.131.126/24 up

sshpass -p "delta_amsbd!" ssh root@10.101.131.127 -o HostKeyAlgorithms=+ssh-rsa -o PubkeyAcceptedAlgorithms=+ssh-rsa
