#include "common.h"

unsigned char blue_noise_16x16[256]=  {
  43,222,57,208,83,189,29,76,136,39,221,150,71,85,227,98,67,
  166,252,138,175,232,113,154,198,92,169,25,213,54,16,238,203,10,
  109,97,37,62,217,9,240,51,121,102,195,162,140,117,177,79,153,
  23,237,125,167,19,82,180,233,3,254,38,187,28,130,219,52,199,
  184,72,146,103,134,205,66,147,127,88,61,242,44,93,249,114,1,
  226,41,251,188,31,46,80,210,225,106,155,190,14,137,161,84,207,
  58,91,220,158,111,176,17,165,7,74,119,214,64,181,30,128,173,
  11,120,20,229,247,53,139,201,234,24,148,40,228,104,246,151,200,
  70,142,86,192,124,96,34,174,81,239,95,191,75,18,50,99,241,
  183,59,4,216,68,245,110,56,204,6,123,163,223,135,209,42,32,
  168,108,152,45,185,159,133,170,143,255,60,178,2,115,157,218,129,
  253,196,22,224,12,212,47,26,107,36,89,194,78,236,94,13,73,
  87,141,118,101,77,235,197,69,215,149,230,55,27,145,202,49,231,
  179,63,248,156,90,182,116,244,131,15,171,122,186,105,164,35,206,
  0,33,144,126,21,5,160,48,100,211,250,65,8,243,132,112,172,
  193,
};

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

void dither_floyd_bidir(image *im, float step) {
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

void dither_cumulative(image *im, float step) {
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
        v= *p / step;
        ta= *(t-w-1); tb= *(t-w); tc= *(t);
        td= *(t+1)= v + tb - ta + tc;
        v= round(ta) - round(tb)
          - round(tc) + round(td);
        *p= v * step;
      }
      memcpy(buf, buf+w+1, len);
    }
  }
  free(buf);
}

void dither_blue_noise(image *im, float step) {
  int x,y,z;
  int w= im->width, h= im->height;
  gray *pi;
  unsigned char *pd;

  for (z= 0; z < 4; z++) {
    pi= im->chan[z];
    if (! pi) continue;
    for (y= 0; y < h; y++) {
      pd= blue_noise_16x16 + 16 * (y % 16);
      for (x= 0; x < w; x++, pi++) {
        *pi= round(
          *pi / step + *pd / 255.0 - 0.5
        ) * step;
        if (x % 16 == 15) pd-= 16;
        pd++;
      }
    }
  }
}

// vim: sw=2 ts=2 sts=2 expandtab:
