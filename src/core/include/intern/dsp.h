#ifndef _DSP_H
#define _DSP_H

#include <math.h>

#include "buffer.h"
int _buf_shift_down_64(void *buf, const unsigned int n_bytes, 
    const unsigned int n_shift);

int _buf_shift_up_64(void *buf, const unsigned int n_bytes, 
    const unsigned int n_shift);

int fir_filter(double_t *src, double_t *dest, double_t *b, \
    unsigned long int b_order, int n_samples);

int dct_2_64f(buffer_t *src, void *dest, unsigned int n_samples);
#endif