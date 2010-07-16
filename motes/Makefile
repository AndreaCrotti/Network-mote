COMPONENT=MoteNetAppC.nc
LOW6PAN_CARRIED = 128
CFLAGS += -DTOSH_DATA_LENGTH=$(LOW6PAN_CARRIED)
#BUILD_EXTRA_DEPS += 
#CLEAN_EXTRA = *.class BlinkMsg.java SerialMsg.py BlinkMsg.py
#JAVA_VERSION = 1.6
# This is the group of communication
DEFAULT_LOCAL_GROUP = 0x55
# set up the transmission power: min=1, max=31
POWER = 1
# including directly python2.6 because no better way in sim.extra was found to change the include PATH
#CFLAGS += -DCC2420_DEF_RFPOWER=$(POWER) -I$(TOSDIR)/lib/printf -I/usr/include/python2.6

#BlinkGUI.class: $(wildcard *.java) BlinkMsg.java
#	javac -target $(JAVA_VERSION) -source $(JAVA_VERSION) *.java

#BlinkConsole.class: $(wildcard *.java) BlinkMsg.java
#	javac -target $(JAVA_VERSION) -source $(JAVA_VERSION) *.java

#BlinkMsg.java:
#	mig java -target=null $(CFLAGS) -java-classname=BlinkMsg Blink.h BlinkMsg -o $@

#BlinkMsg.py:
#	mig python -target=null $(CFLAGS) -python-classname=BlinkMsg Blink.h BlinkMsg -o $@

#SerialMsg.py:
#	mig python -target=null -python-classname=SerialMsg Blink.h BlinkMsg -o SerialMsg.py

include $(MAKERULES)
