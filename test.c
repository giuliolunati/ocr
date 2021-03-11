#include "common.h"

int main(int argc, char **args) {
  image *om, *im= read_image(stdin, 0);
  float x;
  //x= atof(args[1]);
  int w= im->width;
  int h= im->height;
  om= copy_image(im);
  im->depth= 3;
  //im->channel[0]=im->channel[2];
  write_image(im, stdout, 0);
}
// vim: sw=2 ts=2 sts=2 expandtab:
