//#include <IPDispatch.h>
//#include <lib6lowpan.h>
//#include <ip.h>

#include "AM.h"
#include "Serial.h"

module MoteNetAppP{
    uses{
        interface Boot;

        interface SplitControl as SerialControl;
        //interface SplitControl as RadioControl;

        interface Timer<TMilli> as Timer;

        /* interface Send as SerialSend; */
        /* interface Receive as SerialReceive; */
        /* interface Packet as SerialPacket; */

        interface AMSend as SerialSend[am_id_t id];
        interface Receive as SerialReceive[am_id_t id];
        interface Packet as SerialPacket;
        interface AMPacket as SerialAMPacket;
        
        // For forwarding data to the network
        //interface Ieee154Send as RadioSend;
        interface Packet as RadioPacket;

        //interface IPAddress;

        //interface UDP as UDPRadio;
        
        interface Leds;

        // fun
        interface Read<uint16_t> as TempSensor;
    }
}
implementation{
    message_t packet;

    event void Boot.booted() {
        call SerialControl.start();
    }
    event void SerialControl.startDone(error_t err) {
        call Leds.led2On();
        call Timer.startOneShot(1000);
        //call Timer.startPeriodic(2000);
    }
    event void SerialControl.stopDone(error_t err) {
        call Leds.led2Off();
    }

    event void Timer.fired() {
        call TempSensor.read();
    }

    event void TempSensor.readDone(error_t result, uint16_t val) {
        if (result == SUCCESS) {
            unsigned char i = 0;
            unsigned char j = 0;
            static unsigned char seqno = 0;
            nx_uint8_t* const pl = (nx_uint8_t*)(call SerialPacket.getPayload(&packet,0));
            nx_uint8_t* mccmpLen;
            pl[i++] = 0x15;
            seqno = (seqno + 1);
            seqno += !seqno;
            pl[i++] = seqno;
            pl[i++] = 1; //mccmp
            pl[i++] = 0x00;
            mccmpLen = &(pl[i++]);
            {
                j = i;
                pl[i++] = 0x15;
                pl[i++] = 0x00;
                pl[i++] = 0; // echo request
                pl[i++] = 0x00;
                pl[i++] = 0x00; //no payload
                *mccmpLen = i - j;
            }
            call SerialSend.send[0xBF](AM_BROADCAST_ADDR, &packet, i);
        } else {
            call Timer.startOneShot(1000);
        }
        {
            static unsigned char leds = 0;
            call Leds.set(leds++*23);
        }
    }

    event void SerialSend.sendDone[am_id_t id](message_t* m, error_t error) {
        call Timer.startOneShot(1000);
    }
    event message_t* SerialReceive.receive[am_id_t id](message_t* m, void* payload, uint8_t len) {
        call Leds.led1Toggle();
        return m;
    }

}
