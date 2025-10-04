#ifndef _DEVICE_H
#define _DEVICE_H

#include "backend.h"

typedef struct _hm_device hm_device_t;
typedef struct _hm_device_io hm_device_io_t;
typedef struct _hm_device_io_connection hm_device_io_connection_t;

typedef enum {PLAYBACK=0, RECORDING} hm_io_type_t;

int get_device_by_id(hm_device_t **dev, hm_backend_connection_t *backend, unsigned int id);
int get_device_by_name(hm_device_t **dev, hm_backend_connection_t *backend, char *name);
int get_device_io_by_id(hm_backend_connection_t *backend, hm_device_io_t **io, unsigned int card_id, unsigned int id);
int get_device_io_by_name(hm_backend_connection_t *backend, hm_device_io_t **io, unsigned int card_id, char *name);
int default_device_io_connect(hm_device_io_connection_t **io, hm_backend_connection_t *backend, hm_io_type_t dir);

#endif