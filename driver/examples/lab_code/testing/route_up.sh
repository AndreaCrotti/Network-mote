#!/bin/zsh

echo "Lokale Routen setzen"
route add silver-2 gw silver-1
route add silver-1 dev eth0
route del default
route del -net 137.226.154.0 netmask 255.255.255.0

echo 0 > /proc/sys/net/ipv4/conf/**/*redir*
