#!/bin/bash
# setup the gateway to act as a nat machine
# see http://www.revsys.com/writings/quicktips/nat.html to understand it

echo 1 > /proc/sys/net/ipv4/ip_forward
IPT="/sbin/iptables"

if [ $# -lt 2 ]
then
    echo "./gateway.sh <tap> <external>"
    exit 1
fi

TAP=$1
ETH=$2

$IPT -t nat -A POSTROUTING -o $ETH -j MASQUERADE
# accept stuff from the network which is connected
$IPT -A FORWARD -i $ETH -o $TAP -m state --state RELATED,ESTABLISHED -j ACCEPT
# accept everything from tap to external network
$IPT -A FORWARD -i $TAP -o $ETH -j ACCEPT