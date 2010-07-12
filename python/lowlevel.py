#!/usr/bin/env python
import struct
import binascii
import socket
import sys
import os
from scapy.all import *

# doing some experiments with binary manipulation

# No padding is added when using non-native size and alignment, e.g. with <, >, =, and !
# we need in general to not add any padding inside it
print "using a %s endian system" % (sys.byteorder)
PORT = 1111
FMT = "!Q"

def send_pkt():
    BIG_NUM = 2**63
    s = struct.pack(FMT, BIG_NUM)
    # destination should be already localhost
    p = IPv6() / TCP(dport=PORT)
    p.add_payload(s)
    sendp(p)

def sniffing(func):
    def _sniffing():
        print "inside here"
        p = sniff(filter="ip6", count=1)
        print_pkt()
        for x in p:
            print x.show()

    return _sniffing

@sniffing
def print_pkt():
    # maybe should use a raw socket somehow?
    s = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
    s.bind(('::1', PORT))
    s.listen(1)
    conn, addr = s.accept()
    print 'Connected by', addr
    while 1:
        data = conn.recv(1024)
        if not data:
            break
        try:
            num = struct.unpack(FMT, data)
        except struct.error:
            print "format error"
    conn.close()



if __name__ == '__main__':
    if sys.argv[1] == '-c':
        send_pkt()
    else:
        print_pkt()
        

# creating a raw socket, only works with linux
#!/usr/bin/python

import sys
import string
import struct
from socket import *

proto = 0x55aa

s = socket(AF_PACKET, SOCK_RAW, proto)
s.bind(("eth1",proto))

ifName, ifProto, pktType, hwType, hwAddr = s.getsockname()

srcAddr = hwAddr
dstAddr = "\x01\x02\x03\x04\x05\x06"
ethData = "here is some data for an ethernet packet"

txFrame = struct.pack("!6s6sh",dstAddr,srcAddr,proto) + ethData

print "Tx[%d]: "%len(ethData) + string.join(["%02x"%ord(b) for b in ethData]," ")

s.send(txFrame)

rxFrame = s.recv(2048)

dstAddr,srcAddr,proto = struct.unpack("!6s6sh",rxFrame[:14])
ethData = rxFrame[14:]

print "Rx[%d]: "%len(ethData) + string.join(["%02x"%ord(b) for b in ethData]," ")

s.close()
