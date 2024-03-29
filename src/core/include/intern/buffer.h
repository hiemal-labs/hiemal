// testing header that exposes more dsp internals

#ifndef _INTERN_BUFFER_H
#define _INTERN_BUFFER_H

#include <stdbool.h>
#include "api/buffer.h"
#include "api/recording.h"
#include "intern/thread.h"
#include "intern/event.h"

typedef struct _buffer_fd_set buffer_fd_set_t;

unsigned int _rbuf_n_write_bytes(buffer_t *buf);
unsigned int _rbuf_n_read_bytes(buffer_t *buf);

int buffer_fd_set_insert(buffer_fd_set_t *fd_set, int fd);

int buffer_fd_hold(buffer_t *buf);
int buffer_fd_release(buffer_t *buf);
int buffer_fd_drain(buffer_t *buf);

int buffer_event_add(buffer_t *buf, hm_event_t *e);

struct _buffer {
  void *buf;
  unsigned int buf_len_bytes;
  void *read_ptr;
  void *write_ptr;
  buffer_type_t type;
  buffer_state_t state;
  bool ext_buf;
  recording_t *r;
  unsigned int r_id;
  buffer_fd_set_t *fd_set;
  bool fd_hold;
  hm_mutex_t m;
  hm_event_list_t *event_list;
};

#endif
