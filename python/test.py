#!/usr/bin/env python
import unittest
from string import ascii_letters
from random import choice
from select import select
from main import *

class TestTapDevice(unittest.TestCase):
    def setUp(self):
        self.tap = TunTap('tap', MAX_ETHER)
        self.tap.setup()

    def tearDown(self):
        self.tap.close()

    def testReadAfterWrite(self):
        msg = "ciao"
        self.tap.write(msg)
        self.assertEquals(self.tap.read(msg), msg)


class TestMyPacket(unittest.TestCase):
    def test_mypacket(self):
        # here parts is not really important
        st = (1, 2, 1, "ciao ciao")
        p = MyPacket(*st)
        packed = p.pack()
        unpacked = p.unpack(packed)
        self.assertEquals(st[-1], unpacked[-1])


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
        main.COMPRESSOIN = True
        compr = Splitter(rand_big, 0, 128, IPv6(dst="::1"))
        main.compression = False
        nocompr = Splitter(rand_big, 0, 128, IPv6(dst="::1"))
        self.assertTrue(len(compr) <= len(nocompr))

class TestCombined(unittest.TestCase):
    """ Check the Splitter-Merger couple working """
    # TODO: write them more precisely
    def setUp(self):
        self.orig_data = rand_string(1000)
        self.packets = Splitter(self.orig_data, 0, 100, IPv6()).packets
        
    # def test_split_combine(self):
    #     "combining them results still give same output"
    #     m = Merger(self.packets)
    #     print m.raw_data
    #     self.assertEquals(self.orig_data, m.get_data())

    # def test_with_mixed_packets(self):
    #     "Shuffling the packets arrival still works"
    #     from random import shuffle
    #     shuffle(self.packets)
    #     m = Merger(self.packets)
    #     self.assertEquals(self.orig_data, m.get_data())


class TestMerger(unittest.TestCase):
    # see if the merging is correctly
    pass

class TestTwoTapDevices(unittest.TestCase):
    "Sending and reconstructing between two tap devices"
    # to make the loop finally working add two bridges with them
    t1, t2 = TunTap(), TunTap()
    t1.setup()
    t2.setup()
    m1, m2 = Merger(), Merger()
    # try:
    #     # now give them an address and allows sending between each other via network
    #     while True:
    #         ro, wr, ex = select([t1.fd, t2.fd], [t1.fd, t2.fd], [])
    #         if t1 in ro:
    #             pass
    # except KeyboardInterrupt:
    #     t1.close; t2.close()

def rand_string(dim):
    return "".join([choice(ascii_letters) for _ in range(dim)])

if __name__ == '__main__':
    unittest.main()
