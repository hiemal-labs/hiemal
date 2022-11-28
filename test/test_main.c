// This file was generated automatically with util/gendriver.py


#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "test_common.h"

TEST(async_loop_init_delete);
TEST(ring_buffer_init);
TEST(buffer_delete);
TEST(rbuf_reset);
TEST(rbuf_rw_arithmetic);
TEST(rbuf_write);
TEST(rbuf_read);
TEST(shift_down_64);
TEST(shift_up_64);
TEST(fir_filter);
TEST(dct_type_II);
TEST(src_fd);
TEST(sink_fd);
TEST(src_file);
TEST(sink_file);
TEST(math_blas_ddot);
TEST(buffer_pb_create);

struct _test_info {
  char *test_name;
  enum {OK=0,FAILED,SKIPPED} result;
  double elapsed_time;
  bool completed;
};

struct _testing_results {
  unsigned int n_tests;
  struct _test_info *tests;
};

typedef struct _test_info test_info_t;
typedef struct _testing_results testing_results_t;

int print_results_json(char *filename, const testing_results_t *results) {
  FILE *fp = fopen(filename, "w");
  fwrite("{\n", 1, 2, fp);
  fwrite("  \"results\":\n", 1, 13, fp);
  int i;
  // TODO: calculate the exact number of bytes to allocate
  char *line_buf = (char*)malloc(256);
  for (i = 0; i < results->n_tests; i++) {
    test_info_t *current_test = &(results->tests[i]);
    memset(line_buf, 0, 256);
    sprintf(line_buf, "    %s{\"id\": %d, \"name\": \"%s\", \"result\": \"%s\", \"elapsed_time\": %.2f}%s", \
      (i==0) ? "[" : "", i+1, current_test->test_name, (current_test->result == OK) ? "OK" : "FAILED", \
      current_test->elapsed_time, ((i+1)==results->n_tests) ? "]" : ",\n");
    fwrite(line_buf, 1, strlen(line_buf), fp);
  }
  free(line_buf);
  fwrite("\n}\n", 1, 3 ,fp);
  fclose(fp);
  return 0;
}

int main() {
  testing_results_t results;
  results.n_tests = 17;
  results.tests = (test_info_t*)malloc(results.n_tests*sizeof(test_info_t));
  int (*test_list[])() = {test_async_loop_init_delete, test_ring_buffer_init, test_buffer_delete, test_rbuf_reset, test_rbuf_rw_arithmetic, test_rbuf_write, test_rbuf_read, test_shift_down_64, test_shift_up_64, test_fir_filter, test_dct_type_II, test_src_fd, test_sink_fd, test_src_file, test_sink_file, test_math_blas_ddot, test_buffer_pb_create};
  char *test_names[] = {"async_loop_init_delete", "ring_buffer_init", "buffer_delete", "rbuf_reset", "rbuf_rw_arithmetic", "rbuf_write", "rbuf_read", "shift_down_64", "shift_up_64", "fir_filter", "dct_type_II", "src_fd", "sink_fd", "src_file", "sink_file", "math_blas_ddot", "buffer_pb_create"};
  int i, rc;
  unsigned int n_success = 0;
  printf("Running %d tests\n", results.n_tests);
  for (i = 0; i < results.n_tests; i++) {
    test_info_t *current_test = &(results.tests[i]);
    printf("Test %d/%d (%s):", i+1, results.n_tests, test_names[i]);
    fflush(stdout);
    time_t start_time = time(NULL);
    rc = (*test_list[i])();
    time_t end_time = time(NULL);
    current_test->test_name = test_names[i];
    current_test->result = (rc == TEST_SUCCESS) ? OK : FAILED;
    if (rc == TEST_SUCCESS) {
      n_success++;
    }
    current_test->elapsed_time = difftime(end_time, start_time);
    current_test->completed = true;
    printf(" %s (%.2f seconds)\n",  \
      (rc == TEST_SUCCESS ? "\e[1;32mOK\e[0m" : "\e[1;31mFAILED\e[0m"), \
      current_test->elapsed_time);
  }
  printf("%d/%d tests passed\n", n_success, results.n_tests);
  print_results_json("test_results.json", &results);
  free(results.tests);
  return (n_success == results.n_tests) ? 0 : 1;
}
