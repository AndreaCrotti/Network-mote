#!/bin/bash
# setup the TUN device tun$1 as default gateway for ipv6
if [ ! $(which ip) ]
then
    echo "you need iproute2 to setup ipv6 routing"
    exit 1
fi

# check if the address is correct
ip route add default via fec0::1 dev tun$1
