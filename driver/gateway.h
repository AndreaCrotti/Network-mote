#ifndef _GATEWAY_H_
#define _GATEWAY_H_

// Hardcoded sender and destination addresses for the created packets
extern uint16_t sender_address;
extern uint16_t destination_address;

void startGateway(char const *dev);

#endif /* _GATEWAY_H_ */
