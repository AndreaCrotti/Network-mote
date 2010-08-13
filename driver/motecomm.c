#include "motecomm.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


/**
 * Sets up a default mcp connection with all rudimentary objects needed for it (including motecomm_t)
 * If you pass a NULL sif, and your achitecture allows it, the serialif_t will be created automatically
 */
mcp_t* open_mcp_connection(char const* const dev, char* const platform, serialif_t** sif) {
  serialif_t* _sif;
#if !DYNAMIC_MEMORY
  { assert(sif); }
  _sif = *sif;
  { assert(_sif); }
#else
  _sif = sif?*sif:NULL;
#endif
  _sif = serialif(_sif,dev,platform,NULL);
  if (!_sif) {
    return NULL;
  }
  {
    motecomm_t* cmm = motecomm(NULL,_sif);
    assert(_sif);
    assert(cmm);
    if (sif) {
      *sif = _sif;
    }
    return mcp(NULL,cmm);
  }
}

/**** serialif_t ****/

int _serialif_t_send(serialif_t* this, payload_t const payload);
void _serialif_t_read(serialif_t* this, payload_t* const payload);
void _serialif_t_dtor(serialif_t* this);
void _serialif_t_ditch(serialif_t* this, payload_t* const payload);
int _serialif_t_fd(serialif_t* this);
void _serialif_t_open(serialif_t* this, char const* dev, char* const platform, serial_source_msg* ssm);

// rest of these is in serialif.c or serialforwardif.c

// serialif_t constructor
serialif_t* serialif(serialif_t* this, char const* const dev, char* const platform, serial_source_msg* ssm) {
  assert(dev);
  assert(platform);
  SETDTOR(CTOR(this)) _serialif_t_dtor;
  this->send = _serialif_t_send;
  this->read = _serialif_t_read;
  this->ditch = _serialif_t_ditch;
  this->fd = _serialif_t_fd;
  this->source = 0;
  _serialif_t_open(this,dev,platform,ssm);
  if (!this->source) { // there was a problem
    DTOR(this);
    this = NULL; // tell user there was something wrong. He can then check the ssm he gave us (if he did so).
  }
  return this;
}

#ifndef _TOS_MOTECOMM
int _serialforwardif_t_send(serialif_t* this, payload_t const payload);
void _serialforwardif_t_read(serialif_t* this, payload_t* const payload);
void _serialforwardif_t_dtor(serialif_t* this);
void _serialforwardif_t_ditch(serialif_t* this, payload_t* const payload);
int _serialforwardif_t_fd(serialif_t* this);
void _serialforwardif_t_open(serialif_t* this, char const* dev, char* const platform, serial_source_msg* ssm);

// serialforwardif_t constructor
serialif_t* serialforwardif(serialif_t* this, char const* const host, char* const port) {
  assert(host);
  assert(port);
  SETDTOR(CTOR(this)) _serialforwardif_t_dtor;
  this->send = _serialforwardif_t_send;
  this->read = _serialforwardif_t_read;
  this->ditch = _serialforwardif_t_ditch;
  this->fd = _serialforwardif_t_fd;
  this->source = 0;
  _serialforwardif_t_open(this,host,port,0);
  if (!this->source) { // there was a problem
    DTOR(this);
    this = NULL; // tell user there was something wrong. He can then check the ssm he gave us (if he did so).
  }
  return this;
}


int _serialfakeif_t_send(serialif_t* this, payload_t const payload);
void _serialfakeif_t_read(serialif_t* this, payload_t* const payload);
void _serialfakeif_t_dtor(serialif_t* this);
void _serialfakeif_t_ditch(serialif_t* this, payload_t* const payload);
int _serialfakeif_t_fd(serialif_t* this);
void _serialfakeif_t_open(serialif_t* this, char const* dev, char* const platform, serial_source_msg* ssm);

// serialforwardif_t constructor
serialif_t* serialfakeif(serialif_t* this) {
  SETDTOR(CTOR(this)) _serialfakeif_t_dtor;
  this->send = _serialfakeif_t_send;
  this->read = _serialfakeif_t_read;
  this->ditch = _serialfakeif_t_ditch;
  this->fd = _serialfakeif_t_fd;
  this->source = 0;
  _serialfakeif_t_open(this,0,0,0);
  if (!this->source) {
    DTOR(this);
    this = NULL;
  }
  return this;
}
#endif

// XXX:XXX:XXX: THE FOLLOWING 
// XXX:XXX:XXX: CODE IS NOT 
// XXX:XXX:XXX: USED AT THE
// XXX:XXX:XXX: MOMENT, BUY MAY
// XXX:XXX:XXX: BE USED IN
// XXX:XXX:XXX: THE FUTURE

/**** motecomm_t ****/

// public:
/**
 * Send a packet without adding an extra header
 * 
 * @param payload The packet (actual stream and length)
 */
void _motecomm_t_send(motecomm_t* this, payload_t const payload) {
  assert(this);
  this->serialif.send(&(this->serialif),payload);
}

/**
 * Start waiting for a packet from the serial.
 * Make sure there actually is one, or you may get stuck here.
 * Note that this function will not return anything but will call your handler instead.
 */
void _motecomm_t_read(motecomm_t* this) {
  payload_t payload = {.stream = NULL, .len = 0}; 
  assert(this);
  assert(this->motecomm_handler.receive);
  this->serialif.read(&(this->serialif), &payload);
  if (payload.stream) {
      this->motecomm_handler.receive(&(this->motecomm_handler),payload);
  }
  this->serialif.ditch(&(this->serialif),&payload);
}

/**
 * Register a function pointer to a function (embedded in the struct) to be called
 * if there is a packet (you have to call this->read).
 */
void _motecomm_t_set_handler(motecomm_t* this, motecomm_handler_t const handler) {
  assert(this);
  this->motecomm_handler = handler;
}

/// constructor for the motecomm class
motecomm_t* motecomm(motecomm_t* this, serialif_t const* const interf) {
  CTOR(this);
  assert(interf && interf->send);
  this->serialif = *interf; // compile time fixed size, so we can copy directly - members are copied transparently
  this->send = _motecomm_t_send;
  this->read = _motecomm_t_read;
  this->set_handler = _motecomm_t_set_handler;
  return this;
}


/**** mcp_t ****/

// private:
/// implements motecomm_handler_t::receive
void _mcp_t_receive(motecomm_handler_t* that, payload_t const payload) {
  mcp_t* this = (mcp_t*)(that->p);
  assert(payload.stream);

  /* if(payload.len == 0) */
  /*     call Leds.led0Toggle(); */

  if (payload.len < MCP_HEADER_BYTES) {
    return;
  }
  {
    mcp_header_t h;
    payload_t const null_payload = {.stream = NULL, .len = 0};
    h.stream = payload.stream;
    if (h.header->version != MCP_VERSION) {
      if (this->mccmp)
        this->mccmp->send(this->mccmp,MCCMP_UNSUPPORTED,h.header->ident,MCP_HD_VERSION_OFFSET,null_payload);
      return;
    }
    if (h.header->header != MCP_HEADER_BYTES) {
      if (this->mccmp)
        this->mccmp->send(this->mccmp,MCCMP_PARAMETER_PROBLEM,h.header->ident,MCP_HD_HEADER_OFFSET,null_payload);
      return;
    }
    if (h.header->port != 0) {
      if (this->mccmp)
        this->mccmp->send(this->mccmp,MCCMP_UNSUPPORTED,h.header->ident,MCP_HD_PORT_OFFSET,null_payload);
      // TODO: implement additional ports
      return;
    }
    if ((unsigned)h.header->payload+h.header->header > payload.len) {
      if (this->mccmp)
        this->mccmp->send(this->mccmp,MCCMP_PARAMETER_PROBLEM,h.header->ident,MCP_HD_PAYLOAD_OFFSET,null_payload);
      return;
    }
    assert(h.header->type < MCP_TYPE_SIZE);
    {
      mcp_handler_t* hnd = &(this->handler[h.header->type]);
      if (hnd->receive) {
        hnd->receive(hnd,(payload_t){.stream = payload.stream+h.header->header, .len = h.header->payload});
      }
    }
  }
}

// public:
void _mcp_t_set_handler(mcp_t* this, mcp_type_t const type, mcp_handler_t const hnd) {
  assert((unsigned)type < MCP_TYPE_SIZE);
  this->handler[type] = hnd;
}

void _mcp_t_send(struct mcp_t* this, mcp_type_t const type, payload_t const payload) {
  stream_t const dummy_payload = 0;
  stream_t const* stream = &dummy_payload;
  assert(this);
  if (payload.stream) {
    stream = payload.stream;
  }
  {
    mcp_header_t h;
    static stream_t ns[MCP_HEADER_BYTES+MAX_PAYLOAD_SIZE(MCP)];
    //stream_t* ns = malloc(MCP_HEADER_BYTES+payload.len);
    memcpy(ns+MCP_HEADER_BYTES,stream,payload.len);
    h.stream = ns;
    h.header->version = MCP_VERSION;
    h.header->header = MCP_HEADER_BYTES;
    {
      static unsigned short seqno = 0;
      seqno = (seqno+1)&((1<<MCP_HD_IDENT)-1);
      seqno += !seqno;
      h.header->ident = seqno;
    }
    h.header->type = type;
    h.header->port = 0;
    h.header->payload = payload.len;
    (*this->comm)->send(*(this->comm),(payload_t){.stream = ns, .len = MCP_HEADER_BYTES+payload.len});
  }
  //free(ns);
}

motecomm_t* _mcp_t_get_comm(mcp_t* this) {
  return *(this->comm);
}

void _mcp_t_dtor(mcp_t* this) {
  assert(this);
  this->motecomm_handler.p = NULL;
  this->motecomm_handler.receive = NULL;
  (*(this->comm))->set_handler(*(this->comm),this->motecomm_handler);
}

mcp_t* mcp(mcp_t* this, motecomm_t* const uniq_comm) {
  SETDTOR(CTOR(this)) _mcp_t_dtor;
  {
    static motecomm_t* persistent_comm = NULL;
    if (!persistent_comm && uniq_comm)
      persistent_comm = uniq_comm;
    assert(uniq_comm == persistent_comm && "Cannot use different comm objects.");
    assert(persistent_comm && "Uninitialised motecomm_t.");
    assert(persistent_comm);
    this->comm = &persistent_comm;
    this->mccmp = NULL;
    memset((void*)(this->handler),0,sizeof(mcp_handler_t*)*MCP_TYPE_SIZE);
    this->set_handler = _mcp_t_set_handler;
    this->send = _mcp_t_send;
    this->get_comm = _mcp_t_get_comm;
    this->motecomm_handler.p = (void*)this;
    this->motecomm_handler.receive = _mcp_t_receive;
    persistent_comm->set_handler(persistent_comm,this->motecomm_handler);
  }
  return this;
}


/**** mccmp_t ****/

// private:
void _mccmp_t_receive(mcp_handler_t* that, payload_t const payload) {
  mccmp_t* this = (mccmp_t*)(that->p);
  assert(payload.stream);
  if (payload.len < MCCMP_HEADER_BYTES)
    return;
  {
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
    {
      mccmp_problem_handler_t* hnd = &(this->handler[h.header->problem]);
      if (hnd->handle) {
        hnd->handle(hnd, h.header->problem, h.header->ident, h.header->offset, (payload_t){.stream = payload.stream+h.header->header, .len = h.header->payload});
      }
    }
  }
}

void _mccmp_t_echo_request(mccmp_problem_handler_t* that, mccmp_problem_t const problem, unsigned char const ident, unsigned char const offset, payload_t const payload) {
  (void)offset;
  assert(that);
  assert(that->p);
  assert(problem == MCCMP_ECHO_REQUEST);
  {
    mccmp_t* this = (mccmp_t*)(that->p);
    this->send(this,MCCMP_ECHO_REPLY,ident,0,payload);
  }
}

void _mccmp_t_ify_request(mccmp_problem_handler_t* that, mccmp_problem_t const problem, unsigned char const ident, unsigned char const offset, payload_t const payload) {
  (void)offset;
  (void)payload;
  assert(that);
  assert(that->p);
  assert(problem == MCCMP_IFY_REQUEST);
  {
    mccmp_t* this = (mccmp_t*)(that->p);
    this->send(this,MCCMP_IFY_REPLY_CLIENT,ident,0,(payload_t){.stream = ARCHITECTURE_IDENTIFICATION, .len = ARCHITECTURE_IDENTIFICATION_SIZE});
  }
}

// public:
void _mccmp_t_send(mccmp_t* this, mccmp_problem_t const problem, unsigned char const ident, unsigned char const offset, payload_t const payload) {
  stream_t const dummy_payload = 0;
  stream_t const* stream = &dummy_payload;
  assert(this);
  if (payload.stream) {
    stream = payload.stream;
  }
  {
    mccmp_header_t h;
    static stream_t ns[MCCMP_HEADER_BYTES+MAX_PAYLOAD_SIZE(MCCMP)];
    //stream_t* ns = malloc(MCCMP_HEADER_BYTES+payload.len);
    memcpy(ns+MCCMP_HEADER_BYTES,stream,payload.len);
    h.stream = ns;
    h.header->version = MCCMP_VERSION;
    h.header->header = MCCMP_HEADER_BYTES;
    h.header->ident = ident;
    h.header->problem = (unsigned)problem;
    h.header->offset = offset;
    this->mcp->send(this->mcp,MCP_MCCMP,(payload_t){.stream = ns, .len = MCCMP_HEADER_BYTES+payload.len});
  }
  //free(ns);
}

void _mccmp_t_set_handler(mccmp_t* this, mccmp_problem_t const problem, mccmp_problem_handler_t const hnd) {
  assert((unsigned)problem < MCCMP_PROBLEM_HANDLER_SIZE);
  this->handler[problem] = hnd;
}

void _mccmp_t_dtor(mccmp_t* this) {
  assert(this);
  this->parent.p = NULL;
  this->parent.receive = NULL;
  this->mcp->set_handler(this->mcp,MCP_MCCMP,this->parent);
}

mccmp_t* mccmp(mccmp_t* this, mcp_t* const _mcp) {
  SETDTOR(CTOR(this)) _mccmp_t_dtor;
  this->mcp = _mcp;
  this->send = _mccmp_t_send;
  this->set_handler = _mccmp_t_set_handler;
  this->parent.p = (void*)this;
  this->parent.receive = _mccmp_t_receive;
  this->mcp->set_handler(this->mcp,MCP_MCCMP,this->parent);
  this->mcp->mccmp = this;
  memset((void*)(this->handler),0,sizeof(mccmp_problem_handler_t)*MCCMP_PROBLEM_HANDLER_SIZE);
  this->set_handler(this,MCCMP_ECHO_REQUEST,(mccmp_problem_handler_t const){.p = (void*)this, .handle = _mccmp_t_echo_request});
  this->set_handler(this,MCCMP_IFY_REQUEST,(mccmp_problem_handler_t const){.p = (void*)this, .handle = _mccmp_t_ify_request});
  return this;
}

/**** leap_t ****/

void _laep_t_request(laep_t* this) {
  payload_t payload;
  static stream_t stream[LAEP_HEADER_BYTES+MAX_PAYLOAD_SIZE(LAEP)];
  //payload.stream = malloc(LAEP_HEADER_BYTES*sizeof(stream_t));
  payload.stream = stream;
  payload.len = LAEP_HEADER_BYTES;
  {
    laep_header_t h;
    h.stream = payload.stream;
    h.header->version = LAEP_VERSION;
    h.header->header = LAEP_HEADER_BYTES;
    h.header->version = 0;
    h.header->type = LAEP_REQUEST;
    h.header->payload = 0;
    this->mcp->send(this->mcp,MCP_LAEP,payload);
  }
  //free((void*)(payload.stream));
}
void _laep_t_set_handler(laep_t* this, laep_msg_t const msg, laep_handler_t const hnd) {
  assert(msg < LAEP_HANDLER_SIZE);
  this->handler[msg] = hnd;
}
void _laep_t_receive(mcp_handler_t* that, payload_t const payload) {
  laep_t* this = (laep_t*)(that->p);
  assert(payload.stream);
  if (payload.len < LAEP_HEADER_BYTES)
    return;
  {
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
    {
      laep_handler_t* hnd = &(this->handler[h.header->type]);
      if (hnd->handle) {
        la_t addr;
        memset(&addr,0,sizeof(la_t));
        memcpy((&addr)+sizeof(la_t)-payload.len,payload.stream+LAEP_HEADER_BYTES,payload.len);
        hnd->handle(hnd,addr);
      }
    }
  }
}

laep_t* laep(laep_t* this, mcp_t* const _mcp) {
  CTOR(this);
  this->mcp = _mcp;
  this->request = _laep_t_request;
  this->set_handler = _laep_t_set_handler;
  this->parent.p = (void*)this;
  this->parent.receive = _laep_t_receive;
  this->mcp->set_handler(this->mcp,MCP_IFP,this->parent);
  memset((void*)(this->handler),0,sizeof(laep_handler_t)*LAEP_HANDLER_SIZE);
  return this;
}

/**** ifp_t ****/

void _ifp_t_receive(mcp_handler_t* that, payload_t const payload) {
  ifp_t* this = (ifp_t*)(that->p);
  assert(payload.stream);
  if (payload.len < IFP_HEADER_BYTES)
    return;
  {
    ifp_header_t h;
    h.stream = payload.stream;
    if (h.header->version != IFP_VERSION) {
      return;
    }
    if (h.header->header != IFP_HEADER_BYTES) {
      return;
    }
    {
      ifp_handler_t* hnd = &(this->handler);
      if (hnd->handle) {
        hnd->handle(hnd,(payload_t){.len = payload.len, .stream = payload.stream+IFP_HEADER_BYTES});
      }
    }
  }
}

void _ifp_t_send(ifp_t* this, payload_t const payload) {
  static stream_t stream[IFP_HEADER_BYTES+MAX_PAYLOAD_SIZE(IFP)];
  payload_t pl = {.len = IFP_HEADER_BYTES+payload.len, .stream = stream};
  //pl.stream = (stream_t*)malloc(pl.len*sizeof(stream_t));
  ifp_header_t h;
  h.stream = pl.stream;
  h.header->version = IFP_VERSION;
  h.header->header = IFP_HEADER_BYTES;
  h.header->payload = payload.len;
  memcpy((void*)(pl.stream+IFP_HEADER_BYTES),(void*)(payload.stream),payload.len);
  this->mcp->send(this->mcp,MCP_IFP,pl);
  //free((void*)(pl.stream));
}

void _ifp_t_set_handler(ifp_t* this, ifp_handler_t const hnd) {
  assert(this);
  this->handler = hnd;
}


ifp_t* ifp(ifp_t* this, mcp_t* const _mcp) {
  CTOR(this);
  assert(this);
  assert(_mcp);
  this->mcp = _mcp;
  this->parent.p = (void*)this;
  this->parent.receive = _ifp_t_receive;
  this->send = _ifp_t_send;
  this->set_handler = _ifp_t_set_handler;
  memset((void*)&(this->handler),0,sizeof(ifp_handler_t)*1);
  return this;
}
