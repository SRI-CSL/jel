#include <jel/jel.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct {
  int xdim;
  int ydim;
  int bpp;
  unsigned char **pixels;
} myimage;



struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;


/*
 * I don't think we really need this.
 */
METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}


myimage * read_jpeg_file (char *filename) {
  myimage * im;
  FILE *infile;
  struct jpeg_decompress_struct cinfo;
  struct my_error_mgr jerr;
  int row_stride;
  //int samplesperpixel;
  //unsigned char *image;
  JSAMPARRAY buffer;
  int i, k;

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We set up the normal JPEG error routines, then override error_exit. */
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_decompress(&cinfo);
    // fclose(infile); // Not established yet...
    return 0;
  }

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */
  infile = fopen(filename, "rb");
  if (!infile) {
    printf("Could not open %s!\n", filename);
    exit(-1);
  }

  jpeg_stdio_src(&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */ 

  /* JSAMPLEs per row in output buffer */
  row_stride = cinfo.output_width * cinfo.output_components;

  im = malloc(sizeof(myimage));
  im->xdim = cinfo.output_width;
  im->ydim = cinfo.output_height;
  im->bpp = cinfo.output_components;

  im->pixels = (unsigned char **) malloc( sizeof(unsigned char *) * im->ydim );
  for (i = 0; i < im->ydim; i++) im->pixels[i] = malloc(row_stride);

  buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);


  i = 0;
  k = im->ydim;

  //  while (cinfo.output_scanline < cinfo.output_height) {

  while (i < k) {
    /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
    (void) jpeg_read_scanlines(&cinfo, buffer, 1);
    /* Assume put_scanline_someplace wants a pointer and sample count. */

    memcpy(im->pixels[i], buffer[0], row_stride);
    i++;
  }

  (void) jpeg_finish_decompress(&cinfo);

  return im;
}

void free_myimage(myimage* im){
  if(im != NULL){
    if(im->pixels != NULL){
      int i; 
      for(i = 0; i < im->ydim; i++) free(im->pixels[i]);
      free(im->pixels);
    }
    free(im);
  }
}


int save_pnm_file(myimage *im, FILE *f) {
  int i, j, k;

  if (im->bpp == 3) printf("P3\n%d %d\n255\n", im->xdim, im->ydim);
  else printf("P2\n%d %d\n255\n", im->xdim, im->ydim);

  k = im->xdim * im->ydim * im->bpp;

  for (j = 0; j < im->ydim; j++) {
    for (i = 0; i < im->bpp * im->xdim; i++)
      printf("%d ", im->pixels[j][i]);

    printf("\n");
  }

  return k;
}



double image_blockiness(myimage *im) {
  int i, j, k, bpp;
  unsigned char *row;
  double block = 0.0;

  /* In case we are in color: */
  bpp = im->bpp;

  for (j = 0; j < im->ydim; j++) {
    row = im->pixels[j];
    for (i = 7; i < im->xdim - 1; i += 8)
      for (k = 0; k < bpp; k++)
	block += abs( row[ i*bpp + k] - row[ (i+1)*bpp + k ] );
  }

  for (i = 0; i < im->xdim; i++) {
    for ( j = 7; j < im->ydim - 1; j += 8 )
      for ( k = 0; k < bpp; k++)
	block += abs( im->pixels[j][ i*bpp + k ] - im->pixels[j+1][ i*bpp + k ] );
  }
  return block;

}


int main(int argc, char **argv) {
  //int i, k;
  myimage *image = NULL;

  if ( argc != 2 ) {
    printf("Usage: %s <filename>\n", argv[0]);
    exit(-1);
  }

  image = read_jpeg_file(argv[1]);

  
  // 
  // printf("%d %d %d\n", xdim, ydim, bpp);
  // save_pnm_file(image, stdout);

  printf("%f\n", image_blockiness(image));

  free_myimage(image);

  return 0;
}
