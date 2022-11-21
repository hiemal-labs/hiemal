#include "io.h"

#include <unistd.h>


/*! \brief Source (unix FD) 
* \param fd file descriptor
* \param n_bytes number of bytes to read from fd
* \param buf buffer address
* \return exit code
*/
int src_fd(int fd, unsigned int n_bytes, void *buf) {
  read(fd, buf, n_bytes);
  return 0;
}

/*! \brief Sink (unix FD) 
* \param fd file descriptor
* \param n_bytes number of bytes to write to fd
* \param buf buffer address
* \return exit code
*/
int sink_fd(int fd, unsigned int n_bytes, void *buf) {
  write(fd, buf, n_bytes);
  return 0;
}

/*! \brief Source (file) 
* \param f ``FILE*`` handle
* \param n_bytes number of bytes to read from file
* \param buf buffer address
* \return exit code
*/
int src_file(FILE *f, unsigned int n_bytes, void *buf) {
  fread(buf, 1, n_bytes, f);
  return 0;
}

/*! \brief Sink (file) 
* \param fname file name
* \param n_bytes number of bytes to write to file
* \param buf buffer address
* \return exit code
*/
int sink_file(FILE *f, unsigned int n_bytes, void *buf) {
  fwrite(buf, 1, n_bytes, f);
  return 0;
}
