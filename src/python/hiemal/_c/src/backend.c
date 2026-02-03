#include "intern/python.h"

// backend.Backend

struct PyHmBackendObject {
  PyObject_HEAD
};

static struct PyMemberDef hm_backend_Backend_members[] = {
  {NULL}
};

static PyType_Slot hm_backend_Backend_slots[] = {
  {0, NULL}
};

static PyType_Spec hm_backend_Backend_spec = {
  .name = "hm._c.backend.Backend",
  .slots = hm_backend_Backend_slots,
  .flags = Py_TPFLAGS_DEFAULT,
  .basicsize = sizeof(struct PyHmBackendObject)
};

// backend.BackendList

struct PyHmBackendListObject {
  PyObject_HEAD
  Py_ssize_t n_items;
  struct PyHmBackendObject** backends;
};

Py_ssize_t hm_backend_BackendList_sq_len(struct PyHmBackendListObject *self) {
  return self->n_items;
}

PyObject *hm_backend_BackendList_sq_item(struct PyHmBackendListObject *self, Py_ssize_t idx) {
  if (idx < 0 || idx >= self->n_items) {
    PyErr_SetString(PyExc_IndexError, "bad index");
    return NULL;
  }
  return (PyObject*)(self->backends)[idx];
}

static struct PyMemberDef hm_backend_BackendList_members[] = {
  {NULL}
};

static PyType_Slot hm_backend_BackendList_slots[] = {
  {Py_sq_length, hm_backend_BackendList_sq_len},
  {0, NULL}
};

static PyType_Spec hm_backend_BackendList_spec = {
  .name = "hm._c.backend.BackendList",
  .slots = hm_backend_BackendList_slots,
  .flags = Py_TPFLAGS_DEFAULT,
  .basicsize = sizeof(struct PyHmBackendListObject)
};

int add_backend_types(PyObject *m) {
  PyTypeObject *Backend_type = (PyTypeObject*)PyType_FromModuleAndSpec(m, &hm_backend_Backend_spec, NULL);
  PyTypeObject *BackendList_type = (PyTypeObject*)PyType_FromModuleAndSpec(m, &hm_backend_BackendList_spec, NULL);
  PyModule_AddType(m, Backend_type);
  PyModule_AddType(m, BackendList_type);
  return 0;
}