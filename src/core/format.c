#include <inttypes.h>
#include <string.h>

#include "intern/common.h"
#include "intern/format.h"



size_t hm_bytes_per_sample(hm_format_type sample_format) {
  switch (sample_format) {
    case S16LE:
    return 2;

    case F32LE:
    return 4;
  }
}

size_t hm_bytes_per_frame(hm_format_signature *format_signature) {
  return hm_bytes_per_sample(format_signature->sample_format) * format_signature->n_channels;
}

size_t hm_bytes_per_second(hm_format_signature *format_signature) {
  return hm_bytes_per_frame(format_signature) * format_signature->fs;
}

static bool is_float_type(hm_format_type sample_format) {
  switch(sample_format) {
    case S16LE:
    return false;
    
    case F32LE:
    return true;
  }
}

static bool is_int_type(hm_format_type sample_format) {
  switch(sample_format) {
    case S16LE:
    return true;
    
    case F32LE:
    return false;
  }
}

static float format_convert_s16le_f32le(int16_t sample) {
  return ((float)sample)/0x8000;
}

static int16_t format_convert_f32le_s16le(float sample) {
  return (int16_t)(sample*0x8000);
}

size_t hm_format_convert(void *src, hm_format_signature *src_format, void *dest, hm_format_signature *dest_format, size_t n_bytes) {
  int dest_src_sample_ratio = hm_bytes_per_sample(dest_format->sample_format) / hm_bytes_per_sample(src_format->sample_format);
  size_t bytes_to_write = n_bytes * dest_src_sample_ratio;
  size_t n_bytes_written = 0;
  void *src_ptr = src;
  void *dest_ptr = dest;
  if (src_format->sample_format == dest_format->sample_format) {
    memcpy(dest, src, n_bytes);
    return n_bytes;
  }
  while (n_bytes_written < bytes_to_write) {
    if (src_format->sample_format == S16LE && dest_format->sample_format == F32LE) {
      *(float*)dest_ptr = format_convert_s16le_f32le(*(int16_t*)src_ptr);
    }
    else if (src_format->sample_format == F32LE && dest_format->sample_format == S16LE) {
      *(int16_t*)dest_ptr = format_convert_f32le_s16le(*(float*)src_ptr);
    }
    else {
      hm_error_unreachable(__func__);
    }
    n_bytes_written += hm_bytes_per_sample(dest_format->sample_format);
    dest_ptr += hm_bytes_per_sample(dest_format->sample_format);
    src_ptr += hm_bytes_per_sample(src_format->sample_format);
  }
  return n_bytes_written;
}