#include "intern/python.h"

// device.Device

struct PyHmDeviceObject {
  PyObject_HEAD
};

static struct PyMemberDef hm_device_Device_members[] = {
  {NULL}
};

static PyType_Slot hm_device_Device_slots[] = {
  {0, NULL}
};

static PyType_Spec hm_device_Device_spec = {
  .name = "hm._c.device.Device",
  .slots = hm_device_Device_slots,
  .flags = Py_TPFLAGS_DEFAULT,
  .basicsize = sizeof(struct PyHmDeviceObject)
};

int add_device_types(PyObject *m) {
  PyTypeObject *Device_type = (PyTypeObject*)PyType_FromModuleAndSpec(m, &hm_device_Device_spec, NULL);
  PyModule_AddType(m, Device_type);
  return 0;
}