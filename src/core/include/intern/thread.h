#ifndef _INTERN_THREAD_H
#define _INTERN_THREAD_H

#include <stdlib.h>

#ifdef __unix__
#include <pthread.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#endif

#include "intern/common.h"

typedef struct _thread_info thread_info_t;
typedef struct _threads_info threads_info_t;
extern threads_info_t *threads_info;

struct _thread_info {
  HM_LIST_NODE_HEAD(thread_info_t)
  hm_thread_id thread_id;
  const char *created_by;
  enum {POLLING, TIMER, WORKER} thread_type;
};

int thread_init(void *(*fn)(void*), void *args, const char *file, const char *func, int line);

#define HM_THREAD_CREATE(fn, args) thread_init(fn, args, __FILE__, __func__, __LINE__);

int threads_info_reset();

int thread_join(hm_thread_id id);
int thread_join_all();

int thread_mutex_init(hm_mutex_t *m);
int thread_mutex_lock(hm_mutex_t *m);
int thread_mutex_unlock(hm_mutex_t *m);
int thread_mutex_delete(hm_mutex_t *m);
int thread_join(hm_thread_id id);
hm_thread_id thread_id();
#endif
