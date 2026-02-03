#include "intern/python.h"
#include "intern/format.h"

// format.Signature

struct PyHmSignatureObject {
  PyObject_HEAD
  hm_format_signature sig;
};

static struct PyMemberDef hm_format_Signature_members[] = {
  {"fs",  Py_T_UINT, offsetof(struct PyHmSignatureObject, sig.fs), 0, PyDoc_STR("Sampling frequency")},
  {"n_channels",  Py_T_UINT, offsetof(struct PyHmSignatureObject, sig.n_channels), 0, PyDoc_STR("Number of channels")},
  {"interleaved",  Py_T_BOOL, offsetof(struct PyHmSignatureObject, sig.interleaved), 0, PyDoc_STR("Whether or not data is interleaved")},
  {NULL}
};
 
PyObject *hm_format_Signature_tp_str(struct PyHmSignatureObject *self) {
  return PyUnicode_FromFormat("Signature(fs=%u, n_channels=%u, interleaved=%s)",
    self->sig.fs, self->sig.n_channels, self->sig.interleaved ? "True" : "False");
}

int hm_format_Signature_tp_init(struct PyHmSignatureObject *self, PyObject *args, PyObject *kwargs) {
  if (PyTuple_GET_SIZE(args) != 3) {
    PyErr_SetString(PyExc_TypeError, "Signature.__init__() takes 4 positional arguments");
    return -1;
  }
  PyArg_ParseTuple(args, "IIp", &(self->sig.fs), &(self->sig.n_channels), &(self->sig.interleaved));
  return 0;
}

static PyType_Slot hm_format_Signature_slots[] = {
  {Py_tp_members, hm_format_Signature_members},
  {Py_tp_str, (reprfunc)hm_format_Signature_tp_str},
  {Py_tp_repr, (reprfunc)hm_format_Signature_tp_str},
  {Py_tp_init, (initproc)hm_format_Signature_tp_init},
  {0, NULL}
};

static PyType_Spec hm_format_Signature_spec = {
  .name = "hm._c.format.Signature",
  .slots = hm_format_Signature_slots,
  .flags = Py_TPFLAGS_BASETYPE,
  .basicsize = sizeof(struct PyHmSignatureObject)
};

int add_format_types(PyObject *m) {
  PyTypeObject *Signature_type = (PyTypeObject*)PyType_FromModuleAndSpec(m, &hm_format_Signature_spec, NULL);
  PyModule_AddType(m, Signature_type);
  return 0;
}