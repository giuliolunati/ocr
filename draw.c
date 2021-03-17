#include "common.h"

void draw_grid(image *im, int stepx, int stepy) {
  int sbigx= stepx * 10;
  int step4x= stepx / 4;
  int step2x= stepx / 2;
  int sbigy= stepy * 10;
  int step4y= stepy / 4;
  int step2y= stepy / 2;
  int w= im->width;
  int h= im->height;
  int x, y, z;
  gray *p;
  for (z= 0; z < 4; z++) {
    if (! im->chan[z]) continue;
    for (y= 0; y < h; y++) {
      p= im->chan[0] + y * w;
      for(x= 0; x < w; x++, p++) {
        if (
          y % sbigy == 0 ||
          x % sbigx == 0 ||
          (x % stepx == 0 && (y + step4y) % stepy < step2y) ||
          (y % stepy == 0 && (x + step4x) % stepx < step2x)
        ) {if (*p < 0) *p= 1; else *p= -1;}
      }
    }
  }
}

void fill_image(image *im, real v) {
  gray s= MAXVAL * v;
  int z;
  gray *p, *end;

  for (z= 0; z < 4; z++) {
    if (! im->chan[z]) continue;
    for (
      p= im->chan[z], end= p + (im->width * im->height);
      p < end;
      p ++
    ) *p= s;
  }
}

void poke(image *im, int x, int y, int z, gray v) {
  if (x < 0 || x > im->width) error("poke: invalid x");
  if (y < 0 || y > im->height) error("poke: invalid y");
  if (! im->chan[z]) error("poke: invalid chan");
  *(im->chan[z] + im->width * y + x)= MAXVAL * v;
}

// vim: sw=2 ts=2 sts=2 expandtab:
