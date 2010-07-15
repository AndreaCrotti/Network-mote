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

class TestSplitter(unittest.TestCase):
    def test_splitter(self):
        rand_big = "ciao" * 1000
        compr = Splitter(rand_big, 0, 128, IPv6(dst="::1"))
        nocompr = Splitter(rand_big, 0, 128, IPv6(dst="::1"), compression=False)
        for x in compr.split():
            print x.show2()
        for x in nocompr.split():
            print x.show2()
        print "compr = %d, nocompr = %d\n" % (len(compr), len(nocompr))

def rand_string(dim):
    return "".join([choice(ascii_letters) for _ in range(dim)])

if __name__ == '__main__':
    unittest.main()
