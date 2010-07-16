#!/bin/bash
# an then all the traffic will go throgh the bridge automatically
# there could be some options
# see http://openvpn.net/index.php/open-source/documentation/miscellaneous/76-ethernet-bridging.html
# for an example of bridge-start/bridge-stop
# Maybe for some more tricks we could also use iptables

function usage {
    echo "./launch <tap device> <eth device> <local address> <mtu>"
    return 1
}
if [ $# -lt 4 ] ; then
    usage
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
<
# FIXME: the table is messed up after this
function brdown {
# and now we can undo all the settings
    echo "unsetting the bridge"
    ifconfig $BR down
    brctl delbr $BR
    openvpn --rmtun --dev $TAP
    echo "dns and rest are probably fucked up now"
    dhcpcd $ETH
}

set -x
brup
sleep 10
brdown
