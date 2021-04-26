#include "common.h"

int main(int argc, char **args) {
  int d= atoi(args[1]);
  int w= atoi(args[2]);
  int h= atoi(args[3]);
  image *i= image_make(d,w,h);
  i= clone_image(i, d+1, 2*w, h/2);
  image_write(i, args[4]);
  exit(0);
}
// vim: sw=2 ts=2 sts=2 expandtab:
