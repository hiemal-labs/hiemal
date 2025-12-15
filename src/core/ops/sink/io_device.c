#include <string.h>

#include "api/backend.h"
#include "api/buffer.h"
#include "api/device.h"
#include "ops_internal.h"

SINK_OP(io_device) {
  __SINK_IO_DEVICE_ARGS_UNPACK
  hm_backend_connection_t *backend = NULL;
  hm_device_io_connection_t *dest_connection = NULL;
  hm_backend_init(backend_name, &backend);
  default_device_io_connect(&dest_connection, backend, PLAYBACK);

  buffer_t *buf = NULL;
  buffer_init(&buf, n_bytes, LINEAR);
  buffer_write(buf, src, n_bytes);
  int bytes_written = device_io_write_ext(dest_connection, buf, n_bytes);

  buffer_delete(&buf);
  hm_backend_close(&backend);
  return bytes_written;
}