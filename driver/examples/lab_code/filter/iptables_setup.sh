#!/bin/bash

############################################################
# This file is part of ALPHA                               #
# used to add or delete iptables rules for the alphafilter #
#                                                          #
#           written by Florian Weingarten, Johannes Gilger #
############################################################

PORT=1234

call_iptables() {
	iptables -$1 OUTPUT -p udp --dport $PORT -j QUEUE
	iptables -$1  INPUT -p udp --dport $PORT -j QUEUE
	iptables -$1  FORWARD -p udp --dport $PORT -j QUEUE
}

case "$1" in
	start)
		call_iptables "A"
		;;
	stop)
		call_iptables "D"
		;;
	*)
		echo "Usage: $0 <start|stop>"
esac
:
