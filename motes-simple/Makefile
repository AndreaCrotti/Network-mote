BLIP = $(TOSROOT)/support/sdk/c/blip
SHARED = ../shared

CFLAGS += -I$(BLIP) -I$(BLIP)/lib6lowpan -I$(TOSROOT)/tos/types -I$(SHARED) -I/usr/include/python2.6/

COMPONENT=SimpleMoteAppC
# This is the group of communication
DEFAULT_LOCAL_GROUP = 0x55
# set up the transmission power: min=1, max=31
POWER = 1

include $(MAKERULES)

SerialMsg.py:
	mig python -target=null -python-classname=SerialMsg Blink.h BlinkMsg -o SerialMsg.py
