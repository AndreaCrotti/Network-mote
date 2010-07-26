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
ETH=$2
ETH_IP=192.168.226.131

ip link set $TUN up
ip addr add 10.0.0.254 dev $TUN

$IPT -t nat -F
$IPT -Z

# from what they say on the internet THIS rule should be enough
# $IPT -A POSTROUTING -t nat -s 10.1.1.1 -o eth0 -j SNAT --to-source192.168.1.2

# or also this one
# $IPT -t nat -I POSTROUTING -j MASQUERADE

$IPT -t nat -A POSTROUTING -o $ETH -j LOG --log-prefix "Packet coming in on tun: " --log-level 7
#$IPT -A OUTPUT -o $TUN -j LOG --log-prefix "Packet going out to tun" --log-level 7

$IPT -t nat -A PREROUTING -i $TUN -j DNAT --to-destination $ETH_IP

$IPT -t nat -A POSTROUTING -o $ETH -j MASQUERADE
# accept stuff from the network which is connected
$IPT -A FORWARD -i $ETH -o $TUN -m state --state RELATED,ESTABLISHED -j ACCEPT
# accept everything from tap to external network
$IPT -A FORWARD -i $TUN -o $ETH -j ACCEPT
