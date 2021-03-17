#include "common.h"

int main(int argc, char **args) {
  image *om, *im= read_image(stdin, 0);
  float x;
  //x= atof(args[1]);
  int w= im->width;
  int h= im->height;
  om= copy_image(im);
  image_laplace(om, -0.25);
  write_image(om, stdout, 0);
  exit(0);
}
// vim: sw=2 ts=2 sts=2 expandtab:
