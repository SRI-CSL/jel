#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#endif
#include <time.h>
#include <errno.h>

/* Read with retries to make sure all nbytes bytes are obtained. */

int fullread(int fd, char *buf, int nbytes) {
  int so_far, i, k;

  so_far = read(fd, buf, nbytes);

  i = 0;
  while (so_far < nbytes && i < retry_max) {
    if (so_far < 0) so_far = 0;

    k = read(fd, buf+so_far, nbytes-so_far);

    /* Return on error or EOF: */
    if (k < 0) return k;

    so_far += k;
    i++;
    sleep_ms(retry_wait);
  }

  if (so_far < nbytes)
    tprintf(4, "fullread: only %d bytes out of %d requested were read.\n", so_far, nbytes);

  return so_far;
}

int fullwrite(int fd, char *buf, int nbytes) {
  int so_far, i, k;

  so_far = write(fd, buf, nbytes);

  i = 0;
  while (so_far < nbytes && i < retry_max) {
    k = write(fd, buf+so_far, nbytes-so_far);

    /* Return on error or EOF: */
    if (k < 0) return k;

    so_far += k;
    i++;
    sleep_ms(retry_wait);
  }

  if (so_far < nbytes)
    tprintf(4, "fullwrite: only %d bytes out of %d requested were written.\n", so_far, nbytes);

  return so_far;
}
