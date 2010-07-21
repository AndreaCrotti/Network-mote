#!/bin/bash
# setup the TAP device given as first argument as default gateway for ipv6
# first make sure that the device exists, call tun_setup to set it up
TAP=$1
# It might be a good idea to use the address of the gateway here... (not sure)
ADDR="10.0.0.1"

echo "setting up tunnel interface: $TAP"

which ip > /dev/null 2>&1

if [ "$?" != "0" ]
then
    echo "you need iproute2 to setup ipv6 routing"
    exit 1
fi

# Set the device up and assign it the IP-address
ip link set $TAP up
ip addr add $ADDR dev $TAP

# once we have an IP and a netmask we can use it as a real network interface
# ifconfig $TUN $ADDR netmask 255.255.255.0 not needed anymore
ip route del default
ip route add default via $ADDR dev $TAP
