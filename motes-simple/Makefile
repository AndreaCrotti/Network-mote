BUILD_EXTRA_DEPS += SerialMsg.py
CLEAN_EXTRA = SerialMsg.py

CFLAGS += -I$(TOSROOT)/tos/types -I/usr/include/python2.6 -DTOSH_DATA_LENGTH=102

COMPONENT=SimpleMoteAppC
# This is the group of communication
DEFAULT_LOCAL_GROUP = 0x55
# set up the transmission power: min=1, max=31
POWER = 31
CFLAGS += -DCC2420_DEF_RFPOWER=$(POWER)

SerialMsg.py:
	mig python -target=null -python-classname=SerialMsg $(MESSAGE_H) message_t -o SerialMsg.py

include $(MAKERULES)

