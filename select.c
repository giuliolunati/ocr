#include "common.h"

void image_alpha_to_sel(image *im) {
  assert(im);
  long int len= im->width * im->height;
  if (! im->ALPHA) {
    im->ALPHA= malloc(sizeof(gray) * len);
    if (! im->ALPHA) error("image_alpha_to_sel: out of memory");
  }
  gray *a, *s, *z;
  if (im->SEL) {
    for (a= im->ALPHA, s= im->SEL, z= a + len;
      a < z;
      a++, s++
    ) *a= *s;
  } else { 
    for (a= im->ALPHA, z= a + len;
      a < z;
      a++
    ) *a= 1.0;
  }
}

void image_make_sel(image *im, gray v) {
  assert(im);
  int wid= im->width;
  int hei= im->height;
  assert(wid > 0);
  assert(hei > 0);
  gray *p, *end;
  if (! im->SEL) {
    im->SEL= malloc(sizeof(im->SEL) * im->width * im->height);
    if (! im->SEL) error("image_make_sel: out of memory");
  }
  for (p= im->SEL, end= p + wid*hei; p < end; p++) *p= v;
}

void image_sel_rect(image *im, real v,
    // v < 0: outside= -v
    // v = -0.0: outside= 0 (intersect)
    // v = 0.0: inside= 0 (subtract)
    int x0, int y0, int x1, int y1 // corners
    // negative => from bottom/right
  ) {
  assert(im);
  if (! im->SEL) image_make_sel(im,0);
  int wid= im->width;
  int hei= im->height;
  if (x0 < 0) x0 += wid;
  if (x1 <= 0) x1 += wid;
  if (y0 < 0) y0 += hei;
  if (y1 <= 0) y1 += hei;
  if (
    x0 < 0 || x0 > wid ||
    x1 < 0 || x1 > wid ||
    y0 < 0 || y0 > hei ||
    y1 < 0 || y1 > hei ||
    x1 < x0 || y1 < y0
  ) error("image_sel_rect: invalid corners");
  gray *p, *end;
  int x, y;

  if (signbit(v)) {
    for (
      p= im->SEL, end= p + wid*y0;
      p < end; p++
    ) *p= -v;
  }
  for (y= y0; y < y1; y++) {
    if (signbit(v)) {
      for (
        p= im->SEL + wid*y, end= p + x0;
        p < end; p++
      ) *p = -v;
    }
    if (! signbit(v)) {
      for (
        p= im->SEL + wid*y + x0, end= p+x1-x0;
        p < end; p++
      ) *p = v;
    }
    if (signbit(v)) {
      for (
        p= im->SEL + wid*y + x1,
        end= im->SEL + wid*(y+1);
        p < end; p++
      ) *p = -v;
    }
  }
  if (signbit(v)) {
    for (
      p= im->SEL + wid*y1,
      end= im->SEL + wid*hei;
      p < end; p++
    ) *p= -v;
  }
}

void image_sel_fill(image *im, real v0, real v1, real v2, real v3) {
  real v[4];
  v[0]= v0; v[1]= v1; v[2]= v2; v[3]= v3;
  int z;
  gray t, *p, *s, *end;

  for (z= 0; z < 4; z++) {
    if (! im->chan[z]) continue;
    if (isnan(v[z])) continue;
    t= v[z];
    if (!im->SEL) {
      for (
        p= im->chan[z], end= p + (im->width * im->height);
        p < end;
        p ++
      ) *p= t;
    }
    else {
      for (
        s= im->SEL, p= im->chan[z], end= p + (im->width * im->height);
        p < end;
        p++, s++
      ) {
        if (*s == 0) continue;
        if (*s == 1) *p= t;
        else *p += *s * (t - *p);
      }
    }
  }
}

// vim: sw=2 ts=2 sts=2 expandtab:
