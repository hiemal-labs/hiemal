#include <stddef.h>

#include "test_pb_c_wrap.h"
#include "test_common.h"

#ifdef __cplusplus
#error this file is not meant to be compiled as a c++ file
#endif

TEST(buffer_pb_create) {
  int rc;
  void *pb_test = NULL;
  rc = hm_buffer_pb_create(&pb_test);
  ASSERT_TRUE(rc == 0);
  ASSERT_TRUE(pb_test != NULL);
  rc = hm_buffer_pb_delete(&pb_test);
  ASSERT_TRUE(rc == 0);
  ASSERT_TRUE(pb_test == NULL);
  return TEST_SUCCESS;
}