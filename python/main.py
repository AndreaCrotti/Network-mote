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
import struct

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

def compress(packet, seq, dst, size=100):
    # seqno, ordnumber, checksum
    header = "!hHL"
    # add the needed fields
    print "we need %d bytes for the added info" % struct.calcsize(header)
    # maybe we can also sniff instead of reading on the device
    # compressed = zlib.compress(packet)
    compressed = packet
    max_size = size - (struct.calcsize(header) + len(IPv6()))
    print "max size for the payload = %d" % max_size
    inner = lambda s, ord, chk: struct.pack(header + "s", seq, ord, chk, s)
    # get the slicing
    splits = []
    count = 0
    for x in range(0, len(compressed), max_size):
        splits.append(inner(compressed[x:x+max_size], count, 0))
        count += 1

    print "returning %d packets" % len(splits)
    return [IPv6(dst=dst) / x for x in splits]

def reconstruct(packets):
    "Reconstruct the original data from the compressed packets"
    restored = [struct.unpack("!hHLs", str(p.payload)) for p in packets]
    # grouping by sequential number and sorting by ord number after
    data = ""
    for v in sorted(restored, key=lambda x: x[1]):
        print v
        data += v[3]
    # now we can finally uncompress the data
    # orig = zlib.decompress(data)
    return data

from collections import namedtuple
    
def test_compress():
    p = sniff(count=10)
    # print compress(str(p), "::1")

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

from string import ascii_letters
from random import choice

def rand_string(dim):
    return "".join([choice(ascii_letters) for _ in range(dim)])


payload = rand_string(1000)
pkts = compress(payload, 1, "::1")

assert(reconstruct(pkts) == payload)
# for x in pkts:
#     print x.show(), len(x)
