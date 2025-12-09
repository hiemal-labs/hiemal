#ifndef _INTERN_EVENT_H
#define _INTERN_EVENT_H

typedef struct _hm_event hm_event_t;
typedef struct _hm_event_list hm_event_list_t;
typedef struct _buffer_io_event buffer_io_event_t;

typedef int (event_condition)(hm_event_t*, void*);

enum hm_event_obj_type {
  OBJ_FD=0,
  OBJ_MUTEX,
  OBJ_FUTEX,
  OBJ_BUFFER
};

#ifdef __linux__
union hm_event_obj {
  int fd;
  hm_mutex_t mutex;
  uint32_t futex;
  int buf_pipe[2];
};
#endif

struct _hm_event {
  HM_LIST_NODE_HEAD(hm_event_t)
  union hm_event_obj obj;
  enum hm_event_obj_type obj_type;
  event_condition *cond;
};

struct _hm_event_list {
  HM_LIST_HEAD(hm_event_t)
};

struct _buffer_io_event {
  buffer_io_type_t io_type;
  unsigned int n_bytes_transferred;
  unsigned int n_bytes_readable;
  unsigned int n_bytes_writeable;
};

int hm_event_init(hm_event_t **e, enum hm_event_obj_type event_type, event_condition *cond);
int hm_event_delete(hm_event_t **e);
int hm_event_list_init(hm_event_list_t **l);
int hm_event_reflist_init(hm_event_list_t **l);
int hm_event_list_delete(hm_event_list_t **l);

int hm_event_poll(hm_event_t *e);

int hm_event_buffer_wake(hm_event_list_t *buf_event_list, buffer_io_type_t io_type, unsigned int n_bytes_transferred, unsigned int n_bytes_readable, unsigned int n_bytes_writeable);

#endif