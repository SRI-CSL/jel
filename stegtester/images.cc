/* Copyright 2011, 2012, 2013 SRI International
 * See LICENSE for other credits and copying information
 */

#include "images.h"

#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

#include <jel/jel.h>

static jel_knobs_t knobs;
static bool jel_ok = false;

/* needs an API once the dust has settled. */
void set_jel_defaults(){
  knobs.embed_length  = true;
  knobs.ecc_blocklen  = 20;     //see  ijel_get_ecc_blocklen() and ijel_set_ecc_blocklen()
  knobs.freq_pool     = 16;
  knobs.quality_out   = 75;
  knobs.random_seed   = 0;
  jel_ok = true;
}

void set_jel_preferences(jel_knobs_t &knobs_in){
  knobs.embed_length  = knobs_in.embed_length;
  knobs.ecc_blocklen  = knobs_in.ecc_blocklen;    
  knobs.freq_pool     = knobs_in.freq_pool;
  knobs.quality_out   = knobs_in.quality_out;
  knobs.random_seed   = knobs_in.random_seed;
  jel_ok = true;
}

void set_jel_embed_length(bool value){
  knobs.embed_length  = value;
}

#define IMAGES_LOG   "/tmp/stegotester.log"

static image_p *the_images = NULL;
static int      the_images_length = 0;
static int      the_images_offset = 0;

static int      the_images_max_payload = 0;

static image_p alloc_image();
static image_p alloc_image(){
  return (image_p)calloc(1, sizeof(image_t));
}

void free_image(image_p im){
  if(im != NULL){
    free(im->bytes);
    free(im->path);
    free(im);
  }
}

static bool grow_the_images();
static bool grow_the_images(){
  int temp_length = 2 * (the_images_length == 0 ? 1 : the_images_length);
  image_p *temp_images = (image_p *)calloc(temp_length, sizeof(image_p));
  if(temp_images != NULL){
    int index;
    for(index = 0; index < the_images_offset; index++){
      temp_images[index] = the_images[index];
    }
    free(the_images);
    the_images = temp_images;
    the_images_length = temp_length;
    return true;
  } 
  return false;
}

bool file2bytes(const char* path, unsigned char* bytes, size_t bytes_wanted){
  FILE *f;
  size_t bytes_read;
  f = fopen(path, "rb");
  if (f == NULL) { 
    fprintf(stderr, "load_image fopen(%s) failed; %s\n", path, strerror(errno));
    return false;
  }
  /* not very signal proof */
  bytes_read = fread(bytes, sizeof(unsigned char), bytes_wanted, f);
  fclose(f);
  if(bytes_read < bytes_wanted){
    fprintf(stderr, "load_image fread(%s) only read %zd of %zd bytes\n", path, bytes_read, bytes_wanted);
    return false;
  } else {
    return true;
  }
}

/* a playground at present */
static int capacity(image_p image){
  jel_config *jel = jel_init(JEL_NLEVELS);
  int ret = jel_open_log(jel, (char *)IMAGES_LOG);
  if (ret == JEL_ERR_CANTOPENLOG) {
    fprintf(stderr, "Can't open %s!\n", IMAGES_LOG);
    jel->logger = stderr;
  }
  
  ret = jel_set_mem_source(jel, image->bytes, image->size);
  if (ret != 0) {
    fprintf(stderr, "jel: Error - exiting (need a diagnostic!)\n");
  } else {
    ret = jel_capacity(jel);
  }
  jel_log(jel, "In capacity:\n");
  jel_describe(jel);
  jel_close_log(jel);
  jel_free(jel);
  return ret;
}


static image_p load_image(const char* path, char* basename);
static image_p load_image(const char* path, char* basename){
  char name[2048];
  /* none of this is very platform independant; sorry */
  if(snprintf(name, 2048, "%s/%s", path, basename) >= 2048){
    fprintf(stderr, "load_image path too long: %s/%s\n", path, basename);
    return NULL;
  } else {
    struct stat statbuff;
    if(stat(name, &statbuff) == -1){
      fprintf(stderr, "load_image could not stat %s\n", name);
      return NULL;
    }
    if(S_ISREG(statbuff.st_mode)){
      image_p image = alloc_image();
      if(image != NULL){
        image->path = strdup(name);
        image->size = statbuff.st_size;
        image->bytes = (unsigned char *)malloc(image->size);
        image->message_length = 0;
        if(image->bytes != NULL){
          int success = file2bytes(image->path, image->bytes, image->size);
          if(success){
            /* do the jel analysis here */
            image->capacity = capacity(image);
            if(image->capacity >= the_images_max_payload){
              the_images_max_payload = image->capacity;
            }
            if(1){
              fprintf(stderr, "load_image loaded %s of size %zd with capacity %d\n", image->path, image->size, image->capacity);
            }
            return image;
          } else {
            free_image(image);
          }
        }
      }
    }
    return NULL;
  }
}

int load_images(const char* path){
  int retval = 0;
  struct dirent *direntp;
  DIR *dirp;

  if(!jel_ok){ set_jel_defaults(); }
  
  if((path == NULL) || ((dirp = opendir(path)) == NULL)){
    fprintf(stderr, "load_images could not open %s\n", path);
    return retval;
  }

  while((direntp = readdir(dirp)) != NULL){
    if((strcmp(direntp->d_name, ".") == 0) || (strcmp(direntp->d_name, "..") == 0)){
      continue;
    }
    image_p image = load_image(path, direntp->d_name);
    if(image != NULL){
      if(((the_images_length == 0) || (the_images_offset + 1 == the_images_length)) && !grow_the_images()){
        fprintf(stderr, "load_images could not grow storage");
        return retval;
      }
      the_images[the_images_offset++] = image;
      retval++;
    }
  }

  while((closedir(dirp) == -1) && (errno  == EINTR)){ };
  
  return retval;
}

int unload_images(){
  int retval = 0, index;
  for(index = 0; index < the_images_offset; index++){
    free_image(the_images[index]);
    retval++;
  }
  free(the_images);
  the_images = NULL;
  the_images_length = 0;
  the_images_offset = 0;
  return retval;
}


static image_p get_cover_image(int size){
  if((size > the_images_max_payload) ||  (the_images_offset > 0)){
    image_p retval = NULL;
    int index, fails = 0;
    /* try and get a random one first */
    while((retval != NULL) && (fails++ < 10)){
      index = rand() % the_images_offset;
      retval = the_images[index];
      if(retval->capacity > size){
        return retval;
      } else {
        retval = NULL;
      }
    }
    /* haven't got one yet; after 10 tries! better just return the first that works */
    for(index = 0; index < the_images_offset; index++){
      retval = the_images[index];
      if(retval->capacity > size){
        return retval;
      }
    }
  }  
  fprintf(stderr, "get_cover_image failed:  image count = %d; max payload = %d\n",   the_images_offset, the_images_max_payload);
  return NULL;
}

/* since we have to guess the size of the destination */
static image_p embed_message_aux(image_p cover, unsigned char* message, int message_length, unsigned char* destination, int destination_length);
static image_p embed_message_aux(image_p cover, unsigned char* message, int message_length, unsigned char* destination, int destination_length){
  image_p retval = NULL;
  if(destination != NULL){
    jel_config *jel = jel_init(JEL_NLEVELS);
    int bytes_embedded = 0;
    int ret = jel_open_log(jel, (char *)IMAGES_LOG);
    if (ret == JEL_ERR_CANTOPENLOG) {
      fprintf(stderr, "extract_message: can't open %s!\n", IMAGES_LOG);
      jel->logger = stderr;
    }

    ret = jel_set_mem_source(jel, cover->bytes, cover->size);
    if (ret != 0) {
      fprintf(stderr, "jel: error - setting source memory!");
      return NULL;
    } 
    ret = jel_set_mem_dest(jel, destination, destination_length);
    if (ret != 0) {
      fprintf(stderr, "jel: error - setting dest memory!");
      return NULL;
    } 

    jel_setprop(jel, JEL_PROP_EMBED_LENGTH, knobs.embed_length);
    fprintf(stderr, "embed_length: %d\n", knobs.embed_length);
    
    jel_log(jel, "In embed_message_aux:\n");
    jel_describe(jel);

   /* insert the message */
   jel_log(jel, "Before call to jel_embed, message[0] = %d\n", message[0]);
   bytes_embedded = jel_embed(jel, message, message_length);
   jel_log(jel, "After call to jel_embed, message[0] = %d\n", message[0]);

   fprintf(stderr, "jel_embed: bytes_embedded = %d message_length = %d\n", bytes_embedded, message_length);
 
   /* figure out the real size of destination */
   if(bytes_embedded == message_length){ 
     if(jel->jpeglen > 0){
       retval = alloc_image();
       retval->bytes = destination;
       retval->size = jel->jpeglen;
       retval->message_length = message_length;
     }
   } else {
     int  errcode = jel_error_code(jel);    /* Returns the most recent error code. */
     char *errstr = jel_error_string(jel);  /* Returns the most recent error string. */
     fprintf(stderr, "jel: bytes_embedded = %d message_length = %d (%d %s)\n", bytes_embedded, message_length, errcode, errstr);
   }
   jel_close_log(jel);
   jel_free(jel);
  }  
  return retval;
}


  

image_p embed_message(unsigned char* message, int message_length){
  image_p retval = NULL;
  if(message != NULL){
    image_p cover = get_cover_image( message_length );
    if(cover != NULL){
      fprintf(stderr, "embed_message:  embedding %d bytes into %s\n",  message_length, cover->path);
      int failures = 0, destination_length = cover->size;
      unsigned char* destination = NULL;
      
      do {
        destination_length *= 2;
        free(destination);
        destination = (unsigned char*)malloc(destination_length);
        if(destination == NULL){ break; }
        retval = embed_message_aux(cover, message, message_length, destination, destination_length);
        fprintf(stderr, "embed_message_aux:  %d  %p\n",  destination_length, retval);
      } while((failures++ < 10) && (retval == NULL));

      if(retval != NULL){
        fprintf(stderr, "embed_message:  resulting stegged image size = %zd\n",  retval->size);
      }
    }
  }
  return retval;
}

/* if the message_length is not zero, then the message has been embedded without its length, use this one */
int extract_message(unsigned char** messagep, int message_length, unsigned char* jpeg_data, unsigned int jpeg_data_length){
  bool embed_length = (message_length == 0);
  int msglen;
  
  if((messagep != NULL) && (jpeg_data != NULL)){
    fprintf(stderr, "extract_message:  %u\n", jpeg_data_length);
    jel_config *jel = jel_init(JEL_NLEVELS);
    
    int ret = jel_open_log(jel, (char *)IMAGES_LOG);
    if (ret == JEL_ERR_CANTOPENLOG) {
      fprintf(stderr, "extract_message: can't open %s!\n", IMAGES_LOG);
      jel->logger = stderr;
    }

    jel_set_mem_source(jel, jpeg_data, jpeg_data_length);

    fprintf(stderr, "embed_length: %d\n", embed_length);
    fprintf(stderr, "message_length: %d\n", message_length);

    if(embed_length){
      msglen = jel_capacity(jel);
      fprintf(stderr, "extract_message: capacity = %d\n", msglen);
    } else {
      msglen = message_length;
    }
  
    /* 
     * Here, we use 'jel_alloc_buffer( jel )' to let the jel object
     * figure out how big the buffer (message) should be and allocate
     * it.  Caller must explicitly free() this buffer.
     */

    // unsigned char* message = (unsigned char*)calloc(msglen+1, sizeof(unsigned char));
    unsigned char* message = (unsigned char*) jel_alloc_buffer( jel );
    jel_setprop(jel, JEL_PROP_EMBED_LENGTH, embed_length);

    jel_log(jel, "In extract_message:\n");
    jel_describe(jel);

    //    msglen = jel_extract(jel, message, msglen);
    msglen = jel_extract(jel, message, msglen);
    jel_log(jel, "After call to jel_extract, message[0] = %d; jel->data[0] = %d\n", message[0], jel->data[0]);
    jel_log(jel, "extract_message: %d bytes extracted\n", msglen);
    jel_close_log(jel);
    jel_free(jel);
    *messagep = message;
    return msglen;
  }
  return 0;
}


int extract_message_orig(unsigned char** messagep, unsigned char* jpeg_data, unsigned int jpeg_data_length){
  if((messagep != NULL) && (jpeg_data != NULL)){
    fprintf(stderr, "extract_message:  %u\n", jpeg_data_length);
    jel_config *jel = jel_init(JEL_NLEVELS);
    int ret = jel_open_log(jel, (char *)IMAGES_LOG);
    if (ret == JEL_ERR_CANTOPENLOG) {
      fprintf(stderr, "extract_message: can't open %s!\n", IMAGES_LOG);
      jel->logger = stderr;
    }

    jel_set_mem_source(jel, jpeg_data, jpeg_data_length);
    int msglen = jel_capacity(jel);
    fprintf(stderr, "extract_message: capacity = %d\n", msglen);
    unsigned char* message = (unsigned char*)calloc(msglen+1, sizeof(unsigned char));
    jel_setprop(jel, JEL_PROP_EMBED_LENGTH, knobs.embed_length);

    jel_log(jel, "In extract_message:\n");
    jel_describe(jel);

    msglen = jel_extract(jel, message, msglen);
    jel_log(jel, "After call to jel_extract, message[0] = %d; jel->data[0] = %d\n", message[0], jel->data[0]);
    jel_log(jel, "extract_message: %d bytes extracted\n", msglen);
    jel_close_log(jel);
    jel_free(jel);
    *messagep = message;
    return msglen;
  }
  return 0;
}
