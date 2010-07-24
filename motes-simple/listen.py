#!/usr/bin/env python

""" 
A simple Python script that listens on a SF for TestSerial messages.

TODO: check if it's exiting automatically
TODO: try to send something from the mote to see if communication is working or not
TODO: create a class that handles the content and outputs it accordingly
TODO: understand why is not actually listening but exiting quickly
"""

from SerialMsg import SerialMsg
from packet import MyPacket
from tinyos.message import MoteIF
from sys import argv, exit

SERVER = "localhost"
DEF_PORT = "9001"

class Debugger(object):
    def __init__(self, server, port):
        # manages the mote interface
        self.mif = MoteIF.MoteIF()
        # Attach a source to it
        source_addr = "sf@%s:%s" % (server, port)
        print "attaching to source %s" % source_addr
        self.source = self.mif.addSource(source_addr)
        self.mif.addListener(self, SerialMsg)
        self.count = 0

    # Called by the MoteIF's receive thread when a new message is received
    def receive(self, src, msg):
        print "count = %d\n%s" % (self.count, str(MyPacket(msg)))
        self.count += 1

usage = "./listen.py <server> <port>"

if __name__ == "__main__":
    if len(argv) == 1:
        Debugger(SERVER, DEF_PORT)

    if len(argv) == 3:
        Debugger(argv[1], argv[2])

    else:
        print usage
        exit(1)
