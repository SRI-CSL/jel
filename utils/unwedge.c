/*
 * unwedge.c - extract a message that was embedded in a JPEG image
 * using 'wedge'.
 * 
 * The objective of "unwedge" is to extract a message from the image
 * using frequency space, assuming that "wedge" was used to embed the
 * message.  We do this by manipulating the quantized DCT
 * coefficients.  We choose relatively high frequencies that are
 * quantized "just enough" to carry some bits of our message.  We set
 * those frequency coefficients to desired values, then write the
 * results into a new JPEG file.  A companion program "unwedge" will
 * extract the message.
 *
 */

#include <jel/jel.h>
#include <ctype.h>		/* to declare isprint() */

#define UNWEDGE_VERSION "1.0.0.4"

/*
 * Argument-parsing code.
 * The switch parser is designed to be useful with DOS-style command line
 * syntax, ie, intermixed switches and file names, where only the switches
 * to the left of a given file name affect processing of that file.
 * The main program in this file doesn't actually use this capability...
 */


static const char * progname;		/* program name for error messages */
static char * outfilename = NULL;	/* for -outfile switch */
static unsigned char * message = NULL;
static int freq[64];
static int nfreq = 0;
static int msglen = -1;
static int embed_length = 1;
static int ecc = 1;
static int ecclen = 0;


LOCAL(void)
usage (void)
/* complain about bad command line */
{
  fprintf(stderr, "usage: %s [switches] ", progname);
  fprintf(stderr, "inputfile [outputfile]\n");


  fprintf(stderr, "Switches (names may be abbreviated):\n");
  fprintf(stderr, "  -length    N   Decode exactly N characters from the message.\n");
  fprintf(stderr, "  -ecc L          Set ECC block length to L bytes.\n");
  fprintf(stderr, "  -noecc         Do not use error correcting codes.\n");
  fprintf(stderr, "  -outfile name  Specify name for output file\n");
  fprintf(stderr, "  -quanta N      Ask for N quanta for extraction (default=8).\n");
  fprintf(stderr, "                 NOTE: The same value must used for embedding!\n");
  fprintf(stderr, "  -freq a,b,c,d  Decode message using specified freq. component indices.\n");
  fprintf(stderr, "                 If this is specified, -quanta is ignored.\n");
  fprintf(stderr, "                 NOTE: The same values must used for extraction!\n");
  fprintf(stderr, "  -verbose  or  -debug   Emit debug output\n");
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



LOCAL(int)
parse_switches (jel_config *cfg, int argc, char **argv)
/* Parse optional switches.
 * Returns argv[] index of first file-name argument (== argc if none).
 * Any file names with indexes <= last_file_arg_seen are ignored;
 * they have presumably been processed in a previous iteration.
 * (Pass 0 for last_file_arg_seen on the first or only iteration.)
 * for_real is FALSE on the first (dummy) pass; we may skip any expensive
 * processing.
 */
{
  int argn;
  char * arg;

  /* Scan command line options, adjust parameters */

  for (argn = 1; argn < argc; argn++) {
    arg = argv[argn];
    if (*arg != '-') break;

    arg++;			/* advance past switch marker character */

    if (keymatch(arg, "debug", 1) || keymatch(arg, "verbose", 1)) {
      /* Enable debug printouts. */
      /* On first -d, print version identification */
      //static boolean printed_version = FALSE;


    } else if (keymatch(arg, "outfile", 4)) {
      /* Set output file name. */
      if (++argn >= argc)	/* advance to next argument */
	usage();
      outfilename = argv[argn];	/* save it away for later use */

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
    } else if (keymatch(arg, "length", 3)) {
      /* Start block */
      if (++argn >= argc)
	usage();
      msglen = strtol(argv[argn], NULL, 10);
      embed_length = 0;
    } else if (keymatch(arg, "quanta", 5)) {
      /* Start block */
      if (++argn >= argc)
	usage();
      cfg->freqs.nlevels = strtol(argv[argn], NULL, 10);
    } else if (keymatch(arg, "noecc", 5)) {
      /* Whether to assume error correction */
      ecc = 0;
    } else if (keymatch(arg, "version", 7)) {
      fprintf(stderr, "unwedge version %s (libjel version %s)\n",
	      UNWEDGE_VERSION, jel_version_string());
      exit(-1);
    } else {
      usage();			/* bogus switch */
    }
  }

  return argn;			/* return index of next arg (file name) */
}


/*
 * Marker processor for COM and interesting APPn markers.
 * This replaces the library's built-in processor, which just skips the marker.
 * We want to print out the marker as text, to the extent possible.
 * Note this code relies on a non-suspending data source.
 */


#if 0

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

static int ijel_printf_message(jel_config *cfg) {
  return ijel_fprintf_message(cfg, stdout) ;
}

static void ijel_describe_message(jel_config *cfg) {
  jel_freq_spec *f = &(cfg->freqs);
  int i;
  printf("Message:\n");
  printf("   Frequency component indices: [");
  for (i = 0; i < f->nfreqs; i++) printf("%d ", f->freqs[i]);
  printf("]\n   Message length: %d\n", cfg->len);
  printf("   Message max length: %d\n", cfg->maxlen);
  printf("   Message hex: ");
  for (i = 0; i < cfg->len; i++) printf("%2x ", (unsigned char) cfg->data[i] & 0xFF);
}

#endif


static int ijel_fprintf_message(jel_config *cfg, FILE* fp) {
  int i;
  int retval = 0;

  // msg->len always dictates the length:
  for (i = 0; i < cfg->len; i++){
    char c = cfg->data[i];
    fputc(c, fp);
    retval++; 
  }
  return retval;
}



/*
 * The main program.
 */

int
main (int argc, char **argv)
{
  jel_config *jel;
  jel_freq_spec *fspec;

  int max_bytes;
  int ret;
  //int file_index;
  int k; //, bw, bh;
  int bytes_written;
  FILE *sfp, *output_file;

  
  jel = jel_init(JEL_NLEVELS);

  ret = jel_open_log(jel, "/tmp/unwedge.log");
  if (ret == JEL_ERR_CANTOPENLOG) {
    fprintf(stderr, "Can't open /tmp/unwedge.log!!\n");
    jel->logger = stderr;
  }

  jel_log(jel,"unwedge version %s\n", UNWEDGE_VERSION);
  progname = argv[0];
  if (progname == NULL || progname[0] == 0)
    progname = "unwedge";		/* in case C library doesn't provide it */

  if (argc == 1) usage();
  
  k = parse_switches(jel, argc, argv);

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
    fprintf(stderr, "Error - exiting (need a diagnostic!)\n");
    exit(EXIT_FAILURE);
  }
  jel_log(jel,"%s: unwedge input %s\n", progname, argv[k]);

  k++;

  if (argc == k && !outfilename) output_file = stdout;
  else {
    if (!outfilename) outfilename = strdup(argv[k]);

    output_file = fopen(outfilename, "wb");
    jel_log(jel,"%s: wedge output %s\n", progname, outfilename);
  }

  if (ret != 0) {
    jel_log(jel,"%s: jel error %d\n", progname, ret);
    fprintf(stderr, "Error - exiting (need a diagnostic!)\n");
    exit(EXIT_FAILURE);
  }

  /*
   * 'raw' capacity just returns the number of bytes that can be
   * stored in the image, regardless of ECC or other forms of
   * encoding.  On this end, we just need to make sure that the
   * allocated buffer has enough space to load every byte:
   */

  max_bytes = jel_raw_capacity(jel);

  /* Set up the buffer for receiving the incoming message.  Internals
   * are handled by jel_extract: */
  message = malloc(max_bytes*2);

  if (embed_length) jel_log(jel, "%s: Length is embedded.\n", progname);
  else jel_log(jel, "%s: Length is NOT embedded.\n", progname);

  jel_setprop(jel, JEL_PROP_EMBED_LENGTH, embed_length);

  fspec = &(jel->freqs);

  /* If supplied, copy the selected frequency components: */
  if (nfreq > 0) {
    fspec->nfreqs = nfreq;
    for (k = 0; k < nfreq; k++) fspec->freqs[k] = freq[k];
  }

  /* Forces unwedge to read 'msglen' bytes: */
  if (msglen == -1) {
    jel_log(jel, "%s: Message length unspecified.  Setting to maximum: %d\n", progname, max_bytes);
    msglen = max_bytes;
  } else if (msglen > max_bytes) {
    jel_log(jel, "%s: Specified message length %d exceeds maximum %d.  Using maximum.\n", progname, msglen, max_bytes);
    msglen = max_bytes;
  } else {
    jel_log(jel, "%s: Specified message length is %d.\n", progname, msglen);
  }

  msglen = jel_extract(jel, message, msglen);

  jel_log(jel, "%s: %d bytes extracted\n", progname, msglen);

  bytes_written = ijel_fprintf_message(jel, output_file);

  if( bytes_written  != msglen ){
    jel_log(jel, "jel_fprintf_message wrote %d bytes; message contained %d bytes\n", bytes_written, msglen);
  }

  jel_close_log(jel);

  if (sfp != NULL && sfp != stdin) fclose(sfp);
  if (output_file != NULL && output_file != stdout) fclose(output_file);

  free(message);

  jel_free(jel);

  return 0;			/* suppress no-return-value warnings */
}
