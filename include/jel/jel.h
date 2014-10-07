/*
 * libjel: JPEG Embedding Library.  Embeds messages in selected
 * frequency components of a JPEG source, and emits the result on a
 * JPEG destination.  Sources and destinations can be named files,
 * FILE* streams, or memory areas.
 *
 * libjel header.  Defines basic structs and functions for libjel.
 *
 */


#ifndef __JEL_H__

#include <string.h>


//iam: these are needed for jpeglib.h
#include <stddef.h>
#include <stdio.h>

#include <jpeglib.h>
#include <jerror.h>

#include <stdarg.h>

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


#define JEL_NLEVELS 8  /* Default number of quanta for each freq. */

/*
 * A trivial frequency spec for now.  Later, we might want a way to
 * generate varying sequences based on a shared secret.  We will also
 * fold the bit packing strategy in this struct, when it becomes
 * necessary:
 */

  typedef struct {
    int nfreqs;
    int nlevels;
    unsigned int seed;
    int freqs[DCTSIZE2]; /* List of admissible frequency indices. */
    int in_use[DCTSIZE2];   /* List of frequency indices that we will use. */
  } jel_freq_spec;


/* ECC methods - for now, only libecc / rscode is supported, and only
 * if it is found by cmake:
 */

#define JEL_ECC_NONE    0
#define JEL_ECC_RSCODE  1

  /* #define JEL_ECC_BLKSIZE 200 */


/*
 * jel_config struct contains information to be used during the
 * embedding and extraction operations.
 */

typedef struct {

  /* We will always need a source. */
  struct jpeg_decompress_struct srcinfo;
  FILE *srcfp;   /* Non-NULL iff. we are using filenames or FILEs. */

  /* For embedding, we need a destination.  NULL otherwise. */
  struct jpeg_compress_struct dstinfo;
  FILE *dstfp;   /* Non-NULL iff. we are using filenames or FILEs. */

  /* It might be necessary to allocate a separate coefficient array
   * for the destination: */
  jvirt_barray_ptr * coefs;
  jvirt_barray_ptr * dstcoefs;

  struct jpeg_error_mgr  jerr;

  FILE * logger;       /* FILE pointer for logging */

  int verbose;         /* Decide how much to spew out in the
                        * log. Default is 1.  3 is pretty chatty. */

  int quality;         /* Base JPEG quality.  If -1, then the
			* destination preserves the source quality
			* level.  Is set through the jel_setprop call,
			* which will automatically recompute output
			* quant tables.  If not -1, then message
			* embedding uses this quality level. */

  int extract_only;    /* If this is 1, then the dstinfo object is
			  ignored and we will only extract a message.
			  If 0, then we assume that we're
			  embedding. */

  jel_freq_spec freqs; /* The frequency component indices we plan to
			   use for embedding and extraction. */

  int embed_length;    /*  1 if the message length is embedded in the
			   image. */

  int jpeglen;         /* Length of the JPEG-compressed data after embedding.
			  This is -1 if the length cannot be determined. */

  int len;             // Actual length of data
  int maxlen;          // Maximum length of data
  unsigned char *data;   // Data to be encoded
  unsigned char *plain;  // If ECC is in use, NULL or the plain text.

  int jel_errno;
  int ecc_method;
  int ecc_blocklen;
  int bits_per_freq;
  int bytes_per_mcu;

} jel_config;



/*
 * Valid properties for getprop / setprop:
 */
typedef enum {
  JEL_PROP_QUALITY,
  JEL_PROP_EMBED_LENGTH,
  JEL_PROP_ECC_METHOD,
  JEL_PROP_ECC_BLOCKLEN,
  JEL_PROP_FREQ_SEED,
  JEL_PROP_NFREQS,
  JEL_PROP_BYTES_PER_MCU,
  JEL_PROP_BITS_PER_FREQ,
} jel_property;




jel_config * jel_init( int nlevels );  /* Initialize and requests at least 'nlevels' quanta for each freq. */


void jel_free( jel_config *cfg );   /* Free the jel_config struct: */

void jel_describe( jel_config *cfg ); /* Describe the jel object in the log */

/*
 * Set source and destination objects - return 0 on success, negative
 * error code on failure.
 */
int jel_set_file_source(jel_config * cfg, char * filename);
int jel_set_fp_source(jel_config * cfg, FILE *fp);
int jel_set_mem_source(jel_config * cfg, unsigned char *mem, int len);

int jel_set_file_dest(jel_config * cfg, char *filename);
int jel_set_fp_dest(jel_config * cfg, FILE *fp);
int jel_set_mem_dest(jel_config * cfg, unsigned char *mem, int len);


/* Get and set jel_config properties.
 *
 * Examples of properties: 
 *   frequency components
 *   quality
 *   source
 *   destination
 */

int jel_getprop( jel_config * cfg, jel_property prop );
int jel_setprop( jel_config *cfg, jel_property prop, int value );

void jel_error( jel_config * cfg );
char *jel_version_string();

int jel_open_log( jel_config *cfg, char *filename);
int jel_close_log( jel_config *cfg);
int jel_log( jel_config *cfg, const char *format, ... );


/* where:
 *
 * cfg       A properly initialized jel_config object
 *
 * Prints a string on stderr describing the most recent error.
 */


int    jel_error_code( jel_config * cfg );    /* Returns the most recent error code. */
char * jel_error_string( jel_config * cfg );  /* Returns the most recent error string. */

/*
 * jel_capacity returns the number of plaintext bytes we can save in
 * the object.  This is computed taking into account any overhead
 * incurred by the chosen coding schemes (e.g., Reed-Solomon).
 *
 * jel_raw_capacity returns the total number of bytes that can be
 * stored in the object.
 */
int    jel_capacity( jel_config * cfg );      /* Returns the capacity in bytes of the source. */
int    jel_raw_capacity( jel_config * cfg );  /* Returns the "raw" capacity in bytes of the source. */

/* Allocates a buffer that is sufficient to hold any message
 * (per-frame) in the source.  Any such buffer can be passed to
 * free().
 */
void * jel_alloc_buffer( jel_config * cfg );

/*  Embed a message in an image: */

int jel_embed( jel_config * cfg, unsigned char * msg, int len);

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


/* Extract a message from an image. */

int jel_extract( jel_config * cfg, unsigned char * msg, int len);
/* where:
 *
 * cfg       A properly initialized jel_config object
 * msg       A region of memory to contain the extracted message
 * len       The maximum size in bytes of msg
 *
 * Returns the number of bytes that were extracted, or a negative error
 * code.  Note that if 'msg' is not large enough to hold the extracted
 * message, a negative error code will be returned, but msg will still
 * contain the bytes that WERE extracted.
 */

/*
 * Error Codes:
 */

#define JEL_SUCCESS		 0
#define JEL_ERR_NOSUCHPROP	-2
#define JEL_ERR_BADDIMS         -3
#define JEL_ERR_NOMSG           -4
#define JEL_ERR_CANTOPENLOG     -5
#define JEL_ERR_CANTCLOSELOG    -6
#define JEL_ERR_CANTOPENFILE    -7
#define JEL_ERR_INVALIDFPTR     -8
#define JEL_ERR_NODEST          -9

#ifdef __cplusplus
} /* close extern "C" { */
#endif

#define __JEL_H__
#endif
