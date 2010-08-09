#include "serialif.h"
#include "motecomm.h"

#include <serialsource.h>
#include <stdlib.h>
#include <string.h>

// the following implementation is only suited for the pc side, but must be different on the mote side
#if INCLUDE_SERIAL_IMPLEMENTATION

#include "queue.h"
#include <unistd.h>

DEFINE_QUEUE(payload_t)

void _serialif_t_openMessage(serial_source_msg problem);

/**
 * @return The used file descriptor
 */
int _serialif_t_fd(serialif_t* this) {
  assert(this);
  return this->source->fd;
}

/**
 * @param payload Contains both the pointer and length of the data provided
 *                by the ::read method. Call it instead of free!
 */
void _serialif_t_ditch(serialif_t* this, payload_t* const payload) {
  assert(this);
  assert(payload);
  if (payload->stream) {
    free((void*)(payload->stream));
    payload->stream = NULL;
  }
  payload->len = 0;
}

// allow asynchronous checking of the result of the serial_source library
serial_source_msg* _serialif_t_openMessage_target = NULL;

/**
 * This handler is required by the serial_source library.
 *
 * @param problem Will be set to the status of the serial open instruction
 */
void _serialif_t_openMessage(serial_source_msg problem) {
  if (_serialif_t_openMessage_target) {
    *_serialif_t_openMessage_target = problem;
  }
}

/**
 * Implementation of serialif_t::send - do not call explicitly.
 *
 * @param payload What we are supposed to send. We promise not to change it.
 */
int _serialif_t_send(serialif_t* this, payload_t const payload) {
  assert(this);
  static Queue_payload_t* queue = NULL;
  if (!queue)
    queue = queue_payload_t(NULL);
  payload_t buf;
  if (payload.stream) {
    buf.len = sizeof(struct message_header_mine_t)+payload.len*sizeof(stream_t);
    buf.stream = malloc(buf.len);
    LOG_WARN("malloc: %p",buf.stream);
    memset((void*)buf.stream,0,sizeof(struct message_header_mine_t));
    struct message_header_mine_t* mh = (struct message_header_mine_t*)(buf.stream);
    mh->destaddr = 0xFFFF;
    mh->handlerid = 0;
    mh->groupid = 0;
    mh->amid = 0;
    mh->msglen = payload.len;
    memcpy((void*)(buf.stream + sizeof(struct message_header_mine_t)),payload.stream,payload.len*sizeof(stream_t));
    queue->enqueue(queue,buf);
  }
  if (this->onBufferFull && queue->size(queue) >= SERIAL_SEND_BUFFER_MAX) {
    this->onBufferFull();
  }
  if (queue->size(queue) && !this->busy) {
    buf = queue->dequeue(queue);
    if (!buf.stream)
      LOG_WARN("Buffer is not alloc'd");
    LOG_DEBUG("send dump:"); {
      for (unsigned i = 0; i < buf.len; i++) {
        printf("%02X ",buf.stream[i]);
      }
      printf("\n");
    }
    // call the serial_source library for the dirty work
    int result = write_serial_packet(this->source,buf.stream,buf.len);
//    if (!buf.stream)
    LOG_WARN("Freeing %p",buf.stream);
    free((void*)(buf.stream));
    this->busy = 1;
    if (this->onBufferEmpty && queue->size(queue) <= SERIAL_SEND_BUFFER_MIN) {
      this->onBufferEmpty();
    }
    return result;
  }
  return 0;
}

/**
 * Implementation of serialif_t::read - to not call explicitly.
 *
 * @param payload A pointer to the variable WE ARE SUPPOSED TO PUT THE PAYLOAD.
 *                When you are done with it, please call serialif_t::ditch.
 *                Note: the pointer wont be changed, but its content.
 */
void _serialif_t_read(serialif_t* this, payload_t* const payload) {
  assert(this);
  payload_t buf = {.stream = NULL, .len = 0};
  buf.stream = read_serial_packet(this->source,(int*)&(buf.len));
  if (!buf.stream != !buf.len || buf.len < 8) {
    if (buf.stream) {
      free((void*)(buf.stream));
    }
    buf.stream = NULL;
    buf.len = 0;
  }
  payload->len = buf.len - 8;
  if (buf.stream) {
    this->busy = 0;
    if (payload->len) {
      payload->stream = (void*)(malloc(buf.len-8));
      memcpy((void*)(payload->stream),(void*)(buf.stream+8),buf.len - 8);
      free((void*)buf.stream);
    } else {
      // this is an acknowledgement, so invoke the send method
      usleep(SERIAL_FORCE_ACK_SLEEP_US);
    }
    this->send(this,(payload_t){.stream = NULL, .len = 0});
  } else {
    payload->len = 0;
    payload->stream = NULL;
  }
}

/**
 * Custom destructor for serialif_t.
 * Will close the serial connection.
 * Do not call it explicitly. Call mysif->class->dtor(mysif) instead.
 */
void _serialif_t_dtor(serialif_t* this) {
  assert(this);
  if (this->source) {
    close_serial_source(this->source);
  }
}

/**
 * Implementation of serialif_t::open - do not call it explicitly.
 *
 * @param dev Used hardware device (e.g. /dev/ttyUSB0)
 * @param platform Used mote hardware (e.g. telosb)
 * @param ssm Optional pointer to a variable to hold errors thay may be produced. Set to NULL if you don't care.
 */
void _serialif_t_open(serialif_t* this, char const* dev, char* const platform, serial_source_msg* ssm) {
  serial_source_msg _ssm = 128;
  _serialif_t_openMessage_target = &_ssm;
  this->source = open_serial_source(dev,platform_baud_rate(platform),READ_NON_BLOCKING,_serialif_t_openMessage);
  _serialif_t_openMessage_target = NULL;
  this->msg = _ssm;
  if (ssm) {
    *ssm = _ssm;
  }
}

#endif 
