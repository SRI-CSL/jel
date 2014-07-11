/*
 * wcap.c - Report the capacity of a JPEG file.
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <jel/jel.h>

/*
 * Argument-parsing code.
 * The switch parser is designed to be useful with DOS-style command line
 * syntax, ie, intermixed switches and file names, where only the switches
 * to the left of a given file name affect processing of that file.
 * The main program in this file doesn't actually use this capability...
 */

#if 0
static const char * progname;	/* program name for error messages */
static char * outfilename;	/* for -outfile switch */
static char * msgfilename;	/* for -infile switch */
static char * message = NULL;
static int freq[64];
static int nfreq = 0;
#endif

/*
 * The main program.
 */

int
main (int argc, char **argv)
{
  jel_config *jel;
  FILE * input_file;
  int max_bytes, ret;
  int ecc_method;

  if (argc < 2) {
    fprintf(stderr, "usage: wcap <file> [-noecc]\n");
    exit(-1);
  }

  jel = jel_init(JEL_NLEVELS);

  ret = jel_open_log(jel, "/tmp/wcap.log");
  if(ret != 0){
    fprintf(stderr, "Error - exiting (can't open log file)\n");
    exit(EXIT_FAILURE);
  }

  input_file = fopen(argv[1], "rb");
  ret = jel_set_fp_source(jel, input_file);

  if (ret != 0) {
    fprintf(stderr, "Error - exiting (need a diagnostic!)\n");
    exit(EXIT_FAILURE);
  }

  if (argc > 2 && !strcmp(argv[2], "-noecc")) ecc_method = JEL_ECC_NONE;
  else ecc_method = JEL_ECC_RSCODE;

  jel_setprop(jel, JEL_PROP_ECC_METHOD, ecc_method );

  //ecc_method = jel_getprop(jel, JEL_PROP_ECC_METHOD);

  max_bytes = jel_capacity(jel);

  jel_close_log(jel);
  if (input_file != NULL && input_file != stdin) fclose(input_file);

  printf("%d\n", max_bytes);

  exit(0);
}
