#include <stdlib.h>
#include <time.h>

#include "ops_internal.h"
#include "util/time.h"

#include "api/stream.h"

struct hm_stream {
  hm_source_op *src;
  hm_dsp_op *dsp;
  hm_sink_op *sink;
};

int hm_stream_init_empty(hm_stream** stream) {
  *stream = (hm_stream*)calloc(sizeof(hm_stream),1);
  return 0;
}

int hm_stream_init(hm_stream** stream, hm_source_op *src, hm_sink_op *sink, hm_dsp_op *dsp) {
  hm_stream_init_empty(stream);
  (*stream)->src = src;
  (*stream)->sink = sink;
  (*stream)->dsp = dsp;
  return 0;
}

int hm_stream_attach_source(hm_stream *stream, hm_source_op *src) {
  stream->src = src;
  return 0;
}

int hm_stream_attach_dsp(hm_stream *stream, hm_dsp_op *dsp) {
  stream->dsp = dsp;
  return 0;
}

int hm_stream_attach_sink(hm_stream *stream, hm_sink_op *sink) {
  stream->sink = sink;
  return 0;
}

int hm_stream_delete(hm_stream** stream) {
  if (*stream != NULL) {
    free(*stream);
    *stream = NULL;
    return 0;
  }
}

int hm_stream_run(hm_stream *stream, unsigned int msec) {
  hm_time_ms_t start_time = get_current_time();
  hm_source_op *src = stream->src;
  hm_dsp_op *dsp = stream->dsp;
  hm_sink_op *dest = stream->sink;
  unsigned long elapsed_time = 0;
  while (elapsed_time < msec) {
    size_t src_bytes = 0, dest_bytes = 0, bytes_available = 0;
    hm_op_source_bytes_readable(src, &src_bytes);
    hm_op_sink_bytes_writable(dest, &dest_bytes);
    bytes_available = (src_bytes < dest_bytes) ? src_bytes : dest_bytes;
    
    if (dsp == NULL) {
      void *buf = calloc(bytes_available, 1);
      hm_source_op_run(src, buf, bytes_available);
      hm_sink_op_run(dest, buf, bytes_available);
      free(buf);
    }
    else {
      void *src_buf = calloc(bytes_available, 1);
      void *dest_buf = calloc(bytes_available, 1);
      hm_source_op_run(src, src_buf, bytes_available);
      hm_dsp_op_run(dsp, src_buf, dest_buf, bytes_available);
      hm_sink_op_run(dest, dest_buf, bytes_available);
      free(src_buf);
      free(dest_buf);
    }
    sleep_ms(1);
    elapsed_time = elapsed_time_ms(&start_time);
  }
  return 0;
}