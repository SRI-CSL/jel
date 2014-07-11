/*
 * wedge.c - Originally cribbed from djpeg.c but substantially
 * rewritten, which contains the following:
 * 
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 *  The objective of "wedge" is to place a message into the image
 * using frequency space.  We do this by manipulating the quantized
 * DCT coefficients.  We choose relatively high frequencies that are
 * quantized "just enough" to carry some bits of our message.  We set
 * those frequency coefficients to desired values, then write the
 * results into a new JPEG file.  A companion program "unwedge" will
 * extract the message.
 *
 */
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <jel/jel.h>

#include <ctype.h>		/* to declare isprint() */

#define WEDGE_VERSION "1.0.0.4"

#ifdef _WIN32
#define PriSize_t   "u"
#define PriSSize_t   "u"
#else
#define PriSize_t   "zu"
#define PriSSize_t  "zd"
#endif

/*
 * Argument-parsing code.
 * The switch parser is designed to be useful with DOS-style command line
 * syntax, ie, intermixed switches and file names, where only the switches
 * to the left of a given file name affect processing of that file.
 * The main program in this file doesn't actually use this capability...
 */


static const char * progname;		/* program name for error messages */
static char * outfilename = NULL;	/* for -outfile switch */
static char * msgfilename = NULL;		/* for -infile switch */
static unsigned char * message = NULL;
static int freq[64];
static int nfreq = 0;
//static int verbose = 0;
static int embed_length = 1;   /* Always embed the length */
static int message_length;     /* Could be strlen, but not if we read from a file. */
static int quality = 0;        /* If 0, do nothing.  If >0, then set the output quality factor. */
static int ecc = 1;
static int ecclen = 0;

LOCAL(void)
usage (void)
/* complain about bad command line */
{
  fprintf(stderr, "usage: %s [switches] inputfile [outputfile]\n", progname);
  fprintf(stderr, "Embeds a message in a jpeg image.\n");
  fprintf(stderr, "Switches (names may be abbreviated):\n");
  fprintf(stderr, "  -message M      Wedge a string M into the image\n");
  fprintf(stderr, "                  If not supplied, stdin will be used.\n");
  fprintf(stderr, "  -nolength       Do not embed the message length.\n");
  fprintf(stderr, "  -ecc L          Set ECC block length to L bytes.\n");
  fprintf(stderr, "  -noecc          Do not use error correcting codes.\n");
  fprintf(stderr, "  -quanta N       Ask for N quanta for embedding (default=8).\n");
  fprintf(stderr, "                  NOTE: The same value must used for extraction!\n");
  fprintf(stderr, "  -quality Q      Ask for quality level Q for embedding (default=same as input quality).\n");
  fprintf(stderr, "  -freq a,b,c,d   Encode using frequencies corresponding to indices a,b,c,d\n");
  fprintf(stderr, "                  If this is specified, -quanta is ignored.\n");
  fprintf(stderr, "                  NOTE: The same values must used for extraction!\n");
  fprintf(stderr, "  -data    <file> Use the contents of the file as the message (alternative to stdin).\n");
  fprintf(stderr, "  -outfile <file> Filename for output image.\n");
  fprintf(stderr, "  -verbose  or  -debug   Emit debug output\n");
  fprintf(stderr, "  -version        Print version info and exit.\n");
  exit(EXIT_FAILURE);
}

static boolean
keymatch (char * arg, const char * keyword, int minchars)
{
  register int ca, ck;
  register int nmatched = 0;

  while ((ca = *arg++) != '\0') {
    if ((ck = *keyword++) == '\0')
      return FALSE;		/* arg longer than keyword, no good */
    if (isupper(ca))		/* force arg to lcase (assume ck is already) */
      ca = tolower(ca);
    if (ca != ck)
      return FALSE;		/* no good */
    nmatched++;			/* count matched characters */
  }
  /* reached end of argument; fail if it's too short for unique abbrev */
  if (nmatched < minchars)
    return FALSE;
  return TRUE;			/* A-OK */
}

LOCAL(void)
clean_up_statics(void)
{
  if(msgfilename != NULL){ 
    free(msgfilename);
    msgfilename = NULL;
  }

  if(outfilename != NULL){
    free(outfilename);
    outfilename = NULL;
  }

  if(message != NULL){
    free(message);
    message = NULL;
  }

}
  
LOCAL(int)
parse_switches (jel_config *cfg, int argc, char **argv)
{
  int argn;
  char * arg;

  for (argn = 1; argn < argc; argn++) {
    arg = argv[argn];
    if (*arg != '-') break;

    arg++;			/* advance past switch marker character */

    if (keymatch(arg, "data", 4)) {
      /* Set output file name. */
      if (++argn >= argc)	/* advance to next argument */
	usage();
      msgfilename = strdup(argv[argn]);	/* save it away for later use */

    } else if (keymatch(arg, "outfile", 3)) {
      /* Set output file name. */
      if (++argn >= argc)	/* advance to next argument */
	usage();
      outfilename = strdup(argv[argn]);	/* save it away for later use */

    } else if (keymatch(arg, "message", 3)) {
      /* Message string */
      if (++argn >= argc)	/* advance to next argument */
	usage();
      message = (unsigned char *) strdup(argv[argn]);

    } else if (keymatch(arg, "nolength", 5)) {
      /* Whether to embed length */
      embed_length = 0;
    } else if (keymatch(arg, "noecc", 5)) {
      /* Whether to use error correction */
      ecc = 0;
    } else if (keymatch(arg, "ecc", 3)) {
      /* Block length to use for error correction */
      if (++argn >= argc)
	usage();
      ecclen = strtol(argv[argn], NULL, 10);
    } else if (keymatch(arg, "frequencies", 4)) {
      /* freq. components */
      if (++argn >= argc)
	usage();
      if (sscanf(argv[argn], "%d,%d,%d,%d",
		 &freq[0], &freq[1], &freq[2], &freq[3]) != 4)
	usage();
      nfreq = 4;
    } else if (keymatch(arg, "quanta", 4)) {
      /* Start block */
      if (++argn >= argc)
	usage();
      cfg->freqs.nlevels = strtol(argv[argn], NULL, 10);
    } else if (keymatch(arg, "quality", 4)) {
      /* Start block */
      if (++argn >= argc)
	usage();
      quality = strtol(argv[argn], NULL, 10);
    } else if (keymatch(arg, "version", 7)) {
      fprintf(stderr, "wedge version %s (libjel version %s)\n",
	      WEDGE_VERSION, jel_version_string());
      exit(-1);
    } else {
      usage();			/* bogus switch */
    }
  }

  return argn;			/* return index of next arg (file name) */
}


#if 0
/*
 * Not really needed, but kept in case this is useful later:
 * Marker processor for COM and interesting APPn markers.
 * This replaces the library's built-in processor, which just skips the marker.
 * We want to print out the marker as text, to the extent possible.
 * Note this code relies on a non-suspending data source.
 */

LOCAL(unsigned int)
jpeg_getc (j_decompress_ptr cinfo)
/* Read next byte */
{
  struct jpeg_source_mgr * datasrc = cinfo->src;

  if (datasrc->bytes_in_buffer == 0) {
    if (! (*datasrc->fill_input_buffer) (cinfo))
      ERREXIT(cinfo, JERR_CANT_SUSPEND);
  }
  datasrc->bytes_in_buffer--;
  return GETJOCTET(*datasrc->next_input_byte++);
}

METHODDEF(boolean)
print_text_marker (j_decompress_ptr cinfo)
{
  boolean traceit = (cinfo->err->trace_level >= 1);
  INT32 length;
  unsigned int ch;
  unsigned int lastch = 0;

  length = jpeg_getc(cinfo) << 8;
  length += jpeg_getc(cinfo);
  length -= 2;			/* discount the length word itself */

  if (traceit) {
    if (cinfo->unread_marker == JPEG_COM)
      fprintf(stderr, "Comment, length %ld:\n", (long) length);
    else			/* assume it is an APPn otherwise */
      fprintf(stderr, "APP%d, length %ld:\n",
	      cinfo->unread_marker - JPEG_APP0, (long) length);
  }

  while (--length >= 0) {
    ch = jpeg_getc(cinfo);
    if (traceit) {
      /* Emit the character in a readable form.
       * Nonprintables are converted to \nnn form,
       * while \ is converted to \\.
       * Newlines in CR, CR/LF, or LF form will be printed as one newline.
       */
      if (ch == '\r') {
	fprintf(stderr, "\n");
      } else if (ch == '\n') {
	if (lastch != '\r')
	  fprintf(stderr, "\n");
      } else if (ch == '\\') {
	fprintf(stderr, "\\\\");
      } else if (isprint(ch)) {
	putc(ch, stderr);
      } else {
	fprintf(stderr, "\\%03o", ch);
      }
      lastch = ch;
    }
  }

  if (traceit)
    fprintf(stderr, "\n");

  return TRUE;
}
#endif

/*
 * This version uses file descriptors to avoid stdio ambiguities
 * wrt. binary files.
 */
static size_t read_message(char *filename, unsigned char *message, int maxlen, int abort_on_overflow) {
  size_t k;
  int fd;
  //char ch;
  struct stat buf;

  if (filename != NULL) fd = open(filename, O_RDONLY);
  else {
    fprintf(stderr,"This version cannot use stdio for message input.\n");
    exit(-3);
  }

  k = 0;
  if (fd > 0) {
    fstat(fd, &buf);
    k = buf.st_size;

    if (k > maxlen) {
      fprintf(stderr, "Message (%" PriSize_t " bytes) is too long (maxlen=%d)!\n", k, maxlen);
      exit(-1);
    }

    /* Read the whole file: */
    if(read(fd, message, k) == -1){
      fprintf(stderr, "read_message: read failed %s\n", strerror(errno));
    }
    close(fd);
  }
  return k;
}




int
main (int argc, char **argv)
{
  jel_config *jel;
  jel_freq_spec *fspec;
  int abort_on_overflow = 1;
  //int file_index;
  int k, ret;
  int max_bytes;
  FILE *sfp, *dfp;

  jel = jel_init(JEL_NLEVELS);

  ret = jel_open_log(jel, "/tmp/wedge.log");
  if (ret == JEL_ERR_CANTOPENLOG) {
    fprintf(stderr, "Can't open /tmp/wedge.log!!\n");
    jel->logger = stderr;
  }

  jel_log(jel,"wedge version %s\n", WEDGE_VERSION);


  progname = argv[0];
  if (progname == NULL || progname[0] == 0)
    progname = "wedge";		/* in case C library doesn't provide it */

  k = parse_switches(jel, argc, argv);
  if (argc == k) {
    usage();
    exit(-1);
  }

  if (!ecc) {
    jel_setprop(jel, JEL_PROP_ECC_METHOD, JEL_ECC_NONE);
    jel_log(jel, "Disabling ECC.  getprop=%d\n", jel_getprop(jel, JEL_PROP_ECC_METHOD));
  } else if (ecclen > 0) {
    jel_setprop(jel, JEL_PROP_ECC_BLOCKLEN, ecclen);
    jel_log(jel, "ECC block length set.  getprop=%d\n", jel_getprop(jel, JEL_PROP_ECC_BLOCKLEN));
  }

  sfp = fopen(argv[k], "rb");
  if (!sfp) {
    jel_log(jel, "%s: Could not open source JPEG file %s!\n", progname, argv[k]);
    exit(EXIT_FAILURE);
  }

  ret = jel_set_fp_source(jel, sfp);
  if (ret != 0) {
    jel_log(jel, "%s: jel_set_fp_source failed and returns %d.\n", progname, ret);
    fprintf(stderr, "Error - exiting (need a diagnostic!)\n");
    exit(EXIT_FAILURE);
  }
  jel_log(jel,"%s: wedge source %s\n", progname, argv[k]);

  k++;
  if (argc == k && !outfilename) {
    dfp = stdout;
    ret = jel_set_fp_dest(jel, stdout);
  } else {
    if (!outfilename) outfilename = strdup(argv[k]);

    dfp = fopen(outfilename, "wb");
    ret = jel_set_fp_dest(jel, dfp);
    jel_log(jel,"%s: wedge output %s\n", progname, outfilename);

  }

  if (ret != 0) {
    jel_log(jel,"%s: jel error %d\n", progname, ret);
    fprintf(stderr, "Error - exiting (need a diagnostic!)\n");
    exit(EXIT_FAILURE);
  }

  if (message) message_length = strlen( (char *) message);
  else {
    max_bytes = jel_capacity(jel);
    message = (unsigned char*)calloc(max_bytes+1, sizeof(unsigned char));
    
    jel_log(jel,"%s: wedge data %s\n", progname, msgfilename);
    
    message_length = (int) read_message(msgfilename, message, max_bytes, abort_on_overflow);
    jel_log(jel, "%s: Message length to be used is: %d\n", progname, message_length);

  }

  if ( quality > 0 ) {
    jel_log(jel, "%s: Setting output quality to %d\n", progname, quality);
    if ( jel_setprop( jel, JEL_PROP_QUALITY, quality ) != quality )
      jel_log(jel, "Failed to set output quality.\n");
  }
  
  jel_setprop(jel, JEL_PROP_EMBED_LENGTH, embed_length);

  /* If message is NULL, shouldn't we punt? */
  if (!message) {
    jel_log(jel,"No message provided!  Exiting.\n");
    exit(-2);
  }

  fspec = &(jel->freqs);

  if (nfreq > 0) {
    fspec->nfreqs = nfreq;
    for (k = 0; k < nfreq; k++) fspec->freqs[k] = freq[k];
  }

  //  if (verbose) describeMessage(msg);

  /* Insert the message: */
  jel_log(jel,"Message length is %d.\n", message_length);
  jel_embed(jel, message, message_length);

  //max_bytes = jel_capacity(jel);

  jel_log(jel, "%s: JPEG compressed to %d bytes.\n", progname, jel->jpeglen);
  jel_close_log(jel);

  if (sfp != NULL && sfp != stdin) fclose(sfp);
  if (dfp != NULL && dfp != stdout) fclose(dfp);

  jel_free(jel);

  clean_up_statics();

  return 0;			/* suppress no-return-value warnings */
}
