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
    /* Variables */
    /*************/

    // A queue for every mote, in which we save the latest 16 messages to 
    // identify duplicates.
    // The higher byte will hold the sequential number, while the lower byte
    // will hold the number of the chunk.
    uint16_t queues[MAX_MOTES][PACKET_QUEUE_SIZE];
    // Array of pointers to the queues' heads.
    uint16_t *heads[MAX_MOTES];

    // The message that is used for serial acknowledgements.
    message_t ack_msg;

    /*************/
    /* Functions */
    /*************/

    /** 
     * Test, whether an message signature is in the queue (was recently seen).
     * 
     * @param client The TOS_NODE_ID. Should be smaller that MAX_MOTES!!!
     * @param seq_no The sequential number of the message.
     * @param ord_no The chunk number.
     * 
     * @return 1, if the signature is contained, 0 otherwise.
     */
    boolean inQueue(am_addr_t client, seq_no_t seq_no, uint8_t ord_no){
        uint8_t i;
        uint16_t identifier;

        // Build identifier from seq_nr and ord_nr
        identifier = (((uint16_t) seq_no) << 8) | ord_no;

        // Just loop over all elements
        for(i = 0; i < PACKET_QUEUE_SIZE; i++){
            if(queues[client][i] == identifier){
                return 1;
            }
        }
        
        return 0;
    }

    /** 
     * Inserts a new message identifier into one of the queues.
     * 
     * @param client The TOS_NODE_ID. Should be smaller that MAX_MOTES!!!
     * @param seq_nr The sequential number of the message.
     * @param ord_nr The chunk number.
     */
    void addToQueue(am_addr_t client, seq_no_t seq_no, uint8_t ord_no){
        uint16_t identifier;

        // Build identifier from seq_nr and ord_nr
        identifier = (((uint16_t) seq_no) << 8) | ord_no;

        if(heads[client] == &queues[client][PACKET_QUEUE_SIZE - 1]){
            // We are at the end of the queue
            heads[client] = queues[client];
            *heads[client] = identifier;
        }else{
            // Normal insertion
            *(++heads[client]) = identifier;
        }
    }

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
     * A task for sending serial  messages and the used variables.
     */
    am_addr_t sS_dest;
    message_t* sS_m;
    uint8_t sS_len;
    task void sendSerial(){
        call SerialSend.send(sS_dest, sS_m, sS_len);
    }

    /**
     * Sends an acknowledgement for the last packet over the serial.
     * An acknowledgement is just of an empty Active Message.
     */
    task void sendSerialAck(){

        //TODO: Does that work, or does TinyOS give us an error for the 0?
        call SerialSend.send(AM_BROADCAST_ADDR, &ack_msg, 0);
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
        uint8_t i,j;

        // Initialize the queues with the maximal values
        for(i = 0; i < MAX_MOTES; i++){
            heads[i] = queues[i];
            
            for(j = 0; j < PACKET_QUEUE_SIZE; j++){
                queues[i][j] = -1;
            }
        }

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

        // Send an acknowledgement to the connected PC
        post sendSerialAck();

        // broadcast the message over the radio
        sR_dest = AM_BROADCAST_ADDR; sR_m = m; sR_len = len;
        post sendRadio();

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
        myPacketHeader *myph = (myPacketHeader*) m;
        
        am_addr_t source = myph->sender;

        // Test if the message is for us
        if(myph->destination == TOS_NODE_ID){
            // Forward it to the serial
            sS_dest = AM_BROADCAST_ADDR; sS_m = m; sS_len = len;
            post sendSerial();
        }else{
            // Test, whether the message should be broadcasted over the radio
            if(!inQueue(source, myph->seq_no, myph->ord_no)){
                // Add this message to the queue of seen messages
                addToQueue(source, myph->seq_no, myph->ord_no); 

                // Forward it!
                sR_dest = AM_BROADCAST_ADDR; sR_m = m; sR_len = len;
                post sendRadio();
            }
        }
            
        return m;
    }
}
