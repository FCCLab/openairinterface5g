#/bin/bash

wait_interface_exist() {
    local interface=$1
    local counter=0

    echo "Waiting for $interface"
    counter=0
    while ! ifconfig | grep -q "$interface"; do
        sleep 1
        echo "Waiting for interface $interface: $counter"
        ((counter++))
    done
}

wait_interface_up() {
    local interface=$1
    local counter=0

    wait_interface_exist $interface

    echo "Waiting for $interface to be up"
    counter=0
    while ! ifconfig | grep -q "$interface.*UP"; do
        sleep 1
        echo "Waiting for interface $interface to be up: $counter"
        ((counter++))
    done
}

wait_interface_have_ipv4() {
    local interface=$1
    local counter=0

    $wait_interface_up $interface
    
    echo "Waiting for $interface to have IPv4 address"
    while true; do
        IP=$(ifconfig $interface | grep -oP 'inet\s+\K\d+(\.\d+){3}')
        if [ ! -z "$IP" ]; then
            echo "Interface $interface has IP address: $IP"
            break
        fi
        sleep 1
        ((counter++))
        echo "Waiting for interface $interface to have IPv4 address: $counter"
    done
}
