/*
 * JPEG Embedding Library - jel.c
 *
 * This file defines the top-level API for libjel.
 *
 * -Chris Connolly
 * 
 * 1/20/2014
 *
 */

#include <jel/jel.h>

#include "misc.h"

#define WEDGEDEBUG 0
#define UNWEDGEDEBUG 0

/* 
 * iam: we need to be able to be able to fail more gracefully than exit
 * though maybe using setjmp and longjmp doesn't really qualify as
 * graceful. 
 */
#include <setjmp.h>


void jpeg_memory_src (j_decompress_ptr cinfo, unsigned char *data, int size);
void jpeg_memory_dest (j_compress_ptr cinfo, unsigned char* data, int size);



int ijel_stuff_message(jel_config *cfg);
int ijel_unstuff_message(jel_config *cfg);

int ijel_capacity_ecc(int);
int ijel_set_ecc_blocklen(int);


char* jel_error_strings[] = {
  "Success",
  "Unknown",
  "No such property"
};

/*
 * This library needs to be built with -DJEL_VERSION=XXX on the
 * command line.  Be aware that the library version number is
 * currently maintained in the cmake scripts, which will also define
 * the constant using -D.
 */
char *jel_version_string() {
  return(JEL_VERSION);
}

/*
 * Create and initialize a jel_config object.  This object controls
 * embedding and extraction.
 */

jel_config * jel_init( int nlevels ) {
  jel_config * result;
  int ijel_get_ecc_blocklen();
  struct jpeg_compress_struct *cinfo;

  /* Allocate: */
  result = calloc( 1, sizeof(jel_config) );

  /* Initialize the quantum request: */
  result->freqs.nlevels = nlevels;

  /* also zero the nfreqs (valgrind) */
  result->freqs.nfreqs = 0;

  /* Zero seed implies fixed set of frequencies.  Nonzero seed implies
   * use of srand() to generate frequencies. */
  result->freqs.seed = 0;

  /* better zero this too (valgrind); maybe calloc the whole structure?? */
  result->dstfp =  NULL;

  /* This is the output quality.  A quality of -1 means that the same
   * quant tables are used for input and output.  */

  result->verbose = 1;  /* "Normal" logging.  */

  result->quality = -1;

  /* Set up the decompression object: */
  result->srcinfo.err = jpeg_std_error(&result->jerr);
  jpeg_create_decompress( &(result->srcinfo) );
  //  jpeg_set_defaults( &(result->srcinfo) );

  
  /* Set up the compression object: */
  result->dstinfo.err = jpeg_std_error(&result->jerr);
  cinfo = &(result->dstinfo);
  jpeg_create_compress( cinfo );
  cinfo->in_color_space = JCS_GRAYSCALE;
  cinfo->input_components = 1;
  cinfo->dct_method = JDCT_ISLOW;
  jpeg_set_defaults( cinfo );
  jpeg_set_quality( cinfo, 75, FALSE );

  result->srcinfo.dct_method = JDCT_ISLOW; /* Force this as the default. */
  result->dstinfo.dct_method = JDCT_ISLOW; /* Force this as the default. */

  /* Scan command line options, adjust parameters */

  /* Note that the compression object might not be used.  If
   * jel_set_XXX_dest() is called, then we assume that a message is to
   * be embedded and we will need it.  Otherwise, we assume that we
   * are extracting, and will not do anything else with it.
   */

#if 0
  /* Taks a look at this and see if it's sufficient for us to use as
     an error handling mechanism for libjel: */

  /* Add some application-specific error messages (from cderror.h) */
  jerr.addon_message_table = cdjpeg_message_table;
  jerr.first_addon_message = JMSG_FIRSTADDONCODE;
  jerr.last_addon_message = JMSG_LASTADDONCODE;
#endif

  /* Some more defaults: */

  result->embed_length = 1;  /* Embed message length by default.  What about ECC? */
  result->jpeglen = 0;

  /* Should this be the default? */
  result->ecc_method = JEL_ECC_RSCODE;

  // result->ecc_method = JEL_ECC_NONE;
  result->ecc_blocklen = ijel_get_ecc_blocklen();
  
  //  ijel_init_freq_spec(result->freqs);

  /* Be careful.  We need to check error conditions. */
  result->jel_errno = 0;

  /* How to pack: */
  result->bits_per_freq = 2;
  result->bytes_per_mcu = 1;

  /* MCU energy threshold is 20.0: */
  result->ethresh = 20.0;

  return result;
}



void jel_free( jel_config *cfg ) {
  /* Does anything else need to be freed here? */
  jpeg_destroy_decompress(&cfg->srcinfo);
  jpeg_destroy_compress(&cfg->dstinfo);
  memset(cfg, 0, sizeof(jel_config));
  free(cfg);
  return;
}



void jel_describe( jel_config *cfg ) {
  int i, nf;
  jel_log(cfg, "jel_config Object 0x%x {\n", cfg);
  jel_log(cfg, "    srcfp = 0x%x,\n", cfg->srcfp);
  jel_log(cfg, "    dstfp = 0x%x,\n", cfg->dstfp);
  jel_log(cfg, "    quality = %d,\n", cfg->quality);
  jel_log(cfg, "    extract_only = %d,\n", cfg->extract_only);
  jel_log(cfg, "    freqs = ( ");
  nf = cfg->freqs.nfreqs;
  for (i = 0; i < nf; i++) jel_log(cfg, " %d ", cfg->freqs.freqs[i]);
  jel_log(cfg, "),\n");
  jel_log(cfg, "    embed_length = %d,\n", cfg->embed_length);
  jel_log(cfg, "    jpeglen = %d,\n", cfg->jpeglen);
  jel_log(cfg, "    len = %d,\n", cfg->len);
  jel_log(cfg, "    maxlen = %d,\n", cfg->maxlen);
  jel_log(cfg, "    jel_errno = %d,\n", cfg->jel_errno);
  jel_log(cfg, "    ecc_method = ");
  if (cfg->ecc_method == JEL_ECC_NONE) jel_log(cfg, "NONE,\n");
  else if (cfg->ecc_method == JEL_ECC_RSCODE) jel_log(cfg, "RSCODE,\n");
  else jel_log(cfg, "UNRECOGNIZED,\n");
  jel_log(cfg, "    ecc_blocklen = %d\n", cfg->ecc_blocklen);
  jel_log(cfg, "}\n");
}


typedef struct jel_error_mgr {
  struct jpeg_error_mgr mgr;	
  jmp_buf jmpbuff;	
} *jel_error_ptr;


/* iam: use this rather than exit - which is a bit rude as a library */
static void jel_error_exit (j_common_ptr cinfo){

  jel_error_ptr jelerr = (jel_error_ptr) cinfo->err;

  (*cinfo->err->output_message) (cinfo);

  longjmp(jelerr->jmpbuff, 1);

}

    
/*
 * Internal function to open the source and get coefficients:
 */
static int ijel_open_source(jel_config *cfg) {
  int ci;
  jvirt_barray_ptr *coef_arrays = NULL;
  jpeg_component_info *compptr;
  struct jpeg_decompress_struct *srcinfo = &(cfg->srcinfo);
  struct jpeg_compress_struct *dstinfo = &(cfg->dstinfo);
  /* Read file header, set default decompression parameters */
  jpeg_read_header( srcinfo, TRUE);

  /* Read the file as arrays of DCT coefficients: */
  cfg->coefs = jpeg_read_coefficients( srcinfo );

  coef_arrays = (jvirt_barray_ptr *)
      (*dstinfo->mem->alloc_small) ((j_common_ptr) srcinfo, JPOOL_IMAGE,
                                    SIZEOF(jvirt_barray_ptr) * srcinfo->num_components);
  for (ci = 0; ci < srcinfo->num_components; ci++) {
    compptr = srcinfo->comp_info + ci;
    coef_arrays[ci] = (*dstinfo->mem->request_virt_barray)
      ((j_common_ptr) dstinfo, JPOOL_IMAGE, FALSE,
       (JDIMENSION) round_up((long) compptr->width_in_blocks,
                              (long) compptr->h_samp_factor),
       (JDIMENSION) round_up((long) compptr->height_in_blocks,
                              (long) compptr->v_samp_factor),
       (JDIMENSION) compptr->v_samp_factor);
  }
  cfg->dstcoefs = coef_arrays;

  /* Copy the source parameters to the destination object. This sets
   * up the default transcoding environment.  From this point on, the
   * caller can modify the compressor (destination) object as
   * needed. */

  jpeg_copy_critical_parameters( &(cfg->srcinfo), &(cfg->dstinfo) );

  jel_log(cfg, "ijel_open_source: all done.\n");

  cfg->jel_errno = 0;
  return 0;  /* TO DO: more detailed error reporting. */
}




/*
 * Set the source to be a FILE pointer:
 */
int jel_set_fp_source( jel_config *cfg, FILE *fpin ) {

  if (fpin == NULL) return JEL_ERR_INVALIDFPTR;
    
  jpeg_stdio_src( &(cfg->srcinfo), fpin );

  return ijel_open_source( cfg );
}




/*
 * Set the destination to be a FILE pointer:
 */
int jel_set_fp_dest( jel_config *cfg, FILE *fpout ) {

  if (fpout == NULL) return JEL_ERR_INVALIDFPTR;

  jpeg_stdio_dest( &(cfg->dstinfo), fpout );

  cfg->jel_errno = 0;

  return 0;
}



/*
 * In-memory source and destination
 */
int jel_set_mem_source( jel_config *cfg, unsigned char *mem, int size ) {

  /* graceful-ish exit on error  */

  struct jel_error_mgr jerr;
  cfg->srcinfo.err = jpeg_std_error(&jerr.mgr);
  jerr.mgr.error_exit = jel_error_exit;

  if (setjmp(jerr.jmpbuff)) { return -1; }

  jpeg_memory_src( &(cfg->srcinfo), mem, size );

  return ijel_open_source( cfg );
}


int jel_set_mem_dest( jel_config *cfg, unsigned char *mem, int size) {

  jpeg_memory_dest( &(cfg->dstinfo), mem, size );
  cfg->jel_errno = 0;

  return 0;
}



/*
 * Name a file to be used as source:
 */
int jel_set_file_source(jel_config *cfg, char *filename) {
  FILE *fp = fopen(filename, "rb");
  if (fp == NULL) return JEL_ERR_CANTOPENFILE;

  return jel_set_fp_source(cfg, fp);
}



/*
 * Name a file to be used as destination:
 */
int jel_set_file_dest(jel_config *cfg, char *filename) {
  FILE *fp = fopen(filename, "wb");
  if (fp == NULL) return JEL_ERR_CANTOPENFILE;

  return jel_set_fp_dest(cfg, fp);
}



/*
 * Get properties of the config object - returns the value as a void*
 * (to be cast to the desired type, heh heh).  If the property is not
 * recognized, return the NOSUCHPROP error code.
 */
int jel_getprop( jel_config *cfg, jel_property prop ) {

  switch( prop ) {

  case JEL_PROP_QUALITY:
    return cfg->quality;

  case JEL_PROP_EMBED_LENGTH:
    return cfg->embed_length;

  case JEL_PROP_ECC_METHOD:
    return cfg->ecc_method;

  case JEL_PROP_ECC_BLOCKLEN:
    return cfg->ecc_blocklen;

  case JEL_PROP_FREQ_SEED:
    return cfg->freqs.seed;

  case JEL_PROP_NFREQS:
    return cfg->freqs.nfreqs;

  case JEL_PROP_BYTES_PER_MCU:
    return cfg->bytes_per_mcu;

  case JEL_PROP_BITS_PER_FREQ:
    return cfg->bits_per_freq;

  }

  cfg->jel_errno = JEL_ERR_NOSUCHPROP;
  return JEL_ERR_NOSUCHPROP;

}




/*
 * Set properties of the config object - On success, returns the set
 * value as a void* (to be cast to the desired type, heh heh).  If the
 * property is not recognized, return the NOSUCHPROP error code.
 */
int jel_setprop( jel_config *cfg, jel_property prop, int value ) {
  /* What a mess.  Stuff this into an ijel function, or change the
   * freq selection API!! */
  struct jpeg_decompress_struct *cinfo = &(cfg->srcinfo);
  struct jpeg_compress_struct *dinfo = &(cfg->dstinfo);
  JQUANT_TBL *qtable;
  int ijel_find_freqs(JQUANT_TBL *, int *, int, int);
  switch( prop ) {

  case JEL_PROP_QUALITY:
    cfg->quality = value;
    /* Since jel_init initializes a compressor, we can always do this,
     * even if we don't use it: */
    jpeg_set_quality( &(cfg->dstinfo), value, FALSE );
    return value;

  case JEL_PROP_EMBED_LENGTH:
    cfg->embed_length = value;
    return value;

  case JEL_PROP_ECC_METHOD:
#ifdef ECC
    cfg->ecc_method = value;
#else
    cfg->ecc_method = JEL_ECC_NONE;;
#endif
    return value;

  case JEL_PROP_ECC_BLOCKLEN:
    cfg->ecc_blocklen = value;
#ifdef ECC
    ijel_set_ecc_blocklen(value);
#endif
    return value;

  case JEL_PROP_FREQ_SEED:
    cfg->freqs.seed = value;  /* Redundant. */
    srand(value);
    return value;

  case JEL_PROP_NFREQS:
    cfg->freqs.nfreqs = value;
    qtable = dinfo->quant_tbl_ptrs[0];
    if (!qtable) qtable = cinfo->quant_tbl_ptrs[0];
    ijel_find_freqs(qtable, cfg->freqs.freqs, value, cfg->freqs.nlevels);
    return value;

  case JEL_PROP_BYTES_PER_MCU:
    cfg->bytes_per_mcu = value;
    return value;

  case JEL_PROP_BITS_PER_FREQ:
    cfg->bits_per_freq = value;
    return value;

  }

  cfg->jel_errno = JEL_ERR_NOSUCHPROP;
  return JEL_ERR_NOSUCHPROP;

}


/*
 * Open a log file - what to do if we want to use stderr?
 * For now, caller will need to set it explicitly in the cfg object.
 */
int jel_open_log(jel_config *cfg, char *filename) {
  cfg->logger = fopen( filename, "a+" );
  if ( cfg->logger == NULL ) {
    cfg->logger = stderr;
    cfg->jel_errno = JEL_ERR_CANTOPENLOG;
    return cfg->jel_errno;
  } else {
    cfg->jel_errno = 0;
    return 0;
  }
}


/*
 * Close the log file, if any.
 */
int jel_close_log(jel_config *cfg) {
  if (cfg->logger != stderr && cfg->logger != NULL) {
    if (!fclose(cfg->logger)) {
      cfg->logger = NULL;
      cfg->jel_errno = 0;
      return 0;
    } else {
      cfg->logger = NULL;
      cfg->jel_errno = JEL_ERR_CANTCLOSELOG;
      return cfg->jel_errno;
    }
  }
  cfg->logger = NULL;
  cfg->jel_errno = 0;
  return 0;
}


int jel_log( jel_config *cfg, const char *format, ... ){
  int retval = 0;
  if ( cfg->logger != NULL ) {
    va_list arg;
    va_start(arg,format);
    retval = vfprintf(cfg->logger, format, arg); 
    va_end(arg);
    fflush(cfg->logger);
  }
  return retval;
}

/*
 * Return the most recent error code.  Stored in the 'jel_errno' slot of
 * the config object:
 */
int    jel_error_code( jel_config * cfg ) {
  /* Returns the most recent error code. */
  return cfg->jel_errno;
}

char * jel_error_string( jel_config * cfg ) {
  /* Returns the most recent error string. */
  return "Unknown";
}



/*
 * Returns an integer capacity in bytes.  Does not take into account
 * the length embedding.  It is up to the caller to decide whether to
 * embed the message length and to check that length + message will
 * fit.
 */

int    jel_capacity( jel_config * cfg ) {

  /* Returns the capacity in bytes of the source.  This is only
   * meaningful if we know enough about the srcinfo object, i.e., only
   * AFTER we have called 'jel_set_xxx_source'.  At present, only
   * luminance is considered.
   */
  int ijel_ecc_cap(int);
  int ijel_capacity(jel_config *);
  int cap1;

#if 0
  cinfo = &(cfg->srcinfo);

  /* This doesn't take into account the h_ and v_ sampling factors,
   * which might not be 1.
   */

  bwidth = cinfo->comp_info[compnum].width_in_blocks;
  bheight = cinfo->comp_info[compnum].height_in_blocks;

  if (bwidth <= 0 || bheight <= 0) {
    cfg->jel_errno = JEL_ERR_BADDIMS;
    return cfg->jel_errno;
  }

  cap1 = (bwidth * bheight);
  cap2 = (cinfo->output_width / 8) * (cinfo->output_height / 8);

  /* cap2 is consistently 0 */
  jel_log(cfg, "jel_capacity returns %d (bwidth=%d, bheight=%d, image dimension says %d)\n",
	  cap1, bwidth, bheight, cap2);

#endif

  /* This measures capacity by taking into account the energy constraints: */
  cap1 = ijel_capacity(cfg);

  /* If ECC is requested, compute capacity subject to ECC overhead: */
  if (jel_getprop(cfg, JEL_PROP_ECC_METHOD) == JEL_ECC_RSCODE) {
    cap1 = ijel_capacity_ecc(cap1);
    jel_log(cfg, "jel_capacity assuming ECC returns %d\n", cap1);
  }

  if (jel_getprop(cfg, JEL_PROP_EMBED_LENGTH) == 1) {
    cap1 = cap1 - 4;
    jel_log(cfg, "jel_capacity assuming embedded length returns %d\n", cap1);
  }

  cfg->jel_errno = JEL_SUCCESS;
  return cap1;

}



/*
 * Raw capacity - regardless of ECC, this is how many bytes we can
 * store in the image:
 */
int jel_raw_capacity(jel_config *cfg) {
  int ret;
  int prop = jel_getprop(cfg, JEL_PROP_ECC_METHOD);
  jel_setprop(cfg, JEL_PROP_ECC_METHOD, JEL_ECC_NONE);
  ret = jel_capacity(cfg);
  jel_setprop(cfg, JEL_PROP_ECC_METHOD, prop);
  return ret;
}

/* Return a buffer that holds anything up to the capacity of the source: */
void * jel_alloc_buffer( jel_config *cfg ) {
  int k = jel_raw_capacity( cfg );

  if (k > 0) return calloc(k, 1);
  else return NULL;
}



/*
 * Set up the message buffer for embedding:
 */
static void ijel_set_message(jel_config *cfg, unsigned char *msg, int msglen) {
  cfg->data = msg;
  cfg->maxlen = cfg->len = msglen;
}

/*
 * Set up the message buffer for extracting:
 */
static void ijel_set_buffer(jel_config *cfg, unsigned char *buf, int maxlen) {
  cfg->data = buf;
  cfg->maxlen = maxlen;
  cfg->len = 0;
}


static size_t ijel_get_jpeg_length(jel_config * jel) {
  int jpeg_mem_packet_size(j_compress_ptr);
  int jpeg_stdio_packet_size(j_compress_ptr);
  int k;

  if (jel->dstfp == NULL)
    k = jpeg_mem_packet_size( &(jel->dstinfo) );
  else
    k = jpeg_stdio_packet_size( &(jel->dstinfo) );

  return k;
}


/*  
 * Embed a message in an image: 
 */
int jel_embed( jel_config * cfg, unsigned char * msg, int len) {
  /* where:
   *
   * cfg       A properly initialized jel_config object
   * msg       A region of memory containing bytes to be embedded
   * len       The number of bytes of msg to embed
   *
   * Returns the number of bytes that were embedded, or a negative error
   * code.  If the return value is positive but less than 'len', call
   * 'jel_error' for more information.
   */
  int nwedge;
  void ijel_log_qtables(jel_config*);

  cfg->jpeglen = 0;

  /* If message is NULL, shouldn't we punt? */
  if ( !msg ) {
    jel_log(cfg, "No message provided!  Exiting.\n" );
    jel_close_log(cfg);
    cfg->jel_errno = JEL_ERR_NOMSG;
    return cfg->jel_errno;
  }

  /* Insert the message: */
  ijel_set_message(cfg, msg, len);
  nwedge = ijel_stuff_message(cfg);

  if ( cfg->quality > 0 ) {
    jpeg_set_quality( &(cfg->dstinfo), cfg->quality, FALSE );
    jel_log(cfg, "jel_embed: Reset quality to %d after jpeg_copy_critical_parameters.\n", cfg->quality);
  }

  //  ijel_log_qtables(cfg);

  /* Start compressor (note no image data is actually written here) */
  jpeg_write_coefficients( &(cfg->dstinfo), cfg->coefs );

  /* Copy to the output file any extra markers that we want to preserve */
  //  jcopy_markers_execute(&srcinfo, &dstinfo, JCOPYOPT_ALL);

  //  ijel_log_qtables(cfg);

  /* Finish compression and release memory */
  jpeg_finish_compress(&cfg->dstinfo);

  cfg->jpeglen = ijel_get_jpeg_length(cfg);
  jel_log(cfg, "jel_embed: JPEG compressed output size is %d.\n", cfg->jpeglen);

  //ian moved this to jel_free
  //jpeg_destroy_compress(&cfg->dstinfo);

  (void) jpeg_finish_decompress(&cfg->srcinfo);

  //ian moved this to jel_free
  //jpeg_destroy_decompress(&cfg->srcinfo);

  /* Should probably check for JPEG warnings here: */
  return nwedge; /* suppress no-return-value warnings */

}





/* 
 * Extract a message from an image. 
 */

int jel_extract( jel_config * cfg, unsigned char * msg, int maxlen) {
  /* where:
   *
   * cfg       A properly initialized jel_config object
   * msg       A region of memory to contain the extracted message
   * maxlen    The maximum size in bytes of msg
   *
   * Returns the number of bytes that were extracted, or a negative error
   * code.  Note that if 'msg' is not large enough to hold the extracted
   * message, a negative error code will be returned, but msg will still
   * contain the bytes that WERE extracted.
   */
  int msglen;

  /* graceful-ish exit on error */
  struct jel_error_mgr jerr;
  cfg->srcinfo.err = jpeg_std_error(&jerr.mgr);
  jerr.mgr.error_exit = jel_error_exit;
  
  if (setjmp(jerr.jmpbuff)) { return -1; }


  ijel_set_buffer(cfg, msg, maxlen);
  cfg->len = maxlen;
  msglen = ijel_unstuff_message(cfg);
  
  if(UNWEDGEDEBUG){
    jel_log(cfg, "jel_extract: %d bytes extracted\n", msglen);
  }    

  (void) jpeg_finish_decompress(&(cfg->srcinfo));

  //ian moved this to jel_free
  //jpeg_destroy_decompress(&(cfg->srcinfo));

  return msglen;	

}
