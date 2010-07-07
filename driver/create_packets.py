#!/usr/bin/env python
"""
Using scapy to create some test packets
Then we can do some unit testing about what we receive back from the
network interface
"""
import sys
from scapy.all import *
import unittest

# add some dictionaries to test the stuff
# DST="209.85.135.105"
DST="192.168.226.1"
PORT = 80

# create a decorator for IPv6 stuff

def tcp_ack():
    t = TCP(flags='SA', sport=5000, dport=80)
    # maybe needed some resolution
    return IP(dst=DST) / t

def check_ans(pkt):
    ans, _ = sr(pkt)
    # we should also check the content as well, like the flags for example
    if ans != None:
        return True

if __name__ == '__main__':
    # check for correct destination
    dst = sys.argv[1]
