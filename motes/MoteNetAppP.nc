#include <IPDispatch.h>
#include <lib6lowpan.h>
#include <ip.h>

module MoteNetAppP{
    uses{
        interface Boot;

        interface SplitControl as SerialControl;
        interface SplitControl as RadioControl;

        interface Send as SerialSend;
        // For forwarding data to the network
        interface Ieee154Send as RadioSend;
        interface Packet as RadioPacket;

        interface Receive as SerialReceive;
        
        interface IPAddress;

        interface UDP as UDPRadio;
        
        interface Leds;
    }
}
implementation{

}