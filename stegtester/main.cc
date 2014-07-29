#include <jel/jel.h>
#include "images.h"


static int verbose = 0;

static unsigned int construct_jpeg_body(unsigned char* data,  unsigned int data_length, unsigned char**bodyp, int* message_lengthp){
  if((bodyp != NULL) && (message_lengthp != NULL)){
    image_p cover = embed_message(data, data_length);
    if(cover != NULL){
      unsigned int body_length = (unsigned int)cover->size;
      *bodyp = cover->bytes;
      *message_lengthp = cover->message_length;
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


static unsigned int deconstruct_jpeg_body(unsigned char *body, unsigned int body_length, unsigned char** datap, int message_length){
  size_t rval = 0;
  if(datap != NULL){
    unsigned char *message = NULL;
    int message_size = extract_message(&message, message_length, body, body_length);
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
  bool embed_length;
  
  if(argc > 1){
    embed_length = true;
    fprintf(stderr, "embed_length: %d\n", embed_length);
  } else {
    embed_length = false;
    fprintf(stderr, "embed_length: %d\n", embed_length);
  }

  int inM = load_images("data/jpegs");
    
  set_jel_embed_length(embed_length);

  unsigned int data_length = 16416, body_length, message_length;
  unsigned char* data = (unsigned char*)malloc(data_length), *body = NULL, *message = NULL;
  bool ok = file2bytes("data/onion.bin", data, data_length);
  int emessage_length = 0, diff;
  
  if(!ok){  fprintf(stderr, "loading payload failed\n");  goto clean_up; }

  body_length = construct_jpeg_body(data, data_length, &body, &emessage_length);

  fprintf(stderr, "construct_jpeg_body: cover image plus message is %u bytes, emessage_length = %d\n", body_length, emessage_length); 

  message_length =  deconstruct_jpeg_body(body, body_length, &message, embed_length ? 0 : emessage_length);

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
