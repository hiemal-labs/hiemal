/// \file buffer.c

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#ifdef WITH_PROTOBUF
#include "api/recording.h"
#endif
#include "intern/common.h"
#include "intern/buffer.h"

#include "cmake_info.h"

/*! \struct _buffer buffer.c "core/buffer.h"
*   \brief buffer struct
*   \var _buffer::buf
*   pointer to raw buffer
*   \var _buffer::buf_len_bytes
*   number of bytes allocated for buffer
*   \var _buffer::read_ptr
*   read pointer
*   \var _buffer::write_ptr
*   write pointer
*   \var _buffer::type 
*   buffer type (LINEAR or RING)
*   \var _buffer::state
*   buffer state (EMPTY, FULL or NORMAL)
*   \var _buffer::ext_buf
*   was the buffer initialized externally (i.e. not with ``buffer_init()``)?
*/

typedef struct _buffer_fd_set_node buffer_fd_set_node;

struct _buffer_fd_set_node {
  buffer_fd_set_node *next;
  int fd;
};

struct _buffer_fd_set {
  buffer_fd_set_node *head;
  int n_fds;
};

int buffer_fd_set_init(buffer_fd_set_t** fd_set) {
  buffer_fd_set_node *fd_head = 
    (buffer_fd_set_node*)malloc(sizeof(buffer_fd_set_node));
  fd_head->fd = -1;
  fd_head->next = NULL;
  *fd_set = (buffer_fd_set_t*)malloc(sizeof(buffer_fd_set_t));
  (*fd_set)->head = fd_head;
  return 0;
}

int buffer_fd_set_delete(buffer_fd_set_t **fd_set) {
  buffer_fd_set_node *fd_itr = (*fd_set)->head;
  buffer_fd_set_node *fd_next = fd_itr;
  
  while(fd_itr != NULL) {
    fd_next = fd_itr->next;
    free(fd_itr);
    fd_itr = fd_next;    
  }
  free(*fd_set);
  *fd_set = NULL;
  return 0;
}

int buffer_fd_set_insert(buffer_fd_set_t *fd_set, int fd) {
  buffer_fd_set_node *fd_itr = fd_set->head;
  buffer_fd_set_node *fd_prev = fd_itr;

  while(fd_itr != NULL) {
    if (fd == fd_itr->fd) {
      return 0;
    }
    fd_prev = fd_itr;
    fd_itr = fd_itr->next;
  }
  buffer_fd_set_node *new_fd_node = 
    (buffer_fd_set_node*)malloc(sizeof(buffer_fd_set_node));
  new_fd_node->next = NULL;
  new_fd_node->fd = fd;

  fd_prev->next = new_fd_node;
  fd_set->n_fds += 1;
  return 0;
}

int buffer_signal_fds(buffer_fd_set_t *fd_set, uint32_t n_bytes) {
  buffer_fd_set_node *fd_itr = fd_set->head->next;
  while (fd_itr != NULL) {
    write(fd_itr->fd, &n_bytes, 4);
    fd_itr = fd_itr->next;
  }
  return 0;
}

int buffer_fd_hold(buffer_t *buf) {
  buf->fd_hold = true;
  return 0;
}

int buffer_fd_release(buffer_t *buf) {
  buf->fd_hold = false;
  return 0;
}

int buffer_fd_drain(buffer_t *buf) {
  if (buf->state == EMPTY || buf->fd_hold) {
    return 0;
  }
  int bytes_available = buffer_n_read_bytes(buf);
  buffer_signal_fds(buf->fd_set, bytes_available);
  return 0;
}

int buffer_event_add(buffer_t *buf, hm_event_t *e) {
  if (e->obj_type != OBJ_BUFFER) return -1;
  hm_list_append(buf->event_list, e);
  return 0;
}

int buffer_event_remove(buffer_t *buf, hm_event_t *e) {
  return 0;
}

#define ASSERT_RBUF(buf) if (buf->type != RING) { return BAD_ARG; }
#define ASSERT_LBUF(buf) if (buf->type != LINEAR) { return BAD_ARG; }

unsigned int _lbuf_n_write_bytes(buffer_t *buf) {
  ASSERT_LBUF(buf);
  return buf->buf + buf->buf_len_bytes - buf->write_ptr;
}

unsigned int _rbuf_n_write_bytes(buffer_t *buf) {
  long long int diff;
  ASSERT_RBUF(buf);
  switch(buf->state) {
    case EMPTY: 
      return buf->buf_len_bytes;
    case FULL:
      return 0;
    default:
      diff = (buf->read_ptr - buf->write_ptr) % buf->buf_len_bytes;
      if (diff < 0) {
        diff += buf->buf_len_bytes;
      }
      return (unsigned int)diff;
  }
}

unsigned int _lbuf_n_read_bytes(buffer_t *buf) {
  ASSERT_LBUF(buf);
  return buf->write_ptr - buf->read_ptr;
}

unsigned int _rbuf_n_read_bytes(buffer_t *buf) {
  ASSERT_RBUF(buf);
  return (buf->buf_len_bytes - _rbuf_n_write_bytes(buf));
}

unsigned int _rbuf_linear_bytes_write(buffer_t *buf) {
  ASSERT_RBUF(buf);
  unsigned int write_pos = buf->write_ptr - buf->buf;
  switch(buf->state) {
    case FULL:
      return 0;
    case EMPTY:
      return buf->buf_len_bytes - write_pos;
    case NORMAL:
      if (buf->write_ptr < buf->read_ptr) {
        return buf->read_ptr - buf->write_ptr;
      }
       else {
         return buf->buf_len_bytes - write_pos;
       }
  }
}

unsigned int _rbuf_linear_bytes_read(buffer_t *buf) {
  ASSERT_RBUF(buf);
  unsigned int read_pos = buf->read_ptr - buf->buf;
  switch(buf->state) {
    case FULL:
      return buf->buf_len_bytes - read_pos;
    case EMPTY:
      return 0;
    case NORMAL:
      if (buf->read_ptr < buf->write_ptr) {
        return buf->write_ptr - buf->read_ptr;
      }
       else {
         return buf->buf_len_bytes - read_pos;
       }
  }
}

int _set_lbuf_state(buffer_t *buf) {
  ASSERT_LBUF(buf);
  if (buf->read_ptr == buf->write_ptr) {
    buffer_reset(buf);
  }
  else if (buf->write_ptr == buf->buf + buf->buf_len_bytes) {
    buf->state = FULL;
  }
  else {
    buf->state = NORMAL;
  }
  return 0;
}

int _set_rbuf_state(buffer_t *buf, char mode) {
  ASSERT_RBUF(buf);
  if (buf->read_ptr >= buf->buf + buf->buf_len_bytes) {
    buf->read_ptr -= buf->buf_len_bytes;
  }
  if (buf->write_ptr >= buf->buf + buf->buf_len_bytes) {
    buf->write_ptr -= buf->buf_len_bytes;
  }
  if (buf->read_ptr != buf->write_ptr) {
    buf->state = NORMAL;
    return 0;
  }
  switch (mode) {
    case 'r':
      buf->state = EMPTY;
      return 0;
    case 'w':
      buf->state = FULL;
      return 0;
  }
}

unsigned int buffer_n_write_bytes(buffer_t *buf) {
  switch(buf->type) {
    case LINEAR:
      return _lbuf_n_write_bytes(buf);
    case RING:
      return _rbuf_n_write_bytes(buf);
  }
}

unsigned int buffer_n_read_bytes(buffer_t *buf) {
  switch(buf->type) {
    case LINEAR:
      return _lbuf_n_read_bytes(buf);
    case RING:
      return _rbuf_n_read_bytes(buf);
  }
}

unsigned int set_buffer_state(buffer_t *buf, char mode) {
  switch(buf->type) {
    case LINEAR:
      return _set_lbuf_state(buf);
    case RING:
      return _set_rbuf_state(buf, mode);
  }
}

int buffer_reset(buffer_t *buf) {
  buf->read_ptr = buf->buf;
  buf->write_ptr = buf->buf;
  buf->state = EMPTY;
  return 0;
}

int buffer_set_rpos(buffer_t *buf, unsigned int pos) {
  buf->read_ptr = buf->buf + pos;
  return 0;
}

int buffer_set_wpos(buffer_t *buf, unsigned int pos) {
  buf->write_ptr = buf->buf + pos;
}

int buffer_set_state(buffer_t *buf, buffer_state_t state) {
  buf->state = state;
}

/*! \brief Initialize buffer handle
* \param buf address of ``buffer_t*`` handle
* \param n_bytes number of bytes to allocate
* \param type buffer type (RING or LINEAR)
* \return exit code
*/
int buffer_init(buffer_t **buf, unsigned int n_bytes, buffer_type_t type) {
  void *raw_buffer = malloc(n_bytes);
  memset(raw_buffer, 0, n_bytes);
  *buf = (buffer_t*)malloc(sizeof(buffer_t));
  (*buf)->buf = raw_buffer;
  (*buf)->buf_len_bytes = n_bytes;
  (*buf)->read_ptr = raw_buffer;
  (*buf)->write_ptr = raw_buffer;
  (*buf)->type = type;
  (*buf)->state = EMPTY;
  (*buf)->ext_buf = false;
  (*buf)->r = NULL;
  (*buf)->r_id = 0;
  buffer_fd_set_init(&((*buf)->fd_set));
  (*buf)->fd_hold = false;
  thread_mutex_init(&((*buf)->m));
  hm_event_reflist_init(&((*buf)->event_list));
  return 0;
}


/*! \brief Initialize buffer handle with an already-allocated memory area
* \param buf address of ``buffer_t*`` handle
* \param n_bytes number of bytes to allocate
* \param type buffer type (RING or LINEAR)
* \param raw_buffer buffer memory location
* \return exit code
*/
int buffer_init_ext(buffer_t **buf, unsigned int n_bytes, buffer_type_t type, void *raw_buffer) {
  *buf = (buffer_t*)malloc(sizeof(buffer_t));
  (*buf)->buf = raw_buffer;
  (*buf)->buf_len_bytes = n_bytes;
  (*buf)->read_ptr = raw_buffer;
  (*buf)->write_ptr = raw_buffer;
  (*buf)->type = type;
  (*buf)->state = EMPTY;
  (*buf)->ext_buf = true;
  (*buf)->r = NULL;
  (*buf)->r_id = 0;
  buffer_fd_set_init(&((*buf)->fd_set));
  (*buf)->fd_hold = false;
  thread_mutex_init(&((*buf)->m));
  hm_event_reflist_init(&((*buf)->event_list));
  return 0;
}

#ifdef WITH_PROTOBUF
int buffer_add_recording(buffer_t *buf, recording_t *r) {
  buf->r_id = hm_recording_n_buffers(r);
  buf->r = r;
  return 0;
}
#endif

/*! \brief Free buffer memory
* \param buf address of ``buffer_t*`` handle
* \return exit code
*/
int buffer_delete(buffer_t **buf) {
  if (!((*buf)->ext_buf)) {
    free((*buf)->buf);
  }
  buffer_fd_set_delete(&((*buf)->fd_set));
  thread_mutex_delete(&((*buf)->m));
  hm_event_list_delete(&((*buf)->event_list));
  free(*buf);
  *buf = NULL;
  return 0;
}

#define PREPARE_BUF_WRITE(buf) \
  thread_mutex_lock(&(buf->m)); \
  unsigned int n_bytes_available = buffer_n_write_bytes(buf); \
  if (n_bytes_available < n_bytes) { \
    n_bytes = n_bytes_available; \
  }

int _lbuf_write(buffer_t *dest, const void *src, unsigned int n_bytes) {
  ASSERT_LBUF(dest);
  if (n_bytes == 0) {
    return 0;
  }
  PREPARE_BUF_WRITE(dest)
  memcpy(dest->write_ptr, src, n_bytes);
  dest->write_ptr += n_bytes;
  set_buffer_state(dest, '-');
  if (dest->event_list->n_items > 0) hm_event_buffer_wake(dest->event_list, WRITE, n_bytes, buffer_n_read_bytes(dest), buffer_n_write_bytes(dest));
  thread_mutex_unlock(&(dest->m));
  return n_bytes;
}

int _rbuf_write(buffer_t *dest, const void *src, unsigned int n_bytes) {
  ASSERT_RBUF(dest);
  if (n_bytes == 0) {
    return 0;
  }
  PREPARE_BUF_WRITE(dest)
  void *linear_buf_end = dest->buf + dest->buf_len_bytes;
  unsigned int linear_bytes_available = _rbuf_linear_bytes_write(dest);
  bool split_write = (n_bytes > linear_bytes_available) ? true : false;
  if (split_write) {
    //first write to buffer
    memcpy(dest->write_ptr, src, linear_bytes_available);
    //second write to buffer
    const void *remaining_src_buf = src + linear_bytes_available;
    memcpy(dest->buf, remaining_src_buf, \
      n_bytes - linear_bytes_available);
    dest->write_ptr = dest->buf + (n_bytes - linear_bytes_available);
    set_buffer_state(dest, 'w');
  }
  else {
    memcpy(dest->write_ptr, src, n_bytes);
    dest->write_ptr += n_bytes;
    set_buffer_state(dest, 'w');
  }
  if (dest->event_list->n_items > 0) hm_event_buffer_wake(dest->event_list, WRITE, n_bytes, buffer_n_read_bytes(dest), buffer_n_write_bytes(dest));
  thread_mutex_unlock(&(dest->m));
  return n_bytes;
}

int buffer_write(buffer_t *dest, const void *src, unsigned int n_bytes) {
  uint32_t rc;
  #ifdef WITH_PROTOBUF
  if(dest->r != NULL) {
    hm_recording_write(dest->r, dest->r_id, src, n_bytes);
  }
  #endif
  switch(dest->type) {
    case LINEAR:
      rc = _lbuf_write(dest, src, n_bytes);
      if(!(dest->fd_hold)) {
        buffer_signal_fds(dest->fd_set, rc);
      }
      return rc;
    case RING:
      rc = _rbuf_write(dest, src, n_bytes);
      if(!(dest->fd_hold)) {
        buffer_signal_fds(dest->fd_set, rc);
      }
      return rc;
  }
}

#define PREPARE_BUF_READ(buf) \
  thread_mutex_lock(&(buf->m)); \
  unsigned int n_bytes_available = buffer_n_read_bytes(buf); \
  if (n_bytes_available < n_bytes) { \
    n_bytes = n_bytes_available; \
  }

int _lbuf_read(buffer_t *src, void *dest, unsigned int n_bytes) {
  ASSERT_LBUF(src);
  if (n_bytes == 0) {
    return 0;
  }
  PREPARE_BUF_READ(src)
  if (dest != NULL) {
    memcpy(dest, src->read_ptr, n_bytes);
  }
  src->read_ptr += n_bytes;
  set_buffer_state(src, '-');
  if (src->event_list->n_items > 0) hm_event_buffer_wake(src->event_list, WRITE, n_bytes, buffer_n_read_bytes(src), buffer_n_write_bytes(src));
  thread_mutex_unlock(&(src->m));
  return n_bytes;
}

int _rbuf_read(buffer_t *src, void *dest, unsigned int n_bytes) {
  //ASSERT_RBUF(src);
  if (src->type != RING) { 
    return BAD_ARG; 
  }
  if (n_bytes == 0) {
    return 0;
  }
  PREPARE_BUF_READ(src)
  void *linear_buf_end = src->buf + src->buf_len_bytes;
  unsigned int linear_bytes_available = _rbuf_linear_bytes_read(src);
  bool split_read = (n_bytes > linear_bytes_available) ? true : false;
  if (split_read) {
    //first read from buffer
    if (dest != NULL) {
      memcpy(dest, src->read_ptr, linear_bytes_available);
    }
    //second read from buffer
    void *remaining_dest_buf = dest + linear_bytes_available;
    if (dest != NULL) {
      memcpy(remaining_dest_buf, src->buf, \
        n_bytes - linear_bytes_available);
    }
    src->read_ptr = src->buf + (n_bytes - linear_bytes_available);
    set_buffer_state(src, 'r');
  }
  else {
    if (dest != NULL) {
      memcpy(dest, src->read_ptr, n_bytes);
    }
    src->read_ptr += n_bytes;
    set_buffer_state(src, 'r');
  }
  if (src->event_list->n_items > 0) hm_event_buffer_wake(src->event_list, WRITE, n_bytes, buffer_n_read_bytes(src), buffer_n_write_bytes(src));
  thread_mutex_unlock(&(src->m));
  return n_bytes;
}

int buffer_read(buffer_t *src, void *dest, unsigned int n_bytes) {
  switch(src->type) {
    case LINEAR:
      return _lbuf_read(src, dest, n_bytes);
    case RING:
      return _rbuf_read(src, dest, n_bytes);
  }
}

int buffer_view_raw(buffer_t *src, void *dest, unsigned int offset, unsigned int n_bytes) {
  void *start_ptr = src->buf + offset;
  memcpy(dest, start_ptr, n_bytes);
  return 0;
}

int buffer_view(buffer_t *src, void *dest, unsigned int offset, unsigned int n_bytes) {
  PREPARE_BUF_READ(src)
  if (src->type == LINEAR) {
    return buffer_view_raw(src, dest, offset, n_bytes);
  }
  else {
    unsigned int linear_bytes_available = _rbuf_linear_bytes_read(src) - offset;
    bool split_read = (n_bytes > linear_bytes_available) ? true : false;
    if (split_read) {
      memcpy(dest, src->read_ptr + offset, linear_bytes_available);
      void *remaining_dest_buf = dest + linear_bytes_available;
      memcpy(remaining_dest_buf, src->buf, n_bytes - linear_bytes_available);
    }
    else {
      memcpy(dest, src->read_ptr + offset, n_bytes);
    }
    thread_mutex_unlock(&(src->m));
    return n_bytes;
  }
}