#include "hm_protobuf.h"

#include "buffer.pb.h"

// create C entry point for interacting with protocol buffers
extern "C" {



int hm_buffer_pb_create(void **buf_pb) {
  hiemal::buffer *buf = new hiemal::buffer;
  *buf_pb = (void*)buf;
  return 0;
}

int hm_buffer_pb_delete(void **buf_pb) {
  hiemal::buffer *buf = (hiemal::buffer*)(*buf_pb);
  delete buf;
  *buf_pb = NULL;
  return 0;
}

}
