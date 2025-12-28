#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#include <sys/ioctl.h>
#endif

#include "ops_internal.h"

SOURCE_OP_INIT_FN(file) {
  __SOURCE_FILE_ARGS_UNPACK(kwargs)
  __SOURCE_FILE_STATE_UNPACK(state)
  file_source_state_t *op_state = (file_source_state_t*)state;
  state_fp = fopen(filename, "rb");
  op_state->fp = state_fp;
  return 0;
}

SOURCE_OP_FINI_FN(file) {
  __SOURCE_FILE_STATE_UNPACK(state)
  fflush(state_fp);
  fclose(state_fp);
  return 0;
}

SOURCE_OP(file) {
  __SOURCE_FILE_ARGS_UNPACK(kwargs)
  __SOURCE_FILE_STATE_UNPACK(state)
  int bytes_read = fread(dest, 1, n_bytes, state_fp);
  return bytes_read;
}

SOURCE_BYTES_READABLE_FN(file) {
  __SOURCE_FILE_STATE_UNPACK(src_op->state)
#ifdef __linux__
  int fd = fileno(state_fp);
  return ioctl(fd, FIONREAD, bytes_readable);
#endif
}