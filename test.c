#include "common.h"

int main(int argc, char **args) {
  image *im= image_read(args[1]);
  int x= im->width * 0.2;
  int y= im->height * 0.2;
  int w= im->width * 0.6;
  int h= im->height * 0.6;
  image_make_sel(im, 0.5);
  image_sel_rect(im, 1, x, y, w, h);
  image_alpha_to_sel(im);
  image_write(im, args[2]);
  exit(0);
}
// vim: sw=2 ts=2 sts=2 expandtab:
