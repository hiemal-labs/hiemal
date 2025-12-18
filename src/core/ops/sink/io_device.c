#include <string.h>

#include "api/backend.h"
#include "api/buffer.h"
#include "api/device.h"
#include "ops_internal.h"

SINK_OP_INIT_FN(io_device) {
  __SINK_IO_DEVICE_ARGS_UNPACK(kwargs)
  __SINK_IO_DEVICE_STATE_UNPACK(state)
  state_backend = NULL;
  state_dest_connection = NULL;
  hm_backend_init(backend_name, &state_backend);
  default_device_io_connect(&state_dest_connection, state_backend, PLAYBACK);
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
  __SINK_IO_DEVICE_ARGS_UNPACK(sink_op->kwargs)
  return 0;
}