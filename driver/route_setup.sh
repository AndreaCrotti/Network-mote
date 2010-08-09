#!/bin/bash
TUN=$1
# It might be a good idea to use the address of the gateway here... (not sure)
ADDR="10.0.0.1"

echo "setting up tunnel interface: $TUN"

which ip > /dev/null 2>&1

if [ "$?" != "0" ]
then
    echo "you need iproute2 to setup the routing"
    exit 1
fi

# Set the device up and assign it the IP-address
ip link set $TUN up
ip addr add $ADDR dev $TUN

# delete the default and add a route via our tun device
ip route del default
ip route add default via $ADDR dev $TUN
