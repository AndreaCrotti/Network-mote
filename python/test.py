#!/usr/bin/env python
import unittest
import zlib, bz2
from string import ascii_letters
from random import choice
from main import *

class TestMyPacket(unittest.TestCase):
    def test_mypacket(self):
        # here parts is not really important
        st = (1, 2, 1, "icao ciao")
        p = MyPacket(*st)
        packed = p.pack()
        print "we need %d bytes for the packet" % len(p)
        # self.assertEquals(st, p.unpack(packed))

class TestPacker(unittest.TestCase):
    def test_adding(self):
        s = Packer(('seq', 'H'), ('ord', 'H'))
        s1 = Packer(('ss', 'H'))
        summed = s + s1
        self.assertEquals(len(s) + len(s1), len(summed))
        self.assertEquals(s.fmt + s1.fmt, summed.fmt)
        packed_struct = s.pack(1, 2) + s1.pack(10)
        self.assertEquals(summed.pack(1, 2, 10), packed_struct)

class TestSplitter(unittest.TestCase):
    def test_splitter(self):
        rand_big = "ciao" * 1000
        compr = Splitter(rand_big, 0, 128, IPv6(dst="::1"), compression=True)
        nocompr = Splitter(rand_big, 0, 128, IPv6(dst="::1"), compression=False)
        print "compr = %d, nocompr = %d\n" % (len(compr), len(nocompr))

class TestCombined(unittest.TestCase):
    """ Check the Splitter-Merger couple working """
    # TODO: write them more precisely
    def setUp(self):
        self.orig_data = rand_string(1000)
        self.packets = Splitter(self.orig_data, 0, 100, IPv6()).packets
        
    def test_split_combine(self):
        "combining them results still give same output"
        m = Merger(self.packets)
        print m.raw_data
        self.assertEquals(self.orig_data, m.get_data())

    def test_with_mixed_packets(self):
        "Shuffling the packets arrival still works"
        from random import shuffle
        shuffle(self.packets)
        m = Merger(self.packets)
        self.assertEquals(self.orig_data, m.get_data())

def rand_string(dim):
    return "".join([choice(ascii_letters) for _ in range(dim)])

if __name__ == '__main__':
    unittest.main()
