#!/usr/bin/env python
import unittest
from string import ascii_letters
from random import choice
from select import select
from random import shuffle
from main import *

class TestTapDevice(unittest.TestCase):
    def setUp(self):
        self.tap = TunTap('tap', MAX_ETHER)
        self.tap.setup()

    def tearDown(self):
        self.tap.close()

    # def testReadAfterWrite(self):
    #     msg = "ciao"
    #     self.tap.write(msg)
    #     self.assertEquals(self.tap.read(msg), msg)


class TestMyPacket(unittest.TestCase):
    def setUp(self):
        self.st = (1, 2, 1, "ciao ciao")
        self.p = MyPacket(*self.st)
        self.packed = self.p.pack()

    def test_mypacket(self):
        "unpacking a packed packet gives the original value"
        unpacked = self.p.unpack(self.packed)
        self.assertEquals(self.st[-1], unpacked[-1])


class TestPacker(unittest.TestCase):
    def test_adding(self):
        "adding two struct has same effect as the summed struct"
        s = Packer(('seq', 'H'), ('ord', 'H'))
        s1 = Packer(('ss', 'H'))
        summed = s + s1
        self.assertEquals(len(s) + len(s1), len(summed))
        self.assertEquals(s.fmt + s1.fmt, summed.fmt)
        packed_struct = s.pack(1, 2) + s1.pack(10)
        self.assertEquals(summed.pack(1, 2, 10), packed_struct)


class TestSplitter(unittest.TestCase):
    def test_splitter(self):
        "with compression enabled we have less packets"
        rand_big = "ciao" * 1000
        compr = Splitter(rand_big, 0, 128, IPv6(dst="::1"), compression=True)
        nocompr = Splitter(rand_big, 0, 128, IPv6(dst="::1"), compression=False)
        self.assertTrue(len(compr) < len(nocompr))


# TODO: make it compressed/non compressed mode
class TestCombined(unittest.TestCase):
    """ Check the Splitter-Merger couple working """
    # TODO: write them more precisely
    def setUp(self):
        self.orig_data = rand_string(1000)
        self.seq = 0
        self.packets = Splitter(self.orig_data, self.seq, 100, IPv6()).packets
        
    def test_split_combine(self):
        "combining them results still give same output"
        m = Merger(self.packets)
        self.assertTrue(self.seq in m.completed)
        self.assertEquals(m.completed[self.seq], self.orig_data)

    def test_with_mixed_packets(self):
        "Shuffling the packets arrival still works"
        shuffle(self.packets)
        m = Merger(self.packets)
        # see if the original packet is completed
        self.assertTrue(self.seq in m.completed)
        self.assertEquals(m.completed[self.seq], self.orig_data)

    def test_all_mixed(self):
        "Mixing seq_no and ord_no works"
        many_strings = [rand_string(1000) for x in range(100)]
        packets = []
        for idx, s in enumerate(many_strings):
            packets += Splitter(s, idx, 100, IPv6()).packets
        
        shuffle(packets)
        m = Merger(packets)
        self.assertEquals(set(many_strings), set(m.completed.values()))

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
