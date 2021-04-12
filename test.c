#include "common.h"

int main(int argc, char **args) {
  image *g= image_read(args[1]);
  image *t= copy_image(g);
  image_sel_fill(t, 0, 0.5, 0, 0);
  int x= g->width * 0.2;
  int y= g->height * 0.2;
  int w= g->width * 0.6;
  int h= g->height * 0.6;
  //image_sel_rect(g, 1, x, y, w, h);
  image_poisson(t, g, -0.25, 0, 0.001);
  image_write(g, args[2]);
  exit(0);
}
// vim: sw=2 ts=2 sts=2 expandtab:
