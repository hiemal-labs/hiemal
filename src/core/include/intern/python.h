#ifndef HM_INTERN_PYTHON_H
#define HM_INTERN_PYTHON_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stddef.h>

int add_format_types(PyObject *m);

#endif