#include "common.h"

void draw_grid(image *im, int stepx, int stepy) {
  int depth= im->depth;
  if (depth != 1) error("draw_grid: invalid depth");
  int sbigx= stepx * 10;
  int step4x= stepx / 4;
  int step2x= stepx / 2;
  int sbigy= stepy * 10;
  int step4y= stepy / 4;
  int step2y= stepy / 2;
  int w= im->width;
  int h= im->height;
  int x, y;
  gray *p;

  for (y= 0; y < h; y++) {
    p= im->channel[0] + y * w;
    for(x= 0; x < w; x++, p++) {
      if (
        y % sbigy == 0 ||
        x % sbigx == 0 ||
        (x % stepx == 0 && (y + step4y) % stepy < step2y) ||
        (y % stepy == 0 && (x + step4x) % stepx < step2x)
      ) *p= 0;

    }
  }
}

void fill_image(image *im, real v) {
  gray s= MAXVAL * v;
  int i;
  gray *p, *end;
  for (i= 0; i < im->depth; i++) {
    for (
      p= im->channel[i], end= p + (im->width * im->height);
      p < end;
      p ++
    ) *p= s;
  }
}

void poke(image *im, int x, int y, int chan, gray v) {
  if (x < 0 || x > im->width) error("poke: invalid x");
  if (y < 0 || y > im->height) error("poke: invalid y");
  if (chan < 0 || chan > im->depth) error("poke: invalid channel");
  *(im->channel[chan] + im->width * y + x)= MAXVAL * v;
}

// vim: sw=2 ts=2 sts=2 expandtab:
