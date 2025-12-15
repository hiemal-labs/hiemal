#include <string.h>

#include "api/backend.h"
#include "api/buffer.h"
#include "api/device.h"
#include "ops_internal.h"

SOURCE_OP(io_device) {
  __SOURCE_IO_DEVICE_ARGS_UNPACK
  hm_backend_connection_t *backend = NULL;
  hm_device_io_connection_t *src_connection = NULL;
  hm_backend_init(backend_name, &backend);
  default_device_io_connect(&src_connection, backend, RECORDING);

  buffer_t *buf = NULL;
  buffer_init(&buf, n_bytes, LINEAR);

  device_io_read_ext(src_connection, buf, n_bytes);
  int bytes_read = buffer_read(buf, dest, n_bytes);

  buffer_delete(&buf);
  hm_backend_close(&backend);
  return bytes_read;
}