//#include <6lowpan.h>
#include "MoteNetApp.h"

configuration MoteNetAppC{
}
implementation{
    components MainC, LedsC, MoteNetAppP;
    MoteNetAppP.Boot -> MainC;
    MoteNetAppP.Leds -> LedsC;

    // For forwarding the packages received over serial
    /* components Ieee154MessageC as ForwardRadio; */
    /* components new SendQueueC(10, sizeof(message_t)) as RadioQueue; */
    /* RadioQueue.LowSend -> ForwardRadio.Send; */
    /* RadioQueue.AMPacket -> ForwardRadio; */
    /* MoteNetAppP.RadioControl -> IPDispatchC; */
    /* MoteNetAppP.RadioPacket -> ForwardRadio.Packet; */
    /* MoteNetAppP.RadioSend -> RadioQueue; */

    // Components for the serial connection
    /* components SerialDispatcherC as SerialControl, Serial802_15_4C as Serial; */
    /* components new SendQueueC(10, sizeof(message_t)) as SerialQueue; */
    /* MoteNetAppP.SerialControl -> SerialControl; */
    /* MoteNetAppP.SerialReceive -> Serial.Receive; */
    /* MoteNetAppP.SerialSend -> Serial.Send; */
    /* MoteNetAppP.SerialPacket -> Serial.Packet; */

    components SerialActiveMessageC as Serial;
    // Queue for the serial interface
    components new SendQueueC(SERIAL_QUEUE_SIZE, sizeof(message_t)) as SerialQueue;
    SerialQueue.LowSend -> Serial.AMSend[0];
    SerialQueue.AMPacket -> Serial;

    MoteNetAppP.SerialControl -> Serial;
    MoteNetAppP.SerialSend -> SerialQueue;
    MoteNetAppP.SerialReceive -> Serial.Receive[0];
    MoteNetAppP.SerialPacket -> Serial;
    /* MoteNetAppP.SerialAMPacket -> Serial; */

    components IPDispatchC;
    MoteNetAppP.RadioControl -> IPDispatchC;
    MoteNetAppP.IP -> IPDispatchC.IP[15];
    MoteNetAppP.IPAddress -> IPDispatchC;
    
    components Ieee154MessageC as Radio;
    MoteNetAppP.RadioSend -> Radio;

    components new TimerMilliC() as Timer;
    MoteNetAppP.Timer -> Timer;
}
