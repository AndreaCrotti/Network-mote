#!/usr/bin/env zsh
# this script is used to set up the forwarding on the router as well as 
# disabling the annoying routing optimiziation done by the linux kernel

# usage: ./redirect_forwarding.sh <client|router> <router-ip> <client-ip>

if [ ! $# -eq 3 ]; then
	echo "** Usage: ./redirect_forwarding.sh <client|router> <router-ip> <client-ip>";
	exit;
fi

echo "** Disabling all redirects"
echo "0" > /proc/sys/net/ipv4/conf/**/*redirect*

MODE=$1
ROUTER_IP=$2
CLIENT_IP=$3

if [ $MODE = "router" ]; then
	echo "** Enabling forwarding"
	echo "1" >/proc/sys/net/ipv*/conf/all/forwarding
fi

if [ $MODE = "client" ]; then
	if [ -n $CLIENT_IP && -n $ROUTER_IP ]; then
		echo "** Adding route to host $CLIENT_IP via $ROUTER_IP"
		route add -host $CLIENT_IP netmask 0.0.0.0 gw $ROUTER_IP
	fi
fi
