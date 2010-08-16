// This file is only used to build the gateway application with support
// for the serial forwarder.

// this define does all the magic: it tells the gateway.c code that it is supposed to use
// the serial forwarder instead of the real thing (which would be the serial interface)
#define SERIAL_STYLE 1

#include "gateway.c"
