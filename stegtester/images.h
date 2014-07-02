/* Copyright 2011, 2012 SRI International
 * See LICENSE for other credits and copying information
 */


#ifndef _IMAGES_H
#define _IMAGES_H

#include <stddef.h>

typedef struct image *image_p;

typedef struct image {
  size_t size;    /* size of the cover image           */
  char* path;     /* from whence it came               */
  unsigned char* bytes;    /* the actual image                  */
  int capacity;   /* how much jel reckons it can carry */
} image_t;



void free_image(image_p im);

int load_images(const char* path);

int unload_images();

image_p embed_message(unsigned char* message, int message_length);

int extract_message(unsigned char** messagep, unsigned char* jpeg_data, unsigned int jpeg_data_length);

bool file2bytes(const char* path, unsigned char* bytes, size_t bytes_wanted);

#endif
