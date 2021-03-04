#include "common.h"

int main(int argc, char **args) {
  image *om, *im= read_image(stdin, 0);
  float x;
  x= atof(args[1]);
  int w= im->width;
  int h= im->height;
  om= copy_image(im);
  image_laplace(om, -0.25);
  image_dither(om,1,0);
  om= image_poisson(om, -0.25, 0, x);
  diff_image(om, im);
  //image_laplace(om, -0.25);
  //contrast_image(om, -0.0001, 0.0001);
  write_image(om, stdout, 0);
}
// vim: sw=2 ts=2 sts=2 expandtab:
