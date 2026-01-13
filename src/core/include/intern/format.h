#ifndef HM_INTERN_FORMAT_H
#define HM_INTERN_FORMAT_H

#include <stdbool.h>

typedef enum {
  DEFAULT_FORMAT=0,
  S16LE,
  F32LE
} hm_format_type;

typedef struct {
  hm_format_type sample_format;
  unsigned int fs;
  unsigned int n_channels;
  bool interleaved;
} hm_format_signature;

size_t hm_format_convert(void *src, hm_format_signature *src_format, void *dest, hm_format_signature *dest_format, size_t n_bytes);

#endif