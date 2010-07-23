#include "SimpleMoteApp.h"

module SimpleMoteAppP{
    uses{
        // Standard interfaces
        interface Boot;
        interface Leds;

        // Radio interfaces
        interface SplitControl as RadioControl;
        interface AMSend as RadioSend;
        interface Receive as RadioReceive;

        // Serial interfaces
        interface SplitControl as SerialControl;
        interface AMSend as SerialSend;
        interface Receive as SerialReceive;

        // Packet interfaces
        interface Packet;
        interface AMPacket;
    }
}
implementation{


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

    /**********/
    /* Events */
    /**********/
 
    /** 
     * When the device is booted, the radio and the serial device are initialized.
     */
    event void Boot.booted(){
        /* call RadioControl.start(); */
        /* call SerialControl.start(); */
    }

    event void SerialControl.startDone(error_t err){}
    event void SerialControl.stopDone(error_t err){}

    event void SerialSend.sendDone(message_t* m, error_t err){
        serialBlink();
    }
    
    event message_t* SerialReceive.receive(message_t* m, void* payload, uint8_t len){
        return m;
    }


    event void RadioControl.startDone(error_t err){}
    event void RadioControl.stopDone(error_t err){}

    event void RadioSend.sendDone(message_t* m, error_t err){
        radioBlink();
    }
    
    event message_t* RadioReceive.receive(message_t* m, void* payload, uint8_t len){
        return m;
    }
}