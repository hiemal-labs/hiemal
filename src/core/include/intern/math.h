#ifndef HIEMAL_INTERN_MATH_H
#define HIEMAL_INTERN_MATH_H

#include <math.h>
#include <stdint.h>

// NOLINTNEXTLINE(readability-identifier-length)
double_t ddot(char mode, void *a, void *b, unsigned int len);
uint64_t u64sum(const uint64_t *buf, unsigned int len);

#endif