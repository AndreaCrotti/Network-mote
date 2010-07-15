#!/bin/bash
# an then all the traffic will go throgh the bridge automatically
# there could be some options

function usage {
    echo "./launch <tap device> <eth device> <local address> <mtu>"
    exit 1
}



if [ $# -lt 4 ] ; then
    usage
    exit 0
fi

TAP=$1
ETH=$2
ADDR=$3
MTU=$4

BR=br0

# we also need to create the tap device first
function brup {
    openvpn --mktun --dev $TAP
    brctl addbr $BR
    brctl addif $BR $ETH
    brctl addif $BR $TAP
    ifconfig $TAP 0.0.0.0 promisc up
    ifconfig $ETH 0.0.0.0 promisc up
    ifconfig $BR $ADDR netmask 255.255.255.0 mtu $MTU
    # now set up the default routing
    route add default gw $ADDR
}

# FIXME: still some problems with setting up the metric
function brdown {
# and now we can undo all the settings
    echo "unsetting the bridge"
    ifconfig $BR down
    brctl delbr $TAP
    openvpn --rmtun $TAP
}

set -x
brup
trap "brdown" 2
./tuntest
