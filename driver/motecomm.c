#include "motecomm.h"
#include <assert.h>
#include <string.h>

/**** motecomm_t ****/

// public:
void __motecomm_t__send(motecomm_t* this, stream_t const* const stream, streamlen_t const len);
void __motecomm_t__listen(motecomm_t* this);
void __motecomm_t__setHandler(motecomm_t* this, motecomm_handler_t const* const handler);

void motecomm(motecomm_t* this, serialif_t const* const interface) {
  assert(this);
  assert(interface && interface->send);
  this->serialif = *interface; // compile time fixed size, so we can copy directly - members are copied transparently
  this->send = __motecomm_t__send;
  this->listen = __motecomm_t__listen;
  this->setHandler = __motecomm_t__setHandler;
}

void __motecomm_t__listen(motecomm_t* this) {
  assert(this->motecomm_handler.receive);
  stream_t* stream = NULL;
  int len = 0;
  // TODO TODO TODO start reading, put it to stream, length to len
  this->motecomm_handler.receive(&(this->motecomm_handler),stream,len);
}
void __motecomm_t__send(motecomm_t* this, stream_t const* const stream, streamlen_t const len) {
  // TODO TODO TODO send the stream using the serialif
}
void __motecomm_t__setHandler(motecomm_t* this, motecomm_handler_t const* const handler) {
  assert(this);
  assert(handler);
  this->motecomm_handler = *handler;
}


/**** mcp_t ****/

#define MCP_VERSION 1

// private:
void __mcp_t__receive(motecomm_handler_t* that, stream_t const* const stream, streamlen_t const len); // implements motecomm_handler_t::receive

// public:
void __mcp_t__setHandler(mcp_t* this, mcp_type_t const type, mcp_handler_t const* const hnd);

void mcp(mcp_t* this, motecomm_t* const uniqComm) {
  assert(this);
  assert(uniqComm);
  static motecomm_t* persistentComm = NULL;
  if (!persistentComm)
    persistentComm = uniqComm;
  assert(uniqComm == persistentComm && "Cannot use different comm objects.");
  assert(persistentComm);
  this->comm = &persistentComm;
  memset((void*)(this->handler),0,sizeof(mcp_handler_t*)*MCP_TYPE_SIZE);
  this->setHandler = __mcp_t__setHandler;
  this->motecomm_handler.p = (void*)this;
  this->motecomm_handler.receive = __mcp_t__receive;
  persistentComm->setHandler(persistentComm,&(this->motecomm_handler));
}

typedef union {
  stream_t const* stream;
  struct {
    // TODO check if this works correctly -> endianness concerns.
    //      it depends on compiler, os and architecture, how this is resolved.
    // XXX since linux on x{86,64} is lilen, and network convention is bigen, this will most likely not work at all.
    unsigned version :MCP_HD_VERSION;
    unsigned header  :MCP_HD_HEADER;
    unsigned ident   :MCP_HD_IDENT;
    unsigned type    :MCP_HD_TYPE;
    unsigned port    :MCP_HD_PORT;
    unsigned payload :MCP_HD_PAYLOAD;
  }* header;
} mcp_header_t;

void __mcp_t__receive(motecomm_handler_t* that, stream_t const* const stream, streamlen_t const len) {
  mcp_t* this = (mcp_t*)(that->p);
  assert(stream);
  char lengthokay = (len >= MCP_HEADER_BYTES);
  mcp_header_t h;
  h.stream = stream;
  if (h.header->version != MCP_VERSION) {
    // TODO: error 12
    return;
  }
  if (h.header->header != MCP_HEADER_BYTES) {
    // TODO: error 12
    return;
  }
  if (h.header->port != 0) {
    // TODO: error 31 or implement additional ports
    return;
  }
  if (h.header->payload+h.header->header > len) {
    // TODO: error 12
    return;
  }
  assert(h.header->type < MCP_TYPE_SIZE);
  mcp_handler_t* hnd = &(this->handler[h.header->type]);
  if (hnd->receive) {
    hnd->receive(hnd,stream+h.header->header,h.header->payload);
  }
}
void __mcp_t__setHandler(mcp_t* this, mcp_type_t const type, mcp_handler_t const* const hnd) {
  assert((unsigned)type < MCP_TYPE_SIZE);
  assert(hnd);
  this->handler[type] = *hnd;
}

/**** mccmp_t ****/

void mccmp(mccmp_t* this) {
  // TODO
}

/**** leap_t ****/

void leap(leap_t* this) {
  // TODO
}

/**** ifp_t ****/

void ifp(ifp_t* this) {
  // TODO
}
