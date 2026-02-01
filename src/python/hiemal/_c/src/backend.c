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

int add_backend_types(PyObject *m) {
  PyTypeObject *Backend_type = (PyTypeObject*)PyType_FromModuleAndSpec(m, &hm_backend_Backend_spec, NULL);
  PyModule_AddType(m, Backend_type);
  return 0;
}