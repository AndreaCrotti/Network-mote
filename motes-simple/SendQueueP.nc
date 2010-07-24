/**
 * @file   SendQueueP.nc
 * @author Marius Grysla <marius@marius-laptop>
 * @date   Fri Jun  4 16:33:05 2010
 * 
 * @brief  This module implements a buffer queue for outgoing messages.
 *
 */

generic module SendQueueP(uint8_t message_length) {    
    uses {
        // The interface we use for sending
        interface AMSend as LowSend;
        // Access to the header data of the messages
        interface AMPacket;
        // Access to the payload and such
        interface Packet;
        // The tinyOS queue implementations
        interface Queue<message_t*> as Queue;
        // A pool for storing data structures
        interface Pool<message_t> as Pool;

        // For testing
        interface Leds;
    }
    provides {
        // The sending interface that is provided
        interface AMSend;
    }
}
implementation{

    /*************************/
    /* Function declarations */
    /*************************/
    
    task void sendEnqueued(void);

    /********************/
    /* Global Variables */
    /********************/
    
    /***********************/
    /* Interface functions */
    /***********************/

    /** 
     * Implementation of the send method.
     * It is tried to send the message with the LowSend component.
     * If it is busy, we queue the message up and try again later.
     * 
     * @param dest The destination.
     * @param msg A message pointer.
     * @param len The length of the message.
     * 
     * @return EBUSY, if the radio module is busy and the queue is full, FAIL if 
     * enqueueing fails and SUCCESS otherwise. 
     * otherwise.
     */
    command error_t AMSend.send(am_addr_t dest, message_t* msg, uint8_t len){
        message_t* new_msg;
        
        // Write the destination into the message header
        call AMPacket.setDestination(msg, dest);
        
        // Queue the message up if possible
        if (call Queue.size() < call Queue.maxSize()) {
            if (call Queue.empty()) {
                atomic{
                    new_msg = call Pool.get();
                    *new_msg = *msg;
                    if (call Queue.enqueue(new_msg) != SUCCESS)
                        return FAIL;
                }
                // If the queue was empty we need to post the sending task
                post sendEnqueued();
            } else {
                atomic{
                    new_msg = call Pool.get();
                    *new_msg = *msg;
                    if (call Queue.enqueue(new_msg) != SUCCESS)
                        return FAIL;
                }
            }
        } else {
            call Leds.led2Toggle();
            // Queue is full!
            return EBUSY;
        }

        return SUCCESS;
    }
    
    /** 
     * Forwarded from the lower layer.
     * 
     * @param msg A message.
     * @param len The message's length.
     * 
     * @return The Payload.
     */
    command void* AMSend.getPayload(message_t* msg, uint8_t len){
        return call LowSend.getPayload(msg, len);
    }

    /** 
     * Forwarded from the lower layer.
     * 
     * @return The maximal payload length. 
     */
    command uint8_t AMSend.maxPayloadLength(){
        return call LowSend.maxPayloadLength();
    }
    
    /** 
     * Removes a message out of the queue.
     * If the message is currently being sended, the sending layer is notified to abort.
     * 
     * TODO: Implement!
     *
     * @param msg The message to cancel.
     * 
     * @return SUCCESS if the transmission was cancelled, FAIL otherwise. 
     */
    command error_t AMSend.cancel(message_t* msg){
        return call LowSend.cancel(msg);
    }

    
    /*************************/
    /* Event implementations */
    /*************************/

    /** 
     * When the sending of the message is done another message is sent and the above 
     * layer is signaled.
     * 
     * @param msg The sended message.
     * @param error A return value.
     */
    event void LowSend.sendDone(message_t* msg, error_t error){
        message_t* q_msg;

        //call Leds.led1Toggle();

        // Remove the message from the queue
        atomic{
            q_msg = call Queue.dequeue();
            call Pool.put(q_msg);
        }

        dbg("Radio", "Sended a message, remaining queue size is %d \n", call Queue.size());
        
        // Send the next message
        if(!(call Queue.empty())){
            post sendEnqueued();
        }
        
        signal AMSend.sendDone(msg, error);
    }

    /*********/
    /* Tasks */
    /*********/

    /** 
     * A task to sent the first element of the queue over the radio. 
     */
    task void sendEnqueued(void){
        message_t* toSend = call Queue.head();
        am_addr_t address = call AMPacket.destination(toSend);
        uint8_t mlen = call Packet.payloadLength(toSend);

        if (call LowSend.send(address, toSend, TOSH_DATA_LENGTH) != SUCCESS) {
            post sendEnqueued();
        }
    }
}
