#include "intern/device.h"

int get_device_by_id(hm_device_t **dev, hm_backend_connection_t *backend, unsigned int id) {
  return 0;
}

int get_device_by_name(hm_device_t **dev, hm_backend_connection_t *backend, char *name) {
  return 0;
}

int get_device_io_by_id(hm_backend_connection_t *backend, hm_device_io_t **io, unsigned int card_id, unsigned int id) {
  return 0;
}

int get_device_io_by_name(hm_backend_connection_t *backend, hm_device_io_t **io, unsigned int card_id, char *name) {
  return 0;
}

int default_device_io_connect(hm_device_io_connection_t **io, hm_backend_connection_t *backend, hm_io_type_t dir) {
  //hm_device_t *default_dev = (backend->devices)[backend->default_dev_id];
  //hm_device_io_t *default_io = (default_dev->io_devices)[default_dev->default_io_id];
  const struct backend_info *info = get_backend_by_name(backend->backend_name);
  unsigned int io_id = (dir == PLAYBACK) ? backend->default_dev_io[0] : backend->default_dev_io[1];
  (info->io_connect_fn)(io, backend, io_id);
  return 0;
}

int device_io_read(hm_device_io_t *io, buffer_t *buf, unsigned int n_bytes) {
  return (io->read_fn)(io, buf, n_bytes);
}

int device_io_write(hm_device_io_t *io, buffer_t *buf, unsigned int n_bytes) {
  return (io->write_fn)(io, buf, n_bytes);
}

int device_io_read_ext(hm_device_io_connection_t *io, buffer_t *buf, unsigned int n_bytes) {
  return (io->read_fn)(io, buf, n_bytes);
}

int device_io_write_ext(hm_device_io_connection_t *io, buffer_t *buf, unsigned int n_bytes) {
  return (io->write_fn)(io, buf, n_bytes);
}