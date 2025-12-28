#include <limits.h>

#include "ops_internal.h"

SINK_OP_INIT_FN(file) {
  __SINK_FILE_ARGS_UNPACK(kwargs)
  __SINK_FILE_STATE_UNPACK(state)
  file_sink_state_t *op_state = (file_sink_state_t*)state;
  state_fp = fopen(filename, "wb");
  op_state->fp = state_fp;
  return 0;
}

SINK_OP_FINI_FN(file) {
  __SINK_FILE_STATE_UNPACK(state)
  fflush(state_fp);
  fclose(state_fp);
  return 0;
}

SINK_OP(file) {
  __SINK_FILE_ARGS_UNPACK(kwargs)
  __SINK_FILE_STATE_UNPACK(state)
  int bytes_written = fwrite(src, 1, n_bytes, state_fp);
  return bytes_written;
}

SINK_BYTES_WRITABLE_FN(file) {
  __SINK_FILE_STATE_UNPACK(sink_op->state)
  return INT_MAX;
  return 0;
}