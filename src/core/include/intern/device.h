#ifndef _INTERN_DEVICE_H
#define _INTERN_DEVICE_H

#include "api/device.h"

#include "intern/backend.h"
#include "intern/buffer.h"

typedef int (device_io_fn)(hm_device_io_t*, buffer_t*, unsigned int);
typedef int (device_io_connection_io_fn)(hm_device_io_connection_t*, buffer_t*, unsigned int);
typedef size_t (device_io_connection_info_fn)(hm_device_io_connection_t*);

#define HM_DEVICE_HEAD char *name; hm_backend_t io_backend; void *backend_handle; int n_io_devices; hm_device_io_t **io_devices; unsigned int default_io_id;

struct _hm_device {
  HM_DEVICE_HEAD
};

struct _hm_device_io {
  char *name;
  hm_io_type_t type;
  hm_device_t *device;
  hm_backend_t dev_backend;
  void *backend_dev_io_handle; // backend-specific handle for io device
  void *backend_handle; // main backend handle
  device_io_fn *read_fn;
  device_io_fn *write_fn;
};

struct _hm_device_io_connection {
  hm_io_type_t type;
  void *backend_dev_io_handle;
  void *backend_handle;
  device_io_connection_io_fn *read_fn;
  device_io_connection_io_fn *write_fn;
  device_io_connection_info_fn *bytes_readable_fn;
  device_io_connection_info_fn *bytes_writable_fn;
};

#endif
