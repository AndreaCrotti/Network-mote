#include "motecomm.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

/**** motecomm_t ****/

// public:
void __motecomm_t__send(motecomm_t* this, payload_t const payload) {
  (void)this;
  (void)payload;
  // TODO TODO TODO send the stream using the serialif
}

void __motecomm_t__listen(motecomm_t* this) {
  assert(this->motecomm_handler.receive);
  stream_t* stream = NULL;
  int len = 0;
  // TODO TODO TODO start reading, put it to stream, length to len
  this->motecomm_handler.receive(&(this->motecomm_handler),(payload_t){.stream = stream, .len = len});
}

void __motecomm_t__setHandler(motecomm_t* this, motecomm_handler_t const* const handler) {
  assert(this);
  assert(handler);
  this->motecomm_handler = *handler;
}

void motecomm(motecomm_t* this, serialif_t const* const interface) {
  assert(this);
  assert(interface && interface->send);
  this->serialif = *interface; // compile time fixed size, so we can copy directly - members are copied transparently
  this->send = __motecomm_t__send;
  this->listen = __motecomm_t__listen;
  this->setHandler = __motecomm_t__setHandler;
}



/**** mcp_t ****/

// private:

typedef union {
  stream_t const* stream;
  struct {
    unsigned version :MCP_HD_VERSION;
    unsigned header  :MCP_HD_HEADER;
    unsigned ident   :MCP_HD_IDENT;
    unsigned type    :MCP_HD_TYPE;
    unsigned port    :MCP_HD_PORT;
    unsigned payload :MCP_HD_PAYLOAD;
  }__attribute__((__packed__))* header;
} mcp_header_t;

void __mcp_t__receive(motecomm_handler_t* that, payload_t const payload) {// implements motecomm_handler_t::receive
  mcp_t* this = (mcp_t*)(that->p);
  assert(payload.stream);
  if (payload.len < MCP_HEADER_BYTES) {
    return;
  }
  mcp_header_t h;
  payload_t const nullPayload = {.stream = NULL, .len = 0};
  h.stream = payload.stream;
  if (h.header->version != MCP_VERSION) {
    if (this->mccmp)
      this->mccmp->send(this->mccmp,MCCMP_UNSUPPORTED,h.header->ident,MCP_HD_VERSION_OFFSET,nullPayload);
    return;
  }
  if (h.header->header != MCP_HEADER_BYTES) {
    if (this->mccmp)
      this->mccmp->send(this->mccmp,MCCMP_PARAMETER_PROBLEM,h.header->ident,MCP_HD_HEADER_OFFSET,nullPayload);
    return;
  }
  if (h.header->port != 0) {
    if (this->mccmp)
      this->mccmp->send(this->mccmp,MCCMP_UNSUPPORTED,h.header->ident,MCP_HD_PORT_OFFSET,nullPayload);
    // TODO: implement additional ports
    return;
  }
  if ((unsigned)h.header->payload+h.header->header > payload.len) {
    if (this->mccmp)
      this->mccmp->send(this->mccmp,MCCMP_PARAMETER_PROBLEM,h.header->ident,MCP_HD_PAYLOAD_OFFSET,nullPayload);
    return;
  }
  assert(h.header->type < MCP_TYPE_SIZE);
  mcp_handler_t* hnd = &(this->handler[h.header->type]);
  if (hnd->receive) {
    hnd->receive(hnd,(payload_t){.stream = payload.stream+h.header->header, .len = h.header->payload});
  }
}

// public:
void __mcp_t__setHandler(mcp_t* this, mcp_type_t const type, mcp_handler_t const hnd) {
  assert((unsigned)type < MCP_TYPE_SIZE);
  this->handler[type] = hnd;
}

void __mcp_t__send(struct mcp_t* this, mcp_type_t const type, payload_t const payload) {
  assert(this);
  stream_t const dummyPayload = 0;
  stream_t const* stream = &dummyPayload;
  if (payload.stream)
    stream = payload.stream;
  stream_t* ns = malloc(MCP_HEADER_BYTES+payload.len);
  memcpy(ns+MCP_HEADER_BYTES,stream,payload.len);
  mcp_header_t h;
  h.stream = ns;
  h.header->version = MCP_VERSION;
  h.header->header = MCP_HEADER_BYTES;
  static unsigned short seqno = 0;
  seqno = (seqno+1)&((1<<MCP_HD_IDENT)-1);
  seqno += !seqno;
  h.header->ident = seqno;
  h.header->type = type;
  h.header->port = 0;
  h.header->payload = payload.len;
  (*this->comm)->send(*(this->comm),(payload_t){.stream = ns, .len = MCP_HEADER_BYTES+payload.len});
  free(ns);
}

void mcp(mcp_t* this, motecomm_t* const uniqComm) {
  assert(this);
  assert(uniqComm);
  static motecomm_t* persistentComm = NULL;
  if (!persistentComm && !uniqComm)
    persistentComm = uniqComm;
  assert(uniqComm == persistentComm && "Cannot use different comm objects.");
  assert(persistentComm && "Uninitialised motecomm_t.");
  assert(persistentComm);
  this->comm = &persistentComm;
  this->mccmp = NULL;
  memset((void*)(this->handler),0,sizeof(mcp_handler_t*)*MCP_TYPE_SIZE);
  this->setHandler = __mcp_t__setHandler;
  this->send = __mcp_t__send;
  this->motecomm_handler.p = (void*)this;
  this->motecomm_handler.receive = __mcp_t__receive;
  persistentComm->setHandler(persistentComm,&(this->motecomm_handler));
}


/**** mccmp_t ****/

// private:
typedef union {
  stream_t const* stream;
  struct {
    unsigned version :MCCMP_HD_VERSION;
    unsigned header  :MCCMP_HD_HEADER;
    unsigned ident   :MCCMP_HD_IDENT;
    unsigned problem :MCCMP_HD_PROBLEM;
    unsigned offset  :MCCMP_HD_OFFSET;
    unsigned payload :MCCMP_HD_PAYLOAD;
  } __attribute__((__packed__))* header;
} mccmp_header_t;

void __mccmp_t__receive(mcp_handler_t* that, payload_t const payload) {
  mccmp_t* this = (mccmp_t*)(that->p);
  assert(payload.stream);
  if (payload.len < MCP_HEADER_BYTES)
    return;
  mccmp_header_t h;
  h.stream = payload.stream;
  if (h.header->version != MCCMP_VERSION) {
    // drop the packet.
    // we cannot respond with an mccmp message, because that could lead to an infinite loop.
    return;
  }
  if (h.header->header != MCCMP_HEADER_BYTES) {
    return;
  }
  if (h.header->problem < MCCMP_PROBLEM_HANDLER_SIZE) {
    return;
  }
  mccmp_problem_handler_t* hnd = &(this->handler[h.header->problem]);
  if (hnd->handle) {
    hnd->handle(hnd, h.header->problem, h.header->ident, h.header->offset, (payload_t){.stream = payload.stream+h.header->header, .len = h.header->payload});
  }
}

void __mccmp_t__echoRequest(mccmp_problem_handler_t* that, mccmp_problem_t const problem, unsigned char const ident, unsigned char const offset, payload_t const payload) {
  (void)offset;
  assert(that);
  assert(that->p);
  assert(problem == MCCMP_ECHO_REQUEST);
  mccmp_t* this = (mccmp_t*)(that->p);
  this->send(this,MCCMP_ECHO_REPLY,ident,0,payload);
}

void __mccmp_t__ifyRequest(mccmp_problem_handler_t* that, mccmp_problem_t const problem, unsigned char const ident, unsigned char const offset, payload_t const payload) {
  (void)offset;
  (void)payload;
  assert(that);
  assert(that->p);
  assert(problem == MCCMP_IFY_REQUEST);
  mccmp_t* this = (mccmp_t*)(that->p);
  this->send(this,MCCMP_IFY_REPLY_CLIENT,ident,0,(payload_t){.stream = ARCHITECTURE_IDENTIFICATION, .len = ARCHITECTURE_IDENTIFICATION_SIZE});
}

// public:
void __mccmp_t__send(mccmp_t* this, mccmp_problem_t const problem, unsigned char const ident, unsigned char const offset, payload_t const payload) {
  assert(this);
  stream_t const dummyPayload = 0;
  stream_t const* stream = &dummyPayload;
  if (payload.stream)
    stream = payload.stream;
  stream_t* ns = malloc(MCP_HEADER_BYTES+payload.len);
  memcpy(ns+MCP_HEADER_BYTES,stream,payload.len);
  mccmp_header_t h;
  h.stream = ns;
  h.header->version = MCCMP_VERSION;
  h.header->header = MCCMP_HEADER_BYTES;
  h.header->ident = ident;
  h.header->problem = (unsigned)problem;
  h.header->offset = offset;
  this->mcp->send(this->mcp,MCP_MCCMP,(payload_t){.stream = ns, .len = MCCMP_HEADER_BYTES});
  free(ns);
}

void __mccmp_t__setHandler(mccmp_t* this, mccmp_problem_t const problem, mccmp_problem_handler_t const hnd) {
  assert((unsigned)problem < MCCMP_PROBLEM_HANDLER_SIZE);
  this->handler[problem] = hnd;
}

void mccmp(mccmp_t* this, mcp_t* const mcp) {
  assert(this);
  this->mcp = mcp;
  this->send = __mccmp_t__send;
  this->setHandler = __mccmp_t__setHandler;
  this->parent.p = (void*)this;
  this->parent.receive = __mccmp_t__receive;
  memset((void*)(this->handler),0,sizeof(mccmp_problem_handler_t*)*MCCMP_PROBLEM_HANDLER_SIZE);
  this->setHandler(this,MCCMP_ECHO_REQUEST,(mccmp_problem_handler_t const){.p = (void*)this, .handle = __mccmp_t__echoRequest});
  this->setHandler(this,MCCMP_IFY_REQUEST,(mccmp_problem_handler_t const){.p = (void*)this, .handle = __mccmp_t__ifyRequest});
}

/**** leap_t ****/

void leap(leap_t* this, mcp_t* const mcp) {
  assert(this);
  assert(mcp);
  // TODO
}

/**** ifp_t ****/

void ifp(ifp_t* this, mcp_t* const mcp) {
  assert(this);
  assert(mcp);
  // TODO
}
