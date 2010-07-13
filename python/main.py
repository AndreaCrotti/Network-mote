#!/usr/bin/env python
"""
Take some data, send it to the device
TODO: make the compression an option which could be also disabled
"""

from scapy.all import *
# from TOSSIM import Tossim, SerialForwarder, Throttle
import os
import zlib
from select import select
import socket

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
def setup_device(ipaddr, mode='tun'):
    # creating a tun device and sending data on it
    TUNSETIFF = 0x400454ca
    # those values should be read in the if_tun.h directly somehow
    IFF_TUN   = 0x0001
    IFF_TAP   = 0x0002

    if mode == 'tap':
        TUNMODE = IFF_TAP
    else:
        TUNMODE = IFF_TUN

    fd = os.open("/dev/net/tun", os.O_RDWR)
    ifs = ioctl(fd, TUNSETIFF, struct.pack("16sH", "toto%d", TUNMODE))
    ifname = ifs[:16].strip("\x00")
    print "Allocated interface %s. Configure it and use it" % ifname
    os.popen("ifconfig %s %s netmask 255.255.255.0" % (ifname, ipaddr))
    return fd

def compress(packet, dst, size=100):
    # conf.route6.add
    # maybe we can also sniff instead of reading on the device
    compressed = zlib.compress(str(packet), 9)
    # should also setup the source and other fields
    # get the slicing
    splits = [compressed[x:x+size] for x in range(0, len(compressed), size)]
    return [IPv6(dst=dst) / x for x in splits]

payload = "very long message" * 100

# proof of concept, create 2 virtual device, and start some multiprocessing magic with them
def proof():
    ipaddr = "10.0.0.%d"
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    devs = setup_device(ipaddr % 1), setup_device(ipaddr % 2)
    try:
        while True:
            ro, wr, _ = select(devs, devs, [])
            print ro, wr
            # first try to read on one of them, otherwise write
            if ro != []:
                for x in ro:
                    pkt = os.read(x, 1024)
                    print "%d has received packet %s" % (x, str(pkt))
            else:
                # otherwise we write something on both
                for x in wr:
                    print "writing the payload to %d" % x
                    os.write(x, payload)
                    # sending with a socket to the other address
                    other = devs[(devs.index(x) + 1) % 2]

    except KeyboardInterrupt:
        for d in devs:
            print "closing %d" % d
            os.close(d)
proof()
