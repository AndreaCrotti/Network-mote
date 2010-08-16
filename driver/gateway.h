#ifndef _GATEWAY_H_
#define _GATEWAY_H_

// Hardcoded sender and destination addresses for the created packets
extern uint16_t sender_address;
extern uint16_t destination_address;

/** 
 * Starts the gateway/gateway-sf program.
 * 
 * @param dev Device descriptor for the USB port of the mote.
 */
void start_gateway(char const *dev);

#endif /* _GATEWAY_H_ */
