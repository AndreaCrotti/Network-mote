BLIP = $(TOSROOT)/support/sdk/c/blip
BUILD_EXTRA_DEPS += SerialMsg.py
SHARED = ../shared
MESSAGE_H = $(TOSROOT)/tos/types/message.h
CLEAN_EXTRA = SerialMsg.py

#CFLAGS += -I$(BLIP) -I$(BLIP)/lib6lowpan -I$(TOSROOT)/tos/types -I$(SHARED) -I/usr/include/python2.6 -DTOSH_DATA_LENGTH=102
CFLAGS += -I$(BLIP) -I$(BLIP)/lib6lowpan -I$(TOSROOT)/tos/types -I$(SHARED) -I/usr/include/python2.6 -DTOSH_DATA_LENGTH=102

COMPONENT=SimpleMoteAppC
# This is the group of communication
DEFAULT_LOCAL_GROUP = 0x55
# set up the transmission power: min=1, max=31
POWER = 1

SerialMsg.py:
	mig python -target=null -python-classname=SerialMsg $(MESSAGE_H) message_t -o SerialMsg.py

include $(MAKERULES)

