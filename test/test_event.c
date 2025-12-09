#include "test_common.h"

#include <stdlib.h>
#include <string.h>

#include "intern/buffer.h"
#include "intern/event.h"
#include "intern/thread.h"

TEST(event_buffer) {
  unsigned int n_cb_calls = 0;
  hm_event_t *e = NULL;
  buffer_t *buf = NULL;
  int event_cond(hm_event_t *e, void *event_info) {
    n_cb_calls++;
    buffer_io_event_t *_event_info = (buffer_io_event_t*)event_info;
    if (_event_info->n_bytes_readable > _event_info->n_bytes_writeable) {
      return 0;
    } else {
      return 1;
    }
  }
  void *buf_write_thread(void *args) {
    while (buf->event_list->n_items == 0);
    int i = 0;
    unsigned char *ones = (unsigned char*)malloc(sizeof(unsigned char)*0x500);
    memset(ones, 1, 0x500);
    for (i = 0; i < 2; i++) {
      buffer_write(buf, ones, 0x500);
    }
    free(ones);
    return NULL;
  }
  buffer_init(&buf, 0x1000, RING);
  hm_event_init(&e, OBJ_BUFFER, event_cond);
  buffer_event_add(buf, e);
  HM_THREAD_CREATE(buf_write_thread, NULL)
  hm_event_poll(e);
  thread_join_all();
  ASSERT_TRUE(n_cb_calls == 2);
  hm_event_delete(&e);
  buffer_delete(&buf);
  return TEST_SUCCESS;
}