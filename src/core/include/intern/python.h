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

int add_format_types(PyObject *m);
int add_buffer_types(PyObject *m);
int add_backend_types(PyObject *m);
int add_device_types(PyObject *m);

#endif