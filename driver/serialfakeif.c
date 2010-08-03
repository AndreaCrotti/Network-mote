#include "serialfakeif.h"
#include "motecomm.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

typedef struct {
  int out;
  int in;
} serialfake_fd_t;

// the following implementation is only suited for the pc side, but must be different on the mote side
#if INCLUDE_SERIAL_FORWARD_IMPLEMENTATION

/**
 * \returns The used file descriptor
 */
int _serialfakeif_t_fd(serialif_t* this) {
  assert(this);
  return ((serialfake_fd_t*)(this->source))->in;
}

/**
 * \param payload Contains both the pointer and length of the data provided
 *                by the ::read method. Call it instead of free!
 */
void _serialfakeif_t_ditch(serialif_t* this, payload_t* const payload) {
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
int _serialfakeif_t_send(serialif_t* this, payload_t const payload) {
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
  int result = write(((serialfake_fd_t*)(this->source))->out,buf.stream,buf.len);
  free((void*)(buf.stream));
  return result;
}

/**
 * Implementation of serialif_t::read - not to call explicitly.
 *
 * \param payload A pointer to the variable WE ARE SUPPOSED TO PUT THE PAYLOAD.
 *                When you are done with it, please call serialif_t::ditch.
 *                Note: the pointer wont be changed, but its content.
 */
void _serialfakeif_t_read(serialif_t* this, payload_t* const payload) {
  assert(this);
  static unsigned char readbuffer[TOSH_DATA_LENGTH];
  payload_t buf = {.stream = readbuffer, .len = TOSH_DATA_LENGTH};
  read(((serialfake_fd_t*)(this->source))->in,(void*)buf.stream,buf.len);
  if (!buf.stream != !buf.len || buf.len < 8) {
    buf.stream = NULL;
    buf.len = 0;
  }
  payload->len = buf.len - 8;
  payload->stream = (void*)(malloc(buf.len-8));
  if (buf.stream) {
    memcpy((void*)(payload->stream),(void*)(buf.stream+8),buf.len - 8);
  } else {
    free((void*)(payload->stream));
    payload->len = 0;
    payload->stream = NULL;
  }
}

/**
 * Custom destructor for serialif_t.
 * Will close the sf connection.
 * Do not call it explicitly. Call mysif->class->dtor(mysif) instead.
 */
void _serialfakeif_t_dtor(serialif_t* this) {
  assert(this);
  if (this->source) {
    free(this->source);
  }
}

/**
 * Implementation of serialif_t::open - do not call it explicitly.
 *
 * \param dev Must be null for serialfake
 * \param platform Must be null for serialfake
 * \param ssm Must be null for serialfake
 */
void _serialfakeif_t_open(serialif_t* this, char const* dev, char* const platform, serial_source_msg* ssm) {
  assert(!dev);
  assert(!platform);
  assert(!ssm);
  LOG_NOTE("Reading from stdin and writing to stdout.");
  serialfake_fd_t* fds = (serialfake_fd_t*)malloc(sizeof(serialfake_fd_t));
  fds->in = STDIN_FILENO;
  fds->out = STDOUT_FILENO;
  this->source = (serial_source)fds;
}

#endif 
