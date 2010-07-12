#!/usr/bin/env python
"""
Take some data, send it to the device
TODO: make the compression an option which could be also disabled
"""

from scapy.all import *
# from TOSSIM import Tossim, SerialForwarder, Throttle
import os
import zlib

TOSROOT = os.getenv("TOSROOT")
if TOSROOT is None:
    print "you need at least to setup your $TOSROOT variable correctly"
    sys.exit(1)

else:
    sdk = os.path.join(TOSROOT, "suppport", "sdk", "python")
    sys.path.append(sdk)

from tinyos.tossim.TossimApp import NescApp


# s = socket(AF_INET, SOCK_DGRAM)
# s.sendto(MAGIC_WORD, peer)

# setup the correct route via the created device
def setup_device(num, mode='tun'):
    # creating a tun device and sending data on it
    TUNSETIFF = 0x400454ca
    # those values should be read in the if_tun.h directly somehow
    IFF_TUN   = 0x0001
    IFF_TAP   = 0x0002

    if mode == 'tap':
        TUNMODE = IFF_TAP
    else:
        TUNMODE = IFF_TUN

    f = os.open("/dev/net/tun", os.O_RDWR)
    ifs = ioctl(f, TUNSETIFF, struct.pack("16sH", "toto%d", TUNMODE))
    ifname = ifs[:16].strip("\x00")
    print "Allocated interface %s. Configure it and use it" % ifname
    ipaddr = "10.0.0.%s" % str(num)
    os.popen("ifconfig %s %s netmask 255.255.255.0" % (ifname, ipaddr))
    return ipaddr

def compress():
    MAXSIZE = 100
    # conf.route6.add
    # maybe we can also sniff instead of reading on the device
    while True:
        p = sniff(count=1, filter="ip6")[0]
        compressed = zlib.compress(str(p), 9)
        pkts = [compressed[x:x+MAXSIZE] for x in range(len(compressed))]
        # we gain about the 30% over the network
        print "from %s generated %s packets of size %d, gaining %d" % (compressed, len(pkts), MAXSIZE, len(str(p)) - len(compressed))
        # now we can send them and reconstruct them somewhere else

devs = [setup_device(1), setup_device(2)]

payload = "very long message" * 100
import multiprocessing

# proof of concept, create 2 virtual device, and start some multiprocessing magic with them
def proof(num):
    if num == 2:
        dev = setup_device(2)
    # also check that the ordering is correct
    
