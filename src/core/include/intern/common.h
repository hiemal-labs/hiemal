#ifndef HIEMAL_INTERN_COMMON_H
#define HIEMAL_INTERN_COMMON_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "cmake_info.h"
#include "error.h"
#include "intern/logging.h"

#ifdef _MSC_VER
#define __restrict__ __restrict
#endif

#ifdef __unix__
#include <pthread.h>
typedef pthread_t hm_thread_id;
typedef pthread_mutex_t hm_mutex_t;
#endif

// base types
enum { HM_LIST_MAGIC = 0x1, HM_LIST_NODE_MAGIC, HM_ARRAY_MAGIC };

// subtypes
enum {
  HM_LIST_REFNODE_MAGIC = 0x1,
};

#define HM_LIST_ASSERT(name) assert((name)->magic == HM_LIST_MAGIC);
#define HM_LIST_NODE_ASSERT(name) assert((name)->magic == HM_LIST_NODE_MAGIC);
#define HM_ARRAY_ASSERT(name) assert((name)->magic == HM_ARRAY_MAGIC);

#define UNIT_TESTING

#ifdef UNIT_TESTING
#define STATIC
#else
#define STATIC static
#endif

#define STRINGIFY(x) #x

#define HM_FLAG_SET(flags, flag) \
  (((flags) & (uint64_t)(flag)) == (uint64_t)(flag))

enum { COPY_OVERWRITE_BIT = 0, COPY_MOVE_BIT };

#define COPY_OVERWRITE ((uint64_t)1 << (uint64_t)COPY_OVERWRITE_BIT)
#define COPY_MOVE ((uint64_t)1 << (uint64_t)COPY_MOVE_BIT)

typedef void hm_type_t;
typedef void hm_list_t;
typedef void hm_list_node_t;
typedef void hm_array_t;

typedef int(list_node_fn)(hm_list_node_t *node);
typedef int(list_node_fn_userdata)(hm_list_node_t *node, void *userdata);
// get rid of clang-tidy cppcoreguidelines-avoid-non-const-global-variable
// warning for default_node_free_fn
typedef int (*const const_list_node_fn)(hm_list_node_t *node);
typedef int(list_node_cmp_fn)(hm_list_node_t *node, void *userdata);
typedef int(array_item_fn)(void *arr_item);

extern const_list_node_fn const default_node_free_fn;

// NOLINTBEGIN(bugprone-macro-parentheses)
#define HM_TYPE_HEAD uint64_t magic;
#define HM_LIST_HEAD(type) \
  uint64_t magic;          \
  type *head;              \
  unsigned int n_items;
#define HM_LIST_NODE_HEAD(type) \
  uint64_t magic;               \
  unsigned int data_size;       \
  list_node_fn *free_fn;        \
  type *prev;                   \
  type *next;
#define HM_ARRAY_HEAD(type)   \
  uint64_t magic;             \
  type *buf;                  \
  unsigned int n_items;       \
  unsigned int n_items_alloc; \
  unsigned int item_size;

#define HM_LIST_INIT(name) \
  name->head = NULL;       \
  name->n_items = 0;       \
  name->magic = HM_LIST_MAGIC;
#define HM_REFLIST_INIT(name)                                       \
  name->head = NULL;                                                \
  name->n_items = 0;                                                \
  name->magic = ((uint64_t)HM_LIST_REFNODE_MAGIC << (uint64_t)32) | \
                (uint64_t)(HM_LIST_MAGIC);
#define HM_LIST_NODE_INIT(name)         \
  name->free_fn = default_node_free_fn; \
  name->data_size = 0;                  \
  name->prev = NULL;                    \
  name->next = NULL;                    \
  name->magic = HM_LIST_NODE_MAGIC;
#define HM_REFLIST_NODE_INIT(name)                                  \
  name->free_fn = default_node_free_fn;                             \
  name->data_size = 0;                                              \
  name->prev = NULL;                                                \
  name->next = NULL;                                                \
  name->node = NULL;                                                \
  name->magic = ((uint64_t)HM_LIST_REFNODE_MAGIC << (uint64_t)32) | \
                (uint64_t)(HM_LIST_NODE_MAGIC);
#define HM_ARRAY_INIT(name, type) \
  name->buf = NULL;               \
  name->n_items = 0;              \
  name->n_items_alloc = 0;        \
  name->item_size = sizeof(type); \
  name->magic = HM_ARRAY_MAGIC;
#define HM_ARRAY_INIT_LIKE(name, arr) \
  name->buf = NULL;                   \
  name->n_items = 0;                  \
  name->n_items_alloc = 0;            \
  name->item_size = arr->item_size;   \
  name->magic = HM_ARRAY_MAGIC;
// NOLINTEND(bugprone-macro-parentheses)

typedef struct kwargs {
  int argc;
  char **arg_names;
  void **args;
} kwargs_t;

#define CREATE_ARRAY_TYPE(name)    \
  typedef struct _##name##_array { \
    unsigned int n_##name;         \
    name##_t **name##s;            \
  } name##_array_t;

#define MEMBER_ARRAY(type) \
  unsigned int max_##type; \
  unsigned int n_##type;   \
  type##_t **type##s;

/*
typedef struct _array {
  unsigned int n_items;
  unsigned int max_items;
  void **data;
} hm_array_t;
*/

#define IMPL(name)                                                     \
  int name##_impl(unsigned int n_bytes, void *_inputs, void *_outputs, \
                  kwargs_t *kwargs)
#define IMPL_ARGS \
  unsigned int n_bytes, void *_inputs, void *_outputs, kwargs_t *kwargs

#define SET_DEFAULT_ARG(arg, val, nullval) \
  if ((arg) == (nullval)) {                \
    (arg) = (val);                         \
  }

bool is_list(hm_type_t *obj);
bool is_list_node(hm_type_t *obj);
bool is_list_refnode(hm_type_t *obj);
bool is_array(hm_type_t *obj);

#ifdef __clang__
int __attribute((ownership_takes(malloc, 2))) hm_list_insert(
    hm_list_t *list, hm_list_node_t *node, long int index);
#else
int hm_list_insert(hm_list_t *list, hm_list_node_t *node, long int index);
#endif
int hm_list_clear(hm_list_t *list);
int hm_list_copy_array(hm_list_t *list, hm_array_t *arr, unsigned int flags);
hm_list_node_t *hm_list_at(hm_list_t *list, long int index);
hm_list_node_t *hm_list_node_extract(hm_list_node_t *node);
int hm_list_append(hm_list_t *list, hm_list_node_t *node);
int hm_list_itr(hm_list_t *list, list_node_fn *itr_fn);
hm_list_node_t *hm_list_find(hm_list_t *list, list_node_cmp_fn *find_fn,
                             void *userdata);
int hm_list_remove(hm_list_t *list, long int index, list_node_fn *free_fn);
int hm_list_remove_where(hm_list_t *list, list_node_cmp_fn *cmp_fn,
                         list_node_fn *free_fn, void *userdata);
int hm_list_delete(hm_list_t *list);

int hm_array_copy_raw(hm_array_t *arr, void *buf, unsigned int n_items);
int hm_array_clear(hm_array_t *arr);
int hm_array_copy_list(hm_array_t *arr, hm_list_t *list, unsigned int flags);
void *hm_array_at(hm_array_t *arr, unsigned int index);
int hm_array_concat(hm_array_t *arr1, hm_array_t *arr2);
int hm_array_concat_new(hm_array_t *dest, hm_array_t *arr1, hm_array_t *arr2);
int hm_array_concat_raw(hm_array_t *arr, void *data, unsigned int n_items);
int hm_array_resize(hm_array_t *arr, unsigned int n_items);
int hm_array_delete(hm_array_t *arr, array_item_fn *free_fn);

char *next_arg(char *const str, char delim, const char *delim_opt, int *arglen,
               bool *optional);
int kwargs_unpack(kwargs_t *kwargs, char *fmt, ...);

#if defined(__unix__) && defined(__GNUC__)
int xasprintf(char **str, char *fmt, ...) __attribute__((format(printf, 2, 3)));
#else
int xasprintf(char **str, char *fmt, ...)
#endif
int xvsprintf(char *str, const char *fmt, va_list args);
void *xmemcpy(void *dest, const void *src, size_t n);
#endif

int hm_error_unimplemented(const char *fn_name);
int hm_error_unreachable(const char *fn_name);

int hm_warning_unreachable(const char *fn_name);