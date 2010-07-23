from SerialMsg import *

class MyPacket(object):
    # get the TOS_NODE_ID maybe instead
    def __init__(self, msg=None):
        self.msg = msg
    
    def __str__(self):
        # TODO: see if it's possible a sort of reverse from Blink.h enum variables
        return "\ndest: %d\ntype: %d\ninstr: %d\ndata: %d\n" % (self.msg.get_dests(), self.msg.get_type(), self.msg.get_instr(), self.msg.get_data())

    def get_data(self):
        return self.msg.data

    def create_packet(self, dest, typ, instr, data):
        self.msg = SerialMsg()
        # that's because we're always in mote 0 here
        self.msg.set_sender(0)
        self.am_type = self.msg.get_amType()
        self.msg.set_dests(dest)
        self.msg.set_type(typ)
        self.msg.set_instr(instr)
        self.msg.set_data(data)

def make_packet():
    msg = MyPacket()
    dest = input("Insert destination (as a bitmask)\n")
    typ = input("1)led\n2)sensing request\n3)sensing data\n")
    # the data field is only used in response to sensing requests
    data = 0

    if typ == 1:
        instr = input("insert led mask\n")
    elif typ == 2:
        # we can just set to 1 (light) because in the simulation we have only the demosensor
        instr = 1

    msg.create_packet(dest, typ, instr, data)
    return msg
            

# they all take a list of ordered nodes
def turn_leds_all_nodes(nodes, led):
    "takes a list of nodes and the led bitmask"
    vals = (2**(max(nodes) + 2) - 1, 1, led, 0)
    msg = MyPacket()
    msg.create_packet(*vals)
    return msg

def sens_random_node(nodes):
    "Takes a random node and turn it on"
    from random import choice
    dest = 2 ** choice(nodes[1:])
    msg = MyPacket()
    msg.create_packet(2 ** dest, 2, 1, 0)
    return msg
