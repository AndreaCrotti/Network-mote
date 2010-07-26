#!/bin/bash
# setup the gateway to act as a nat machine
# see http://www.revsys.com/writings/quicktips/nat.html to understand it

set -x
echo 1 > /proc/sys/net/ipv4/ip_forward
IPT="iptables"

if [ $# -lt 2 ]
then
    echo "./gateway.sh <tun> <external>"
    exit 1
fi

TUN=$1
ETH=$2

ifconfig $TUN up

$IPT -F
$IPT -Z

$IPT -t nat -A POSTROUTING -o $ETH -j MASQUERADE
# accept stuff from the network which is connected
$IPT -A FORWARD -i $ETH -o $TUN -m state --state RELATED,ESTABLISHED -j ACCEPT
# accept everything from tap to external network
$IPT -A FORWARD -i $TUN -o $ETH -j ACCEPT
