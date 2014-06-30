#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <ecc.h>

// #define BLOCKLEN 127      /* Use 128 for now. */
// #define BLOCKLEN 31
// #define BLOCKLEN 21
#define BLOCKLEN 20
// #define BLOCKLEN 18  
// #define BLOCKLEN 5     /* Use 6 for now. */
#define ML (BLOCKLEN-NPAR)

/*
 * Converts the plaintext message in msg into ecc-encoded blocks.
 * Each block's first byte is a length, followed by at most 255 bytes.
 */
static int init = 1;
static int block_len = BLOCKLEN;
static int max_mlen = BLOCKLEN-NPAR;

int ijel_set_ecc_blocklen(int new_len) {
  block_len = new_len;
  max_mlen = block_len - NPAR;
  return max_mlen;
}


int ijel_get_ecc_blocklen() {
  return block_len;
}


/*
 * Return value is a malloc'ed buffer that contains ecc-encoded data.
 * The *outlen pointer is updated to contain the length of the
 * ecc-encoded data buffer.
 *
 * Memory management: Caller manages the "message", but we should
 * manage the ecc buffer internally to avoid memory leaks.  The ECC
 * encode & decode functions allocate, but the model (so far) is that
 * these buffers simply replace the existing message pointers. 
 *
 * Seeing as the whole point of ECC is to be robust against errors, it
 * doesn't make too much sense to embed length when using ECC,
 * although this can be done.  This is also redundant, since ECC
 * blocks contain length information already.
 *
 * Note that rscode demands that the ECC blocks be no larger than 255
 * bytes!
 */



unsigned char *ijel_encode_ecc(unsigned char *msg, int msglen, int *outlen) {
  int n_out, in_len, nblocks, i, msgchunk;
  unsigned char message[256];
  unsigned char *out, *next_out;
  unsigned char *in;

  if (init) {
    /* make the race as short as possible */
    init = 0;
    initialize_ecc();
  }

  /* This is the max number of bytes of message that we will put in
   * each block, EXCLUSIVE of length and parity: */
  msgchunk = max_mlen - 1;

  /* This is the number of ECC blocks we will need to encode all of
   * the msg including parity bytes - always add an extra block to
   * terminate.  This block will have the value (1) msgchunk, or (2)
   * the number of remaining bytes (even if 0), and will always be of
   * length less than msgchunk.  We detect the end of the message by
   * encountering a block whose length is less than msgchunk.
   */
  nblocks = (msglen / msgchunk) + 1;

  // if ( (msglen % msgchunk) > 0 ) nblocks++;

  n_out = nblocks * block_len;     /* Number of bytes we need for ECC output */
  out = calloc(n_out+1, 1);       /* Allocate and add 1 extra byte for NUL */
  next_out = out;                 /* Initialize the output buffer position */

  in = msg;         /* Pointer to unfinished business */
  in_len = msglen;  /* Remaining message length */

  *outlen = 0;      /* Total output length. */

  for (i = 0; i < nblocks; i++) {
    assert(in_len >= 0);

    memset(message, 0, 256);
    //    if ( i < (nblocks-1) ) message[0] = msgchunk;

    if ( in_len >= msgchunk ) message[0] = msgchunk;
    else if ( in_len < msgchunk ) message[0] = in_len;
    else fprintf(stderr, "End of loop, but in_len = %d!\n", in_len);

    if (in_len > 0) memcpy(message+1, in, msgchunk);
    //    fprintf(stderr, "block %d: message[0] = %d\n", i, message[0]);

    /* Add 1 to msgchunk to account for the length byte at message[0]: */
    encode_data(message, msgchunk+1, next_out);

    /* ECC blocks are block_len bytes long.  Keep on truckin' */
    next_out += block_len;
    *outlen += block_len;

    /* Adjust pointer and remaining length: */
    in += msgchunk;
    in_len -= msgchunk;
    if (in_len < 0) in_len = 0;
  }    

  //  fprintf(stderr, "ijel_encode_ecc: Incoming length is %d => ECC length is %d\n", msglen, *outlen);

  return(out);
}


/*
 * Return value is a malloc'ed buffer that contains ecc-encoded data.
 * The *outlen pointer is updated to contain the length of the
 * ecc-encoded data buffer.  Caller must free when done.
 */

unsigned char *ijel_decode_ecc(unsigned char *ecc, int ecclen, int *msglen) {
  int mlen, nblocks, in_len, i, k; //n_out, 
  unsigned char *out, *next_out;
  unsigned char *in;
  int done = 0;

  if (init) {
    /* make the race as short as possible */
    init = 0;
    initialize_ecc();
  }

  /* 
   * The size of ECC-encoded data, ecclen, must be a multiple of block_len,
   * otherwise it is trash:
   */
  if ( (ecclen % block_len) != 0 ) {
    fprintf(stderr, "ijel_decode_ecc error:  ecclen is %d, not a multiple of block_len!\n", ecclen);
    return NULL;
  }

  nblocks = ecclen / block_len; /* The number of ECC blocks in the buffer. */

  //n_out = nblocks * max_mlen;   /* Maximum number of bytes we need for decoded output */

  out = malloc(nblocks * block_len);  /* Allocate buffer */
  next_out = out;         /* Initialize the output buffer position */

  in = ecc;         /* Pointer to unfinished business */
  in_len = ecclen;  /* Remaining message length */

  *msglen = 0;
  for (i = 0; i < nblocks && !done; i++) {
    assert(in_len >= 0);

    /* Decoding happens in-place, always of length max_mlen (message without
     * parity):  */
    decode_data(in, block_len);
    
    /* Error correction if needed: */
    if ( (k = check_syndrome()) != 0) {   /* check if syndrome is all zeros */
      correct_errors_erasures (in, block_len, 0, 0);
    }

    /* On input, we had set the first byte to be the length of text
     * within the block.  This will be 127 for all but the last block,
     * where it will be in [1,255]:
     */
    mlen = in[0];
    //    fprintf(stderr, "block %d: in[0] = %d\n", i, in[0]);

    if (mlen < max_mlen-1) done = 1;

    if (mlen > 0) {
      memcpy(next_out, in+1, mlen);

      next_out += mlen;
      *msglen += mlen;

      in += block_len;
      in_len -= block_len;
    }
  }

  //  fprintf(stderr, "ijel_decode_ecc: ECC length is %d => Outgoing length is %d\n", ecclen, *msglen);

  return(out);
}




/*
 * Return the capacity after ECC overhead has been factored in.
 *
 * We are given a byte count 'nbytes' that is the raw capacity of an
 * image.  Figure out how many block_len-byte ECC packets can fit in
 * that space, and then compute the number of bytes of message that
 * can be supported with ECC.  This is always less than 'nbytes'.
 */
int ijel_capacity_ecc(int nbytes) {
  int nblocks = (nbytes / block_len);  /* conservative estimate */
  
  return nblocks * (max_mlen-1);  /* We can fit this many plaintext bytes AFTER RS coding. */
}





/*
 * Given a message length 'msglen', compute the number of bytes of ECC
 * that will be used to encode that message:
 *
 * was ijel_ecc_length
 */
int ijel_message_ecc_length(int msglen) {
  int block_text_length = max_mlen-1;                   /* Subtract 1 for the length byte. */
  int nblocks = (msglen / block_text_length) + 1; /* Number of chunks of original message */

  /* Note that we add 1 above to account for a "remainder" block that
     has less than max_mlen bytes.  We always encode such a block, even if
     its length is 0, to terminate the ECC-encoded message. */

  /* nblocks is the number of ECC blocks required, so return the result in bytes: */
  return nblocks * block_len; 
}



/*
 * After reading 'nbytes' bytes to be decoded, ceiling that up to a
 * length that is a multiple of the ECC block length:
 */
int ijel_ecc_block_length(int nbytes) {
  int nblocks = nbytes / block_len;
  if (nbytes % block_len > 0) nblocks++;

  return nblocks * block_len;
}





/*
 * Sanity checker - encode and decode should be inverses.
 */
int ijel_ecc_sanity_check(unsigned char *msg, int msglen) {
  int i;
  int buffer_sz = msglen+1;
  unsigned char *buffer1 = calloc(buffer_sz, 1);
  unsigned char *buffer2 = calloc(buffer_sz, 1);
  int msgchunk = 80 + NPAR < buffer_sz ? 80 : buffer_sz - NPAR;
  int xor = 0;

  memcpy(buffer1, msg, msgchunk);
  encode_data(buffer1, msgchunk, buffer2);
  decode_data(buffer2, msgchunk+NPAR);
  
  for (i = 0; i < msgchunk; i++)
    if (buffer2[i] != buffer1[i]) xor++;
  
  //  fprintf(stderr, "Different bytes: %d\n", xor);
  
  free(buffer1);

  free(buffer2);


  return xor;
}


/*
 * Versions that ignore the length field of the blocks.  At some
 * point, we will want to improve this to eliminate the length byte
 * and save space, but for now we just want to see if things improve
 * when length is treated as a shared secret in the ECC case:
 */

unsigned char *ijel_encode_ecc_nolength(unsigned char *msg, int msglen, int *outlen) {
  /* Ok, secretly this is identical to ijel_encode_ecc.  We reserve
     the right to modify it though, e.g., to eliminate the length
     byte. */
  int n_out, in_len, nblocks, i, msgchunk;
  unsigned char message[256];
  unsigned char *out, *next_out;
  unsigned char *in;

  if (init) {
    /* make the race as short as possible */
    init = 0;
    initialize_ecc();
  }

  /* This is the max number of bytes of message that we will put in
   * each block, EXCLUSIVE of length and parity: */
  msgchunk = max_mlen;

  /* This is the number of ECC blocks we will need to encode all of
   * the msg including parity bytes - always add an extra block to
   * terminate.  This block will have the value (1) msgchunk, or (2)
   * the number of remaining bytes (even if 0), and will always be of
   * length less than msgchunk.  We detect the end of the message by
   * encountering a block whose length is less than msgchunk.
   */
  nblocks = (msglen / msgchunk) + 1;

  // if ( (msglen % msgchunk) > 0 ) nblocks++;

  n_out = nblocks * block_len;     /* Number of bytes we need for ECC output */
  out = calloc(n_out+1, 1);       /* Allocate and add 1 extra byte for NUL */
  next_out = out;                 /* Initialize the output buffer position */

  in = msg;         /* Pointer to unfinished business */
  in_len = msglen;  /* Remaining message length */

  *outlen = 0;      /* Total output length. */

  for (i = 0; i < nblocks && in_len > 0; i++) {
    assert(in_len >= 0);

    memset(message, 0, 256);

    if (in_len > 0) memcpy(message, in, msgchunk);

    /* Add 1 to msgchunk to account for the length byte at message[0]: */
    encode_data(message, msgchunk, next_out);

    /* ECC blocks are block_len bytes long.  Keep on truckin' */
    next_out += block_len;
    *outlen += block_len;

    /* Adjust pointer and remaining length: */
    in += msgchunk;
    in_len -= msgchunk;
    if (in_len < 0) in_len = 0;
  }    

  //  printf("ijel_encode_ecc_nolength: Incoming length is %d => ECC length is %d\n", msglen, *outlen);

  return(out);
}





/*
 * Return value is a malloc'ed buffer that contains ecc-encoded data.
 * The *outlen pointer is updated to contain the length of the
 * ecc-encoded data buffer.  Caller must free when done.
 */

unsigned char *ijel_decode_ecc_nolength(unsigned char *ecc, int ecclen, int length) {
  int nblocks, in_len, i, k; //n_out, 
  int msgchunk;
  int msglen;
  unsigned char *out, *next_out;
  unsigned char *in;
  int done = 0;

  if (init) {
    /* make the race as short as possible */
    init = 0;
    initialize_ecc();
  }

  /* 
   * The size of ECC-encoded data, ecclen, must be a multiple of block_len,
   * otherwise it is trash:
   */
  if ( (ecclen % block_len) != 0 ) {
    fprintf(stderr, "ijel_decode_ecc error:  ecclen is %d, not a multiple of block_len!\n", ecclen);
    return NULL;
  }

  nblocks = ecclen / block_len; /* The number of ECC blocks in the buffer. */

  /* This is the max number of bytes of message that we will put in
   * each block, EXCLUSIVE of parity (no length byte here): */
  msgchunk = max_mlen;

  //n_out = nblocks * max_mlen;   /* Maximum number of bytes we need for decoded output */

  out = malloc(nblocks * block_len);  /* Allocate buffer */
  next_out = out;                    /* Initialize the output buffer position */

  in = ecc;         /* Pointer to unfinished business */
  in_len = ecclen;  /* Remaining message length */

  msglen = 0;
  for (i = 0; i < nblocks && !done; i++) {
    assert(in_len >= 0);

    /* Decoding happens in-place, always of length max_mlen (message without
     * parity):  */
    decode_data(in, block_len);
    
    /* Error correction if needed: */
    if ( (k = check_syndrome()) != 0) {   /* check if syndrome is all zeros */
      correct_errors_erasures (in, block_len, 0, 0);
    }

    // if (mlen < max_mlen-1) done = 1;
    if (msglen >= length) done = 1;

    memcpy(next_out, in, msgchunk); 

    next_out += msgchunk;
    msglen += msgchunk;

    in += block_len;
    in_len -= block_len;
  }

  //  printf("ijel_decode_ecc_nolength: ECC length is %d => Outgoing length is %d\n", ecclen, msglen);

  return(out);
}

