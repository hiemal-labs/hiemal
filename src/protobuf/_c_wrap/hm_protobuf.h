#ifndef _HM_PROTOBUF_H
#define _HM_PROTOBUF_H

#ifdef __cplusplus
extern "C" {
#endif

int hm_buffer_pb_create(void **buf_pb);
int hm_buffer_pb_delete(void **buf_pb);

#ifdef __cplusplus
}
#endif

#endif
