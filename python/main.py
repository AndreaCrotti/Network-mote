#!/usr/bin/env python
"""
Take some data, send it to the device
TODO: make the compression an option which could be also disabled
TODO: setup a nice logger
"""

# from TOSSIM import Tossim, SerialForwarder, Throttle
import os
import zlib
from select import select
import socket
import struct
import sys
import logging
import subprocess
from copy import deepcopy
from fcntl import ioctl
from collections import namedtuple
from scapy.all import IPv6

DEFCOMPRESS = True
MYHEADER = 'Hhl'

TOSROOT = os.getenv("TOSROOT")
if TOSROOT is None:
    print "you need at least to setup your $TOSROOT variable correctly"
    sys.exit(1)

else:
    sdk = os.path.join(TOSROOT, "suppport", "sdk", "python")
    sys.path.append(sdk)

from tinyos.tossim.TossimApp import NescApp

class TunTap(object):
    "Tun tap interface class management"
    TUNSETIFF = 0x400454ca
    # those values should be read in the if_tun.h directly somehow
    IFF_TUN = 0x0001
    IFF_TAP = 0x0002

    def __init__(self, mode, max_size):
        self.mode = mode
        self.max_size = max_size

    def setup(self):
        # creating a tun device and sending data on it

        if self.mode == 'tap':
            TUNMODE = TunTap.IFF_TAP
            # setup the bridge
        else:
            TUNMODE = TunTap.IFF_TUN
            # setup the routing stuff

        self.fd = os.open("/dev/net/tun", os.O_RDWR)
        ifs = ioctl(self.fd, TunTap.TUNSETIFF, struct.pack("16sH", "tap%d", TUNMODE))
        ifname = ifs[:16].strip("\x00")
        print "Allocated interface %s. Configure it and use it" % ifname

    def close(self):
        os.close(self.fd)

    def read(self):
        return os.read(self.fd, self.max_size)

    def write(self, data):
        assert(len(data) < self.max_size)
        os.write(self.fd, data)

    # TODO: see if implementing fileno could be useful

# s = socket(AF_INET, SOCK_DGRAM)
# s.sendto(MAGIC_WORD, peer)

class Splitter(object):
    """
    Class used for splitting our data, argument must be a string or serializable
    """
    def __init__(self, data, seq, max_size, ip_header, compression=DEFCOMPRESS):
        self.ip_header = ip_header
        self.seq = seq
        self.max_size = max_size
        if compression:
            self.data = zlib.compress(data)
        else:
            self.data = data
        self.packets = self.split()

    def __len__(self):
        return len(self.packets)

    def split(self):
        "Returns all the packets encapsulated in the two layers"
        res = []
        count = 0
        idx = 0
        tot_len = len(self.data)
        while idx < tot_len:
            # we get an external already configured header and we add the payload
            head = deepcopy(self.ip_header)
            # don't have to worry about overflows!
            pkt = MyPacket(self.seq, count, self.data[idx:idx + self.max_size])
            # the len is automatically set by scapy!!
            head.add_payload(pkt.pack())
            res.append(head)
            count += 1
            idx += self.max_size

        return res

# add something to see the header
class Packer(object):
    def __init__(self, *header):
        # TODO: make it configurable
        self.order = "!"
        self.fmt = self.order + ''.join(h[1] for h in header)
        self.tup = namedtuple('header', (h[0] for h in header))

    def __str__(self):
        return self.fmt

    def __len__(self):
        return struct.calcsize(self.fmt)
    
    def pack(self, *data):
        try:
            return struct.pack(self.fmt, *data)
        except struct.error:
            # TODO: add some better error here
            print "Error in formatting\n format %s, data %s" % (self.fmt, str(*data))
            return None

    def unpack(self, bytez):
        return struct.unpack(self.fmt, bytez)

# TODO: use some metaprogramming to create the right class
class MyPacket(object):
    """
    Class of packet type
    TODO: when the data is changing also the checksum should change automatically
    """
    def __init__(self, seq, ord, data, chk_function=zlib.crc32):
        # we can pass any checksum function that gives a 32 bit result
        # checksum might be also disable maybe
        self.seq = seq
        self.ord = ord
        self.data = data
        self.chk = chk_function(data)
        self.packet = Packer(('seq', 'H'), ('ord', 'H'), ('chk', 'L'), ('data', '%ds' % len(self.data)))

    def __str__(self):
        return self.bytez

    def __len__(self):
        return len(self.packet)

    # TODO: add some smart check of the input?
    def pack(self):
        return self.packet.pack(self.seq, self.ord, self.chk, self.data)

    def unpack(self, bytez):
        return self.packet.unpack(bytez)

class Merger(object):
    "reconstructing original data"
    def __init__(self, packets, compression=DEFCOMPRESS):
        self.packets = packets
        self.compression = compression

    # see how we can decompress not knowing the dimension
    def merge(self):
        pass

def reconstruct(packets):
    "Reconstruct the original data from the compressed packets"
    print "len = %d" % len(packets[0].payload)
    restored = [struct.unpack("!hHL%ds" % max_size,  p.payload) for p in packets]
    # grouping by sequential number and sorting by ord number after
    data = ""
    for v in sorted(restored, key=lambda x: x[1]):
        print v
        data += v[3]
    # now we can finally uncompress the data
    # orig = zlib.decompress(data)
    return data

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
                    print "writing the payload to %d" % xb
                    os.write(x, payload)
                    # sending with a socket to the other address
                    other = devs[(devs.index(x) + 1) % 2]

    except KeyboardInterrupt:
        for d in devs:
            print "closing %d" % d
            os.close(d)

def test_select():
    # trying out a select using a couple of devices
    sock1 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock2 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock3 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # 1 is the client and we do the selection on 2 and 3
    addr1, addr2 = (("", 10000), ("", 100000))
    sock2.bind(addr1)
    sock3.bind(addr2)

    # this select stuff with sockets works pretty well
    from random import random
    while True:
        if random() > 0.5:
            sock1.sendto("ciao ", addr1)
        else:
            sock1.sendto("ciao ", addr2)
        r, w, _ = select([sock2, sock3], [], [])
        print r,w
        for x in r:
            print "reading data %s" % str(x.recv(1024))

def test_usb_writing():
    dev = "/dev/ttyUSB0"
    fd = os.open(dev, os.O_RDWR)
    # try to read and write from that and see what happens


# rewrite this using only scapy
def test_mtu_speed():
    t = TunTap('tap', 1024)
    t.setup()
    addr = "10.0.1.1"
    subprocess.Popen("/sbin/ifconfig tap0 %s" % addr, shell=True)
    for mtu in range(50, 1000, 100):
        print "\n\nfor mtu %d we have ping" % mtu
        subprocess.Popen("ifconfig tap0 mtu %d" % mtu, shell=True)
        os.popen("/sbin/ifconfig tap0 mtu %d" % mtu).read()
        # then ping and see how fast it is
        subprocess.Popen("ping -p ff -c 5 %s" % addr, shell=True)

def usage():
    print "usage: ./main.py <device>"
    sys.exit(os.EX_USAGE)

def main():
    if len(sys.argv) < 2:
        usage()
    else:
        device = "/dev/ttyUSB%s" % sys.argv[1]
        t = TunTap('tap')
        t.setup()
        mote_fd = os.open(device, os.O_RDWR)
        try:
            while True:
                ro, wr, ex = select([t.fd, mote_fd], [t.fd, mote_fd], [])
                # now we read the ethernet packets from the tap device and send
                # them to the mote writing them out
                if t.fd in ro:
                    # compress and send to the serial interface
                    pass
                elif mote_fd in ro:
                    # reconstruct the packet
                    pass_

        except KeyboardInterrupt:
            # use "with" instead if possible
            t.close()
            os.close(mote_fd)

if __name__ == '__main__':
    test_mypacket()
