#include <stddef.h>
#include <stdint.h>
#include <poll.h>
#include <unistd.h>

#include <sys/time.h>

#ifdef __linux__
#define _GNU_SOURCE
#include <sys/syscall.h>
#include <linux/futex.h>
int futex_wait(uint32_t *uaddr, uint32_t val, const struct timespec *timeout) {
  return (int)syscall(SYS_futex, uaddr, FUTEX_WAIT, timeout);
}

int futex_wake(uint32_t *uaddr, uint32_t val) {
  return (int)syscall(SYS_futex, uaddr, FUTEX_WAKE);
}
#else
#include <sys/time.h>
#include <sys/futex.h>
int futex_wait(uint32_t *uaddr, uint32_t val, const struct timespec *timeout) {
  return futex(uaddr, FUTEX_WAIT, timeout, NULL);
}
int futex_wake(uint32_t *uaddr, uint32_t val) {
  return futex(uaddr, FUTEX_WAIT, NULL, NULL);
}
#endif

#include "api/buffer.h"

#include "intern/common.h"
#include "intern/event.h"
#include "intern/thread.h"

int hm_event_list_init(hm_event_list_t **l) {
  *l = (hm_event_list_t*)malloc(sizeof(hm_event_list_t));
  HM_LIST_INIT((*l))
  return 0;
}

int hm_event_reflist_init(hm_event_list_t **l) {
  *l = (hm_event_list_t*)malloc(sizeof(hm_event_list_t));
  HM_REFLIST_INIT((*l))
  return 0;
}

int hm_event_init(hm_event_t **e, enum hm_event_obj_type event_type, event_condition *cond) {
  *e = (hm_event_t*)malloc(sizeof(hm_event_t));
  (*e)->obj_type = event_type;
  (*e)->cond = cond;
  if (event_type == OBJ_BUFFER) {
    pipe((*e)->obj.buf_pipe);
  }
  return 0;
}

int hm_event_list_delete(hm_event_list_t **l) {
  hm_list_delete(*l);
  *l = NULL;
  return 0;
}

int hm_event_delete(hm_event_t **e) {
  if((*e)->obj_type == OBJ_BUFFER) {
    close((*e)->obj.buf_pipe[0]);
    close((*e)->obj.buf_pipe[1]);
  }
  free(*e);
  return 0;
}

int hm_event_list_add(hm_event_list_t *l, hm_event_t *e) {
  return 0;
}

int hm_poll_fd(hm_event_t *e) {
  struct pollfd fd;
  fd.fd = (e->obj).fd;
  poll_fd:
  poll(&fd, 1, -1);
  if (e->cond(e, NULL) != 0) goto poll_fd;
  return 0;
}

int hm_poll_fd_list(hm_event_list_t *l) {
  struct pollfd *fds = (struct pollfd*)malloc(l->n_items * sizeof(struct pollfd));
  unsigned int i = 0;
  hm_event_t *e = l->head;
  for (i = 0; i < l->n_items; i++) {
    if ((e->obj_type) != OBJ_FD) return -1;
    fds[i].fd = e->obj.fd;
    fds[i].events = POLLIN;
    e = e->next;
  }
  poll_fds:
  poll(fds, l->n_items, -1);
  for (i = 0; i < l->n_items; i++) {
    if (HM_FLAG_SET(fds[i].fd, POLLIN)) {
      e = hm_list_at(l, i);
      if (e->cond(e, NULL) != 0) goto poll_fds;
      else break;
    }
  }
  free(fds);
  return 0;
}

int hm_poll_buffer(hm_event_t *e) {
  struct pollfd fd;
  fd.fd = (e->obj).buf_pipe[0];
  fd.events = POLLIN;
  buffer_io_event_t *buf_io_info = NULL;
  poll_fd:
  poll(&fd, 1, -1);
  read(fd.fd, &buf_io_info, sizeof(buffer_io_event_t*));
  if (e->cond(e, buf_io_info) != 0) {
    free(buf_io_info);
    goto poll_fd;
  } else {
    free(buf_io_info);
  }
  return 0;
}

int hm_event_buffer_wake(hm_event_list_t *buf_event_list, buffer_io_type_t io_type, unsigned int n_bytes_transferred, unsigned int n_bytes_readable, unsigned int n_bytes_writeable) {
  buffer_io_event_t *buf_io_info = NULL;
  // TODO: use reference counting to wake up multiple events instead of making copies of
  // the buffer event struct
  int i = 0;
  hm_event_t *e = NULL;
  int buf_pipe_write = -1;
  for (i = 0; i < buf_event_list->n_items; i++) {
    e = (hm_event_t*)hm_list_at(buf_event_list, i);
    buf_io_info = (buffer_io_event_t*)malloc(sizeof(buffer_io_event_t));
    buf_io_info->io_type = io_type;
    buf_io_info->n_bytes_transferred = n_bytes_transferred;
    buf_io_info->n_bytes_readable = n_bytes_readable;
    buf_io_info->n_bytes_writeable = n_bytes_writeable;
    buf_pipe_write = (e->obj).buf_pipe[1];
    write(buf_pipe_write, &buf_io_info, sizeof(buf_io_info));
  }
  return 0;
}

int hm_event_poll(hm_event_t *e) {
  if (e->obj_type == OBJ_BUFFER) hm_poll_buffer(e);
  else return -1;
  return 0;
}

int hm_event_poll_list(hm_event_list_t *l) {
  return 0;
}
