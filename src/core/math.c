#include "intern/math.h"

#include <math.h>
#include <stdint.h>

// NOLINTNEXTLINE(readability-identifier-length)
double_t ddot(char mode, void *a, void *b, unsigned int len) {
  unsigned int arr_i = 0;
  double_t res = 0;
  if (mode == 'f') {
    for (arr_i = 0; arr_i < len; arr_i++) {
      res += (((double_t *)a)[arr_i] * ((double_t *)b)[len - (arr_i + 1)]);
    }
  } else {
    for (arr_i = 0; arr_i < len; arr_i++) {
      res += (((double_t *)a)[arr_i] * ((double_t *)b)[arr_i]);
    }
  }
  return res;
}

uint64_t u64sum(const uint64_t *buf, unsigned int len) {
  unsigned int arr_i = 0;
  uint64_t res = 0;
  for (arr_i = 0; arr_i < len; arr_i++) {
    res += buf[arr_i];
  }
  return res;
}