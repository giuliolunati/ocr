#include "common.h"

int main(int argc, char **args) {
  image *g= image_read(args[1]);
  float v= atof(args[2]);
  int x0= atoi(args[3]);
  int y0= atoi(args[4]);
  int x1= atoi(args[5]);
  int y1= atoi(args[6]);
  image_sel_rect(g, v, x0, y0, x1, y1);
  image_sel_fill(g, 0, 0.5, 0, 0);
  image_write(g, args[7]);
  exit(0);
}
// vim: sw=2 ts=2 sts=2 expandtab:
