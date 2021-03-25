#include "common.h"

int main(int argc, char **args) {
  image *om, *im= image_read(args[1]);
  image_write(im, NULL);
  exit(0);
  float x;
  //x= atof(args[1]);
  int w= im->width;
  int h= im->height;
  om= copy_image(im);
  image_laplace(om, -0.25);
}
// vim: sw=2 ts=2 sts=2 expandtab:
