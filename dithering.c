#include "common.h"

void quantize_image(image *im, float steps) {
  int h= im->height;
  int w= im->width;
  gray *p, *end= p;
  float v;
  int i, z;
  for (z= 0; z < 4; z++) {
    p= im->chan[z];
    if (! p) continue;
    end= p + w*h;
    for (; p < end; p++) {
      v= *p;
      *p = roundf((v-128) * steps) / steps + 128;
    }
  }
}

void dither_floyd_bidir(image *im, int step) {
  int h= im->height;
  int w= im->width;
  int z;
  int i= 0;
  float v;
  int x, y;
  gray *p;
  for (z=0; z < 4; z++) {
    if (! im->chan[z]) continue;
    for (y= 0; y < h; y += 2) {
      p= im->chan[z] + y*w;
      for (x= 0; x < w; x++,p++) {
        v= *p;
        *p= round(*p / step) * step;
        v= (v-*p)/16;
        if (x+1 < w) {
          *(p+1) += 7*v;
          if (y+1 < h) *(p+w+1) += v;
        }
        if (y+1 < h) {
          if (x > 0) *(p+w-1) += 3*v;
          *(p+w) += 5*v;
        }
      }
      if (y >= h) break;
      p= im->chan[z] + (y+2)*w - 1;
      for (x= w-1; x >= 0; x--,p--) {
        v= *p;
        *p= round(*p / step) * step;
        v= (v-*p)/16;
        if (x > 0) {
          *(p-1) += 7*v;
          if (y+1 < h) *(p+w-1) += v;
        }
        if (y+1 < h) {
          if (x > 0) *(p+w-1) += 3*v;
          *(p+w) += 5*v;
        }
      }
    }
  }
}

void dither_cumulative(image *im, int steps) {
  int h= im->height;
  int w= im->width;
  int z;
  double v;
  int x, y;
  gray *p;
  double *t, ta, tb, tc, td;
  int len= sizeof(double) * (w+1);
  double *buf= malloc(2 * len);
  for (z=0; z < 4; z++) {
    if (! im->chan[z]) continue;
    for (x= 0; x <= w; x++) buf[x]= 0.0;
    for (y= 0; y < h; y++) {
      p= im->chan[z] + y*w;
      t= buf + w+1; *t= 0.0;
      for (x= 0; x < w; x++,p++,t++) {
        v= *p + 0.00196078431372549019 * steps;
        ta= *(t-w); tb= *(t-w+1); tc= *(t);
        td= *(t+1)= v + tb - ta + tc;
        v= roundf(ta) - roundf(tb)
          - roundf(tc) + roundf(td);
        *p= v / steps - 0.00196078431372549019;
      }
      memcpy(buf, buf+w+1, len);
    }
  }
}

// vim: sw=2 ts=2 sts=2 expandtab:
