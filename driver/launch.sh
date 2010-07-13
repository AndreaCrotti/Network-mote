#!/bin/bash
# we can pass some options here an
# operations to do in case we want to create a bridge could be

#brctl addbr br0
#brctl addif br0 eth0
#ifconfig eth0 down
#ifconfig br0 192.168.1.1

# an then all the traffic will go throgh the bridge automatically
# there could be some options

set -x
TAP=$1
ETH=$2
ADDR=$3

brctl addbr $TAP
brctl addif $TAP $ETH
ifconfig $ETH down
ifconfig $TAP $ADDR

# now we can execute our program
make && ./tunneltest

# TODO: do we need to trap C-c at this level?
# and now we can undo all the settings
brctl delif $TAP $ETH
brctl delbr $TAP
ifconfig $ETH up

