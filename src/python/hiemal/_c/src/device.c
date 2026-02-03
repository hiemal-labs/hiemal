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

// device.DeviceList

struct PyHmDeviceListObject {
  PyObject_HEAD
  Py_ssize_t n_items;
  struct PyHmDeviceObject** devices;
};

Py_ssize_t hm_device_DeviceList_sq_len(struct PyHmDeviceListObject *self) {
  return self->n_items;
}

PyObject *hm_device_DeviceList_sq_item(struct PyHmDeviceListObject *self, Py_ssize_t idx) {
  if (idx < 0 || idx >= self->n_items) {
    PyErr_SetString(PyExc_IndexError, "bad index");
    return NULL;
  }
  return (PyObject*)(self->devices)[idx];
}

static struct PyMemberDef hm_device_DeviceList_members[] = {
  {NULL}
};

static PyType_Slot hm_device_DeviceList_slots[] = {
  {Py_sq_length, hm_device_DeviceList_sq_len},
  {0, NULL}
};

static PyType_Spec hm_device_DeviceList_spec = {
  .name = "hm._c.device.DeviceList",
  .slots = hm_device_DeviceList_slots,
  .flags = Py_TPFLAGS_DEFAULT,
  .basicsize = sizeof(struct PyHmDeviceListObject)
};

int add_device_types(PyObject *m) {
  PyTypeObject *Device_type = (PyTypeObject*)PyType_FromModuleAndSpec(m, &hm_device_Device_spec, NULL);
  PyTypeObject *DeviceList_type = (PyTypeObject*)PyType_FromModuleAndSpec(m, &hm_device_DeviceList_spec, NULL);
  PyModule_AddType(m, Device_type);
  PyModule_AddType(m, DeviceList_type);
  return 0;
}