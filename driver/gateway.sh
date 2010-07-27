#!/bin/bash
# setup the gateway to act as a nat machine
# see http://www.revsys.com/writings/quicktips/nat.html to understand it

set -x
sysctl -w net.ipv4.ip_forward=1

IPT="iptables"

if [ $# -lt 2 ]
then
    echo "./gateway.sh <tun> <external>"
    exit 1
fi

TUN=$1
TUN_IP=10.0.0.254
ETH=$2
ETH_IP=$(ifconfig $ETH | grep "inet addr:" | awk -F: '{ print $2 }' | awk '{ print $1 }')

ip link set $TUN up
ip addr add $TUN_IP dev $TUN

$IPT -t nat -F
$IPT -Z

# from what they say on the internet THIS rule should be enough
$IPT -A POSTROUTING -t nat -s $TUN_IP -o $ETH -j SNAT --to-source $ETH_IP --log-prefix "Packet coming in on tun: " --log-level 7

# or also this one
# $IPT -t nat -I POSTROUTING -j MASQUERADE

# $IPT -t nat -A POSTROUTING -o $ETH -j LOG --log-prefix "Packet coming in on tun: " --log-level 7
# #$IPT -A OUTPUT -o $TUN -j LOG --log-prefix "Packet going out to tun" --log-level 7

# $IPT -t nat -A PREROUTING -i $TUN -j DNAT --to-destination $ETH_IP

# $IPT -t nat -A POSTROUTING -o $ETH -j MASQUERADE
# # accept stuff from the network which is connected
# $IPT -A FORWARD -i $ETH -o $TUN -m state --state RELATED,ESTABLISHED -j ACCEPT
# # accept everything from tap to external network
# $IPT -A FORWARD -i $TUN -o $ETH -j ACCEPT
