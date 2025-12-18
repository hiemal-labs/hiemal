#include <string.h>

#include "api/backend.h"
#include "api/buffer.h"
#include "api/device.h"
#include "ops_internal.h"

SINK_OP_INIT_FN(io_device) {
  __SINK_IO_DEVICE_ARGS_UNPACK(kwargs)
  __SINK_IO_DEVICE_STATE_UNPACK(state)
  io_device_sink_state_t *op_state = (io_device_sink_state_t*)state;
  state_backend = NULL;
  state_dest_connection = NULL;
  hm_backend_init(backend_name, &state_backend);
  default_device_io_connect(&state_dest_connection, state_backend, PLAYBACK);
  op_state->backend = state_backend;
  op_state->dest_connection = state_dest_connection;
  return 0;
}

SINK_OP_FINI_FN(io_device) {
  __SINK_IO_DEVICE_STATE_UNPACK(state)
  hm_backend_close(&state_backend);
  return 0;
}

SINK_OP(io_device) {
  __SINK_IO_DEVICE_ARGS_UNPACK(kwargs)
  __SINK_IO_DEVICE_STATE_UNPACK(state)
  buffer_t *buf = NULL;
  buffer_init(&buf, n_bytes, LINEAR);
  buffer_write(buf, src, n_bytes);
  int bytes_written = device_io_write_ext(state_dest_connection, buf, n_bytes);

  buffer_delete(&buf);
  return bytes_written;
}

SINK_BYTES_WRITABLE_FN(io_device) {
  __SINK_IO_DEVICE_STATE_UNPACK(sink_op->state)
  return device_io_bytes_writable(state_dest_connection, bytes_writable);
  return 0;
}