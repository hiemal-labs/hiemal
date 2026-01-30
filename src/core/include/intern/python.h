#ifndef HM_INTERN_PYTHON_H
#define HM_INTERN_PYTHON_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#if PY_VERSION_HEX < 0x030A0000
#error "Minimum python version is 3.10"
#endif

#include <stddef.h>

int add_format_types(PyObject *m);

#endif