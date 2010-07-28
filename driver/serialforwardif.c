#include "serialforwardif.h"
#include "motecomm.h"
#include "stdlib.h"

#include <sfsource.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "util.h"


// the following implementation is only suited for the pc side, but must be different on the mote side
#if INCLUDE_SERIAL_FORWARD_IMPLEMENTATION

/**
 * \returns The used file descriptor
 */
int _serialforwardif_t_fd(serialif_t* this) {
  assert(this);
  return this->source->fd;
}

/**
 * \param payload Contains both the pointer and length of the data provided
 *                by the ::read method. Call it instead of free!
 */
void _serialforwardif_t_ditch(serialif_t* this, payload_t* const payload) {
  assert(this);
  assert(payload);
  if (payload->stream) {
    free((void*)(payload->stream));
    payload->stream = NULL;
  }
  payload->len = 0;
}

/**
 * Implementation of serialif_t::send - do not call explicitly.
 *
 * \param payload What we are supposed to send. We promise not to change it.
 */
int _serialforwardif_t_send(serialif_t* this, payload_t const payload) {
  assert(this);
  payload_t buf;
  buf.len = sizeof(struct message_header_mine_t)+payload.len*sizeof(stream_t);
  buf.stream = malloc(buf.len);
  memset((void*)buf.stream,0,sizeof(struct message_header_mine_t));
  struct message_header_mine_t* mh = (struct message_header_mine_t*)(buf.stream);
  mh->destaddr = 0xFFFF;
  mh->handlerid = 0;
  mh->groupid = 0;
  mh->amid = 0;
  mh->msglen = payload.len;
  memcpy((void*)(buf.stream + sizeof(struct message_header_mine_t)),payload.stream,payload.len*sizeof(stream_t));
  // call the sf library for the dirty work
  int result = write_sf_packet(this->source->fd,buf.stream,buf.len);
  free((void*)(buf.stream));
  return result;
}

/**
 * Implementation of serialif_t::read - to not call explicitly.
 *
 * \param payload A pointer to the variable WE ARE SUPPOSED TO PUT THE PAYLOAD.
 *                When you are done with it, please call serialif_t::ditch.
 *                Note: the pointer wont be changed, but its content.
 */
void _serialforwardif_t_read(serialif_t* this, payload_t* const payload) {
  assert(this);
  payload_t buf = {.stream = NULL, .len = 0};
  buf.stream = read_sf_packet(this->source->fd,(int*)&(buf.len));
  if (!buf.stream != !buf.len || buf.len < 8) {
    if (buf.stream) {
      free((void*)(buf.stream));
    }
    buf.stream = NULL;
    buf.len = 0;
  }
  payload->len = buf.len - 8;
  payload->stream = (void*)(malloc(buf.len-8));
  if (buf.stream) {
    memcpy((void*)(payload->stream),(void*)(buf.stream+8),buf.len - 8);
    free((void*)buf.stream);
  } else {
    payload->stream = NULL;
  }
}

/**
 * Custom destructor for serialif_t.
 * Will close the sf connection.
 * Do not call it explicitly. Call mysif->class->dtor(mysif) instead.
 */
void _serialforwardif_t_dtor(serialif_t* this) {
  assert(this);
  if (this->source) {
    close(this->source->fd);
    free(this->source);
  }
}

/**
 * Implementation of serialif_t::open - do not call it explicitly.
 *
 * \param dev Used hardware device (e.g. /dev/ttyUSB0)
 * \param platform Used mote hardware (e.g. telosb)
 * \param ssm Pointer to __int__! Will be set to 0 if the connection could not be opened. Pass NULL if you do not care.
 */
void _serialforwardif_t_open(serialif_t* this, char const* dev, char* const platform, serial_source_msg* ssm) {
  int port = atoi(platform);
  assert((!this->source) && "source already created or uninitialised!");
  char const* const host = dev;
  LOG_NOTE("Using host: '%s' at port: '%d'",host,port);
  int fd = open_sf_source(host,port);
  if (ssm) {
    *(int*)ssm = fd >= 0;
  }
  if (fd >= 0) {
    this->source = malloc(sizeof(struct serial_source_t)); // hack
    this->source->fd = fd;
  } else {
    LOG_ERROR("sf could not be opened");
    exit(1);
  }
}

#endif 
