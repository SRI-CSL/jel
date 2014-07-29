/* Copyright 2011, 2012 SRI International
 * See LICENSE for other credits and copying information
 */


#ifndef _IMAGES_H
#define _IMAGES_H

#include <stddef.h>
#include <stdint.h>

typedef struct image *image_p;

typedef struct image {
  size_t size;             /* size of the cover image                             */
  char* path;              /* from whence it came                                 */
  unsigned char* bytes;    /* the actual image                                    */
  int capacity;            /* how much jel reckons it can carry                   */
  int message_length;      /* the length of the embedded message, if there is one */
} image_t;

typedef struct jel_knobs {
  bool embed_length;        /* embed the message length as part of the message   default = true  */
  int32_t ecc_blocklen;     /* ecc block length                                  default = 20    */
  int32_t freq_pool;        /* size of the frquency pool                         default = 16    */
  int32_t quality_out;      /* the quality of the resulting jpeg                 default = 75?   */
  int32_t random_seed;      /* the seed to random choice of 4 freqs from the freq_pool           */
} jel_knobs_t;


void set_jel_defaults();

/* need to do this after loading the images (since that sets the defaults) */
void set_jel_embed_length(bool value);

void free_image(image_p im);

int load_images(const char* path);

int unload_images();

image_p embed_message(unsigned char* message, int message_length);

int extract_message(unsigned char** messagep, int message_length, unsigned char* jpeg_data, unsigned int jpeg_data_length);

int extract_message_orig(unsigned char** messagep, unsigned char* jpeg_data, unsigned int jpeg_data_length);

bool file2bytes(const char* path, unsigned char* bytes, size_t bytes_wanted);

#endif
