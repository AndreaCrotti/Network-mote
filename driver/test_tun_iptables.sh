#!/bin/bash
# setup a tun device and setup some iptables rules for it

set -x
TUN=tun1

killall test_tun
rm test_tun
gcc -Wall -o test_tun test_tun_iptables.c
# start everything and in parallel do this
./test_tun &
sleep 3

IPT="/usr/sbin/iptables"
ip addr add 10.0.0.1 dev $TUN

$IPT -F
$IPT -Z
$IPT -A INPUT -i $TUN -j LOG --log-level 4
$IPT -A OUTPUT -o $TUN -j LOG --log-level 4
