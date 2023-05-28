#include <string.h>

#include "intern/backend.h"
#include "intern/device.h"

#ifdef WITH_PULSEAUDIO
#include "intern/pulse.h"
#endif

static const struct backend_info available_backends[] = {
  #ifdef WITH_PULSEAUDIO
  {"PULSEAUDIO",
    hm_pulse_connection_init,
    hm_pulse_connection_close,
    hm_pulse_dump_info,
    hm_pulse_io_connect_by_id},
    #endif
};

const struct backend_info *get_backend_by_name(char *name) {
  unsigned int n_backends = sizeof(available_backends) / sizeof(struct backend_info);
  int i = 0;
  for (i = 0; i < n_backends; i++) {
    if (strcmp(name, available_backends[i].name) == 0) {
      return &(available_backends[i]);
    }
  }
  return NULL;
}

int hm_backend_init(char *name, hm_backend_connection_t **backend_ptr) {
  const struct backend_info *info = get_backend_by_name(name);
  if (info == NULL) {
    return -1;
  }

  return (info->init_fn)(backend_ptr);
}

int hm_backend_close(hm_backend_connection_t **backend_ptr) {
  char *name = (*backend_ptr)->backend_name;
  const struct backend_info *info = get_backend_by_name(name);
  if (info == NULL) {
    return -1;
  }
  return (info->delete_fn)(backend_ptr);
}