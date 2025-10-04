#include "intern/common.h"
#include "intern/buffer.h"
#include "intern/pulse.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  HM_DEVICE_HEAD
  int pulse_card_id;
} hm_pulse_device_t;

typedef struct {
  pa_threaded_mainloop *mainloop;
  pa_context *hm_context;
  pa_server_info *server_info;
} hm_pulse_handle_t;

typedef struct {
  hm_pulse_device_t **dev;
  int dev_i;
  hm_pulse_handle_t *pulse_handle;
} indexed_dev_handle;

struct hm_pulse_io_node {
  HM_LIST_NODE_HEAD(struct hm_pulse_io_node)
  hm_device_io_t *io_device;
  int card_id;
};

struct hm_pulse_io_list {
  HM_LIST_HEAD(struct hm_pulse_io_node)
  hm_pulse_handle_t *pulse_handle;
};

struct int_with_handle {
  int i;
  hm_pulse_handle_t *pulse_handle;
};

int get_dev_io_id(struct hm_pulse_io_list *l, const char *name) {
  struct hm_pulse_io_node *node_itr = l->head;
  int i = 0;
  while (node_itr != NULL) {
    if (strcmp(node_itr->io_device->name, name) == 0) {
      return i;
    }
    i++;
    node_itr = node_itr->next;
  }
  return -1;
}

int get_index_from_card_id(hm_pulse_device_t **cards, int card_id, int n_cards) {
  int i = 0;
  for (i = 0; i < n_cards; i++) {
    if ((cards[i])->pulse_card_id == card_id) return i;
  }
  return -1;
}

int io_list_to_array(struct hm_pulse_io_list *l, hm_backend_connection_t *pulse_backend) {
  int n_cards = pulse_backend->n_devices;
  int *card_io_count = (int*)calloc(n_cards,sizeof(int));
  struct hm_pulse_io_node *node_itr = l->head;
  int i = 0;
  for (i = 0; i < l->n_items; i++) {
    card_io_count[node_itr->card_id] += 1;
    node_itr = node_itr->next;
  }

  for (i = 0; i < n_cards; i++) {
    (pulse_backend->devices[i])->n_io_devices = 0;
    (pulse_backend->devices[i])->io_devices = (hm_device_io_t**)malloc(card_io_count[i]*sizeof(hm_device_io_t*));
  }

  node_itr = l->head;
  hm_pulse_device_t *cur_device = NULL;
  for (i = 0; i < l->n_items; i++) {
    int card_index = get_index_from_card_id((hm_pulse_device_t**)(pulse_backend->devices), node_itr->card_id, n_cards);
    cur_device = (hm_pulse_device_t*)((pulse_backend->devices)[card_index]);
    (cur_device->io_devices)[cur_device->n_io_devices] = node_itr->io_device;
    cur_device->n_io_devices += 1;
    node_itr = node_itr->next;
  }
  free(card_io_count);
  return 0;
}

void _hm_pulse_context_state_cb(pa_context *c, void *userdata) {
  pa_threaded_mainloop *m = (pa_threaded_mainloop*)userdata;
  pa_context_state_t state = pa_context_get_state(c);
  if (state == PA_CONTEXT_READY || state == PA_CONTEXT_TERMINATED || state == PA_CONTEXT_FAILED)
    pa_threaded_mainloop_signal(m, 0);
}

void _hm_pulse_server_info_cb(pa_context *c, const pa_server_info *i, void *userdata) {
  hm_pulse_handle_t *hm_pulse = (hm_pulse_handle_t*)userdata;
  pa_threaded_mainloop *m = hm_pulse->mainloop;
  hm_pulse->server_info = (pa_server_info*)i;
  pa_threaded_mainloop_signal(m, 0);
}

void _hm_pulse_card_list_cb (pa_context *c, const pa_card_info *i, int eol, void *userdata) {
  indexed_dev_handle *indexed_dev = (indexed_dev_handle*)userdata;
  if (eol > 0) {
    pa_threaded_mainloop *m = indexed_dev->pulse_handle->mainloop;
    pa_threaded_mainloop_signal(m, 0);
    return;
  }
  hm_pulse_device_t **dev = indexed_dev->dev + indexed_dev->dev_i;
  *dev = (hm_pulse_device_t*)calloc(sizeof(hm_pulse_device_t), 1);
  (*dev)->name = strdup(i->name);
  (*dev)->io_backend = PULSEAUDIO;
  (*dev)->pulse_card_id = i->index;
  (indexed_dev->dev_i)++;
}

void _hm_pulse_source_list_cb (pa_context *c, const pa_source_info *i, int eol, void *userdata) {
  struct hm_pulse_io_list *list = (struct hm_pulse_io_list*)userdata;
  if (eol > 0) {
    pa_threaded_mainloop *m = list->pulse_handle->mainloop;
    pa_threaded_mainloop_signal(m, 0);
    return;
  }
  struct hm_pulse_io_node *new_node = (struct hm_pulse_io_node*)malloc(sizeof(struct hm_pulse_io_node));
  HM_LIST_NODE_INIT(new_node)
  new_node->io_device = (hm_device_io_t*)malloc(sizeof(hm_device_io_t));
  new_node->io_device->name = strdup(i->name);
  new_node->io_device->type = RECORDING;
  new_node->card_id = i->card;
  hm_list_append(list, new_node);
}

void _hm_pulse_sink_list_cb (pa_context *c, const pa_sink_info *i, int eol, void *userdata) {
  struct hm_pulse_io_list *list = (struct hm_pulse_io_list*)userdata;
  if (eol > 0) {
    pa_threaded_mainloop *m = list->pulse_handle->mainloop;
    pa_threaded_mainloop_signal(m, 0);
    return;
  }
  struct hm_pulse_io_node *new_node = (struct hm_pulse_io_node*)malloc(sizeof(struct hm_pulse_io_node));
  HM_LIST_NODE_INIT(new_node)
  new_node->io_device = (hm_device_io_t*)malloc(sizeof(hm_device_io_t));
  new_node->io_device->name = strdup(i->name);
  new_node->io_device->type = PLAYBACK;
  new_node->card_id = i->card;
  hm_list_append(list, new_node);
}

void _hm_pulse_n_cards_cb (pa_context *c, const pa_card_info *i, int eol, void *userdata) {
  struct int_with_handle *ii = (struct int_with_handle*)userdata;
  if (eol > 0) {
    pa_threaded_mainloop *m = ii->pulse_handle->mainloop;
    pa_threaded_mainloop_signal(m, 0);
    return;
  }
  (ii->i)++;
}

int hm_pulse_io_write(hm_device_io_t *io, buffer_t *buf, unsigned int n_bytes) {
  pa_stream *s = (pa_stream*)(io->backend_handle);
  void *pa_buf = malloc(n_bytes);
  buffer_read(buf, pa_buf, n_bytes);
  pa_stream_write(s, pa_buf, n_bytes, NULL, 0, PA_SEEK_RELATIVE);
  free(pa_buf);
  return 0;
}

int hm_pulse_io_read(hm_device_io_t *io, buffer_t *buf, unsigned int n_bytes)  {
  return 0;
}

int hm_pulse_n_cards(hm_backend_connection_t *pulse_backend) {
  struct int_with_handle n_cards;
  n_cards.i = 0;
  n_cards.pulse_handle = (hm_pulse_handle_t*)(pulse_backend->backend_handle);
  hm_pulse_handle_t *pulse_handle = (hm_pulse_handle_t*)(pulse_backend->backend_handle);
  pa_context *hm_pa_context = (pa_context*)(pulse_handle->hm_context);
  pa_operation *o = 
    pa_context_get_card_info_list(hm_pa_context, _hm_pulse_n_cards_cb, (void*)(&n_cards));
  pa_threaded_mainloop *m = pulse_handle->mainloop;
  while(pa_operation_get_state(o) != PA_OPERATION_DONE)
    pa_threaded_mainloop_wait(m);
  pa_operation_unref(o);
  return n_cards.i;
}

int hm_pulse_connection_init(hm_backend_connection_t **pulse_backend) {
  pa_threaded_mainloop *m = pa_threaded_mainloop_new();
  pa_mainloop_api *m_api = pa_threaded_mainloop_get_api(m);
  pa_context *hm_pa_context = pa_context_new(m_api, "hiemal");
  pa_context_set_state_callback(hm_pa_context, _hm_pulse_context_state_cb, (void*)m);
  pa_context_connect(hm_pa_context, NULL, 0, NULL);
  pa_threaded_mainloop_lock(m);
  pa_threaded_mainloop_start(m);
  while(pa_context_get_state(hm_pa_context) != PA_CONTEXT_READY)
    pa_threaded_mainloop_wait(m);
  *pulse_backend = (hm_backend_connection_t*)malloc(sizeof(hm_backend_connection_t));
  hm_pulse_handle_t *pulse_handle = 
    (hm_pulse_handle_t*)malloc(sizeof(hm_pulse_handle_t));
  pulse_handle->mainloop = m;
  pulse_handle->hm_context = hm_pa_context;
  (*pulse_backend)->backend_name = "pulseaudio";
  (*pulse_backend)->backend_type = PULSEAUDIO;
  (*pulse_backend)->backend_handle = (void*)pulse_handle;
  int n_cards = hm_pulse_n_cards(*pulse_backend);
  (*pulse_backend)->n_devices = n_cards;
  (*pulse_backend)->devices = (hm_device_t**)malloc(n_cards*sizeof(hm_pulse_device_t*));
  indexed_dev_handle dev_cb_handle;
  dev_cb_handle.dev = (hm_pulse_device_t**)((*pulse_backend)->devices);
  dev_cb_handle.dev_i = 0;
  dev_cb_handle.pulse_handle = pulse_handle;
  struct hm_pulse_io_list *list = (struct hm_pulse_io_list*)malloc(sizeof(struct hm_pulse_io_list));
  HM_LIST_INIT(list)
  list->pulse_handle = pulse_handle;
  // run callbacks to get context/server info
  pa_operation *o = 
    pa_context_get_card_info_list(hm_pa_context, _hm_pulse_card_list_cb, (void*)(&dev_cb_handle));
  while(pa_operation_get_state(o) != PA_OPERATION_DONE)
    pa_threaded_mainloop_wait(m);
  pa_operation_unref(o);
  o = pa_context_get_source_info_list(hm_pa_context, _hm_pulse_source_list_cb, (void*)list);
  while(pa_operation_get_state(o) != PA_OPERATION_DONE)
    pa_threaded_mainloop_wait(m);
  pa_operation_unref(o);
  o = pa_context_get_sink_info_list(hm_pa_context, _hm_pulse_sink_list_cb,(void*)list);
  while(pa_operation_get_state(o) != PA_OPERATION_DONE)
    pa_threaded_mainloop_wait(m);
  pa_operation_unref(o);
  o = pa_context_get_server_info(hm_pa_context, _hm_pulse_server_info_cb, (void*)pulse_handle);
  while(pa_operation_get_state(o) != PA_OPERATION_DONE)
    pa_threaded_mainloop_wait(m);
  pa_operation_unref(o);
  pa_threaded_mainloop_unlock(m);
  io_list_to_array(list, *pulse_backend);
  (*pulse_backend)->dev_io_list = (hm_list_t*)list;
  int default_sink = get_dev_io_id(list, pulse_handle->server_info->default_sink_name);
  int default_src = get_dev_io_id(list, pulse_handle->server_info->default_source_name);
  (*pulse_backend)->default_dev_io[0] = default_sink;
  (*pulse_backend)->default_dev_io[1] = default_src;
  return 0;
}

int hm_pulse_io_connect_by_id(hm_device_io_connection_t **io, hm_backend_connection_t *pulse_backend, unsigned int id) {
  struct hm_pulse_io_node *io_node_itr = ((struct hm_pulse_io_list*)(pulse_backend->dev_io_list))->head;
  int i = 0;
  for (i = 0; i < id; i++) {
    io_node_itr = io_node_itr->next;
  }
  hm_pulse_handle_t *pulse_handle = (hm_pulse_handle_t*)(pulse_backend->backend_handle);
  pa_context *c = pulse_handle->hm_context;
  pa_stream *new_stream = pa_stream_new(c, "hiemal", &(pulse_handle->server_info->sample_spec), NULL);
  switch (io_node_itr->io_device->type) {
    case PLAYBACK:
      pa_stream_connect_playback(new_stream, io_node_itr->io_device->name, NULL, 0, NULL, NULL);
    case RECORDING:
      pa_stream_connect_record(new_stream, io_node_itr->io_device->name, NULL, 0);
  }
  *io = (hm_device_io_connection_t*)malloc(sizeof(hm_device_io_connection_t));
  (*io)->backend_handle = (void*)new_stream;
  (*io)->type = io_node_itr->io_device->type;
  (*io)->read_fn = hm_pulse_io_read;
  (*io)->write_fn = hm_pulse_io_write;
  return 0;
}

int hm_pulse_default_io_connect(hm_device_io_connection_t **io, 
    hm_backend_connection_t *pulse_backend, hm_io_type_t dir) {
  hm_pulse_handle_t *pulse_handle = (hm_pulse_handle_t*)(pulse_backend->backend_handle);
  pa_context *c = pulse_handle->hm_context;
  pa_stream *new_stream = pa_stream_new(c, "hiemal", &(pulse_handle->server_info->sample_spec), NULL);
  switch (dir) {
    case PLAYBACK:
      pa_stream_connect_playback(new_stream, "default", NULL, 0, NULL, NULL);
    case RECORDING:
      pa_stream_connect_record(new_stream, "default", NULL, 0);
  }
  *io = (hm_device_io_connection_t*)malloc(sizeof(hm_device_io_connection_t));
  (*io)->backend_handle = (void*)new_stream;
  (*io)->type = dir;
  (*io)->read_fn = hm_pulse_io_read;
  (*io)->write_fn = hm_pulse_io_write;
  return 0;
}

int hm_pulse_dump_info(hm_backend_connection_t *pulse_backend) {
  hm_pulse_handle_t *hm_pulse = (hm_pulse_handle_t*)(pulse_backend->backend_handle);
  printf("Backend: PulseAudio\n\n");
  printf("Devices:\n");

  int i, j;
  hm_device_t *cur_dev = NULL;
  for (i = 0; i < pulse_backend->n_devices; i++) {
    cur_dev = (pulse_backend->devices)[i];
    printf("  %s\n", (cur_dev->name));
    printf("    IO devices:\n");
    for (j = 0; j < cur_dev->n_io_devices; j++) {
      printf("      %s\n", (cur_dev->io_devices)[j]->name);
    }
  }
  printf("Server Info:\n");
  printf("  Default Source: %s\n", hm_pulse->server_info->default_source_name);
  printf("  Default Sink: %s\n", hm_pulse->server_info->default_sink_name);
  return 0;
}

int hm_pulse_connection_close(hm_backend_connection_t **pulse_backend) {
  hm_pulse_handle_t *h = (hm_pulse_handle_t*)((*pulse_backend)->backend_handle);
  pa_context *hm_pa_context = (pa_context*)(h->hm_context);
  pa_threaded_mainloop *hm_mainloop = (pa_threaded_mainloop*)(h->mainloop);

  pa_threaded_mainloop_stop(hm_mainloop);
  pa_context_disconnect(hm_pa_context);
  pa_context_unref(hm_pa_context);
  pa_threaded_mainloop_free(hm_mainloop);
  
  int i = 0, j = 0;
  hm_device_t *cur_device = NULL;
  for (i = 0; i < ((*pulse_backend)->n_devices); i++) {
    cur_device = ((*pulse_backend)->devices)[i];
    for (j = 0; j < cur_device->n_io_devices; j++) {
      free((cur_device->io_devices)[j]);
    }
    free(cur_device->io_devices);
    free(cur_device);
  }
  free((*pulse_backend)->devices);
  hm_list_delete((*pulse_backend)->dev_io_list);
  free(*pulse_backend);
  *pulse_backend = NULL;
  return 0;
}