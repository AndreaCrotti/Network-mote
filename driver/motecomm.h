/**
 *  This file is supposed to handle all communication with the connected sensor node.
 *  However, you must instatiate the connection yourself.
 *  @author Oscar Dustmann
 *  @date 2010-07-06
 */

#ifndef _MOTECOMM_H
#define _MOTECOMM_H


#ifdef _TOS_MOTECOMM
#define INCLUDE_SERIAL_IMPLEMENTATION 0
#define DYNAMIC_MEMORY 0
#define ALLOW_ASSERT 0
#endif


#include "motecomm.sizes.h"
#include "hostname.h"

#ifdef _TOS_MOTECOMM
typedef void* serial_source;
typedef void* serial_source_msg;
#else
#include <serialsource.h>
#endif
#include <stdint.h>

#define ARCHITECTURE_IDENTIFICATION ((stream_t const* const)HOSTNAME)
#define ARCHITECTURE_IDENTIFICATION_SIZE sizeof(ARCHITECTURE_IDENTIFICATION)

// for a not completely understood reason, nibbles in bitfields are interpreted
// the wrong way around, hence, 15 is converted to 5,1 which is obviously not what we want.
// However: if your architecture does it differently, pass it so as compile flag.
#ifndef NX_SWAP_NIBBLES
#define NX_SWAP_NIBBLES 1
#endif

#ifndef READ_NON_BLOCKING
#define READ_NON_BLOCKING 0
#endif

/****************************************************************
 *  util                                                        *
 ****************************************************************/
#include "util.h"


//payload_t* gluePayloadMalloc(payload_t const* const first, payload_t const* const second);

forward(mcp_t);
forward(serialif_t);
forward(mcp_t)* openMcpConnection(char const* const dev, char* const platform, forward(serialif_t)** sif);

/****************************************************************
 *  serialif_t                                                  *
 ****************************************************************/

class (serialif_t,
  serial_source source;
  serial_source_msg msg;
  int (*send)(serialif_t* this, payload_t const payload);
  void (*read)(serialif_t* this, payload_t* const payload);
  void (*ditch)(serialif_t* this, payload_t** payload);
  int (*fd)(serialif_t* this);
);

/**
 *  Create a new serialif_t object.
 *
 *  \param dev Name of the local usb Device: e.g. "/dev/ttyUSB0"
 *  \param platform Name of the architecture we want to talk to: e.g. "telosb"
 */
serialif_t* serialif(serialif_t* this, char const* const dev, char* const platform, serial_source_msg* ssm);

/****************************************************************
 *  motecomm_t                                                  *
 ****************************************************************/

typedef struct motecomm_handler_t {
  void* p;
  void (*receive)(struct motecomm_handler_t* this, payload_t const payload);
} motecomm_handler_t;

class (motecomm_t,
  serialif_t serialif;
  motecomm_handler_t motecomm_handler;
  void (*send)(motecomm_t* this, payload_t const payload);
  void (*read)(motecomm_t* this);
  void (*setHandler)(motecomm_t* this, motecomm_handler_t const handler);
);

// motecomm_t constructor
motecomm_t* motecomm(motecomm_t* this, serialif_t const* const interf);

/****************************************************************
 *  mcp_t                                                       *
 ****************************************************************/

#define MCP_VERSION 1

typedef enum {
  MCP_NONE = 0,
  MCP_MCCMP = 1,
  MCP_LAEP = 2,
  MCP_IFP = 4
} mcp_type_t;

#define MCP_TYPE_SIZE 5

typedef struct mcp_handler_t {
  void* p;
  virtual void (*receive)(struct mcp_handler_t* this, payload_t payload);
} mcp_handler_t;

forward(mccmp_t);

class (mcp_t,
  motecomm_t** comm;
  motecomm_handler_t motecomm_handler;
  forward(mccmp_t)* mccmp;
  mcp_handler_t handler[MCP_TYPE_SIZE];
  void (*setHandler)(mcp_t* this, mcp_type_t const type, mcp_handler_t const hnd);
  void (*send)(mcp_t* this, mcp_type_t const type, payload_t const payload);
  motecomm_t* (*getComm)(mcp_t* this);
);

// mcp_t constructor
// note: there can be only one uniqComm in the whole system. Future fixes may change that.
mcp_t* mcp(mcp_t* this, motecomm_t* const uniqComm);

/****************************************************************
 *  mccmp_t                                                     *
 ****************************************************************/

#define MCCMP_VERSION 1

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

typedef struct mccmp_problem_handler_t {
  void* p;
  void (*handle)(struct mccmp_problem_handler_t* this, mccmp_problem_t const problem, unsigned char const ident, unsigned char const offset, payload_t const payload);
} mccmp_problem_handler_t;

class (mccmp_t,
  mcp_handler_t parent;
  mccmp_problem_handler_t handler[MCCMP_PROBLEM_HANDLER_SIZE];
  mcp_t* mcp;
  void (*send)(mccmp_t* this, mccmp_problem_t const problem, unsigned char const ident, unsigned char const offset, payload_t const payload);
  void (*setHandler)(mccmp_t* this, mccmp_problem_t const problem, mccmp_problem_handler_t const hnd);
);

mccmp_t* mccmp(mccmp_t* this, mcp_t* const _mcp);

/****************************************************************
 *  laep_t                                                      *
 ****************************************************************/

#define LAEP_VERSION 1

typedef struct {
  unsigned char byte[16];
} la_t;

typedef enum {
  LAEP_REPLY = 0,
  LAEP_REQUEST = 1
} laep_msg_t;

#define LAEP_HANDLER_SIZE 2

typedef struct laep_handler_t {
  void* p;
  void (*handle)(struct laep_handler_t* this, la_t const address);
} laep_handler_t;

class (laep_t,
  mcp_handler_t parent;
  mcp_t* mcp;
  laep_handler_t handler[LAEP_HANDLER_SIZE];
  void (*request)(laep_t* this);
  void (*setHandler)(laep_t* this, laep_msg_t const msg, laep_handler_t const hnd);
);

laep_t* laep(laep_t* this, mcp_t* const _mcp);

/****************************************************************
 *  ifp_t                                                       *
 ****************************************************************/

#define IFP_VERSION 1

typedef struct ifp_handler_t {
  void* p;
  void (*handle)(struct ifp_handler_t* this, payload_t const payload);
} ifp_handler_t;

class (ifp_t,
  mcp_handler_t parent;
  mcp_t* mcp;
  ifp_handler_t handler;
  void (*send)(ifp_t* this, payload_t const payload);
  void (*setHandler)(ifp_t* this, ifp_handler_t const hnd);
);

ifp_t* ifp(ifp_t* this, mcp_t* const _mcp);

#endif
