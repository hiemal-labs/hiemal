#include <stdlib.h>

#include "ops.h"

#include "test_common.h"

TEST(fir_filter_op) {
  int rc;
  unsigned int n_samples = 6;
  unsigned int n_bytes = n_samples * sizeof(double);
  long unsigned int b_order = 3;
  double signal[] = {1,2,3,4,5,6};
  double filtered_signal_ref[] = {0.1, 0.5, 2.4, 4.3, 6.2, 8.1};
  double b[] = {0.1, 0.3, 1.5};
  double filtered_signal[6];
  double tol = 1e-5;
  {
  double filtered_signal[6] = {0};
  int rc = hm_dsp_fir_filter(signal, filtered_signal, n_bytes, b, b_order);
  ASSERT_TRUE(rc == n_bytes);
  ASSERT_ALMOST_EQUAL_1D(filtered_signal, 
    filtered_signal_ref,n_samples,tol);
  }

  // test result using hm_op_run
  {
  double filtered_signal[6] = {0};
  hm_dsp_op *fir_op = hm_dsp_fir_filter_op(HM_FORMAT_DEFAULT, HM_FORMAT_DEFAULT, b, b_order);
  int rc = hm_dsp_op_run(fir_op, signal, filtered_signal, n_bytes);
  ASSERT_TRUE(rc == n_bytes);
  ASSERT_ALMOST_EQUAL_1D(filtered_signal, 
    filtered_signal_ref,n_samples,tol);
  hm_dsp_op_delete(fir_op);
  }
  return TEST_SUCCESS;
}