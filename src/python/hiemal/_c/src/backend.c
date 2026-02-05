#include "intern/python.h"

// backend.Backend
PyObject *hm_backend_Backend_tp_str(struct PyHmBackendObject *self) {
  PyObject *str = PyUnicode_FromFormat("Backend(name=%s)", self->name);
  return str;
}

static struct PyMemberDef hm_backend_Backend_members[] = {
  {NULL}
};

static PyType_Slot hm_backend_Backend_slots[] = {
  {Py_tp_str, (reprfunc)hm_backend_Backend_tp_str},
  {Py_tp_repr, (reprfunc)hm_backend_Backend_tp_str},
  {0, NULL}
};

static PyType_Spec hm_backend_Backend_spec = {
  .name = "hm._c.backend.Backend",
  .slots = hm_backend_Backend_slots,
  .flags = Py_TPFLAGS_DEFAULT,
  .basicsize = sizeof(struct PyHmBackendObject)
};

// backend.BackendList
PyObject *hm_backend_BackendList_tp_str(struct PyHmBackendListObject *self) {
  if (self->n_items == 0) {
    return PyUnicode_FromString("BackendList([])");
  }
  
  PyObject *str = PyUnicode_FromFormat("BackendList([");
  int itr = 0;
  for (itr = 0; itr < self->n_items; itr++) {
    const char *format_str = (itr == self->n_items - 1) ? "%s" : "%s,";
    PyUnicode_AppendAndDel(&str, PyUnicode_FromFormat(format_str, self->backends[itr]->name));
  }
  PyUnicode_AppendAndDel(&str, PyUnicode_FromFormat("],default=%s)", self->backends[0]->name));
  return str;
}

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

void hm_backend_BackendList_dealloc(struct PyHmBackendListObject *self) {
  // if (self->buf != NULL) {
  //   buffer_delete(&(self->buf));
  // }

  int itr = 0;
  for (itr = 0; itr < self->n_items; itr++) {
    Py_DECREF(self->backends[itr]);
  }
  PyMem_Free(self->backends);

  // Py_DECREF(type) is necessary for heap-based types according to
  // https://docs.python.org/3/howto/isolating-extensions.html#defining-tp-dealloc
  PyTypeObject *type = Py_TYPE(self);
  type->tp_free(self);
  Py_DECREF(type);
}

static struct PyMemberDef hm_backend_BackendList_members[] = {
  {NULL}
};

static PyType_Slot hm_backend_BackendList_slots[] = {
  {Py_tp_str, (reprfunc)hm_backend_BackendList_tp_str},
  {Py_tp_repr, (reprfunc)hm_backend_BackendList_tp_str},
  {Py_sq_length, hm_backend_BackendList_sq_len},
  {Py_sq_item, hm_backend_BackendList_sq_item},
  {0, NULL}
};

static PyType_Spec hm_backend_BackendList_spec = {
  .name = "hm._c.backend.BackendList",
  .slots = hm_backend_BackendList_slots,
  .flags = Py_TPFLAGS_DEFAULT,
  .basicsize = sizeof(struct PyHmBackendListObject)
};

int init_backends(PyObject *m) {
  PyTypeObject *Backend_type = (PyTypeObject*)PyObject_GetAttrString(m, "Backend");
  PyTypeObject *BackendList_type = (PyTypeObject*)PyObject_GetAttrString(m, "BackendList");
  struct PyHmBackendListObject *backend_list = (struct PyHmBackendListObject*)PyObject_Vectorcall((PyObject*)BackendList_type, NULL, 0, NULL);
  PyModule_AddObject(m, "backends", (PyObject*)backend_list);
  const char **backend_names = hm_backend_list();
  const char *backend_itr = *backend_names;
  unsigned int n_backends = 0;
  while (backend_itr != NULL) {
    n_backends++;
    backend_itr = backend_names[n_backends];
  }
  struct PyHmBackendObject **backends = PyMem_Malloc(n_backends * sizeof(struct PyHmBackendObject*));
  int itr = 0;
  for (itr = 0; itr < n_backends; itr++) {
    backends[itr] = (struct PyHmBackendObject*)PyObject_Vectorcall((PyObject*)Backend_type, NULL, 0, NULL);
    backends[itr]->name = backend_names[itr];
  }
  backend_list->backends = backends;
  backend_list->n_items = n_backends;
  return 0;
}

int add_backend_types(PyObject *m) {
  PyTypeObject *Backend_type = (PyTypeObject*)PyType_FromModuleAndSpec(m, &hm_backend_Backend_spec, NULL);
  PyTypeObject *BackendList_type = (PyTypeObject*)PyType_FromModuleAndSpec(m, &hm_backend_BackendList_spec, NULL);
  PyModule_AddType(m, Backend_type);
  PyModule_AddType(m, BackendList_type);
  return 0;
}