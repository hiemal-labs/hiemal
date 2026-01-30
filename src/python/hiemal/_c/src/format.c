#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <stddef.h>

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

// module

static int hm_format_module_exec(PyObject *m) {
  PyTypeObject *Signature_type = (PyTypeObject*)PyType_FromModuleAndSpec(m, &hm_format_Signature_spec, NULL);
  PyModule_AddType(m, Signature_type);

  PyObject *all = PyObject_Vectorcall((PyObject*)&PyList_Type, NULL, 0, NULL);
  PyList_Append(all, PyUnicode_FromString("Signature"));
  PyModule_Add(m, "__all__", (PyObject*)all);
  return 0;
}

static struct PyModuleDef_Slot hm_format_module_slots[] = {
  {Py_mod_exec, hm_format_module_exec},
#if PY_VERSION_HEX >= 0x030C0000
  {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#endif
#if PY_VERSION_HEX >= 0x030D0000
  {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
#if PY_VERSION_HEX >= 0x030E0000
  {Py_tp_token, Py_TP_USE_SPEC},
#endif
  {0, NULL}
};

static PyModuleDef hm_format_module_def = {
  .m_base = PyModuleDef_HEAD_INIT,
  .m_name = "hiemal._c.format",
  .m_slots = hm_format_module_slots
};

PyMODINIT_FUNC PyInit_format() {
  return PyModuleDef_Init(&hm_format_module_def);
}