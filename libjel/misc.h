//jpeg libraries insist on this macro.
#define SIZEOF(object)	((size_t) sizeof(object))

//clone of jround_up
long round_up (long a, long b);

int fullread(int fd, char *buf, int nbytes);

int fullwrite(int fd, char *buf, int nbytes);

