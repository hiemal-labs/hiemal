#ifndef _BACKEND_H
#define _BACKEND_H

typedef struct _hm_backend_connection hm_backend_connection_t;

int hm_backend_init(char *name, hm_backend_connection_t **backend_ptr);
int hm_backend_close(hm_backend_connection_t **backend_ptr);
int hm_backend_dump_info(hm_backend_connection_t *backend_ptr);
const char** hm_backend_list();
#endif