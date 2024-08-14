#/bin/bash

wait_interface_up() {
    local interface=$1
    local counter=0

    # echo "Waiting for $interface"
    # counter=0
    # while ! ip link show "$interface" 2> /dev/null; do
    #     sleep 1
    #     echo "Waiting for interface $interface: $counter"
    #     ((counter++))
    # done

    # echo "Waiting for $interface to be up"
    # counter=0
    # while ! ip link show "$interface" | grep -q "state UP"; do
    #     sleep 1
    #     echo "Waiting for interface $interface to be up: $counter"
    #     ((counter++))
    # done
    
    # echo "Waiting for $interface to have IPv4 address"
    # counter=0
    # while ! ip -4 addr show "$interface" | grep -q "inet.*global"; do
    #     sleep 1
    #     echo "Waiting for interface $interface to have IPv4 address: $counter"
    #     ((counter++))
    # done
    # echo ""

    echo "Waiting for $interface"
    counter=0
    while ! ifconfig | grep -q "$interface"; do
        sleep 1
        echo "Waiting for interface $interface: $counter"
        ((counter++))
    done

    echo "Waiting for $interface to be up"
    counter=0
    while ! ifconfig | grep -q "$interface.*UP"; do
        sleep 1
        echo "Waiting for interface $interface to be up: $counter"
        ((counter++))
    done
    
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

    # ifconfig $interface
}

