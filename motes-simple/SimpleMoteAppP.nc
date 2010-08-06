#include "SimpleMoteApp.h"
//#include "../shared/structs.h"

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
    /* Variables */
    /*************/

    // A queue for every mote, in which we save the latest 16 messages to 
    // identify duplicates.
    // The higher byte will hold the sequential number, while the lower byte
    // will hold the number of the chunk.
    uint16_t queues[16][MAX_MOTES];
    // Array of pointers to the queues' heads.
    uint16_t *heads[MAX_MOTES];
    // Array of pointers to the queues' tails.
    uint16_t *tails[MAX_MOTES];

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

    /*********/
    /* Tasks */
    /*********/

    /** 
     * A task for sending radio messages and the used variables.
     */
    am_addr_t sR_dest;
    message_t* sR_m;
    uint8_t sR_len;
    task void sendRadio(){
        call RadioSend.send(sR_dest, sR_m, sR_len);
    }

    /**
     * Sends an acknowledgement for the last packet over the serial.
     * An acknowledgement is just of an empty Active Message.
     */
    task void sendSerialAck(){
        const message_t ack_msg;

        //TODO: Does that work, or does TinyOS give us an error for the 0?
        SerialSend.send(&ack_msg, 0);
    }

    /**********/
    /* Events */
    /**********/
 
    /** 
     * When the device is booted, the radio and the serial device are initialized.
     * 
     * @see tos.interfaces.Boot.booted
     */
    event void Boot.booted(){
        call RadioControl.start();
        call SerialControl.start();
    }

    /** 
     * Called, when the serial module was started.
     * 
     * @see tos.interfaces.SplitControl.startDone
     */
    event void SerialControl.startDone(error_t err){}
    /** 
     * Called, when the serial module was stopped.
     * 
     * @see tos.interfaces.SplitControl.stopDone
     */
    event void SerialControl.stopDone(error_t err){}
    
    /** 
     * Called, when message was sent over the serial device.
     * 
     * @see tos.interfaces.Send.sendDone
     */
    event void SerialSend.sendDone(message_t* m, error_t err){
        if(err == SUCCESS){
            serialBlink();
        }else{
            failBlink();
        }
    }
    

    /** 
     * This event is called, when a new message was received over the serial.
     * 
     * @see tos.interfaces.Receive.receive
     */
    event message_t* SerialReceive.receive(message_t* m, void* payload, uint8_t len){
        
        // broadcast the message over the radio
        sR_dest = AM_BROADCAST_ADDR; sR_m = m; sR_len = len;
        post sendRadio();

        // Send an acknowledgement to the connected PC
        post sendSerialAck();

        return m;
    }

    /** 
     * Called, when the radio module was started.
     * 
     * @see tos.interfaces.SplitControl.startDone
     */
    event void RadioControl.startDone(error_t err){}
    /** 
     * Called, when the radio module was stopped.
     * 
     * @see tos.interfaces.SplitControl.stopDone
     */
    event void RadioControl.stopDone(error_t err){}

    /** 
     * Called, when message was sent over the radio.
     * 
     * @see tos.interfaces.Send.sendDone
     */
    event void RadioSend.sendDone(message_t* m, error_t err){
        if(err == SUCCESS){
            radioBlink();
        }else{
            failBlink();
        }

    }
    
    /** 
     * This event is called, when a new message was received over the radio.
     * 
     * @see tos.interfaces.Receive.receive
     */
    event message_t* RadioReceive.receive(message_t* m, void* payload, uint8_t len){

        // Just forward the message over the serial device
        call SerialSend.send(AM_BROADCAST_ADDR, m, len);

        return m;
    }
}
