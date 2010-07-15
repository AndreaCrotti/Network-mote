#!/usr/bin/env python
import unittest
import zlib, bz2
from string import ascii_letters
from random import choice
from main import *

class TestMyPacket(unittest.TestCase):
    def test_mypacket(self):
        st = (1, 2, "icao ciao")
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

class TestSplitter(unittest.TestCase):
    def test_splitter(self):
        rand_big = "ciao" * 1000
        compr = Splitter(rand_big, 0, 128, IPv6(dst="::1"))
        nocompr = Splitter(rand_big, 0, 128, IPv6(dst="::1"), compression=False)
        print "compr = %d, nocompr = %d\n" % (len(compr), len(nocompr))

class TestMerger(unittest.TestCase):
    pass

class TestCombined(unittest.TestCase):
    """ Check the Splitter-Merger couple working """
    def test_split_combine(self):
        data = rand_string(1000)
        s = Splitter(data, 0, 100, IPv6())
        for x in s.packets:
            print len(x.payload)

        m = Merger(s.packets)
        print m.raw_data


def rand_string(dim):
    return "".join([choice(ascii_letters) for _ in range(dim)])

if __name__ == '__main__':
    unittest.main()
