#include <string.h>

#include "api/backend.h"
#include "api/buffer.h"
#include "api/device.h"
#include "ops_internal.h"

SOURCE_OP_INIT_FN(io_device) {
  __SOURCE_IO_DEVICE_ARGS_UNPACK(kwargs)
  __SOURCE_IO_DEVICE_STATE_UNPACK(state)
  io_device_source_state_t *op_state = (io_device_source_state_t*)state;
  state_backend = NULL;
  state_src_connection = NULL;
  hm_backend_init(backend_name, &state_backend);
  default_device_io_connect(&state_src_connection, state_backend, RECORDING);
  op_state->backend = state_backend;
  op_state->src_connection = state_src_connection;
  return 0;
}

SOURCE_OP_FINI_FN(io_device) {
  __SOURCE_IO_DEVICE_STATE_UNPACK(state)
  hm_backend_close(&state_backend);
  return 0;
}

SOURCE_OP(io_device) {
  __SOURCE_IO_DEVICE_ARGS_UNPACK(kwargs)
  __SOURCE_IO_DEVICE_STATE_UNPACK(state)
  buffer_t *buf = NULL;
  buffer_init(&buf, n_bytes, LINEAR);

  device_io_read_ext(state_src_connection, buf, n_bytes);
  int bytes_read = buffer_read(buf, dest, n_bytes);

  buffer_delete(&buf);
  return bytes_read;
}

SOURCE_BYTES_READABLE_FN(io_device) {
  __SOURCE_IO_DEVICE_STATE_UNPACK(src_op->state)
  return device_io_bytes_readable(state_src_connection, bytes_readable);
}