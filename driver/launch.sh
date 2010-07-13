#!/bin/bash
# we can pass some options here an
# operations to do in case we want to create a bridge could be

# brctl addbr br0
# brctl addif br0 eth0
# ifconfig eth0 down
# ifconfig br0 192.168.1.1

# an then all the traffic will go throgh the bridge automatically
# there could be some options

function usage {
    echo "./launch <tap device> <eth device> <local address>"
    exit 1
}

if [ $# -lt 3 ] ; then
    usage
    exit 0
fi
             
set -x
TAP=$1
ETH=$2
ADDR=$3

function brup {
    "setting up the bridge"
    brctl addbr $TAP
    brctl addif $TAP $ETH
    ifconfig $ETH down
    ifconfig $TAP $ADDR
}

# FIXME: still some problems with setting up the metric
function brdown {
# TODO: do we need to trap C-c at this level?
# and now we can undo all the settings
    echo "unsetting the bridge"
    ifconfig $TAP down
    brctl delif $TAP $ETH
    brctl delbr $TAP
    ifconfig $ETH up
    ifconfig $ETH $ADDR
    # hack for Arch only
    /etc/rc.d/network restart
}


# now we can execute our program
brup
# when we quit with C-c it will clean automatically
trap "brdown" 2
make && ./tuntest
