/**
 * @file   SendQueueC.nc
 * @author Marius Grysla, Andrea Crotti, Oscar Dustmann
 * @date   Fri Jun  4 17:35:24 2010
 * 
 * @brief  Configuration file for the SendQueue module.
 * 
 * This file wires the SendQueue module and creates the needed components.
 * To use the queue with different low layer sending components, the AMSend interface is
 * used and has to be wired by the user. 
 */

generic configuration SendQueueC(uint8_t queue_length, uint8_t message_length) @safe() {
    provides interface AMSend;
    
    // Use the AMSend interface to be compatible with different sending components
    uses interface AMSend as LowSend;
    uses interface AMPacket;
}
implementation {
    // Component creation
    components new SendQueueP(message_length) as SendQueueP;
    components new TimerMilliC() as RetryTimer;
    
    // Using the existing queue component in TinyOS
    components new QueueC(message_t*, queue_length) as Queue;
    // And a pool for storing messages
    components new PoolC(message_t, queue_length) as Pool;

    // Wiring
    SendQueueP.LowSend = LowSend;
    SendQueueP.AMPacket = AMPacket;
    SendQueueP.Queue -> Queue;
    SendQueueP.Pool -> Pool;

    AMSend = SendQueueP;

    // For testing
    components LedsC as Leds;
    SendQueueP.Leds -> Leds;
}