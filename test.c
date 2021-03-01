#include "common.h"

int main(int argc, char **args) {
  image *om, *im= read_image(stdin, 0);
  float x;
  //x= atof(args[1]);
  int w= im->width;
  int h= im->height;
  om= image_half(im);
  om= image_redouble(om, w, h);
  diff_image(om, im);
  write_image(om, stdout, 0);
}
// vim: sw=2 ts=2 sts=2 expandtab:
