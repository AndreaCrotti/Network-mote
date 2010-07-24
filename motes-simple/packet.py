from SerialMsg import SerialMsg

class MyPacket(object):
    # get the TOS_NODE_ID maybe instead
    def __init__(self, msg=None):
        self.msg = msg
    
    def __str__(self):
        # TODO: see if it's possible a sort of reverse from Blink.h enum variables
        return "empty message"

    def get_data(self):
        return self.msg.data

    def create_packet(self):
        self.msg = SerialMsg()
        # that's because we're always in mote 0 here
        self.am_type = self.msg.get_amType()

def make_packet():
    msg = MyPacket()
    msg.create_packet()
    return msg
