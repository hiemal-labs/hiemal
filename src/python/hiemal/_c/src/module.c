#include "intern/python.h"

static int hm_c_module_exec(PyObject *m) {
  add_format_types(m);

  PyObject *all = PyObject_Vectorcall((PyObject*)&PyList_Type, NULL, 0, NULL);
  PyList_Append(all, PyUnicode_FromString("Signature"));
  PyModule_Add(m, "__all__", (PyObject*)all);
  return 0;
}

static struct PyModuleDef_Slot hm_c_module_slots[] = {
  {Py_mod_exec, hm_c_module_exec},
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

static PyModuleDef hm_c_module_def = {
  .m_base = PyModuleDef_HEAD_INIT,
  .m_name = "hiemal._c",
  .m_slots = hm_c_module_slots
};

PyMODINIT_FUNC PyInit__c() {
  return PyModuleDef_Init(&hm_c_module_def);
}