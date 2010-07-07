/**
 *  This file is supposed to handle all communication with the connected sensor node.
 *  However, you must instatiate the connection yourself.
 *  @author Oscar Dustmann
 *  @date 2010-07-06
 */

#ifndef _MOTECOMM_H
#define _MOTECOMM_H

#include "motecomm.sizes.h"

#define virtual

/**** util ****/

typedef struct {
  // TODO: what essentials do we require, all members are just guessed !
  void* foo;
  void (*send)(void* foo, char const* const stream, int len);
  void (*listen)(void* foo, char* const stream, int* const len, int const maxlen);
} serialif_t;

typedef unsigned char stream_t;
typedef unsigned int streamlen_t;
typedef unsigned short channelid_t;

/**** motecomm_t ****/

typedef struct motecomm_handler_t {
  void* p;
  void (*receive)(struct motecomm_handler_t* this, stream_t const* const stream, streamlen_t const len);
} motecomm_handler_t;

typedef struct motecomm_t {
  serialif_t serialif;
  motecomm_handler_t motecomm_handler;
  void (*send)(struct motecomm_t* this, stream_t const* const stream, streamlen_t const len);
  void (*listen)(struct motecomm_t* this);
  void (*setHandler)(struct motecomm_t* this, motecomm_handler_t const* const handler);
} motecomm_t;


// motecomm_t constructor
void motecomm(motecomm_t* this, serialif_t const* const interface);

/**** mcp_t ****/

typedef enum {
  MCP_NONE = 0,
  MCP_MCCMP = 1,
  MCP_LEAP = 2,
  MCP_IFP = 4
} mcp_type_t;

// essentially a purely abstract base class
typedef struct mcp_handler_t {
  virtual void (*receive)(struct mcp_handler_t* this, stream_t const* const stream, streamlen_t const len);
} mcp_handler_t;

typedef struct mcp_t {
  motecomm_t** comm;
  motecomm_handler_t motecomm_handler;
  mcp_handler_t handler[MCP_TYPE_SIZE];
  void (*setHandler)(struct mcp_t* this, mcp_type_t const type, mcp_handler_t const* const hnd);
} mcp_t;

// mcp_t constructor
void mcp(mcp_t* this, motecomm_t* const uniqComm);

/**** mccmp_t ****/

typedef struct mccmp_t {
  mcp_handler_t parent;
} mccmp_t;

void mccmp(mccmp_t* this);

/**** leap_t ****/

typedef struct leap_t {
  mcp_handler_t parent;
} leap_t;

void leap(leap_t* this);

/**** ifp_t ****/

typedef struct ifp_t {
  mcp_handler_t parent;
} ifp_t;

void ifp(ifp_t* this);

#endif
