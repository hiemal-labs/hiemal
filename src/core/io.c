#include "intern/common.h"
#include "intern/io.h"
#include "intern/buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>

IMPL(src_fd) {
  long int fd = (long int)_inputs;
  int cur_pos = lseek(fd, 0, SEEK_CUR);
  struct stat s;
  fstat(fd, &s);
  int fd_size = s.st_size;
  int bytes_available = fd_size - cur_pos;
  if (n_bytes > bytes_available) {
    n_bytes = bytes_available;
  }
  buffer_t *buf = (buffer_t*)_outputs;
  void *tmp_buf = (void*)malloc(n_bytes);
  read(fd, tmp_buf, n_bytes);
  buffer_write(buf, tmp_buf, n_bytes);
  free(tmp_buf);
  return n_bytes;  
}
/*! \brief Source (unix FD) 
* \param fd file descriptor
* \param n_bytes number of bytes to read from fd
* \param buf buffer address
* \return exit code
*/
int src_fd(int fd, unsigned int n_bytes, void *buf) {
  int rc;
  buffer_t *dest = NULL;
  buffer_init_ext(&dest, n_bytes, RING, buf);
  rc = src_fd_impl(n_bytes, (void*)(long int)fd, (void*)dest, NULL);
  buffer_delete(&dest);
  return rc;
}


IMPL(sink_fd) {
  buffer_t *buf = (buffer_t*)_inputs;
  int bytes_available = buffer_n_read_bytes(buf);
  if (n_bytes > bytes_available) {
    n_bytes = bytes_available;
  }
  long int fd = (long int)_outputs;
  void *tmp_buf = (void*)malloc(n_bytes);
  buffer_read(buf, tmp_buf, n_bytes);
  write(fd, tmp_buf, n_bytes);
  free(tmp_buf);
  return n_bytes;  
}

/*! \brief Sink (unix FD) 
* \param fd file descriptor
* \param n_bytes number of bytes to write to fd
* \param buf buffer address
* \return exit code
*/
int sink_fd(int fd, unsigned int n_bytes, void *buf) {
  int rc;
  buffer_t *src = NULL;
  buffer_init_ext(&src, n_bytes, RING, buf);
  src->state = FULL;
  rc = sink_fd_impl(n_bytes, (void*)src, (void*)(long int)fd, NULL);
  buffer_delete(&src);
  return rc;
}


IMPL(src_file) {
  FILE *f = (FILE*)_inputs;
  int fd = fileno(f);
  int cur_pos = lseek(fd, 0, SEEK_CUR);
  struct stat s;
  fstat(fd, &s);
  int fd_size = s.st_size;
  int bytes_available = fd_size - cur_pos;
  if (n_bytes > bytes_available) {
    n_bytes = bytes_available;
  }
  buffer_t *buf = (buffer_t*)_outputs;
  void *tmp_buf = (void*)malloc(n_bytes);
  fread(tmp_buf, 1, n_bytes, f);
  buffer_write(buf, tmp_buf, n_bytes);
  free(tmp_buf);
  return n_bytes;  
}

/*! \brief Source (file) 
* \param f ``FILE*`` handle
* \param n_bytes number of bytes to read from file
* \param buf buffer address
* \return exit code
*/
int src_file(FILE *f, unsigned int n_bytes, void *buf) {
  int rc;
  buffer_t *dest = NULL;
  buffer_init_ext(&dest, n_bytes, RING, buf);
  rc = src_file_impl(n_bytes, (void*)f, (void*)dest, NULL);
  buffer_delete(&dest);
  return rc;
}


IMPL(sink_file) {
  buffer_t *buf = (buffer_t*)_inputs;
  int bytes_available = buffer_n_read_bytes(buf);
  if (n_bytes > bytes_available) {
    n_bytes = bytes_available;
  }
  FILE* f = (FILE*)_outputs;
  void *tmp_buf = (void*)malloc(n_bytes);
  buffer_read(buf, tmp_buf, n_bytes);
  fwrite(tmp_buf, 1, n_bytes, f);
  free(tmp_buf);
  return n_bytes;  
}

/*! \brief Sink (file) 
* \param f ``FILE*`` handle
* \param n_bytes number of bytes to write to file
* \param buf buffer address
* \return exit code
*/
int sink_file(FILE *f, unsigned int n_bytes, void *buf) {
  int rc;
  buffer_t *src = NULL;
  buffer_init_ext(&src, n_bytes, RING, buf);
  src->state = FULL;
  rc = sink_file_impl(n_bytes, (void*)src, (void*)f, NULL);
  buffer_delete(&src);
  return rc;
}
