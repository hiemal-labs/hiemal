#include <stdio.h>
#include <stdlib.h>

#include <hiemal/ops.h>

// record 5 seconds of audio from the default recording device and send it to
// the default playback device
int main() {
  int sample_size = 2;
  int fs = 44.1e3;
  int n_channels = 2;
  int buffer_len = 10;
  int frame_size = sample_size * n_channels;
  int buffer_size = frame_size * buffer_len * fs;

  void *buf = calloc(buffer_size, 1);

  printf("Recording (5s)...\n");
  hm_source_io_device(buf, buffer_size, "pulseaudio");
  printf("Playback (5s)...\n");
  hm_sink_io_device(buf, buffer_size, "pulseaudio");

  free(buf);
  return 0;
}
