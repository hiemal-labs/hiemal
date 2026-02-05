#ifndef HM_INTERN_PYTHON_H
#define HM_INTERN_PYTHON_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#if PY_VERSION_HEX < 0x030A0000
#error "Minimum python version is 3.10"
#endif

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

#include <stddef.h>

#include "api/backend.h"
#include "api/buffer.h"
#include "intern/format.h"

struct PyHmBackendObject {
  PyObject_HEAD
  const char *name;
  hm_backend_connection_t *conn;
  struct PyHmDeviceListObject *devices;
};

struct PyHmBackendListObject {
  PyObject_HEAD
  Py_ssize_t n_items;
  struct PyHmBackendObject** backends;
};

struct PyHmBufferObject {
  PyObject_HEAD
  buffer_t *buf;
};

struct PyHmDeviceObject {
  PyObject_HEAD
  struct PyHmBackendObject *backend;
};

struct PyHmDeviceListObject {
  PyObject_HEAD
  Py_ssize_t n_items;
  struct PyHmDeviceObject** devices;
};

struct PyHmSignatureObject {
  PyObject_HEAD
  hm_format_signature sig;
};

int add_format_types(PyObject *m);
int add_buffer_types(PyObject *m);
int add_backend_types(PyObject *m);
int add_device_types(PyObject *m);

int init_backends(PyObject *m);

#endif