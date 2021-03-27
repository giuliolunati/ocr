#include "common.h"

void image_alpha_to_sel(image *im) {
  assert(im);
  long int len= im->width * im->height;
  if (! im->ALPHA) {
    assert(im->depth % 2);
    im->ALPHA= malloc(sizeof(gray) * len);
    if (! im->ALPHA) error("image_alpha_to_sel: out of memory");
    im->depth ++;
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

void image_sel_make(image *im, gray v) {
  assert(im);
  int wid= im->width;
  int hei= im->height;
  assert(wid > 0);
  assert(hei > 0);
  gray *p, *end;
  if (! im->SEL) {
    im->SEL= malloc(sizeof(im->SEL) * im->width * im->height);
    if (! im->SEL) error("image_sel_make: out of memory");
  }
  for (p= im->SEL, end= p + wid*hei; p < end; p++) *p= v;
}

void image_sel_rect(image *im, real v,
    // v < 0: outside= -v
    // v = -0.0: outside= 0 (intersect)
    // v = 0.0: inside= 0 (subtract)
    int x0, int y0, int w, int h // origin, size
  ) {
  assert(im);
  if (! im->SEL) image_sel_make(im,0);
  int wid= im->width;
  int hei= im->height;
  if (w < 0) {w= -w; x0 -= w;}
  if (h < 0) {h= -h; y0 -= h;}
  x0= MAX(x0, 0); x0= MIN(x0, wid);
  y0= MAX(y0, 0); y0= MIN(y0, hei);
  w= MIN(w, wid-x0); h= MIN(h, hei-y0);
  gray *p, *end;
  int x, y;

  if (signbit(v)) {
    for (
      p= im->SEL, end= p + wid*y0;
      p < end; p++
    ) *p= -v;
  }
  for (y= y0; y < y0 + h; y++) {
    if (signbit(v)) {
      for (
        p= im->SEL + wid*y, end= p + x0;
        p < end; p++
      ) *p = -v;
    }
    if (! signbit(v)) {
      for (
        p= im->SEL + wid*y + x0, end= p+w;
        p < end; p++
      ) *p = v;
    }
    if (signbit(v)) {
      for (
        p= im->SEL + wid*y + x0 + w,
        end= im->SEL + wid*(y+1);
        p < end; p++
      ) *p = -v;
    }
  }
  if (signbit(v)) {
    for (
      p= im->SEL + wid*(y0+h),
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
