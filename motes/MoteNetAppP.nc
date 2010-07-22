#include <IPDispatch.h>
#include <lib6lowpan.h>
#include <stdint.h>

#include <ip.h>

#include "AM.h"
#include "Serial.h"
#include "hostname.h"
#define _TOS_MOTECOMM // deactivate funny linux specific weirdos like malloc or calls to the serialsource lib
#include "../driver/motecomm.h"
#include "../driver/chunker.h"

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

        interface Timer<TMilli> as Timer;

    }
}
implementation{
#define __TOS_MOTECOMM // deactivate funny linux specific weirdos like malloc or calls to the serialsource lib
#include "../driver/util.c"
#include "../driver/motecomm.c" // ! watch out, the c code is actually pasted here

    /*************/
    /* Variables */
    /*************/

    message_t* ser_in_consume;
    message_t ser_out;

    message_t packet;
    serialif_t if_sif;
    motecomm_t if_motecomm;
    mcp_t if_mcp;
    mccmp_t if_mccmp;
    laep_t if_laep;
    ifp_t if_ifp;

    // This motes IP-address
    struct in6_addr ip_address;

    // For outgoing packets
    struct split_ip_msg ip_out;
    uint8_t ip_out_data[MAX_CARRIED];
    /* struct generic_header gen_header_out; */
    struct myPacketHeader myp_header_out;

    // For incoming packets
    struct ipv6Packet ip_in;
    /* struct generic_header gen_header_in; */
    struct myPacketHeader myp_header_in;

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

    int _serialif_t_send(serialif_t* this, payload_t const payload) {
        if (payload.len <= call SerialPacket.maxPayloadLength()) {
            memcpy((void*)(call SerialPacket.getPayload(&ser_out,0)),payload.stream,payload.len);
            if(call SerialSend.send(AM_BROADCAST_ADDR, &ser_out, payload.len) 
               == SUCCESS){
                serialBlink();
            }else{
                failBlink();
            }
            return 1;
        } else {
            return 0;
        }
    }
    void _serialif_t_read(serialif_t* this, payload_t* const payload) {
        if (ser_in_consume) {
            payload->stream = (stream_t*)(call SerialPacket.getPayload(ser_in_consume,0));
            payload->len = call SerialPacket.payloadLength(ser_in_consume);
            /* call Leds.set(payload->len); */
            ser_in_consume = 0;
        }
    }
    void _serialif_t_dtor(serialif_t* this) {
        (void)this;
    }
    void _serialif_t_ditch(serialif_t* this, payload_t* const payload) {
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

    /************/
    /* Handlers */
    /************/
    void payload_rec_handler(motecomm_handler_t* that, payload_t const payload){
        struct ipv6Packet packetBuf; 
        unsigned real_payload_len;

        radioBlink();

        
        // Check whether the packet is not to small
        if(payload.len < sizeof(struct ipv6PacketHeader)){
            failBlink();
            return;
        }

        packetBuf = *(struct ipv6Packet*)(payload.stream);
        real_payload_len = payload.len - sizeof(struct ipv6PacketHeader);
        
        // Copy the payload
        memcpy(&ip_out_data, &(packetBuf.header.packetHeader), sizeof(myPacketHeader));
        memcpy(&ip_out_data + sizeof(myPacketHeader), &(packetBuf.payload), 
               real_payload_len);
        
        ip_out.headers = NULL;
        ip_out.data_len = real_payload_len + sizeof(myPacketHeader); 
        ip_out.data = (uint8_t*)&ip_out_data;
        ip_out.hdr = packetBuf.header.ip6_hdr;

        // Set this mote as the sender
        ip_out.hdr.ip6_src = ip_address;

        call IP.send(&ip_out);
    }
    /*********/
    /* Tasks */
    /*********/
    /* task void sendIPPacket(){ */

    /* } */

    /**********/
    /* Events */
    /**********/
    event void Boot.booted() {
        ser_in_consume = 0;
        serialif(&if_sif,0,0,0);
        
        motecomm(&if_motecomm,&if_sif);
        /* mcp(&if_mcp,&if_motecomm); */
        /* mccmp(&if_mccmp,&if_mcp); */
        /* laep(&if_laep,&if_mcp); */
        /* ifp(&if_ifp,&if_mcp); */
        
        // Define Handlers
        if_motecomm.setHandler(&if_motecomm, (motecomm_handler_t){.p = 0, .receive=payload_rec_handler});
        
        call IPAddress.setShortAddr(1);

        //Testing
        if(call IPAddress.haveAddress()){
            radioBlink();
        }else{
            failBlink();
        }
        call Timer.startPeriodic(1000);

        // Initialize devices
        call RadioControl.start();
        call SerialControl.start();

        // Store this mote's IP
        call IPAddress.getIPAddr(&ip_address);
    }
    
    event void Timer.fired(){
        if(call IPAddress.haveAddress()){
            radioBlink();
        }else{
            failBlink();
        }
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
        serialBlink();
        
        // Convert the data to an ipv6Packet again
        ip_in.header.ip6_hdr = *iph;
        ip_in.header.packetHeader = *((myPacketHeader*) payload);
        memcpy(ip_in.payload, payload + sizeof(myPacketHeader), 
               payload_len - sizeof(myPacketHeader));

        // Send the Payload over the serial 
        if_ifp.send(&if_ifp, (payload_t){
                .stream = (stream_t*)&ip_in, .len = payload_len + sizeof(ip6_hdr) });
    }

    event void SerialControl.startDone(error_t err) {
        
    }

    event void SerialControl.stopDone(error_t err) {
        
    }
    
    event void SerialSend.sendDone(message_t* m, error_t error) {
        
    }
    
    event message_t* SerialReceive.receive(message_t* m, void* payload, uint8_t len) {
        
        /* failBlink(); */
        
        ser_in_consume = m;
        if_motecomm.read(&if_motecomm);
               
        return m;
    }

}
