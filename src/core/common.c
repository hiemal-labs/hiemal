// usr safe c11 string functions if available
#ifdef __STDC_LIB_EXT1__
#define __STDC_WANT_LIB_EXT1__ 1
#endif

#include "intern/common.h"

#include <ctype.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intern/math.h"

#define HM_TYPE_ID(magic) ((magic) & 0xFFFFFFFF)
#define HM_SUBTYPE_ID(magic) \
  (((magic) & ((uint64_t)(0xFFFFFFFF) << (uint64_t)32)) >> (uint64_t)32)

struct hm_type {
  HM_TYPE_HEAD
  uint8_t data[];
};

struct hm_list_node {
  HM_LIST_NODE_HEAD(struct hm_list_node)
  uint8_t data[];
};

struct hm_list_refnode {
  HM_LIST_NODE_HEAD(struct hm_list_refnode)
  hm_list_node_t *node;
};

struct hm_list {
  HM_LIST_HEAD(struct hm_list_node)
};

struct hm_array {
  HM_ARRAY_HEAD(void)
};

int default_node_free(hm_list_node_t *node) {
  free(node);
  return 0;
}

const_list_node_fn const default_node_free_fn = default_node_free;

bool is_list(hm_type_t *obj) {
  struct hm_type *_obj = (struct hm_type *)obj;
  return (_obj->magic == HM_LIST_MAGIC) ? true : false;
}

bool is_list_node(hm_type_t *obj) {
  struct hm_type *_obj = (struct hm_type *)obj;
  return (_obj->magic == HM_LIST_NODE_MAGIC) ? true : false;
}

int hm_list_insert(hm_list_t *list, hm_list_node_t *node, long int index) {
  struct hm_list *_list = (struct hm_list *)list;
  struct hm_list_node *_node = (struct hm_list_node *)node;
  if (index < 0) {
    index = _list->n_items;
  }
  if (_list->n_items < (unsigned int)index) {
    return -1;
  }
  struct hm_list_node **node_itr = &(_list->head);
  struct hm_list_node **node_prev = node_itr;

  int itr = 0;
  for (itr = 0; itr < index; itr++) {
    node_prev = node_itr;
    node_itr = &((*node_itr)->next);
  }
  if (HM_SUBTYPE_ID(_list->magic) == 0) {
    _node->next = *node_itr;
    _node->prev = (index == 0) ? NULL : *node_prev;
    if (*node_itr) {
      (*node_itr)->prev = _node;
    }
    *node_itr = _node;
  } else if (HM_SUBTYPE_ID(_list->magic) == HM_LIST_REFNODE_MAGIC) {
    struct hm_list_refnode *new_refnode =
        (struct hm_list_refnode *)malloc(sizeof(struct hm_list_refnode));
    HM_REFLIST_NODE_INIT(new_refnode);
    new_refnode->node = node;
    new_refnode->next = (struct hm_list_refnode *)(*node_itr);
    new_refnode->prev =
        (index == 0) ? NULL : (struct hm_list_refnode *)(*node_prev);
    if (*node_itr) {
      (*node_itr)->prev = (struct hm_list_node *)new_refnode;
    }
    *node_itr = (struct hm_list_node *)new_refnode;
  }
  _list->n_items++;
  return 0;
}

int hm_list_clear(hm_list_t *list) {
  struct hm_list *_list = (struct hm_list *)list;
  struct hm_list_node *node_itr = _list->head;
  struct hm_list_node *node_next = NULL;

  while (node_itr != NULL) {
    node_next = node_itr->next;
    if (node_itr->free_fn != NULL) {
      (node_itr->free_fn)(node_itr);
    }
    node_itr = node_next;
  }
  _list->n_items = 0;
  _list->head = NULL;
  return 0;
}

int hm_list_copy_array(hm_list_t *list, hm_array_t *arr, unsigned int flags) {
  struct hm_array *_arr = (struct hm_array *)arr;
  if (HM_FLAG_SET(flags, COPY_OVERWRITE)) {
    hm_list_clear(list);
  }
  struct hm_list_node *new_node = NULL;
  size_t itr = 0;
  for (itr = 0; itr < _arr->n_items; itr++) {
    new_node = (hm_list_node_t *)malloc(offsetof(struct hm_list_node, data) +
                                        _arr->item_size);
    if (xmemcpy(new_node->data, (char*)(_arr->buf) + (itr * _arr->item_size),
                _arr->item_size) != NULL) {
      new_node->free_fn = default_node_free_fn;
      hm_list_append(list, new_node);
    } else {
      free(new_node);
    }
  }
  return 0;
}

hm_list_node_t *hm_list_at(hm_list_t *list, long int index) {
  struct hm_list *_list = (struct hm_list *)list;
  struct hm_list_node *node_itr = _list->head;
  if (index < 0) {
    index = _list->n_items;
  }
  if (_list->n_items < (unsigned int)index) {
    return NULL;
  }
  int itr = 0;
  for (itr = 0; itr < index; itr++) {
    node_itr = node_itr->next;
  }
  if (HM_SUBTYPE_ID(_list->magic) == HM_LIST_REFNODE_MAGIC) {
    return ((struct hm_list_refnode *)node_itr)->node;
  } else {
    return node_itr;
  }
}

hm_list_node_t *hm_list_node_extract(hm_list_node_t *node) {
  struct hm_list_node *_node = (struct hm_list_node *)node;
  if (HM_SUBTYPE_ID(_node->magic) == HM_LIST_REFNODE_MAGIC) {
    return ((struct hm_list_refnode *)node)->node;
  } else {
    return node;
  }
}

int hm_list_append(hm_list_t *list, hm_list_node_t *node) {
  return hm_list_insert(list, node, -1);
}

int hm_list_itr_userdata(hm_list_t *list, list_node_fn_userdata *itr_fn,
                         void *userdata) {
  struct hm_list *_list = (struct hm_list *)list;
  struct hm_list_node *node_itr = _list->head;
  while (node_itr != NULL) {
    (*itr_fn)(node_itr, userdata);
    node_itr = node_itr->next;
  }
  return 0;
}

int hm_list_itr(hm_list_t *list, list_node_fn *itr_fn) {
  struct hm_list *_list = (struct hm_list *)list;
  struct hm_list_node *node_itr = _list->head;
  while (node_itr != NULL) {
    (*itr_fn)(node_itr);
    node_itr = node_itr->next;
  }
  return 0;
}

hm_list_node_t *hm_list_find(hm_list_t *list, list_node_cmp_fn *find_fn,
                             void *userdata) {
  struct hm_list *_list = (struct hm_list *)list;
  struct hm_list_node *node_itr = _list->head;
  while (node_itr != NULL) {
    if ((*find_fn)(node_itr, userdata) == 0) {
      return node_itr;
    }
    node_itr = node_itr->next;
  }
  return NULL;
}

int hm_list_remove(hm_list_t *list, long int index, list_node_fn *free_fn) {
  struct hm_list *_list = (struct hm_list *)list;
  if (index < 0) {
    index = _list->n_items - 1;
  }
  if (_list->n_items < (unsigned int)index) {
    return -1;
  }
  struct hm_list_node **node_itr = &(_list->head);

  int itr = 0;
  for (itr = 0; itr < index; itr++) {
    node_itr = &((*node_itr)->next);
  }
  struct hm_list_node *old_node = *node_itr;
  *node_itr = old_node->next;
  if (*node_itr != NULL) {
    (*node_itr)->prev = old_node->prev;
  }
  (free_fn) ? (void)((*free_fn)(old_node)) : free(old_node);
  _list->n_items--;
  return 0;
}

int hm_list_remove_where(hm_list_t *list, list_node_cmp_fn *cmp_fn,
                         list_node_fn *free_fn, void *userdata) {
  struct hm_list *_list = (struct hm_list *)list;
  // struct hm_list_node **node_itr = (struct hm_list_node**)&(_list->head);
  // struct hm_list_node **node_prev = node_itr;
  struct hm_list_node *node_itr = _list->head;
  // struct hm_list_node *old_node = NULL;
  struct hm_list_node *node_prev = NULL;
  while (node_itr != NULL) {
    node_prev = node_itr;
    // old_node = *node_itr;
    node_itr = node_itr->next;
    if ((*cmp_fn)(node_prev, userdata) == 0) {
      //*node_prev = (struct hm_list_node*)(old_node->next);
      if (node_prev->prev != NULL) {
        node_prev->prev->next = node_itr;
      } else {
        _list->head = node_itr;
      }
      // if (*node_prev != NULL) (*node_prev)->prev = *node_prev;
      node_itr->prev = node_prev->prev;
      (free_fn) ? (void)((*free_fn)(node_prev)) : free(node_prev);
      _list->n_items--;
    }
  }
  return 0;
}

int hm_list_delete(hm_list_t *list) {
  hm_list_clear(list);
  free(list);
  return 0;
}

int hm_array_resize(hm_array_t *arr, unsigned int n_items) {
  struct hm_array *_arr = (struct hm_array *)arr;
  if (_arr->n_items_alloc < n_items) {
    void *new_buffer =
        realloc(_arr->buf, 2 * (size_t)(n_items)*_arr->item_size);
    if (new_buffer) {
      _arr->buf = new_buffer;
      _arr->n_items_alloc = n_items;
    } else {
      return -1;
    }
  }
  return 0;
}

int hm_array_copy_raw(hm_array_t *arr, void *buf, unsigned int n_items) {
  struct hm_array *_arr = (struct hm_array *)arr;
  hm_array_resize(arr, n_items);
  _arr->n_items = n_items;
  xmemcpy(_arr->buf, buf, (size_t)(n_items)*_arr->item_size);
  return 0;
}

int hm_array_clear(hm_array_t *arr) {
  struct hm_array *_arr = (struct hm_array *)arr;
  free(_arr->buf);
  _arr->buf = NULL;
  _arr->n_items = 0;
  _arr->n_items_alloc = 0;
  return 0;
}

struct list_data {
  void **list_data;
  uint64_t *list_data_len;
  int itr;
};

int get_list_data(hm_list_node_t *node, void *userdata) {
  struct list_data *list_data_ptr = userdata;
  void **list_data = list_data_ptr->list_data;
  uint64_t *list_data_len = list_data_ptr->list_data_len;
  int *itr = &(list_data_ptr->itr);
  struct hm_list_node *_node = (struct hm_list_node *)node;
  list_data[*itr] = _node->data;
  list_data_len[*itr] = _node->data_size;
  (*itr)++;
  return 0;
}

int hm_array_copy_list(hm_array_t *arr, hm_list_t *list, unsigned int flags) {
  struct hm_list *_list = (struct hm_list *)list;
  if (HM_FLAG_SET(flags, COPY_OVERWRITE)) {
    hm_array_clear(arr);
  }
  void **list_data = (void **)calloc(_list->n_items, sizeof(void *));
  uint64_t *list_data_len =
      (uint64_t *)calloc(_list->n_items, sizeof(uint64_t));
  struct list_data userdata = {list_data, list_data_len, 0};

  hm_list_itr_userdata(list, get_list_data, &userdata);
  unsigned int itr = 0;
  uint64_t offset_itr = 0;  // running sum of list node sizes
  uint64_t new_arr_len = u64sum(list_data_len, _list->n_items);
  // TODO: add coherence check re. non-uniform lists/arrays
  void *new_arr_data = malloc(new_arr_len);
  for (itr = 0; itr < _list->n_items; itr++) {
    xmemcpy((char*)new_arr_data + offset_itr, list_data[itr], list_data_len[itr]);
    offset_itr += list_data_len[itr];
  }
  hm_array_concat_raw(arr, new_arr_data, _list->n_items);
  free((void *)list_data_len);
  free((void *)list_data);
  free(new_arr_data);
  return 0;
}

void *hm_array_at(hm_array_t *arr, unsigned int index) {
  struct hm_array *_arr = (struct hm_array *)arr;
  if (index < _arr->n_items) {
    return (char*)(_arr->buf) + (index * (uint64_t)(_arr->item_size));
  } else {
    return NULL;
  }
}

int hm_array_concat(hm_array_t *arr1, hm_array_t *arr2) {
  struct hm_array *_arr1 = (struct hm_array *)arr1;
  struct hm_array *_arr2 = (struct hm_array *)arr2;
  unsigned int new_size = _arr1->n_items + _arr2->n_items;
  hm_array_resize(arr1, new_size);
  xmemcpy((char*)(_arr1->buf) + ((size_t)(_arr1->n_items) * _arr1->item_size),
          _arr2->buf, (size_t)(_arr2->n_items) * _arr2->item_size);
  _arr1->n_items = new_size;
  return 0;
}

int hm_array_concat_raw(hm_array_t *arr, void *data, unsigned int n_items) {
  struct hm_array *_arr1 = (struct hm_array *)arr;
  struct hm_array *_new_arr =
      (struct hm_array *)malloc(sizeof(struct hm_array));
  HM_ARRAY_INIT_LIKE(_new_arr, _arr1)
  hm_array_resize(_new_arr, n_items);
  _new_arr->n_items = n_items;
  xmemcpy(_new_arr->buf, data, n_items * (size_t)(_new_arr->item_size));
  hm_array_concat(arr, _new_arr);
  hm_array_delete(_new_arr, NULL);
  return 0;
}

int hm_array_delete(hm_array_t *arr, array_item_fn *free_fn) {
  struct hm_array *_arr = (struct hm_array *)arr;
  if (free_fn != NULL) {
    size_t itr = 0;
    for (itr = 0; itr < _arr->n_items; itr++) {
      free_fn((char*)(_arr->buf) + (itr * _arr->item_size));
    }
  }
  free(_arr->buf);
  free(arr);
  return 0;
}

int isvarchar(int _char) { return (isalnum(_char) || _char == '_') ? 1 : 0; }

char *next_arg(char *const str, char delim, const char *delim_opt, int *arglen,
               bool *optional) {
  SET_DEFAULT_ARG(delim, ';', '\0');
  SET_DEFAULT_ARG(delim_opt, "()", NULL);
  char *next = NULL;
  next = str;
  if (!isvarchar(*next)) {
    while (!isvarchar(*next)) {
      if (*next == delim_opt[0]) {
        *optional = true;
      } else if (*next == delim_opt[1]) {
        *optional = false;
      }
      next++;
    }
    *arglen = -1;
    return next;
  }
  bool last_arg = true;
  while (isvarchar(*next) && *next != '\0' && *next != delim) {
    next++;
  }
  *arglen = (int)(next - str);
  while (!isvarchar(*next) && *next != '\0') {
    if (*next == delim_opt[0]) {
      *optional = true;
    } else if (*next == delim_opt[1]) {
      *optional = false;
    }
    next++;
  }
  char *end = next;
  while (*end != '\0') {
    if (isvarchar(*end)) {
      last_arg = false;
    }
    end++;
  }
  return last_arg ? NULL : next;
}

int kwargs_unpack(kwargs_t *kwargs, char *fmt, ...) {
  char *fmt_itr = fmt;
  char *fmt_itr_prev = NULL;
  int arglen = 0;
  bool opt = false;
  bool opt_prev = opt;
  char *key = NULL;
  int n_keys = kwargs->argc;
  bool found_key = false;
  int itr = 0;

  void **arg_itr = NULL;

  va_list vars;
  va_start(vars, fmt);
  while (fmt_itr != NULL) {
    fmt_itr_prev = fmt_itr;
    opt_prev = opt;

    fmt_itr = next_arg(fmt_itr, ';', NULL, &arglen, &opt);
    if (arglen < 1) {
      continue;
    }
    arg_itr = va_arg(vars, void **);
    key = strndup(fmt_itr_prev, arglen);
    found_key = false;
    for (itr = 0; itr < n_keys; itr++) {
      if (strcmp(key, kwargs->arg_names[itr]) == 0) {
        *arg_itr = (kwargs->args)[itr];
        found_key = true;
        break;
      }
    }
    if (!found_key && !opt_prev) {
      free(key);
      va_end(vars);
      return -1;  // missing required key
    }
    free(key);
  }
  va_end(vars);
  return 0;
}

int xasprintf(char **str, char *fmt, ...) {
  va_list vars;
  va_list vars_copy;
  va_start(vars, fmt);
  va_copy(vars_copy, vars);
  // ignore clang-tidy warning here because we are using vsnprintf to calculate
  // formatted string size
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  int n_bytes = vsnprintf(NULL, 0, fmt, vars) + 1;
  if (n_bytes < 0) {
    va_end(vars);
    va_end(vars_copy);
    return n_bytes + 1;  // return the vsnprintf output
  }
  *str = (char *)malloc(n_bytes);
  int res = xvsprintf(*str, fmt, vars_copy);
  if (res < 0) {
    va_end(vars);
    va_end(vars_copy);
    return res;
  }
  va_end(vars);
  va_end(vars_copy);
  return 0;
}

int xvsprintf(char *str, const char *fmt, va_list args) {
#ifdef __STDC_LIB_EXT1__
  return vsnprintf_s(str, maxlen, fmt, args);
#else
  if ((str == NULL) || (fmt == NULL)) {
    return -1;
  }
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  return vsprintf(str, fmt, args);
#endif
}

void *xmemcpy(void *dest, const void *src, size_t n) {
#ifdef __STDC_LIB_EXT1__
  return memcpy_s(dest, n, src, n);
#else
  if ((dest == NULL) || (src == NULL)) {
    return NULL;
  }
  // NOLINTNEXTLINE(clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling)
  return memcpy(dest, src, n);
#endif
}

int hm_error_unimplemented(const char *fn_name) {
  return hm_log_err("unimplemented", fn_name);
  exit(1);
}

int hm_warn_unimplemented(const char *fn_name) {
  return hm_log_warn("unimplemented", fn_name);
  exit(1);
}

int hm_error_unreachable(const char *fn_name) {
  return hm_log_err("reached section marked as unreachable, creating core dump", fn_name);
  abort();
}