BLIP = $(TOSROOT)/support/sdk/c/blip
SHARED = ../shared

INCLUDE = -I$(BLIP) -I$(BLIP)/lib6lowpan -I$(TOSROOT)/tos/types -I$(SHARED)

COMPONENT=MoteNetAppC
# This is the group of communication
DEFAULT_LOCAL_GROUP = 0x55
# set up the transmission power: min=1, max=31
POWER = 31

CFLAGS += -DCC2420_DEF_RFPOWER=$(POWER)

include $(MAKERULES)
