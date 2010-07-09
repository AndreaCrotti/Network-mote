//#include <6lowpan.h>

configuration MoteNetAppC{
}
implementation{
    components MainC, LedsC, MoteNetAppP;

    // For forwarding the packages received over serial
//    components Ieee154MessageC as ForwardRadio;

    // Components for the serial connection
    /* components SerialDispatcherC as SerialControl, Serial802_15_4C as Serial; */
    /*   MoteNetAppP.SerialControl -> SerialControl; */
    /*   MoteNetAppP.SerialReceive -> Serial.Receive; */
    /*   MoteNetAppP.SerialSend -> Serial.Send; */
    /*   MoteNetAppP.SerialPacket -> Serial.Packet; */

    components SerialActiveMessageC as Serial;
    MoteNetAppP.SerialControl -> Serial;
    MoteNetAppP.SerialSend -> Serial;
    MoteNetAppP.SerialReceive -> Serial;
    MoteNetAppP.SerialPacket -> Serial;
    MoteNetAppP.SerialAMPacket -> Serial;


    components new TimerMilliC() as Timer;

//    components IPDispatchC;
    
//    components new UdpSocketC() as UDP;

    MoteNetAppP.Boot -> MainC;
    MoteNetAppP.Leds -> LedsC;

    MoteNetAppP.Timer -> Timer;


//    MoteNetAppP.RadioControl -> IPDispatchC;
//    MoteNetAppP.RadioPacket -> ForwardRadio.Packet;
//    MoteNetAppP.RadioSend -> ForwardRadio;

//    MoteNetAppP.IPAddress -> IPDispatchC;
//    MoteNetAppP.UDPRadio -> UDP;

    // fun
    components new SensirionSht11C() as SensirionC;

    MoteNetAppP.TempSensor -> SensirionC.Temperature;
}
