#include "intern/python.h"

#include "api/buffer.h"

struct PyHmBufferObject {
  PyObject_HEAD
  buffer_t *buf;
};

int hm_buffer_Buffer_init(struct PyHmBufferObject *self, PyObject *args, PyObject *kwargs) {
  if (PyTuple_GET_SIZE(args) != 2) {
    PyErr_SetString(PyExc_TypeError, "Buffer.__init__() takes 3 positional arguments");
    return -1;
  }

  unsigned int n_bytes = 0;
  buffer_type_t buffer_type = RING;
  char* buffer_type_str = NULL;
  PyArg_ParseTuple(args, "Is", &n_bytes, &buffer_type_str);
  if Py_IsTrue((PyErr_Occurred())) {
    return -1;
  }
  if (strcmp(buffer_type_str, "RING") == 0) {
    buffer_type = RING;
  }
  else if (strcmp(buffer_type_str, "LINEAR") == 0) {
    buffer_type = LINEAR;
  }
  else {
    PyErr_SetString(PyExc_TypeError, "buffer_type must be either 'RING' or 'LINEAR'");
    return -1;
  }
  buffer_init(&(self->buf), n_bytes, buffer_type);
  return 0;
}

void hm_buffer_Buffer_dealloc(struct PyHmBufferObject *self) {
  if (self->buf != NULL) {
    buffer_delete(&(self->buf));
  }

  // Py_DECREF(type) is necessary for heap-based types according to
  // https://docs.python.org/3/howto/isolating-extensions.html#defining-tp-dealloc
  PyTypeObject *type = Py_TYPE(self);
  type->tp_free(self);
  Py_DECREF(type);
}

PyObject *hm_buffer_Buffer_get_bytes_readable(struct PyHmBufferObject *self, void*) {
  buffer_t *buf = self->buf;
  unsigned int bytes_readable = buffer_n_read_bytes(buf);
  return PyLong_FromUnsignedLong((unsigned long)bytes_readable);
}

PyObject *hm_buffer_Buffer_get_bytes_writable(struct PyHmBufferObject *self, void*) {
  buffer_t *buf = self->buf;
  unsigned int bytes_writable = buffer_n_write_bytes(buf);
  return PyLong_FromUnsignedLong((unsigned long)bytes_writable);
}

PyObject *hm_buffer_Buffer_size(struct PyHmBufferObject *self, void*) {
  buffer_t *buf = self->buf;
  unsigned int size = buffer_size(buf);
  return PyLong_FromUnsignedLong((unsigned long)size);
}

static PyGetSetDef hm_buffer_Buffer_get_setters[] = {
  {"bytes_readable", (getter)hm_buffer_Buffer_get_bytes_readable, NULL, NULL, NULL},
  {"bytes_writable", (getter)hm_buffer_Buffer_get_bytes_writable, NULL, NULL, NULL},
  {"size", (getter)hm_buffer_Buffer_size, NULL, NULL, NULL},
  {NULL}
};

static struct PyMemberDef hm_buffer_Buffer_members[] = {
  {NULL}
};

PyObject *hm_buffer_Buffer_array(PyObject *self, PyObject*, PyObject*) {
  npy_intp dims[] = {3,3};
  import_array();
  return PyArray_SimpleNew(2, dims, NPY_UINT8);
}

PyObject *hm_buffer_Buffer_clear(struct PyHmBufferObject *self) {
  buffer_reset(self->buf);
  Py_RETURN_NONE;
}

static struct PyMethodDef hm_buffer_Buffer_methods[] = {
  {"__array__", (PyCFunction)hm_buffer_Buffer_array, METH_VARARGS | METH_KEYWORDS, NULL},
  {"numpy", (PyCFunction)hm_buffer_Buffer_array, METH_NOARGS, NULL},
  {"clear", (PyCFunction)hm_buffer_Buffer_clear, METH_NOARGS, NULL},
  {NULL}
};

static PyType_Slot hm_buffer_Buffer_slots[] = {
  {Py_tp_members, hm_buffer_Buffer_members},
  {Py_tp_methods, hm_buffer_Buffer_methods},
  {Py_tp_getset, hm_buffer_Buffer_get_setters},
  {Py_tp_init, (initproc)hm_buffer_Buffer_init},
  {Py_tp_dealloc, (destructor)hm_buffer_Buffer_dealloc},
  {0, NULL}
};

static PyType_Spec hm_buffer_Buffer_spec = {
  .name = "hm._c.buffer.Buffer",
  .slots = hm_buffer_Buffer_slots,
  .flags = Py_TPFLAGS_BASETYPE,
  .basicsize = sizeof(struct PyHmBufferObject)
};

int add_buffer_types(PyObject *m) {
  PyTypeObject *Buffer_type = (PyTypeObject*)PyType_FromModuleAndSpec(m, &hm_buffer_Buffer_spec, NULL);
  PyModule_AddType(m, Buffer_type);
  return 0;
}