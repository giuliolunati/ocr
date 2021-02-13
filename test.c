#include "common.h"

int main(int argc, char **args) {
  image *om, *im= read_image(stdin, 0);
  int w= im->width;
  int h= im->height;
  gray *p= im->channel[0];
  real a=2, b=-0.5; 
  convolution_3x3(im, 4, -1, -1, 0, 0);
  om= deconvolution_3x3(im, 4, -1, -1, 0, 16);
  write_image(om, stdout, 0);
}
// vim: sw=2 ts=2 sts=2 expandtab:
