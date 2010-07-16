#!/usr/bin/env python
"""
Take some data, send it to the device
TODO: make the compression an option which could be also disabled
TODO: setup a nice logger
"""

# from TOSSIM import Tossim, SerialForwarder, Throttle
import os
import zlib
# from select import select
# import socket
import struct
import sys
import logging
import subprocess
import getopt
from math import ceil
from copy import deepcopy
from fcntl import ioctl
from collections import namedtuple
# from scapy.all import IPv6

COMPRESSION = True
POPEN = lambda cmd: subprocess.Popen(cmd, shell=True)
ORDER = "!"
CHK = zlib.crc32
MAX_ETHER = 10 * 1024


class TunTap(object):
    "Tun tap interface class management"
    TUNSETIFF = 0x400454ca
    # those values should be read in the if_tun.h directly somehow
    IFF_TUN = 0x0001
    IFF_TAP = 0x0002

    def __init__(self, mode='tap', max_size=MAX_ETHER):
        self.mode = mode
        self.max_size = max_size

    def setup(self):
        "creating a tun device and sending data on it"
        if self.mode == 'tap':
            TUNMODE = TunTap.IFF_TAP
            # setup the bridge
        else:
            TUNMODE = TunTap.IFF_TUN
            # setup the routing stuff

        self.fd = os.open("/dev/net/tun", os.O_RDWR)
        ifs = ioctl(self.fd, TunTap.TUNSETIFF,
                    struct.pack("16sH", "tap%d", TUNMODE))
        ifname = ifs[:16].strip("\x00")
        logging.debug("Allocated interface %s" % ifname)

    def close(self):
        os.close(self.fd)

    def read(self):
        return os.read(self.fd, self.max_size)

    def write(self, data):
        assert(len(data) < self.max_size)
        os.write(self.fd, data)

    # TODO: see if implementing fileno could be useful


class Splitter(object):
    """
    Class used for splitting our data, argument must be a string
    """

    def __init__(self, data, seq_no, max_size, ip_header):
        self.ip_header = ip_header
        self.seq_no = seq_no
        self.max_size = max_size
        if COMPRESSION:
            self.data = zlib.compress(data)
        else:
            self.data = data
        self.packets = self.split()

    def __len__(self):
        return len(self.packets)

    def split(self):
        "Returns all the packets encapsulated in the two layers"
        res = []
        tot_len = len(self.data)
        num_packets = int(ceil(float(tot_len) / self.max_size))
        idx = 0
        for x in range(num_packets):
            # we get an external already configured header and we add the payload
            head = deepcopy(self.ip_header)
            # don't have to worry about overflows!
            pkt = MyPacket(self.seq_no, x, num_packets, self.data[idx:idx + self.max_size])
            # the len is automatically set by scapy!!?? Not done this at the moment
            head.add_payload(pkt.pack())
            res.append(head)
            idx += self.max_size

        return res


class Packer(object):
    "Class to easily pack and unpack data using namedtuples"

    def __init__(self, *header):
        # TODO: make it configurable
        self.fmt = ''.join(h[1] for h in header)
        self.tup = namedtuple('header', (h[0] for h in header))

    def __str__(self):
        return self.fmt

    def __len__(self):
        return struct.calcsize(self.fmt)

    def __add__(self, y):
        ret = deepcopy(self)
        ret.fmt += y.fmt
        # do something also with the namedtuple
        ret.tup = namedtuple('header', self.tup._fields + y.tup._fields)
        return ret

    def pack(self, *data):
        try:
            return struct.pack(ORDER + self.fmt, *data)
        except struct.error:
            # TODO: add some better error here
            print "formatting\n format %s, data %s" % (self.fmt, str(data))
            return None

    def unpack(self, bytez):
        return struct.unpack(ORDER + self.fmt, bytez)


class MyPacket(object):

    HEADER = Packer(('seq_no', 'H'), ('ord_no', 'H'),
                    ('parts', 'h'), ('chk', 'L'))

    def __init__(self, seq_no, ord_no, parts, data):
        # we can pass any checksum function that gives a 32 bit result
        # checksum might be also disable maybe
        self.seq_no = seq_no
        self.ord_no = ord_no
        self.data = data
        self.parts = parts
        self.chk = CHK(data)
        # TODO: try to use the struct "p" (pascal)
        self.packet = MyPacket.HEADER + Packer(('data', '%ds' % len(data)))

    def __len__(self):
        return len(self.packet)

    # TODO: add some smart check of the input?
    def pack(self):
        # TODO: make this list automatically generable somehow
        return self.packet.pack(self.seq_no, self.ord_no,
                                self.parts, self.chk, self.data)

    def unpack(self, bytez):
        return self.packet.unpack(bytez)


class Merger(object):
    """
    Reconstructing the data, protocol doesn't matter since we use len and .payload
    (we just need the len attribute being set
    Merger should keep all the packets until it didn't construct something
    """

    def __init__(self, packets=None):
        self.temp = {} # dict of packets in construction
        self.completed = {} # dict of successfully built packets
        # make it simpler
        if packets:
            for p in packets:
                self.add(p)

    def add(self, packet):
        "Add a new packet, manipulating the dictionaries"
        # check that this is actually what we expect to have
        data_length = len(packet.payload) - len(MyPacket.HEADER)
        unpacker = MyPacket.HEADER + Packer(('data', '%ds' % data_length))
        # compute the checksum to see if it's correct
        seq_no, ord_no, parts, chk, data = unpacker.unpack(str(packet.payload))
        if chk != CHK(data):
            print "cheksum on the packet is not correct"
        else:
            # we can actually add it to temp
            if seq_no not in self.temp:
                self.temp = [None] * parts
            else:
                # it should not happen that we get the same message twice
                assert(ord_no not in self.temp[seq_no])
                self.temp[ord_no] = data
                self.update_if_completed(seq_no)

    def update_if_completed(self, seq_no):
        if (seq_no in self.temp) and (None not in self.temp[seq_no]):
            print "all the chunks for packet %d are arrived" % seq_no
            # TODO: check if deepcopy is really needed there?
            self.completed[seq_no] = deepcopy(self.temp[seq_no])
            del self.temp[seq_no]
        print self.temp, self.completed


def setup_tos():
    "setup the tinyos env for serial forwarder"
    TOSROOT = os.getenv("TOSROOT")
    if TOSROOT is None:
        print "you need at least to setup your $TOSROOT variable correctly"
        return False
    else:
        sdk = os.path.join(TOSROOT, "suppport", "sdk", "python")
        sys.path.append(sdk)
        return True


def usage():
    print "usage: ./main.py <device>"
    sys.exit(os.EX_USAGE)


def main():
    opts, _ = getopt.getopt(sys.argv[1:], 'vcgd:', ['--verbose', '--client', '--gateway', '--device'])
    # setting the logger
    logger = logging.getLogger()

    device = None
    for o, v in opts:
        if o == '-d':
            device = "/dev/ttyUSB%s" % v
        if o == '-v':
            logger.setLevel(logging.DEBUG)

    if not device:
        print "no device configured"
        sys.exit(1)

    # # pass the max size of reading also
    # t = TunTap()
    # t.setup()
    # mote_fd = os.open(device, os.O_RDWR)
    # try:
    #     while True:
    #         ro, wr, ex = select([t.fd, mote_fd], [t.fd, mote_fd], [])
    #         # now we read the ethernet packets from the tap device and send
    #         # them to the mote writing them out
    #         if t.fd in ro:
    #             # compress and send to the serial interface
    #             pass
    #         elif mote_fd in ro:
    #             # reconstruct the packet
    #             pass_

    # except KeyboardInterrupt:
    #     # use "with" instead if possible
    #     t.close()
    #     os.close(mote_fd)

if __name__ == '__main__':
    main()
