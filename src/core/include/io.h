#ifndef _IO_H
#define _IO_H

#include <stdio.h>

int src_fd(int fd, unsigned int n_bytes, void *buf);
int sink_fd(int fd, unsigned int n_bytes, void *buf);
int src_file(FILE *f, unsigned int n_bytes, void *buf);
int sink_file(FILE *f, unsigned int n_bytes, void *buf);

#endif
