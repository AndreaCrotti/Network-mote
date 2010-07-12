#include "motecomm.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>


#include <stdio.h>

class_t* __class_t__ctor(void** obj, unsigned typesz) {
  assert(obj);
  class_t* result = NULL;;
  char ctorAllocd = !*obj;
  if (ctorAllocd)
    *obj = malloc(typesz);
  result = (class_t*)*obj;
  // in a class the class_t struct is always at the first address
  result->ctorAllocd = ctorAllocd;
  result->dtor = NULL;
  return result;
}
void __class_t__dtor(void** obj) {
  assert(obj);
  class_t* c = (class_t*)*obj;
  if (c->dtor) {
    c->dtor(*obj);
  }
  if (c->ctorAllocd) {
    free(*obj);
    *obj = NULL;
  }
}

payload_t* gluePayloadMalloc(payload_t const* const first, payload_t const* const second) {
  if (second && second->len && (!first || !first->len))
    return gluePayloadMalloc(second,NULL);
  payload_t* tar = (payload_t*)malloc(sizeof(payload_t));
  stream_t f, s;
  payload_t fp = {.len = 0, .stream = &f};
  payload_t sp = {.len = 0, .stream = &s};
  if (first) {
    fp = *first;
  }
  if (second) {
    sp = *second;
  }
  tar->len = fp.len + sp.len;
  if (tar->len) {
    tar->stream = malloc(tar->len*sizeof(stream_t));
    memcpy((void*)(tar->stream),(void*)fp.stream,fp.len);
    memcpy((void*)(tar->stream)+fp.len,(void*)sp.stream,sp.len);
  }
  return tar;
}

void freePayload(payload_t** payload) {
  assert(payload);
  if (*payload) {
    if ((*payload)->stream) {
      free((void*)((*payload)->stream));
      (*payload)->stream = NULL;
    }
    free((void*)*payload);
    *payload = NULL;
  }
}

mcp_t* openMcpConnection(char const* const dev, char* const platform, serialif_t** sif) {
  serialif_t* _sif = sif?*sif:NULL;
  _sif = serialif(_sif,dev,platform,NULL);
  if (!_sif) {
    return NULL;
  }
  motecomm_t* cmm = motecomm(NULL,_sif);
  assert(_sif);
  assert(cmm);
  if (sif) {
    *sif = _sif;
  }
  return mcp(NULL,cmm);
}

/**** serialif_t ****/

serial_source_msg* __serialif_t__openMessage_target = NULL;

void __serialif_t__openMessage(serial_source_msg problem) {
  if (__serialif_t__openMessage_target) {
    *__serialif_t__openMessage_target = problem;
  }
}

int __serialif_t__send(serialif_t* this, payload_t const payload) {
  assert(this);
  return write_serial_packet(this->source,payload.stream,payload.len);
}

void __serialif_t__read(serialif_t* this, payload_t** const payload) {
  assert(this);
  assert(payload);
  payload_t buf;
  buf.stream = read_serial_packet(this->source,(int*)&(buf.len));
  if (!buf.stream != !buf.len) {
    if (buf.stream) {
      free((void*)(buf.stream));
    }
    buf.stream = NULL;
    buf.len = 0;
  }
  if (*payload) {
    (*payload)->len = buf.len;
    if (buf.stream) {
      memcpy((void*)((*payload)->stream),(void*)(buf.stream),buf.len);
    } else {
      (*payload)->stream = NULL;
    }
  } else {
    *payload = gluePayloadMalloc(&buf,NULL);
  }
}

void __serialif_t__dtor(serialif_t* this) {
  assert(this);
  if (this->source) {
    close_serial_source(this->source);
  }
}

serialif_t* serialif(serialif_t* this, char const* const dev, char* const platform, serial_source_msg* ssm) {
  assert(dev);
  assert(platform);
  SETDTOR(CTOR(this)) __serialif_t__dtor;
  this->send = __serialif_t__send;
  this->read = __serialif_t__read;
  assert(this && this->send);
  serial_source_msg _ssm = 128;
  __serialif_t__openMessage_target = &_ssm;
  this->source = open_serial_source(dev,platform_baud_rate(platform),READ_NON_BLOCKING,__serialif_t__openMessage);
  __serialif_t__openMessage_target = NULL;
  this->msg = _ssm;
  if (ssm) {
    *ssm = _ssm;
  }
  if (!this->source) { // there was a problem
    DTOR(this);
    this = NULL; // tell user there was something wrong. He can then theck the ssm he gave us (if he did so).
  }
  return this;
}

/**** motecomm_t ****/

// public:
void __motecomm_t__send(motecomm_t* this, payload_t const payload) {
  assert(this);
  this->serialif.send(&(this->serialif),payload);
}

void __motecomm_t__read(motecomm_t* this) {
  assert(this);
  assert(this->motecomm_handler.receive);
  payload_t* payload = NULL; //will be created
  this->serialif.read(&(this->serialif),&payload);
  if (payload) {
    if (payload->stream) {
      payload_t p = *payload;
      if (p.len >= 8) {
        p.len -= 8;
        p.stream = malloc(p.len*sizeof(stream_t));
        memcpy((void*)(p.stream),((void*)(payload->stream))+8,p.len);
        this->motecomm_handler.receive(&(this->motecomm_handler),p);
        free((void*)(p.stream));
      }
    }
    freePayload(&payload);
  }
}

void __motecomm_t__setHandler(motecomm_t* this, motecomm_handler_t const handler) {
  assert(this);
  this->motecomm_handler = handler;
}

motecomm_t* motecomm(motecomm_t* this, serialif_t const* const interface) {
  CTOR(this);
  assert(interface && interface->send);
  this->serialif = *interface; // compile time fixed size, so we can copy directly - members are copied transparently
  this->send = __motecomm_t__send;
  this->read = __motecomm_t__read;
  this->setHandler = __motecomm_t__setHandler;
  return this;
}


/**** mcp_t ****/

// private:

typedef union {
  stream_t const* stream;
  struct {
#if NX_SWAP_NIBBLES
    unsigned header  :MCP_HD_HEADER;
    unsigned version :MCP_HD_VERSION;
#else
    unsigned version :MCP_HD_VERSION;
    unsigned header  :MCP_HD_HEADER;
#endif
    unsigned ident   :MCP_HD_IDENT;
    unsigned type    :MCP_HD_TYPE;
    unsigned port    :MCP_HD_PORT;
    unsigned payload :MCP_HD_PAYLOAD;
  }__attribute__((__packed__))* header;
} mcp_header_t;

/// implements motecomm_handler_t::receive
void __mcp_t__receive(motecomm_handler_t* that, payload_t const payload) {
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

motecomm_t* __mcp_t__getComm(mcp_t* this) {
  return *(this->comm);
}

void __mcp_t__dtor(mcp_t* this) {
  assert(this);
  this->motecomm_handler.p = NULL;
  this->motecomm_handler.receive = NULL;
  (*(this->comm))->setHandler(*(this->comm),this->motecomm_handler);
}

mcp_t* mcp(mcp_t* this, motecomm_t* const uniqComm) {
  SETDTOR(CTOR(this)) __mcp_t__dtor;
  static motecomm_t* persistentComm = NULL;
  if (!persistentComm && uniqComm)
    persistentComm = uniqComm;
  assert(uniqComm == persistentComm && "Cannot use different comm objects.");
  assert(persistentComm && "Uninitialised motecomm_t.");
  assert(persistentComm);
  this->comm = &persistentComm;
  this->mccmp = NULL;
  memset((void*)(this->handler),0,sizeof(mcp_handler_t*)*MCP_TYPE_SIZE);
  this->setHandler = __mcp_t__setHandler;
  this->send = __mcp_t__send;
  this->getComm = __mcp_t__getComm;
  this->motecomm_handler.p = (void*)this;
  this->motecomm_handler.receive = __mcp_t__receive;
  persistentComm->setHandler(persistentComm,this->motecomm_handler);
  return this;
}


/**** mccmp_t ****/

// private:
typedef union {
  stream_t const* stream;
  struct {
#if NX_SWAP_NIBBLES
    unsigned header  :MCCMP_HD_HEADER;
    unsigned version :MCCMP_HD_VERSION;
#else
    unsigned version :MCCMP_HD_VERSION;
    unsigned header  :MCCMP_HD_HEADER;
#endif
    unsigned ident   :MCCMP_HD_IDENT;
    unsigned problem :MCCMP_HD_PROBLEM;
    unsigned offset  :MCCMP_HD_OFFSET;
    unsigned payload :MCCMP_HD_PAYLOAD;
  } __attribute__((__packed__))* header;
} mccmp_header_t;

void __mccmp_t__receive(mcp_handler_t* that, payload_t const payload) {
  mccmp_t* this = (mccmp_t*)(that->p);
  assert(payload.stream);
  if (payload.len < MCCMP_HEADER_BYTES)
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
  if (h.header->problem >= MCCMP_PROBLEM_HANDLER_SIZE) {
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
  stream_t* ns = malloc(MCCMP_HEADER_BYTES+payload.len);
  memcpy(ns+MCCMP_HEADER_BYTES,stream,payload.len);
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

void __mccmp_t__dtor(mccmp_t* this) {
  assert(this);
  this->parent.p = NULL;
  this->parent.receive = NULL;
  this->mcp->setHandler(this->mcp,MCP_MCCMP,this->parent);
}

mccmp_t* mccmp(mccmp_t* this, mcp_t* const mcp) {
  SETDTOR(CTOR(this)) __mccmp_t__dtor;
  this->mcp = mcp;
  this->send = __mccmp_t__send;
  this->setHandler = __mccmp_t__setHandler;
  this->parent.p = (void*)this;
  this->parent.receive = __mccmp_t__receive;
  this->mcp->setHandler(this->mcp,MCP_MCCMP,this->parent);
  this->mcp->mccmp = this;
  memset((void*)(this->handler),0,sizeof(mccmp_problem_handler_t)*MCCMP_PROBLEM_HANDLER_SIZE);
  this->setHandler(this,MCCMP_ECHO_REQUEST,(mccmp_problem_handler_t const){.p = (void*)this, .handle = __mccmp_t__echoRequest});
  this->setHandler(this,MCCMP_IFY_REQUEST,(mccmp_problem_handler_t const){.p = (void*)this, .handle = __mccmp_t__ifyRequest});
  return this;
}

/**** leap_t ****/

typedef union {
  stream_t const* stream;
  struct {
#if NX_SWAP_NIBBLES
    unsigned header  :LAEP_HD_HEADER;
    unsigned version :LAEP_HD_VERSION;
    unsigned type    :LAEP_HD_TYPE;
    unsigned ipv     :LAEP_HD_IPV;
#else
    unsigned version :LAEP_HD_VERSION;
    unsigned header  :LAEP_HD_HEADER;
    unsigned ipv     :LAEP_HD_IPV;
    unsigned type    :LAEP_HD_TYPE;
#endif
    unsigned payload :LAEP_HD_PAYLOAD;
  } __attribute__((__packed__))* header;
} laep_header_t;

void __laep_t__request(laep_t* this) {
  payload_t payload;
  payload.stream = malloc(LAEP_HEADER_BYTES*sizeof(stream_t));
  payload.len = 0;
  laep_header_t h;
  h.stream = payload.stream;
  h.header->version = LAEP_VERSION;
  h.header->header = LAEP_HEADER_BYTES;
  h.header->version = 0;
  h.header->type = LAEP_REQUEST;
  h.header->payload = 0;
  this->mcp->send(this->mcp,MCP_LAEP,payload);
  free((void*)(payload.stream));
}
void __laep_t__setHandler(laep_t* this, laep_msg_t const msg, laep_handler_t const hnd) {
  assert(msg < LAEP_HANDLER_SIZE);
  this->handler[msg] = hnd;
}
void __laep_t__receive(mcp_handler_t* that, payload_t const payload) {
  laep_t* this = (laep_t*)(that->p);
  assert(payload.stream);
  if (payload.len < LAEP_HEADER_BYTES)
    return;
  laep_header_t h;
  h.stream = payload.stream;
  if (h.header->version != LAEP_VERSION) {
    return;
  }
  if (h.header->header != LAEP_HEADER_BYTES) {
    return;
  }
  if (h.header->type >= LAEP_HANDLER_SIZE) {
    return;
  }
  if (h.header->ipv != 4 && h.header->ipv != 6) {
    return;
  }
  if (h.header->payload != 1 << (h.header->ipv / 2)) {
    return;
  }
  if ((unsigned)(h.header->payload+LAEP_HEADER_BYTES) > payload.len) {
    return;
  }
  laep_handler_t* hnd = &(this->handler[h.header->type]);
  if (hnd->handle) {
    la_t addr = 0;
    unsigned i;
    for (i = 0; i < h.header->payload; ++i) {
      addr <<= 8;
      addr |= payload.stream[i+LAEP_HEADER_BYTES];
    }
    hnd->handle(hnd,addr);
  }
}

laep_t* laep(laep_t* this, mcp_t* const mcp) {
  CTOR(this);
  this->mcp = mcp;
  this->request = __laep_t__request;
  this->setHandler = __laep_t__setHandler;
  this->parent.p = (void*)this;
  this->parent.receive = __laep_t__receive;
  this->mcp->setHandler(this->mcp,MCP_IFP,this->parent);
  memset((void*)(this->handler),0,sizeof(laep_handler_t)*LAEP_HANDLER_SIZE);
  return this;
}

/**** ifp_t ****/

typedef union {
  stream_t const* stream;
  struct {
#if NX_SWAP_NIBBLES
    unsigned header  :IFP_HD_HEADER;
    unsigned version :IFP_HD_VERSION;
#else
    unsigned version :IFP_HD_VERSION;
    unsigned header  :IFP_HD_HEADER;
#endif
    unsigned payload :IFP_HD_PAYLOAD;
  } __attribute__((__packed__))* header;
} ifp_header_t;

void __ifp_t__receive(mcp_handler_t* that, payload_t const payload) {
  ifp_t* this = (ifp_t*)(that->p);
  assert(payload.stream);
  if (payload.len < IFP_HEADER_BYTES)
    return;
  ifp_header_t h;
  h.stream = payload.stream;
  if (h.header->version != IFP_VERSION) {
    return;
  }
  if (h.header->header != IFP_HEADER_BYTES) {
    return;
  }
  ifp_handler_t* hnd = &(this->handler);
  if (hnd->handle) {
    hnd->handle(hnd,(payload_t){.len = payload.len, .stream = payload.stream+IFP_HEADER_BYTES});
  }
}

void __ifp_t__send(ifp_t* this, payload_t const payload) {
  payload_t pl = {.len = IFP_HEADER_BYTES+payload.len, .stream = NULL};
  pl.stream = (stream_t*)malloc(pl.len*sizeof(stream_t));
  ifp_header_t h;
  h.stream = pl.stream;
  h.header->version = IFP_VERSION;
  h.header->header = IFP_HEADER_BYTES;
  h.header->payload = payload.len;
  memcpy((void*)(pl.stream+IFP_HEADER_BYTES),(void*)(payload.stream),payload.len);
  this->mcp->send(this->mcp,MCP_IFP,pl);
  free((void*)(pl.stream));
}

void __ifp_t__setHandler(ifp_t* this, ifp_handler_t const hnd) {
  assert(this);
  this->handler = hnd;
}


ifp_t* ifp(ifp_t* this, mcp_t* const mcp) {
  CTOR(this);
  assert(this);
  assert(mcp);
  this->mcp = mcp;
  this->parent.p = (void*)this;
  this->parent.receive = __ifp_t__receive;
  this->send = __ifp_t__send;
  this->setHandler = __ifp_t__setHandler;
  memset((void*)&(this->handler),0,sizeof(ifp_handler_t)*1);
  return this;
}





#if STANDALONE

int main(int argc, char** args) {
  assert(argc >= 3);
  mcp_t* mcp = openMcpConnection(args[1],args[2],NULL);
  if (!mcp) {
    printf("There was an error opening the connection to %s over device %s.",args[2],args[1]);
    return 1; 
  }
  assert(mcp);
  assert(mcp->getComm);
  assert(mcp->getComm(mcp));
  assert(mcp->getComm(mcp)->read);
  mccmp(NULL,mcp);
  while (1) {
    mcp->getComm(mcp)->read(mcp->getComm(mcp));
    printf("there was a message\n");
  }
  return 0;
}

#endif
