#include "common.h"

int main(int argc, char **args) {
  image *om, *im= make_image(600, 501, 1);//read_image(stdin, 0);
  float x;
  //x= atof(args[1]);
  int w= im->width;
  int h= im->height;
  image_double(im,1,0);
  exit(0);
  convolve_3x3(im, 1, -0.25, -0.25, 0);
  image_dither(im, 64, 1);
  om= deconvolve_3x3(im, 1, -0.25, -0.25, 0, 0, x);
  write_image(om, stdout, 0);
}
// vim: sw=2 ts=2 sts=2 expandtab:
