#include <jel/jel.h>
#include "images.h"

static int verbose = 0;

static unsigned int construct_jpeg_body(unsigned char* data,  unsigned int data_length, unsigned char**bodyp){
  if(bodyp != NULL){
    image_p cover = embed_message(data, data_length);
    if(cover != NULL){
      unsigned int body_length = (unsigned int)cover->size;
      *bodyp = cover->bytes;
      /* steal ownership of the bytes */
      cover->bytes = NULL;
      free_image(cover);
      return body_length;
    } else {
      fprintf(stderr, "embed_message failed");
    }
  }
  return 0;
}


static unsigned int deconstruct_jpeg_body(unsigned char *body, unsigned int body_length, unsigned char** datap){
  size_t rval = 0;
  if(datap != NULL){
    unsigned char *message = NULL;
    int message_size = extract_message(&message, body, body_length);
    if(message != NULL){
      *datap = message;
      rval = (unsigned int) message_size;
      return rval;
    } else {
      fprintf(stderr, "extract_message failed");
    }
  }
  return 0;
}




int main(int argc, char** argv){
  if(argc > 1){
    verbose = 1;
  }
  int inM = load_images("data/images");
  unsigned int data_length = 16416, body_length, message_length;
  unsigned char* data = (unsigned char*)malloc(data_length), *body = NULL, *message = NULL;
  bool ok = file2bytes("data/onion.bin", data, data_length);
  int diff;
  
  if(!ok){  fprintf(stderr, "loading payload failed\n");  goto clean_up; }

  body_length = construct_jpeg_body(data, data_length, &body);

  fprintf(stderr, "construct_jpeg_body: cover image plus message is %u bytes\n", body_length); 

  message_length =  deconstruct_jpeg_body(body, body_length, &message);

  fprintf(stderr, "deconstruct_jpeg_body: extracted a message of length %u bytes\n", message_length);

  if(data_length != message_length){
    fprintf(stderr, "FAIL: %u in %u out\n", data_length, message_length);
  } else if ((diff = memcmp(data, message, data_length)) != 0){
    if(verbose){
      unsigned int j;
      for(j = 0; j < data_length; j++){
        if(message[j] != data[j]){
          fprintf(stderr, "FAIL: j = %d:  %u  %u\n", j, message[j], data[j]);
        }
      }
    }
    fprintf(stderr, "FAIL: memcmp: %d \n", diff);
  } else {
    fprintf(stderr, "SUCCESS\n");
  }

 clean_up:

  int outM = unload_images();

  free(data);
  free(body);
  free(message);
  
  if(verbose){
    fprintf(stderr, "images debugging: %d %d\n", inM, outM);
  }

  return 0;
}
