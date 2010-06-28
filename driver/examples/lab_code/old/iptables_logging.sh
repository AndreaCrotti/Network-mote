#!/bin/bash

#
# This script is part of ALPHA
# used for developing purposes
# adds iptables logging rules for all tun traffic and encapsulated traffic
#
# Florian Weingarten <Florian.Weingarten@RWTH-Aachen.de>
#

if [ $# -ne 1 ]
then
	echo "Usage: $0 <tun/tap interface>"
	exit 0
fi

TUN="$1"
OPTIONS="--log-ip-options --log-tcp-options"
PORT=2323

# Delete all rules
echo "Flushing tables"
iptables -F
iptables -t nat -F

# Log $TUN interface traffic
echo "Adding $TUN log rules"
iptables        -A INPUT       -i $TUN -j LOG $OPTIONS --log-prefix "INPUT: "
iptables -t nat -A OUTPUT      -o $TUN -j LOG $OPTIONS --log-prefix "NAT OUTPUT: "
iptables        -A OUTPUT      -o $TUN -j LOG $OPTIONS --log-prefix "OUTPUT: "
iptables        -A FORWARD     -o $TUN -j LOG $OPTIONS --log-prefix "FORWARD OUTPUT: "
iptables        -A FORWARD     -i $TUN -j LOG $OPTIONS --log-prefix "FORWARD INPUT: "
iptables -t nat -A PREROUTING  -i $TUN -j LOG $OPTIONS --log-prefix "PREROUTING: "
iptables -t nat -A POSTROUTING -o $TUN -j LOG $OPTIONS --log-prefix "POSTROUTING: "

# Log everything on test port, not just on $TUN interface
for P in tcp udp; do
 echo "Adding port $PORT log rules for $P"
 iptables        -A INPUT       -p $P --dport $PORT -j LOG $OPTIONS --log-prefix "INPUT: ";
 iptables -t nat -A OUTPUT      -p $P --dport $PORT -j LOG $OPTIONS --log-prefix "NAT OUTPUT: ";
 iptables        -A OUTPUT      -p $P --dport $PORT -j LOG $OPTIONS --log-prefix "OUTPUT: ";
 iptables        -A FORWARD     -p $P --dport $PORT -j LOG $OPTIONS --log-prefix "FORWARD OUTPUT: ";
 iptables        -A FORWARD     -p $P --dport $PORT -j LOG $OPTIONS --log-prefix "FORWARD INPUT: ";
 iptables -t nat -A PREROUTING  -p $P --dport $PORT -j LOG $OPTIONS --log-prefix "PREROUTING: ";
 iptables -t nat -A POSTROUTING -p $P --dport $PORT -j LOG $OPTIONS --log-prefix "POSTROUTING: ";
done

echo "---- foobar ----" >> /var/log/syslog

