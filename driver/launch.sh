#!/bin/bash
# an then all the traffic will go throgh the bridge automatically
# there could be some options

function usage {
    echo "./launch <tap device> <eth device> <local address> <mtu>"
    exit 1
}

if [ $# -lt 3 ] ; then
    usage
    exit 0
fi

TAP=$1
ETH=$2
ADDR=$3
MTU=$4

function brup {
    "setting up the bridge"
    brctl addbr $TAP
    brctl addif $TAP $ETH
    ifconfig $ETH down
    ifconfig $TAP $ADDR mtu $MTU
}

# FIXME: still some problems with setting up the metric
function brdown {
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

set -x
# now we can execute our program
brup
# when we quit with C-c it will clean automatically
trap "brdown" 2
make && ./tuntest
