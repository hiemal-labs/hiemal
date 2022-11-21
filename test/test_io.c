#include <stdio.h>
#include <string.h>
#include "test_io.h"
#include "test_common.h"

TEST(src_fd) {
  // set up
  FILE *tmp_file = tmpfile();
  unsigned int n_bytes = 5;
  char test_data[] = "12345";
  fwrite(test_data, 1, n_bytes, tmp_file);
  rewind(tmp_file);
  int test_fd = fileno(tmp_file);

  // test
  int rc;
  char buf[5];
  ASSERT_TRUE(test_fd > 0);
  rc = src_fd(test_fd, n_bytes, buf);
  ASSERT_TRUE(rc == 0);
  ASSERT_TRUE(memcmp(buf, test_data, n_bytes) == 0);

  // tear down
  fclose(tmp_file);
  return TEST_SUCCESS;
}

TEST(sink_fd) {
  // set up
  FILE *tmp_file = tmpfile();
  int test_fd = fileno(tmp_file);
  unsigned int n_bytes = 5;
  char test_data[] = "12345";

  // test
  int rc;
  rc = sink_fd(test_fd, n_bytes, test_data);
  ASSERT_TRUE(rc == 0);
  rewind(tmp_file);
  char buf[5];
  fread(buf, 1, n_bytes, tmp_file);
  ASSERT_TRUE(memcmp(buf, test_data, n_bytes) == 0);

  // tear down
  fclose(tmp_file);
  return TEST_SUCCESS;
}

TEST(src_file) {
  // set up
  FILE *tmp_file = tmpfile();
  unsigned int n_bytes = 5;
  char test_data[] = "12345";
  fwrite(test_data, 1, n_bytes, tmp_file);
  rewind(tmp_file);

  // test
  int rc;
  char buf[5];
  rc = src_file(tmp_file, n_bytes, buf);
  ASSERT_TRUE(rc == 0);
  ASSERT_TRUE(memcmp(buf, test_data, n_bytes) == 0);

  // tear down
  fclose(tmp_file);
  return TEST_SUCCESS;
}

TEST(sink_file) {
  // set up
  FILE *tmp_file = tmpfile();
  unsigned int n_bytes = 5;
  char test_data[] = "12345";

  // test
  int rc;
  rc = sink_file(tmp_file, n_bytes, test_data);
  ASSERT_TRUE(rc == 0);
  rewind(tmp_file);
  char buf[5];
  fread(buf, 1, n_bytes, tmp_file);
  ASSERT_TRUE(memcmp(buf, test_data, n_bytes) == 0);

  // tear down
  fclose(tmp_file);
  return TEST_SUCCESS;
}
