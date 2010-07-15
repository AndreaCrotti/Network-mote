//#include <IPDispatch.h>
//#include <lib6lowpan.h>
//#include <ip.h>

#include "AM.h"
#include "Serial.h"
#include "hostname.h"
#define _TOS_MOTECOMM // deactivate funny linux specific weirdos like malloc or calls to the serialsource lib
#include "../driver/motecomm.h"

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
#define __TOS_MOTECOMM // deactivate funny linux specific weirdos like malloc or calls to the serialsource lib
#include "../driver/util.c"
#include "../driver/motecomm.c" // ! watch out, the c code is actually pasted here

message_t ser_in;
message_t* ser_in_consume;
message_t ser_out;

int _serialif_t_send(serialif_t* this, payload_t const payload) {
  if (payload.len <= call SerialPacket.maxPayloadLength()) {
    memcpy((void*)(call SerialPacket.getPayload(&ser_out,0)),payload.stream,payload.len);
    call SerialSend.send[0xBF](AM_BROADCAST_ADDR, &ser_out, payload.len);
    return 1;
  } else {
    return 0;
  }
}
void _serialif_t_read(serialif_t* this, payload_t* const payload) {
  if (ser_in_consume) {
    payload->stream = (stream_t*)(call SerialPacket.getPayload(ser_in_consume,0));
    payload->len = call SerialPacket.payloadLength(ser_in_consume);
    ser_in_consume = 0;
  }
}
void _serialif_t_dtor(serialif_t* this) {
  (void)this;
}
void _serialif_t_ditch(serialif_t* this, payload_t** payload) {
  (void)this;
  (void)payload;
}
int _serialif_t_fd(serialif_t* this) {
  (void)this;
  return 0;
}
void _serialif_t_open(serialif_t* this, char const* dev, char* const platform, serial_source_msg* ssm) {
  (void)this;
  (void)dev;
  (void)platform;
  (void)ssm;
}

    message_t packet;
    serialif_t if_sif;
    motecomm_t if_motecomm;
    mcp_t if_mcp;
    mccmp_t if_mccmp;
    laep_t if_laep;
    ifp_t if_ifp;

    event void Boot.booted() {
        ser_in_consume = 0;
        serialif(&if_sif,0,0,0);
        motecomm(&if_motecomm,&if_sif);
        mcp(&if_mcp,&if_motecomm);
        mccmp(&if_mccmp,&if_mcp);
        laep(&if_laep,&if_mcp);
        ifp(&if_ifp,&if_mcp);
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
