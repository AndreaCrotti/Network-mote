#include <IPDispatch.h>
#include <lib6lowpan.h>
#include <ip.h>

#include "AM.h"
#include "Serial.h"
#include "hostname.h"
#define _TOS_MOTECOMM // deactivate funny linux specific weirdos like malloc or calls to the serialsource lib
#include "../driver/motecomm.h"

module MoteNetAppP{
    uses{
        interface Boot;

        interface SplitControl as SerialControl;
        interface SplitControl as RadioControl;

        interface AMSend as SerialSend;
        interface Receive as SerialReceive;
        interface Packet as SerialPacket;

        /* interface AMSend as SerialSend[am_id_t id]; */
        /* interface Receive as SerialReceive[am_id_t id]; */
        /* interface Packet as SerialPacket; */
        /* interface AMPacket as SerialAMPacket; */
        
        // For forwarding data to the network
        /* interface Ieee154Send as RadioSend; */
        /* interface Packet as RadioPacket; */

        interface IPAddress;
        interface IP;
    
        interface Ieee154Send as RadioSend;
    
        interface Leds;

    }
}
implementation{
#define __TOS_MOTECOMM // deactivate funny linux specific weirdos like malloc or calls to the serialsource lib
#include "../driver/motecomm.c" // ! watch out, the c code is actually pasted here

    message_t packet;

    /*************/
    /* Functions */
    /*************/
    /** 
     * Toggles a LED when a message is send to the radio. 
     */
    void radioBlink(){
        call Leds.led0Toggle();
    }
    /** 
     * Toggles a LED when a message is send to the serial. 
     */
    void serialBlink(){
        call Leds.led1Toggle();
    }
    /** 
     * Toggles a LED when a message couldn't be send and is dropped 
     */
    void failBlink(){
        call Leds.led2Toggle();
    }

    event void Boot.booted() {
        // Initialize devices
        call RadioControl.start();
        call SerialControl.start();
    }

    event void RadioControl.startDone(error_t err){
    }

    event void RadioControl.stopDone(error_t err){
    }

    event void RadioSend.sendDone(message_t* m, error_t err){

    }

    event void IP.recv(struct ip6_hdr *iph, void *payload, struct ip_metadata *meta){
        // Get the transmitted payload length
        uint16_t payload_len = iph->plen;
        // Send the Payload over the serial 
        if(call SerialSend.send(AM_BROADCAST_ADDR, payload, payload_len) == SUCCESS){
            serialBlink();
        }else{
            failBlink();
        }
    }

    event void SerialControl.startDone(error_t err) {
        
    }

    event void SerialControl.stopDone(error_t err) {
        
    }
    
    event void SerialSend.sendDone(message_t* m, error_t error) {
        
    }
    
    event message_t* SerialReceive.receive(message_t* m, void* payload, uint8_t len) {
        call Leds.led1Toggle();
        return m;
    }

}
