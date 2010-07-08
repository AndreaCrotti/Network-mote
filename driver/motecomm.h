/**
 *  This file is supposed to handle all communication with the connected sensor node.
 *  However, you must instatiate the connection yourself.
 *  @author Oscar Dustmann
 *  @date 2010-07-06
 */

#ifndef _MOTECOMM_H
#define _MOTECOMM_H

#include <serialsource.h>

#include "motecomm.sizes.h"
#include "hostname.h"

#define ARCHITECTURE_IDENTIFICATION ((stream_t const* const)HOSTNAME)
#define ARCHITECTURE_IDENTIFICATION_SIZE sizeof(ARCHITECTURE_IDENTIFICATION)


#define virtual

/****************************************************************
 *  util                                                        *
 ****************************************************************/

typedef struct {
  serial_source source;
  void (*send)(void* foo, char const* const stream, int len);
  void (*listen)(void* foo, char* const stream, int* const len, int const maxlen);
} serialif_t;

typedef unsigned char stream_t;
typedef unsigned int streamlen_t;

typedef struct {
  stream_t const* stream;
  streamlen_t len;
} payload_t;

/****************************************************************
 *  motecomm_t                                                  *
 ****************************************************************/

typedef struct motecomm_handler_t {
  void* p;
  void (*receive)(struct motecomm_handler_t* this, payload_t const payload);
} motecomm_handler_t;

typedef struct motecomm_t {
  serialif_t serialif;
  motecomm_handler_t motecomm_handler;
  void (*send)(struct motecomm_t* this, payload_t const payload);
  void (*listen)(struct motecomm_t* this);
  void (*setHandler)(struct motecomm_t* this, motecomm_handler_t const* const handler);
} motecomm_t;


// motecomm_t constructor
void motecomm(motecomm_t* this, serialif_t const* const interface);

/****************************************************************
 *  mcp_t                                                       *
 ****************************************************************/

#define MCP_VERSION 1

typedef enum {
  MCP_NONE = 0,
  MCP_MCCMP = 1,
  MCP_LEAP = 2,
  MCP_IFP = 4
} mcp_type_t;

#define MCP_TYPE_SIZE 5

typedef struct mcp_handler_t {
  void* p;
  virtual void (*receive)(struct mcp_handler_t* this, payload_t payload);
} mcp_handler_t;

struct mccmp_t; //forward decl

typedef struct mcp_t {
  motecomm_t** comm;
  motecomm_handler_t motecomm_handler;
  struct mccmp_t* mccmp;
  mcp_handler_t handler[MCP_TYPE_SIZE];
  void (*setHandler)(struct mcp_t* this, mcp_type_t const type, mcp_handler_t const hnd);
  void (*send)(struct mcp_t* this, mcp_type_t const type, payload_t const payload);
} mcp_t;

// mcp_t constructor
// note: there can be only one uniqComm in the whole system. Future fixes may change that.
void mcp(mcp_t* this, motecomm_t* const uniqComm);

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

typedef struct mccmp_t {
  mcp_handler_t parent;
  mccmp_problem_handler_t handler[MCCMP_PROBLEM_HANDLER_SIZE];
  mcp_t* mcp;
  void (*send)(struct mccmp_t* this, mccmp_problem_t const problem, unsigned char const ident, unsigned char const offset, payload_t const payload);
  void (*setHandler)(struct mccmp_t* this, mccmp_problem_t const problem, mccmp_problem_handler_t const hnd);
} mccmp_t;

void mccmp(mccmp_t* this, mcp_t* const mcp);

/****************************************************************
 *  leap_t                                                      *
 ****************************************************************/

typedef struct leap_t {
  mcp_handler_t parent;
} leap_t;

void leap(leap_t* this, mcp_t* const mcp);

/****************************************************************
 *  ifp_t                                                       *
 ****************************************************************/

typedef struct ifp_t {
  mcp_handler_t parent;
} ifp_t;

void ifp(ifp_t* this, mcp_t* const mcp);

#endif
