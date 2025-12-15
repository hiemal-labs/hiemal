#ifndef _INTERN_BACKEND_H
#define _INTERN_BACKEND_H

#include "api/backend.h"
#include "intern/common.h"

typedef enum {ALSA=0, PULSEAUDIO} hm_backend_t;

typedef struct _hm_device hm_device_t;
typedef struct _hm_device_io_connection hm_device_io_connection_t;
struct _hm_backend_connection {
  char *backend_name;
  hm_backend_t backend_type;
  int n_devices;
  hm_device_t **devices;
  hm_list_t *dev_io_list;
  unsigned int default_dev_io[2]; // PLAYBACK, RECORDING
  void *backend_handle;
};


typedef int (backend_init_fn)(hm_backend_connection_t**);
typedef int (backend_delete_fn)(hm_backend_connection_t**);
typedef int (backend_fn)(hm_backend_connection_t*);
typedef int (backend_io_connect_fn)(hm_device_io_connection_t**, hm_backend_connection_t*, unsigned int);

struct backend_info {
  char *name;
  backend_init_fn *init_fn;
  backend_delete_fn *delete_fn;
  backend_fn *dump_info_fn;
  backend_io_connect_fn *io_connect_fn;
};

const struct backend_info *get_backend_by_name(const char *name);

#endif