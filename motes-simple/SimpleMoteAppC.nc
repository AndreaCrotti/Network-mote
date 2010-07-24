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
    components new SendQueueC(RADIO_QUEUE_SIZE, sizeof(message_t)) as RadioQueue;
    RadioQueue.LowSend -> Radio.AMSend[AM_SIMPLE_RADIO];
    RadioQueue.AMPacket -> Radio;
    RadioQueue.Packet -> Radio;

    SimpleMoteAppP.RadioControl -> Radio;
    SimpleMoteAppP.RadioReceive -> Radio.Receive[AM_SIMPLE_RADIO];
    SimpleMoteAppP.RadioSend -> RadioQueue;
    /* SimpleMoteAppP.RadioSend -> Radio.AMSend[AM_SIMPLE_RADIO]; */
  
    // Serial components
    components SerialActiveMessageC as Serial;
    components new SendQueueC(SERIAL_QUEUE_SIZE, sizeof(message_t)) as SerialQueue;
    SerialQueue.LowSend -> Serial.AMSend[AM_SIMPLE_SERIAL];
    SerialQueue.AMPacket -> Serial;
    SerialQueue.Packet -> Serial;

    SimpleMoteAppP.SerialControl -> Serial;
    SimpleMoteAppP.SerialReceive -> Serial.Receive[AM_SIMPLE_SERIAL];
    SimpleMoteAppP.SerialSend -> SerialQueue;

    // Packet interfaces
    SimpleMoteAppP.Packet -> Radio;
    SimpleMoteAppP.AMPacket -> Radio;
}
