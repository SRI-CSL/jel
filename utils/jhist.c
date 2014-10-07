/*
 * jhist: Use libjel to generate DCT histograms and emit them on
 * stdout.
 * 
 * By default, emits lines of the form <k> <a1> ... <a64> where <k> is
 * the bin number, representing coefficient values ranging from -1024
 * to 1024, and <an> is the count for bin k at frequency n.
 *
 */

#include <jel/jel.h>
#include <ctype.h>
#include <stdlib.h>

/*
 * Argument-parsing code.
 * The switch parser is designed to be useful with DOS-style command line
 * syntax, ie, intermixed switches and file names, where only the switches
 * to the left of a given file name affect processing of that file.
 * The main program in this file doesn't actually use this capability...
 */


#define JHIST_VERSION "0.1"

static const char * progname;		/* program name for error messages */
static char * outfilename = NULL;	/* for -outfile switch */
static int symmetry = 0;

LOCAL(void)
usage (void)
/* complain about bad command line */
{
  fprintf(stderr, "usage: %s [switches]", progname);
  fprintf(stderr, "inputfile [outputfile]\n");
  exit(EXIT_FAILURE);
}


/*
 * Switch parsing:
 */

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


static int
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

    } else if (keymatch(arg, "symmetry", 1)) {
      symmetry = 1;
    } else if (keymatch(arg, "version", 7)) {
      fprintf(stderr, "jhist version %s (libjel version %s)\n",
	      JHIST_VERSION, jel_version_string());
      exit(-1);
    } else {
      usage();			/* bogus switch */
    }
  }

  return argn;			/* return index of next arg (file name) */
}


#define HSIZE 2048

/* Accumulate DCT histograms over the image: */

int ijel_print_hist(jel_config *cfg) {

  /* Assumes 64 frequencies, and an 11-bit signed integer representation: */

  int dct_hist[DCTSIZE2][HSIZE];
  struct jpeg_decompress_struct *cinfo = &(cfg->srcinfo);
  jvirt_barray_ptr *coef_arrays = cfg->coefs;

  // jel_log(cfg, "ijel_print_hist: coef_arrays = %llx\n", coef_arrays);

  static int compnum = 0;  /* static?  Really?  This is the component number, 0=luminance.  */
  int hk;
  int blk_y, bheight, bwidth, offset_y, i, k;
  JDIMENSION blocknum; //, MCU_cols;
  jvirt_barray_ptr comp_array = coef_arrays[compnum];
  jpeg_component_info *compptr;
  JCOEF *mcu;
  JBLOCKARRAY row_ptrs;

  //size_t block_row_size = (size_t) SIZEOF(JCOEF)*DCTSIZE2*cinfo->comp_info[compnum].width_in_blocks;

  //  jel_log(cfg, "ijel_print_hist: block_row_size = %d\n", block_row_size);

  bheight = cinfo->comp_info[compnum].height_in_blocks;
  bwidth = cinfo->comp_info[compnum].width_in_blocks;

  //  jel_log(cfg, "ijel_print_hist: bwidth x bheight = %dx%d\n", bwidth, bheight);

  //MCU_cols = cinfo->image_width / (cinfo->max_h_samp_factor * DCTSIZE);

  compptr = cinfo->comp_info + compnum;

  for (k = 0; k < DCTSIZE2; k++)
    for (i = 0; i < HSIZE; i++)
      dct_hist[k][i] = 0;
	  
  for (blk_y = 0; blk_y < bheight;
       blk_y += compptr->v_samp_factor) {

    row_ptrs = ((cinfo)->mem->access_virt_barray) 
      ( (j_common_ptr) cinfo, comp_array, blk_y,
	(JDIMENSION) compptr->v_samp_factor, FALSE);

    for (offset_y = 0; offset_y < compptr->v_samp_factor;
	 offset_y++) {
      for (blocknum=0; blocknum < bwidth;  blocknum++) {
	mcu =(JCOEF*) row_ptrs[offset_y][blocknum];

        for ( k = 0; k < DCTSIZE2; k++ ) {
          hk = mcu[k] + (HSIZE >> 1);
          dct_hist[k][hk]++;
        }
      }
    }
  }

  if (symmetry) {
    int f0, f1;
    /* Should be the same organization as below, so change this: */
    for ( k = 0; k < DCTSIZE2; k++) {
      f1 = f0 = 0;
      for (i = 0; i < HSIZE; i++) {
        if ( i < (HSIZE>>1) ) {
          if ( i & 1 ) f0 += dct_hist[k][i];
          else f1 += dct_hist[k][i];
        } else if ( i > (HSIZE>>1) ) {
          if ( i & 1 ) f1 += dct_hist[k][i];
          else f0 += dct_hist[k][i];
        }
      }
      printf("%d %d %d %d\n", k, f0, f1, f1-f0);
    }
  } else {
    /*
     * By default, loop over bins (going from -1024 to 1024), and for
     * each amplitude bin, print each of the 64 values (one per
     * frequency):
     */
    for (i = 0; i < HSIZE; i++) {    /* Loop over amplitude bins */
      printf("%d", i);
      /* Now loop over frequencies: */
      for (k = 0; k < DCTSIZE2; k++) printf(" %d", dct_hist[k][i]);
      printf("\n");
    }
  }

  return k;
}


/* 
 * Print the DCT histograms and then clean up:
 */

static int jel_print_hist( jel_config * cfg ) {
  ijel_print_hist(cfg);

  (void) jpeg_finish_decompress(&(cfg->srcinfo));
  jpeg_destroy_decompress(&(cfg->srcinfo));

  return 0;

}


/*
 * The main program.
 */

int
main (int argc, char **argv)
{
  jel_config *jel;
  int ret;
  int k;
  //int bytes_written;
  FILE *sfp, *output_file;

  
  jel = jel_init(JEL_NLEVELS);

  ret = jel_open_log(jel, "/tmp/jhist.log");
  if (ret == JEL_ERR_CANTOPENLOG) {
    fprintf(stderr, "Can't open /tmp/jhist.log!!\n");
    jel->logger = stderr;
  }

  jel_log(jel, "jhist version %s (libjel version %s)\n",
	  JHIST_VERSION, jel_version_string());

  progname = argv[0];
  if (progname == NULL || progname[0] == 0)
    progname = "jhist";		/* in case C library doesn't provide it */

  if (argc <= 1) usage();

  k = parse_switches(jel, argc, argv);

  sfp = fopen(argv[k], "rb");
  if ( !sfp ) {
    fprintf(stderr, "Error: Can't open input file %s!\n", argv[k]);
    exit(EXIT_FAILURE);
  }

  ret = jel_set_fp_source(jel, sfp);
  if (ret != 0) {
    fprintf(stderr, "Error: Can't set fp source!\n");
    exit(EXIT_FAILURE);
  }

  jel_log(jel,"%s: jhist input %s\n", progname, argv[k]);

  k++;

  if (argc == k && !outfilename) output_file = stdout;
  else {
    if (!outfilename) outfilename = strdup(argv[k]);

    output_file = fopen(outfilename, "wb");
    jel_log(jel,"%s: jhist output %s\n", progname, outfilename);
  }

  if (ret != 0) {
    jel_log(jel,"%s: jel error %d\n", progname, ret);
    fprintf(stderr, "Error - exiting (need a diagnostic!)\n");
    exit(EXIT_FAILURE);
  }

  jel_print_hist(jel);

  jel_close_log(jel);

  if (sfp != NULL && sfp != stdin) fclose(sfp);
  if (output_file != NULL && output_file != stdout) fclose(output_file);

  return 0;			/* suppress no-return-value warnings */
}
