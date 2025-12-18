#ifndef HM_STREAM_H
#define HM_STREAM_H

typedef struct hm_stream hm_stream;

int hm_stream_init_empty(hm_stream** stream);
int hm_stream_init(hm_stream** stream, hm_source_op *src, hm_sink_op *sink, hm_dsp_op *dsp);
int hm_stream_attach_source(hm_stream *stream, hm_source_op *src);
int hm_stream_attach_dsp(hm_stream *stream, hm_dsp_op *dsp);
int hm_stream_attach_sink(hm_stream *stream, hm_sink_op *sink);
int hm_stream_delete(hm_stream** stream);
int hm_stream_run(hm_stream *stream, unsigned int msec);

#endif