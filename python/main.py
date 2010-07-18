#!/usr/bin/env python
"""
Take some data, send it to the device
TODO: make the compression an option which could be also disabled
TODO: setup a nice logger
"""

# from TOSSIM import Tossim, SerialForwarder, Throttle
import os
import zlib
import socket
import struct
import sys
import logging
import subprocess
import getopt
from math import ceil
from copy import deepcopy
from fcntl import ioctl
from collections import namedtuple

COMPRESSION = True
POPEN = lambda cmd: subprocess.Popen(cmd, shell=True)
ORDER = "!"
CHK = zlib.crc32
MAX_ETHER = 1500 # normal max MTU over ethernet
MSG = "ciao" * 1000


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

    def __init__(self, data, seq_no, max_size, compression=COMPRESSION):
        self.seq_no = seq_no
        self.max_size = max_size
        if compression:
            self.data = zlib.compress(data)
        else:
            self.data = data
        self.packets = self.split()

    def __len__(self):
        return len(self.packets)

    def __str__(self):
        return self.packets

    def split(self):
        "Returns all the packets encapsulated in the two layers"
        res = []
        tot_len = len(self.data)
        logging.debug("splitting the data in %d chunks" % tot_len)
        num_packets = int(ceil(float(tot_len) / self.max_size))
        idx = 0

        for ord_no in range(num_packets):
            # check if adding the right value
            to_add = self.data[idx:idx + self.max_size]
            pkt = MyPacket(self.seq_no, ord_no, num_packets, to_add)
            res.append(pkt.pack())
            idx += self.max_size

        return res


# then the values will be computed outside
class Packet(object):
    " Generic class of the packet, with a header and some data "

    def __init__(self, header, data, *values):
        self.header = Packer(*header)
        assert(len(header) == len(values))
        self.packet = self.header + Packer(('data', '%ds' % len(data)))
        # also data now should be in the fields
        self.fields = self.header.get_fields()
        self.values = values
        self.data = data

    def __len__(self):
        return len(self.packet)

    def __str__(self):
        return str(self.packet)

    def pack(self):
        " merge together data and values and pack them"
        return self.packet.pack(*(self.values + (self.data, )))

    # use maybe __setattr__ to setup the attributes
    def unpack(self, bytez):
        return self.packet.unpack(bytez)


# TODO: generalize somehow the input of more variable length fields
class UnPacket(object):

    # for now supposing it's a string
    def __init__(self, header, data):
        self.len = len(data) - len(header)
        self.unpacker = header + Packer(('data', '%ds' % self.len), )
        self.data = data

    def unpack(self):
        return self.unpacker.unpack(self.data)


class MyPacket(Packet):

    HEADER = (('seq_no', 'H'), ('ord_no', 'H'),
              ('parts', 'h'), ('chk', 'l'))

    def __init__(self, seq_no, ord_no, parts, data):
        super(MyPacket, self).__init__(MyPacket.HEADER, data,
                                       seq_no, ord_no, parts, CHK(data))


class Packer(object):
    "Class to easily pack and unpack data using namedtuples"

    def __init__(self, *header):
        self.fmt = ''.join(h[1] for h in header)
        self.tup = namedtuple('header', (h[0] for h in header))
        # TODO: add some smart way to avoid the "+ ORDER"

    def __str__(self):
        return self.fmt

    def __len__(self):
        return struct.calcsize(ORDER + self.fmt)

    def __add__(self, other):
        ret = deepcopy(self)
        ret.fmt += other.fmt
        # _fields stuff not really correct
        ret.tup = namedtuple('header', self.tup._fields + other.tup._fields)
        return ret

    def get_fields(self):
        return self.tup._fields

    def pack(self, *data):
        try:
            return struct.pack(ORDER + self.fmt, *data)
        except struct.error:
            # TODO: add some better error here
            print "error format %s, data %s" % (self.fmt, str(data))
            return None

    def unpack(self, bytez):
        return struct.unpack(ORDER + self.fmt, bytez)


class Merger(object):
    """
    Merger should keep all the packets until it didn't construct something
    """

    def __init__(self, packets=None, compression=COMPRESSION):
        self.temp = {} # dict of packets in construction
        self.completed = {} # dict of successfully built packets
        self.compression = compression
        # make it simpler
        if packets:
            for p in packets:
                self.add(p)

    def add(self, packet):
        "Add a new packet, manipulating the dictionaries"
        # make it more automatic, don't have to call this from here
        head = Packer(*MyPacket.HEADER)
        un = UnPacket(head, str(packet))
        seq_no, ord_no, parts, chk, data = un.unpack()

        if chk != CHK(data):
            print "cheksum on the packet is not correct"
        else:
            # we can actually add it to temp
            if seq_no not in self.temp:
                self.temp[seq_no] = [None] * parts
            # it should not happen that we get the same message twice
            assert(ord_no not in self.temp[seq_no])
            self.temp[seq_no][ord_no] = data
            self.update_if_completed(seq_no)

    def update_if_completed(self, seq_no):
        if (seq_no in self.temp) and (None not in self.temp[seq_no]):
            # deepcopying and merging everything together
            merged = "".join(deepcopy(self.temp[seq_no]))
            if self.compression:
                merged = zlib.decompress(merged)
            self.completed[seq_no] = merged
            del self.temp[seq_no]

    def get_packet(self):
        "None if no packet are completed, otherwise the first found"
        if self.completed is None:
            return None
        else:
            key = self.completed.keys()[0]
            val = self.completed[key]
            del self.completed[key]
            return val


class Communicator(object):
    "Glue everything together with two entities talking together"

    PORT = 10000
    # TODO: try with a select to make it easier

    @staticmethod
    def server():
        "waits for data and reconstruct the message"
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        merger = Merger()
        sock.bind(("", Communicator.PORT))
        while True:
            word = sock.recv(MAX_ETHER)
            # gets some packets and reconstruct them
            merger.add(word)
            if len(merger.completed) > 0:
                print "got the message recomposing now"
                break
        assert(merger.completed[0] == MSG)
        sock.close()

    @staticmethod
    def client(data):
        "split data got in and send them over the socket"
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sp = Splitter(data=data, seq_no=0, max_size=100)
        for p in sp.packets:
            sock.sendto(p, ("", Communicator.PORT))


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
    opts, _ = getopt.getopt(sys.argv[1:], 'vcgd:')
    logger = logging.getLogger()

    MODE = None
    for o, v in opts:
        if o == '-d':
            device = "/dev/ttyUSB%s" % v
        if o == '-v':
            logger.setLevel(logging.DEBUG)
        if o == '-c':
            MODE = 'client'
        if o == '-g':
            MODE = 'server'

        if MODE == 'client':
            Communicator.client(MSG)

        if MODE == 'server':
            Communicator.server()

if __name__ == '__main__':
    main()
