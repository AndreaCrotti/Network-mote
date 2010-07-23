#include "SimpleMoteApp.h"

configuration SimpleMoteAppC{
}
implementation{
    // The Program
    components SimpleMoteAppP;

    // Standard components
    components MainC, LedsC;
    SimpleMoteAppP.Boot -> MainC;
    SimpleMoteAppP.Leds -> LedsC;

    // Radio components
    components ActiveMessageC as Radio;
    SimpleMoteAppP.RadioControl -> Radio;
    SimpleMoteAppP.RadioReceive -> Radio.Receive[AM_SIMPLE_RADIO];
    SimpleMoteAppP.RadioSend -> Radio.AMSend[AM_SIMPLE_RADIO];
    
    // Serial components
    components SerialActiveMessageC as Serial;
    SimpleMoteAppP.SerialControl -> Serial;
    SimpleMoteAppP.SerialReceive -> Serial.Receive[AM_SIMPLE_SERIAL];
    SimpleMoteAppP.SerialSend -> Serial.AMSend[AM_SIMPLE_SERIAL];

    SimpleMoteAppP.Packet -> Radio;
    SimpleMoteAppP.AMPacket -> Radio;
}