//#include <string.h>
#include <stdio.h>
//#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#endif
#include <time.h>
#include <errno.h>

long round_up (long a, long b)
/* Compute a rounded up to next multiple of b, ie, ceil(a/b)*b */
/* Assumes a >= 0, b > 0 */
{
  a += b - 1L;
  return a - (a % b);
}


/* Read with retries to make sure all nbytes bytes are obtained. */

#define retry_max 10

/* milliseconds */
#define retry_wait 10  

#ifdef _WIN32
/* milliseconds */
void sleep_ms(int ms){    Sleep(ms); }
#else
/* microseconds */
void sleep_ms(int ms){    usleep(ms * 1000); }
#endif  



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
    fprintf(stderr, "fullread: only %d bytes out of %d requested were read.\n", so_far, nbytes);

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
    fprintf(stderr, "fullwrite: only %d bytes out of %d requested were written.\n", so_far, nbytes);

  return so_far;
}
