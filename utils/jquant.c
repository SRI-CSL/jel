/*
 * C. Connolly, SRI International - 8/25/2013
 *
 * Show the quant table for a specific jpeg quality value (0,100] and
 * islow dct.  Optionally show the frequency indices that permit the
 * use of N bits (where N is presented on the command line).
 */
#include <jel/jel.h>
#include <stdlib.h>

static int nfreqs = 4;
static int nlevels = 8;
int ijel_find_freqs(JQUANT_TBL *, int *, int, int);

#if 0
static int quality = 80;
static int verbose = 0;
static int print_quanta = 0;
static int print_bits = 0;
static int print_huff = 0;

static int usage (char *progname)
/* complain about bad command line */
{
  fprintf(stderr, "usage: %s [switches] \n", progname);

  fprintf(stderr, "   -h:    Print huffman tables.\n");
  fprintf(stderr, "   -v:    Print quantization tables.\n");
  fprintf(stderr, "   -n M:  Use only M frequency components (default=4).\n");
  fprintf(stderr, "   -q N:  Use quality level N.\n");
  fprintf(stderr, "   -l K:  What frequencies would\n");
  fprintf(stderr, "          allow us to pack K levels\n");
  fprintf(stderr, "          in each frequency component?\n");
  fprintf(stderr, "   -a     Print the number of quanta for each frequency component.\n");
  fprintf(stderr, "   -b     Print the number of bits supported by each frequency component.\n");
  exit(-1);
}


#if 0
/* Parse optional switches.
 * Returns argv[] index of first file-name argument (== argc if none).
 * Any file names with indexes <= last_file_arg_seen are ignored;
 * they have presumably been processed in a previous iteration.
 * (Pass 0 for last_file_arg_seen on the first or only iteration.)
 * for_real is FALSE on the first (dummy) pass; we may skip any expensive
 * processing.
 */
static int parse_switches (int argc, char **argv)
{
  int argn;
  char * arg;

  /* Scan command line options, adjust parameters */

  for (argn = 1; argn < argc; argn++) {
    arg = argv[argn];
    if (arg[0] != '-') usage(argv[0]);
    arg++;			/* advance past switch marker character */

    if (arg[0] == 'v') verbose = 1;
    else if (arg[0] == 'a') print_quanta = 1;
    else if (arg[0] == 'q') {
      if (++argn >= argc) usage(argv[0]);
      quality = strtol(argv[argn], NULL, 10);
    } else if (arg[0] == 'l') {
      if (++argn >= argc) usage(argv[0]);
      nlevels = strtol(argv[argn], NULL, 10);
    } else if (arg[0] == 'h') {
      print_huff = 1;
    } else if (arg[0] == 'n') {
      if (++argn >= argc) usage(argv[0]);
      nfreqs = strtol(argv[argn], NULL, 10);
    } else {
      usage(argv[0]);			/* bogus switch */
    }
  }

  return argn;			/* return index of next arg (file name) */
}
#endif
#endif


void print_qtable(JQUANT_TBL *a) {
  int i;

  for (i = 0; i < DCTSIZE2; i++) {
    if (i % 8 == 0) printf("\n");
    printf("%4d ", a->quantval[i]);
  }
  printf("\n");
}


#if 0 /* This next is only here for reference! */

typedef struct {
  /* These two fields directly represent the contents of a JPEG DHT marker */
  UINT8 bits[17];		/* bits[k] = # of symbols with codes of */
				/* length k bits; bits[0] is unused */
  UINT8 huffval[256];		/* The symbols, in order of incr code length */
  /* This field is used only during compression.  It's initialized FALSE when
   * the table is created, and set TRUE when it's been output to the file.
   * You could suppress output of a table by setting this to TRUE.
   * (See jpeg_suppress_tables for an example.)
   */
  boolean sent_table;		/* TRUE when table has been output */
} JHUFF_TBL;
#endif

void print_huffman_table(JHUFF_TBL *table) {
  int k, sym;

  if (!table) printf("No table.\n");
  else {

    printf("  Bit lengths:\n");
    for (k = 1; k < 17; k++)
      printf("      %d symbols of length %d bits\n", table->bits[k], k);

    printf("  Symbols:\n");
    for (k = 0; k < 256; k++) {
      sym = table->huffval[k];
      printf("       %d => %d (%x)\n", k, sym, sym);
    }
  }
  printf("-------------\n");
}

void print_huffman_info( struct jpeg_compress_struct *info ) {
  //  JHUFF_TBL * dc_huff_tbl_ptrs[NUM_HUFF_TBLS];
  //  JHUFF_TBL * ac_huff_tbl_ptrs[NUM_HUFF_TBLS];
  int k;
  
  for (k = 0; k < NUM_HUFF_TBLS; k++) {
    printf("\n------Huffman DC table %d:\n", k);
    print_huffman_table( info->dc_huff_tbl_ptrs[k] );
    printf("\n------Huffman AC table %d:\n", k);
    print_huffman_table( info->dc_huff_tbl_ptrs[k] );
  }
}


int main(int argc, char **argv) {
  j_decompress_ptr cinfo;
  int k, ret;
  int i;
  //   int quanta[64];
  int good_freqs[64];
  jel_config *jel;
  int * ijel_get_quanta(JQUANT_TBL *q, int *quanta);
  void   ijel_log_qtables(jel_config*);
  FILE *sfp;
  char *progname = argv[0];

  //  i = parse_switches(argc, argv);

  jel = jel_init(JEL_NLEVELS);
  jel->logger = stderr;

  if ( argc > 1 ) {

    k = 1;
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
  }


#if 1
  /*
   * Set up the compression machinery:
   */
  cinfo = &(jel->srcinfo);

  /*
   * Ask for frequencies to use for the requested quality level:
   */
  nfreqs = ijel_find_freqs(cinfo->quant_tbl_ptrs[0], good_freqs, nfreqs, nlevels);
  printf("Highest frequency indices:\n");
  for (i = 0; i < nfreqs; i++) {
    if (i == 0) printf("%d", good_freqs[i]);
    else printf(",%d", good_freqs[i]);
  }
  printf("\n");

#if 0
  /*
   * If desired, print the quanta corresponding to the returned
   * frequencies:
   */
  if (print_quanta) {
    printf("\n");
    ijel_get_quanta(cinfo->quant_tbl_ptrs[0], quanta);
    for (i = 0; i < nfreqs; i++) {
      if (i == 0) printf("%d", quanta[good_freqs[i]]);
      else printf(",%d", quanta[good_freqs[i]]);
    }
  }
  
#endif
#endif

  //  if (print_huff) print_huffman_info(&cinfo);

  /*
   * TMI?  Print the quant tables:
   */
  ijel_log_qtables(jel);

  return 0;
}
