/// \file buffer.h

#ifndef _BUFFER_H
#define _BUFFER_H

#define BUF_WRITE_BLOCKING    0x0
#define BUF_WRITE_NONBLOCKING 0x1

typedef struct _buffer buffer_t;
typedef struct _recording recording_t;
typedef struct _buf_array {
  unsigned int n_buffer;
  buffer_t **buffers;
} buf_array_t;
typedef enum _rbuf_io_resource rbuf_io_resource_t;
typedef enum {RING=0, LINEAR} buffer_type_t;
typedef enum {NORMAL=0, EMPTY, FULL} buffer_state_t;
typedef enum {READ=0, WRITE} buffer_io_type_t;

int buffer_set_rpos(buffer_t *buf, unsigned int pos);
int buffer_set_wpos(buffer_t *buf, unsigned int pos);
int buffer_set_state(buffer_t *buf, buffer_state_t state);
unsigned int buffer_n_write_bytes(buffer_t *buf);
unsigned int buffer_n_read_bytes(buffer_t *buf);

int buffer_init(buffer_t **buf, unsigned int n_bytes, \
  buffer_type_t type);
int buffer_init_ext(buffer_t **buf, unsigned int n_bytes, \
  buffer_type_t type, void *raw_buffer);
int buffer_copy(buffer_t **new_buf, buffer_t *src_buf);

int buffer_add_recording(buffer_t *buf, recording_t *r);

int buffer_delete(buffer_t **buf);

int buffer_reset(buffer_t *buf);

int buffer_write(buffer_t *dest, const void *src, unsigned int n_bytes);

int buffer_read(buffer_t *src, void *dest, unsigned int n_bytes);

int buffer_view(buffer_t *src, void *dest, unsigned int offset, unsigned int n_bytes);

int buffer_size(buffer_t *buf);
#endif
