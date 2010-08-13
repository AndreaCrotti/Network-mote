/**
 *  This file is supposed to handle all communication with the connected sensor node.
 *  However, you must instatiate the connection yourself.
 *  @author Oscar Dustmann
 *  @date 2010-07-06
 */

#ifndef _MOTECOMM_H
#define _MOTECOMM_H


// if we use this module on the mote side, we have to change certain behavioural properties
// hence we define distinct meaningful flags
#ifdef _TOS_MOTECOMM
// the actual implementation (in the c file) of the sif function may be ommitted in order to be
// implemented differently in nesc
#define INCLUDE_SERIAL_IMPLEMENTATION 0
#define INCLUDE_SERIAL_FORWARD_IMPLEMENTATION 0
// dynamic memory allocation is not a good idea in tinyos, hence the comodities offered within this
// module are deactivated (that means you have to allocate your objects yourself)
#define DYNAMIC_MEMORY 0
// assertions are not meaningful in tinyos, hence, we will ignore assert(...) statements in tinyos
#define ALLOW_ASSERT 0
#endif

// for a not completely understood reason, nibbles in bitfields are interpreted
// the wrong way around, hence, 15 is converted to 5,1 which is obviously not what we want.
// However: if your architecture does it differently, pass it so as compile flag.
#ifndef NX_SWAP_NIBBLES
#define NX_SWAP_NIBBLES 1
#endif

#ifndef INCLUDE_SERIAL_FORWARD_IMPLEMENTATION
#define INCLUDE_SERIAL_FORWARD_IMPLEMENTATION INCLUDE_SERIAL_IMPLEMENTATION
#endif

#include "motecomm.sizes.h"
#include "hostname.h"

// in tinyos we do not have the luxury of the preimplemented serial_source library, hence, we just
// define used serial_* types as pointers to void - for more generality
// if not in tinyos, we have to use the serial_source library which implements the lowest layer of
// communiation over the usb port
#ifdef _TOS_MOTECOMM
typedef void* serial_source;
typedef void* serial_source_msg;
typedef char time_t;
#else
#include <serialsource.h>
#include <sfsource.h>
#include <sys/time.h>
#endif
#include <stdint.h>

// just a wrapper for the define of autogenerated hostname.h file
#define ARCHITECTURE_IDENTIFICATION ((stream_t const* const)HOSTNAME)
#define ARCHITECTURE_IDENTIFICATION_SIZE sizeof(ARCHITECTURE_IDENTIFICATION)

// if set to 1, calls to read (and write) will return immidiatly, no matter wether
// there was data or not. If not used wisely, this can lead to a very high
// processor usage (polling). It is therefore recommended to keep this 0 at all times.
#ifndef READ_NON_BLOCKING
#define READ_NON_BLOCKING 0
#endif

/****************************************************************
 *  util                                                        *
 ****************************************************************/
#include "util.h"


forward(mcp_t);
forward(serialif_t);

// wrapper build up a default mcp connection, you can do it on your own if you want to - in tos, you have to
forward(mcp_t)* openMcpConnection(char const* const dev, char* const platform, forward(serialif_t)** sif);

/****************************************************************
 *  serialif_t                                                  *
 ****************************************************************/

// most basic class to interact with the serial port. this class will behave
// differently depending on where it is used: the mote or the pc.
// it uses the functions offered by the serial_source lib.
class (serialif_t,
  serial_source source;
  serial_source_msg msg;
  // will be called (if set) when the buffer is full
  void (*onBufferFull)(void);
  // will be called (if set) when the buffer is ready to accept data
  void (*onBufferEmpty)(void);
  // send a datastream over the serial
  int (*send)(serialif_t* this, payload_t const payload);
  // initiate a read operation (will block if READ_NON_BLOCKING if 0)
  // gives you a stream administrated by this class: DO NOT FREE IT YOURSELF
  void (*read)(serialif_t* this, payload_t* const payload);
  // dispatch of the memory allocated by serialif_t::read
  void (*ditch)(serialif_t* this, payload_t* const payload);
  // return the used file descriptor (if any) -- deprecated
  int (*fd)(serialif_t* this);
);

/**
 *  Create a new serialif_t object.
 *
 *  @param dev Name of the local usb Device: e.g. "/dev/ttyUSB0"
 *  @param platform Name of the architecture we want to talk to: e.g. "telosb"
 */
serialif_t* serialif(serialif_t* this, char const* const dev, char* const platform, serial_source_msg* ssm);
#ifndef _TOS_MOTECOMM
serialif_t* serialforwardif(serialif_t* this, char const* const host, char* const port);
serialif_t* serialfakeif(serialif_t* this);
#endif

/****************************************************************
 *  motecomm_t                                                  *
 ****************************************************************/

// handler interface for the motecomm. You can put a pointer to your object in p.
typedef struct motecomm_handler_t {
  void* p;
  // will be called when a message is received and valid - do not free the payload yourself
  void (*receive)(struct motecomm_handler_t* this, payload_t const payload);
} motecomm_handler_t;

// class to handle tinyos active messages
// will add the message_t header to outgoing payload and strip the header to incoming
class (motecomm_t,
  serialif_t serialif;
  motecomm_handler_t motecomm_handler;
  // send out a payload to the mote
  void (*send)(motecomm_t* this, payload_t const payload);
  // initiate reading (handlers will be called if appropriate)
  void (*read)(motecomm_t* this);
  // initialise a handler to be called - motecomm only offers one possible handler
  void (*setHandler)(motecomm_t* this, motecomm_handler_t const handler);
);

/**
 *  Create a new motecomm_t object.
 *
 *  @param interf An already initialised serialif_t object. NULL is illegal!
 */
motecomm_t* motecomm(motecomm_t* this, serialif_t const* const interf);

/****************************************************************
 *  mcp_t                                                       *
 ****************************************************************/

#define MCP_VERSION 1

// mcp subprotocols (the next upper layer)
typedef enum {
  MCP_NONE = 0,
  MCP_MCCMP = 1,
  MCP_LAEP = 2,
  MCP_IFP = 4
} mcp_type_t;
#define MCP_TYPE_SIZE 5

// handler type for mcp
// you may put a pointer to your own object in p
typedef struct mcp_handler_t {
  void* p;
  // handler to be called if a matching packet is received
  virtual void (*receive)(struct mcp_handler_t* this, payload_t payload);
} mcp_handler_t;

forward(mccmp_t);

// main layer class
// here the first custom header is used. Can be configured to call any (none, all) possible
// protocols of the next layer. The actual types is arbitrary. 
class (mcp_t,
  motecomm_t** comm;
  motecomm_handler_t motecomm_handler;
  forward(mccmp_t)* mccmp;
  mcp_handler_t handler[MCP_TYPE_SIZE];
  // install a handler for a sub protocol, previously set handlers for the same protocol
  // will be overridden - NULL is a valid handler (deactivates triggering).
  // handlers will be called with a payload, where the mcp header was removed.
  void (*setHandler)(mcp_t* this, mcp_type_t const type, mcp_handler_t const hnd);
  // send a packet of the indicated sub protocol (the header of the sub protocol is of course
  // expected to have already added to the payload) - only the mcp header will be added
  void (*send)(mcp_t* this, mcp_type_t const type, payload_t const payload);
  // getter for the internally used motecomm_t object
  motecomm_t* (*getComm)(mcp_t* this);
);

/**
 *  Create a new mcp_t object.
 *
 *  @param uniqComm The motecomm_t object to use.
 *                  Note: there can be only one uniqComm in the whole system.
 *                  Future fixes may change that.
 *                  The first time an mcp is created you must not supply NULL, however
 *                  succeeding calls may either pass NULL or the original object.
 */
mcp_t* mcp(mcp_t* this, motecomm_t* const uniqComm);

/****************************************************************
 *  mccmp_t                                                     *
 ****************************************************************/

#define MCCMP_VERSION 1

// mccmp message type (what does the communication partner want to tell us?)
typedef enum {
  MCCMP_ECHO_REQUEST = 0,
  MCCMP_ECHO_REPLY = 1,
  MCCMP_IFY_REQUEST = 4,
  MCCMP_IFY_REPLY_MOTE = 5,
  MCCMP_IFY_REPLY_CLIENT = 6,
  MCCMP_PARAMETER_PROBLEM = 12,
  MCCMP_UNSUPPORTED = 13
} mccmp_problem_t;
#define MCCMP_PROBLEM_HANDLER_SIZE 14

// handler type for mccmp
// you may put a pointer to your object in p
typedef struct mccmp_problem_handler_t {
  void* p;
  // the handler will receive information about the message type (allowing to have one handler for different problems)
  // and additional information of the mccmp packet. Of course, the mccmp header will be stripped.
  void (*handle)(struct mccmp_problem_handler_t* this, mccmp_problem_t const problem, unsigned char const ident, unsigned char const offset, payload_t const payload);
} mccmp_problem_handler_t;

class (mccmp_t,
  mcp_handler_t parent;
  mccmp_problem_handler_t handler[MCCMP_PROBLEM_HANDLER_SIZE];
  mcp_t* mcp;
  // initiate sending a mccmp packet. It is your obligation to provide a meaningful payload, if appropriate.
  void (*send)(mccmp_t* this, mccmp_problem_t const problem, unsigned char const ident, unsigned char const offset, payload_t const payload);
  // install a handler for a specific message type (e.g. an echo reply)
  void (*setHandler)(mccmp_t* this, mccmp_problem_t const problem, mccmp_problem_handler_t const hnd);
);

/**
 *  Create a new mccmp_t object.
 *
 *  @param _mcp used mcp_t object (lower layer)
 *              This mccmp_t object will then install itself as default mccmp implementation in _mcp!
 */
mccmp_t* mccmp(mccmp_t* this, mcp_t* const _mcp);

/****************************************************************
 *  laep_t                                                      *
 ****************************************************************/

#define LAEP_VERSION 1

// ipv6 address used in this protocoll
typedef struct {
  unsigned char byte[16];
} la_t;
#define DEFAULT_LOCAL_ADDRESS {{0,0,0,0}}

typedef enum {
  LAEP_REPLY = 0,
  LAEP_REQUEST = 1
} laep_msg_t;
#define LAEP_HANDLER_SIZE 2

// handler for laep
// you may put a pointer to your object in p
typedef struct laep_handler_t {
  void* p;
  // the handler (which is called when a packet is received) will get the address of the sender)
  void (*handle)(struct laep_handler_t* this, la_t const address);
} laep_handler_t;

class (laep_t,
  mcp_handler_t parent;
  mcp_t* mcp;
  laep_handler_t handler[LAEP_HANDLER_SIZE];
  // start a request for the address of the communication partner
  void (*request)(laep_t* this);
  // install a handler for a message type
  // you SHOULD install your handler to handle requests, as this is not implemented by laep_t
  void (*setHandler)(laep_t* this, laep_msg_t const msg, laep_handler_t const hnd);
);

/**
 *  Create a new laep_t object.
 *
 *  @param _mcp used mcp_t object (lower layer)
 */
laep_t* laep(laep_t* this, mcp_t* const _mcp);

/****************************************************************
 *  ifp_t                                                       *
 ****************************************************************/

#define IFP_VERSION 1

// handler type for ifp
// you may put a pointer to your object in p
typedef struct ifp_handler_t {
  void* p;
  void (*handle)(struct ifp_handler_t* this, payload_t const payload);
} ifp_handler_t;

// deprecated class [currently unused], instad of sending an mcp(ifp(PAY)) packet we simply send PAY
// class to implement ifp
// tell the communication partner to forward the payload
class (ifp_t,
  mcp_handler_t parent;
  mcp_t* mcp;
  ifp_handler_t handler;
  // send the payload (adding an ifp header)
  void (*send)(ifp_t* this, payload_t const payload);
  // set a handler for incoming ip forward request (e.g. you could redirect it to tun)
  void (*setHandler)(ifp_t* this, ifp_handler_t const hnd);
);

/**
 *  Create a new ifp_t object.
 *
 *  @param _mcp used mcp_t object (lower layer)
 */
ifp_t* ifp(ifp_t* this, mcp_t* const _mcp);

#endif
