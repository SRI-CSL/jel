/*
 * JPEG Embedding Library - jel.c
 *
 * libjel internals - this file contains internal message embedding
 * and extraction code.  Not intended to be exposed as an API.
 *
 * -Chris Connolly
 * 
 * 1/20/2014
 *
 */

#include <math.h>
#include "jel/jpeg-6b/jinclude.h"
#include "jel/jpeg-6b/jpeglib.h"
#include "jel/jel.h"

#ifdef ECC
unsigned char *ijel_encode_ecc(unsigned char *, int, int *);
unsigned char *ijel_decode_ecc(unsigned char *, int, int *);
unsigned char *ijel_encode_ecc_nolength(unsigned char *, int, int *);
unsigned char *ijel_decode_ecc_nolength(unsigned char *, int, int);
int ijel_ecc_sanity_check(unsigned char *, int);
int ijel_message_ecc_length(int);
int ijel_ecc_block_length(int);
int ijel_set_ecc_blocklen(int);
int ijel_get_ecc_blocklen();
#endif

/*
 * findFreqs: Given a quant table, find frequencies that have at
 * least 'nlevels' quanta.  Return an array of indices, starting at
 * the highest (most heavily quantized) and working toward the lowest,
 * but containing not more than nfreq component indices.  These
 * components will be used for encoding.
 *
 */

int ijel_find_freqs(JQUANT_TBL *q, int *i, int nfreq, int nlevels) {
  int j, quanta, m;

  m = 0;

  /* Corresponds to JDCT_ISLOW, in which the quantval[j] are shifted
   * up by 3 bits, and the corresponding range for DCT coefs is
   * [-1024,1023] ([0,2047] unsigned).
   *
   * Note that quantval[j] is a 16-bit unsigned quantity.
   */

  for (j = DCTSIZE2-1; j >= 0; j--) {
    if (m < nfreq) {
      quanta = 255 / q->quantval[j];
      if (quanta >= nlevels) {
        i[m] = j;
        m++;
      }
    }
  }

  return m;
}


int * ijel_get_quanta(JQUANT_TBL *q, int *quanta) {
  int j;

  for (j = DCTSIZE2-1; j >= 0; j--)
    quanta[j] = 255 / q->quantval[j];

  return quanta;
}


/* More information could be stored in Message structs, e.g., how many
 * bits per freq. to use.
 */

static void insert_byte( unsigned char v, int *freq, JCOEF *mcu ) {
  mcu[ freq[0] ] = (0x3 &  v      );
  mcu[ freq[1] ] = (0x3 & (v >> 2));
  mcu[ freq[2] ] = (0x3 & (v >> 4));
  mcu[ freq[3] ] = (0x3 & (v >> 6));
}


static unsigned char extract_byte( int *freq, JCOEF *mcu ) {
  return
      (0x03 &  mcu[ freq[0] ])       |
    (  0x0C & (mcu[ freq[1] ] << 2)) |
    (  0x30 & (mcu[ freq[2] ] << 4)) |
    (  0xC0 & (mcu[ freq[3] ] << 6));
}


/*
 * Primary embedding function:
 *
 * By default, this function will wedge the message into the
 * monochrome component, 1 byte per MCU.  Returns the number of
 * characters inserted.
 *
 * We check for ECC here.  If present and requested, ECC will be
 * performed and written to the buffer 'message'.  Otherwise 'message'
 * simply points to the message buffer itself.  After embedding, any
 * ECC buffer must be deallocated.
 */

int ijel_stuff_message(jel_config *cfg) {

  struct jpeg_decompress_struct *cinfo = &(cfg->srcinfo);
  struct jpeg_compress_struct *dinfo = &(cfg->dstinfo);
  jvirt_barray_ptr *coef_arrays = cfg->coefs;
  FILE *logger = cfg->logger;
  jel_freq_spec *fspec = &(cfg->freqs);
  unsigned char *message = cfg->data;
  int msglen = cfg->len;

#ifdef ECC
  unsigned char *raw = cfg->data;
  int ecc = 0;
  int plain_len = 0;
#endif

  /* This could use some cleanup to make sure that we really need all
   * these variables! */
  int echo = 0;
  int embed_k = 4; /* Allows us to embed 4 bytes of length. */
  int compnum = 0; /* Component (0 = luminance, 1 = U, 2 = V) */
  int length_in;
  /* need to be able to know what went wrong in deployments */
  int debug = (logger != NULL);
  unsigned char byte;
  int blk_y, bheight, bwidth, offset_y, i, k;
  //  JDIMENSION blocknum, MCU_cols;
  JDIMENSION blocknum;
  jvirt_barray_ptr comp_array = coef_arrays[compnum];
  jpeg_component_info *compptr;
  JQUANT_TBL *qtable;
  JCOEF *mcu;
  JBLOCKARRAY row_ptrs;
  //size_t block_row_size = (size_t) SIZEOF(JCOEF)*DCTSIZE2*cinfo->comp_info[compnum].width_in_blocks;

  jel_log(cfg, "ijel_stuff_message: 1st 5 bytes of plain text = %d %d %d %d %d\n", 
	  raw[0], raw[1], raw[2], raw[3], raw[4]);

#if ECC
  plain_len = msglen; /* Save the plaintext length */

  if (jel_getprop(cfg, JEL_PROP_ECC_METHOD) == JEL_ECC_RSCODE) {

    if (ijel_ecc_sanity_check(raw, msglen))
      jel_log(cfg, "ijel_stuff_message: FYI, sanity check failed.\n");

    if (!cfg->embed_length) message = ijel_encode_ecc_nolength(raw, msglen, &i);
    else message = ijel_encode_ecc(raw,  msglen, &i);

    jel_log(cfg, "ijel_stuff_message: 1st 5 bytes of ECC data = %d %d %d %d %d\n", 
	    message[0], message[1], message[2], message[3], message[4]);

    if (!message) message = raw; /* No ecc */
    else {
      jel_log(cfg, "ijel_stuff_message: ECC enabled, %d bytes of message encoded in %d bytes.\n", msglen, i); 
      msglen = i;
      ecc = 1;
    }
  }
#endif

  /* If not already specified, find a set of frequencies suitable for
     embedding 8 bits per MCU.  Use the destination object, NOT cinfo,
     which is the source: */
  if (fspec->nfreqs == 0) {
    /* If we explicitly set the output quality, then this will be
     * non-NULL, but otherwise we will need to get the tables from the
     * source: */
    qtable = dinfo->quant_tbl_ptrs[0];
    if (!qtable) qtable = cinfo->quant_tbl_ptrs[0];

    fspec->nfreqs = ijel_find_freqs(qtable, fspec->freqs, 4, fspec->nlevels);
  }

  /* Check to see that we have at least 4 good frequencies.  This
     implicitly assumes that we are packing 8 bits per MCU.  We will
     want to change that in future versions. */ 
  if (fspec->nfreqs < 4) {
    if( debug ) {
      jel_log(cfg, "ijel_stuff_message: Sorry - not enough good frequencies at this quality factor.\n");
      fflush(logger);
    }
    return 0;
  }
  

  /* If requested, be verbose: */
  if ( debug ) {     /* Do we want to use verbosity levels? */
    jel_log(cfg, "(:components #(");
    for (i = 0; i < fspec->nfreqs; i++) jel_log(cfg, "%d ", fspec->freqs[i]);
    jel_log(cfg, "))\n");
  }

  /* Need to double check the message length, to see whether it's
     compatible with the image.  Message gets truncated if it's longer
     than the number of MCU's in the luminance channel.  We will want
     to expand to the color components too:
   */
  bheight = cinfo->comp_info[compnum].height_in_blocks;
  bwidth = cinfo->comp_info[compnum].width_in_blocks;
  //  printf("In theory, we can store %d bytes\n", bheight*bwidth);

  //  MCU_cols = cinfo->image_width / (cinfo->max_h_samp_factor * DCTSIZE);
  compptr = cinfo->comp_info + compnum;

  /* Redundant counters?  4 bytes from length_in will be embedded>.. */
  length_in = msglen;
  k = 0;

  if (!cfg->embed_length) embed_k = 0;
  else jel_log(cfg, "ijel_stuff_message: embedded length = %d bytes\n", length_in);

  for (blk_y = 0; blk_y < bheight && k < msglen;
       blk_y += compptr->v_samp_factor) {

    row_ptrs = ( (cinfo)->mem->access_virt_barray ) 
      ((j_common_ptr) cinfo,
       comp_array,
       blk_y,
       (JDIMENSION) compptr->v_samp_factor,
       TRUE);

    for (offset_y = 0; offset_y < compptr->v_samp_factor && k < msglen;
	 offset_y++) {

      for (blocknum=0; blocknum < bwidth && k < msglen; blocknum++) {
	mcu =(JCOEF*) row_ptrs[offset_y][blocknum];

	if (embed_k > 0) {  /* Message length goes first: */
	  byte = (unsigned char) (0xFF & length_in);
	  insert_byte( byte, fspec->freqs, mcu );
	  length_in = length_in >> 8;
	  embed_k--;
	} else {            /* Bytes of the message: */
	  if (echo) printf("%c", message[k]);
	  insert_byte( (unsigned char) (message[k] & 0xFF), fspec->freqs, mcu );
	  k++;
	}
      }
    }
  }

#if ECC
  /* If we call */
  if (ecc) {
    free(message);
    /* If we got here, we were successful and should set the return
     * value to be the number of bytes of plaintext that were
     * embedded: */
    k = plain_len;
  }
  
#endif

  return k;
}



/* Extract a message from the DCT info: */

int ijel_unstuff_message(jel_config *cfg) {

  struct jpeg_decompress_struct *cinfo = &(cfg->srcinfo);
  jvirt_barray_ptr *coef_arrays = cfg->coefs;
  FILE *logger = cfg->logger;
  jel_freq_spec *fspec = &(cfg->freqs);
  unsigned char *message = cfg->data;

  static int compnum = 0;  /* static?  Really?  This is the component number, 0=luminance.  */
  int plain_len = 0;
  int msglen = 0;
  int embed_k = 4;         /* For now, we will always embed 4 bytes of message length first. */
  int length_in = 0;
  int bits_up = 0;
  int echo = 0;
  int v;
  int blk_y, bheight, bwidth, offset_y, i, k;
  JDIMENSION blocknum; // , MCU_cols;
  jvirt_barray_ptr comp_array = coef_arrays[compnum];
  jpeg_component_info *compptr;
  JCOEF *mcu;
  JBLOCKARRAY row_ptrs;

  /* need to be able to know what went wrong in deployments */
  int debug = (logger != NULL);
  //size_t block_row_size = (size_t) SIZEOF(JCOEF)*DCTSIZE2*cinfo->comp_info[compnum].width_in_blocks;

  /* This uses the source quant tables, which is fine: */
  if (fspec->nfreqs == 0)
    fspec->nfreqs = ijel_find_freqs( cinfo->quant_tbl_ptrs[0], fspec->freqs, 4, fspec->nlevels);

  if ( fspec->nfreqs < 4 ) {
    if(debug){
      jel_log(cfg, "ijel_unstuff_message: Sorry - not enough good frequencies at this quality factor.\n");
    } else {
      printf("Sorry - not enough good frequencies at this quality factor.\n");
    }
    return -1;
  }

  if ( cinfo->err->trace_level > 0 ) {
    printf("(:components #(");
    for (i = 0; i < fspec->nfreqs; i++) printf("%d ", fspec->freqs[i]);
    printf("))\n");
  }

  bheight = cinfo->comp_info[compnum].height_in_blocks;
  bwidth = cinfo->comp_info[compnum].width_in_blocks;
  //  printf("In theory, we can store %d bytes\n", bheight*bwidth);

  //  MCU_cols = cinfo->image_width / (cinfo->max_h_samp_factor * DCTSIZE);
  compptr = cinfo->comp_info + compnum;

  /* Initialize msglen to some positive value.  We will reset this
     once we get the length in: */
  if ( cfg->embed_length ) msglen = 4;  
  else {
    /* If the length is not embedded, it was supplied on the command
     * line and passed in through the message 'len' field:
     */
    embed_k = 0;
    msglen = length_in = cfg->len;
#if ECC
    plain_len = msglen;
    /* If ECC is in use, then we need to adjust msglen, since it will
     * be the length in bytes of the original message. */
    if (cfg->ecc_method == JEL_ECC_RSCODE) {
      //      msglen = length_in = ijel_ecc_length(msglen);
      msglen = length_in = ijel_message_ecc_length(msglen);
      jel_log(cfg, "ijel_unstuff_message: msglen=%d, length_in=%d, cfg->len=%d\n",
	      msglen, length_in, cfg->len);
    }
#endif    
  }

  jel_log(cfg, "ijel_unstuff_message: msglen=%d, length_in=%d, cfg->len=%d\n",
	  msglen, length_in, cfg->len);
	  
  k = 0;

  for (blk_y = 0; blk_y < bheight && k < msglen;
       blk_y += compptr->v_samp_factor) {

    row_ptrs = ((cinfo)->mem->access_virt_barray) 
      ( (j_common_ptr) cinfo, comp_array, blk_y,
	(JDIMENSION) compptr->v_samp_factor, FALSE);

    for (offset_y = 0; offset_y < compptr->v_samp_factor && k < msglen;
	 offset_y++) {
      for (blocknum=0; blocknum < bwidth && k < msglen;  blocknum++) {
	mcu =(JCOEF*) row_ptrs[offset_y][blocknum];

	v = extract_byte(fspec->freqs, mcu);

	if (embed_k <= 0) {
	  message[k++] = v;
	  if (echo) printf("%c", v);
	} else {  /* Message length goes first: */
	  length_in = length_in | (v << bits_up);
	  bits_up += 8;
	  embed_k--;
	  if (embed_k <= 0) {
	    msglen = length_in;
	    if (msglen > cfg->maxlen) msglen = cfg->maxlen;
	    cfg->len = msglen;
	  }
	}
      }
    }
  }

  if (cfg->embed_length) jel_log(cfg, "ijel_unstuff_message: embedded length = %d bytes\n", length_in);

#if ECC
  if (jel_getprop(cfg, JEL_PROP_ECC_METHOD) == JEL_ECC_RSCODE) {
    /* If we have reached here, we are using rscode for Reed-Solomon
     * error correction.  The codeword is in 'message', obtained from
     * cfg->data, but must be RS decoded to reconstruct the original
     * plaintext.  The value of k needs to be ceiling'ed up to the
     * nearest block to get all of the ECC blocks.
     */
    int truek;
    unsigned char *raw;
    truek = ijel_ecc_block_length(k);
    jel_log(cfg, "ijel_unstuff_message: ijel_ecc_length(%d) => %d\n", k, truek);

    jel_log(cfg, "ijel_unstuff_message: 1st 5 bytes of ECC data = %d %d %d %d %d\n", 
	    message[0], message[1], message[2], message[3], message[4]);

    /* If we are not embedding length, then plaintext length is a
     * shared secret and we pass it: */
    if (cfg->embed_length) raw = ijel_decode_ecc(message,  truek, &i);
    else {
      raw = ijel_decode_ecc_nolength(message, truek, plain_len);
      i = plain_len;
    }

    /* 'raw' is a newly-allocated buffer.  When should it be freed?? */
    if (raw) {
      jel_log(cfg, "ijel_unstuff_message: ECC enabled, %d bytes of ECC data decoded into %d bytes of message.\n", k, i); 
      /* Fails on Linux: */
      //      free(message);
      k = i;

      /* What happens to the original buffer in cfg->data??? */
      //      cfg->data = raw;
      memcpy(cfg->data, raw, k);

      jel_log(cfg, "ijel_unstuff_message: 1st 5 bytes of plain text = %d %d %d %d %d\n", 
	      raw[0], raw[1], raw[2], raw[3], raw[4]);
 
      /* Raw was allocated above solely because of ECC.  Free it here? */
      free(raw);

   }

  }
#endif

  cfg->len = k;
  jel_log(cfg, "ijel_unstuff_message: k=%d\n", k);

  return k;
}



void ijel_print_qtable(jel_config *c, JQUANT_TBL *a) {
  int i;

  for (i = 0; i < DCTSIZE2; i++) {
    if (i % 8 == 0) jel_log(c, "\n");
    jel_log(c, "%4d ", a->quantval[i]);
  }
  jel_log(c, "\n");
}



void ijel_log_qtables(jel_config *c) {
  struct jpeg_compress_struct *comp;
  struct jpeg_decompress_struct *decomp;
  JQUANT_TBL *qptr;
  int i;

  decomp = &(c->srcinfo);

  jel_log(c, "Quant tables for source:\n");
  for (i = 0; i < NUM_QUANT_TBLS; i++) {
    qptr = decomp->quant_tbl_ptrs[i];
    if (qptr != NULL) {
      jel_log(c, "%lx\n", (long unsigned int) qptr);
      ijel_print_qtable(c, qptr);
    }
  }
  jel_log(c, "\n\n");

  comp = &(c->dstinfo);

  jel_log(c, "Quant tables for destination:\n");
  for (i = 0; i < NUM_QUANT_TBLS; i++) {
    qptr = comp->quant_tbl_ptrs[i];
    if (qptr != NULL) {
      jel_log(c, "%lx\n", (long unsigned int) qptr);
      ijel_print_qtable(c, qptr);
    }
  }
  jel_log(c, "\n");
}



void ijel_log_hufftables(jel_config *c) {
  return;
}


void ijel_buffer_dump( unsigned char *data, int nbytes) {
  int i;
  for (i = 0; i < nbytes; i++) printf(" %d ", data[i]);
  printf("\n");
}
