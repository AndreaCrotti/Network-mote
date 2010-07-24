from SerialMsg import *

class MyPacket(object):
    def __init__(self, msg=None):
        self.msg = msg
    
    def __str__(self):
        return str(self.msg)

    def get_data(self):
        return self.msg.data
