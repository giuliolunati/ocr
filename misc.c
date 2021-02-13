#include "common.h"

//// ERRORS ////

void error(const char *msg) {
  fprintf(stderr, "ERROR: ");
  fprintf(stderr, "%s", msg);
  fprintf(stderr, "\n");
  exit(1);
}

void error1(const char *msg, const char *param) {
  fprintf(stderr, "ERROR: ");
  fprintf(stderr, "%s %s", msg, param);
  fprintf(stderr, "\n");
  exit(1);
}

image *image_background(image *im) {
  // find light background
  real d= im->ex;
  if (d <= 0) d= default_ex;
  d= 0.333 / d;
  d= exp(-d);
  int x, y, z, h= im->height, w= im->width;
  image *om= make_image(w, h, im->depth);
  om->ex= im->ex;
  real t, *v0, *v1;
  v0= malloc(w * sizeof(*v0));
  v1= malloc(w * sizeof(*v1));
  gray *pi;
  gray *po;
  for (z= 0; z < im->depth; z++) {
    pi= im->channel[z];
    po= om->channel[z];
    for (y= 0; y < h; y++) {
      for (x= 0; x < w; x++) v1[x]= *(pi++);
      for (x= 1; x < w; x++) {
        t= v1[x - 1] * d;
        if (v1[x] < t) v1[x]= t;
      }
      for (x= w - 2; x >= 0; x--) {
        t= v1[x + 1] * d;
        if (v1[x] < t) v1[x]= t;
      }
      if (y > 0) for (x= 0; x < w; x++) {
        t= v0[x] * d;
        if (v1[x] < t) v1[x]= t;
      }
      for (x= 0; x < w; x++) {
        *(po++)= round(v1[x]);
      }
      memcpy(v0, v1, w * sizeof(*v0));
    }
    for (y= 1; y < h; y++) {
      po -= w;
      for (x= w - 1; x >= 0; x--) v1[x]= *(--po);
      for (x= 0; x < w; x++) {
        t= v0[x] * d;
        if (v1[x] < t) v1[x]= t;
      }
      for (x= 0; x < w; x++, po++) {
        *(po)= round(v1[x]);
      }
      memcpy(v0, v1, w * sizeof(*v0));
    }
  }
  free(v0); free(v1);
  return om;
}

void divide_image(image *a, image *b) {
  int h= a->height;
  int w= a->width;
  int depth= a->depth;
  int i, z;
  gray *pa, *pb;
  if (b->height != h || b->width != w) error("divide_image: size mismatch.");
  if (b->depth != depth) error("divide_image: depth mismatch.");
  for (z= 0; z < depth; z++) {
    pa= a->channel[z]; pb= b->channel[z];
    assert(pa && pb);
    for (i= 0; i < w * h; i++) {
      *pa= (real)*pa / *pb * MAXVAL;
      pa++; pb++;
    }
  }
}

vector *histogram_of_image(image *im, int chan) {
  vector *hi= make_vector(256);
  hi->len= hi->size;
  gray *p= im->channel[chan];
  real *d= hi->data;
  int x, y, h= im->height, w= im->width;
  for (y= 0; y < h; y++) {
    for (x= 0; x < w; x++) {
      if (*p < 0) d[0] += 1;
      else
      if (*p > MAXVAL) d[255] += 1;
      else
      d[(int)(*p / KP)] += 1;
      p++;
    }
  }
  return hi;
}

void contrast_image(image *im, real black, real white) {
  gray *end= im->channel[0] + (im->width * im->height);
  gray *p;
  real a, b;
  int depth= im->depth;
  assert(1 <= depth && depth <= 4);
  black *= MAXVAL;
  white *= MAXVAL;
  if (depth != 1) error("contrast_image: invalid depth");
  if (white == black) {
    for (p= im->channel[0]; p < end; p++) {
      if (*p <= black) *p= 0;
      else *p= MAXVAL;
    }
    return;
  }
  a= MAXVAL / (white - black) ;
  b= -a * black;

  if (black < white) {
    for (p= im->channel[0]; p < end; p++) {
      if (*p <= black) *p= 0;
      else if (*p >= white) *p= MAXVAL;
      else *p= *p * a + b;
    }
    return;
  }

  else { // white < black
    for (p= im->channel[0]; p < end; p++) {
      if (*p >= black) *p= 0;
      else if (*p <= white) *p= MAXVAL;
      else *p= *p * a + b;
    }
    return;
  }
}

void normalize_image(image *im, real strength) {
  vector *d, *v= histogram_of_image(im, 0);
  real *p= v->data;
  int i, b, w, depth= im->depth;

  if (depth != 1) error("normalize_image: invalid depth");

  for (i= 0; i < v->len; i++, p++) {
    *p= log(1 + *p);
  }
  d= copy_vector(v);
  cumul_vector(d);
  diff_vector(d, 10);
  diff_vector(d, 14);
  p= d->data;
  for (i= 12; i < d->len; i++) {
    if (p[i] > 0) break;
  }
  for (; i < d->len; i++) {
    if (p[i] < 0) break;
  }
  b= i - 13;
  for (i= d->len - 1; i >= 0; i--) {
    if (p[i] < 0) break;
  }
  for (; i >= 0; i--) {
    if (p[i] > 0) break;
  }
  w= i - 11;
  destroy_vector(d);
  d= copy_vector(v);
  p= d->data;
  for (i= 0; i < d->len; i++) {
    p[i] *= i;
  }
  cumul_vector(d);
  cumul_vector(v);
  i= (p[255] - p[w])
    / (v->data[255] - v->data[w]);
  i= 1.46 * (w - i) * strength;
  // 1.46 =~ sqrt(2/15) * 4
  //= sqrt(int(0, 1, (1-x^2)*x^2))
  //   / int(0, 1, (1-x^2)*x)
  w= w + i;
  i= (p[0] - p[b])
    / (v->data[0] - v->data[b]);
  i= 1.46 * (i - b) * strength;
  b= b - i;
  //fprintf(stderr, "b: %d w: %d\n", b, w);

  contrast_image(im, b/255.0, w/255.0);
  destroy_vector(v);
  destroy_vector(d);
}

void mean_y(image *im, uint d) {
  int depth= im->depth;
  if (depth != 1) error("mean_y: invalid depth");
  uint w= im->width;
  uint h= im->height;
  real *v= calloc(w * (d + 1), sizeof(*v));
  real *r1, *r, *rd;
  int y, i;
  gray *p, *q, *end;
  for (y= 0; y < h; y++) {
    i= (y+1) % (d+1);
    rd= v + w * i;
    i= (i+d) % (d+1);
    r= v + w * i;
    i= (i+d) % (d+1);
    r1= v + w * i;
    p= im->channel[0] + y * w;
    i= y - d/2;
    if (y >= d) q= im->channel[0] + i * w;
    else q= 0;
    end= p + w;
    for (; p < end; p++, r1++, r++, rd++) {
      *r= *r1 + *p;
      if (q) *(q++)= (*r - *rd) / d;
    }
  }
  free(v);
}

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

void darker_image(image *a, image *b) {
  int h= a->height;
  int w= a->width;
  int depth= a->depth;
  if (depth != 1) error("invalid depth");
  int i;
  gray *pa, *pb;
  if (b->height != h || b->width != w) error("darker_image: size mismatch.");
  if (b->depth != depth) error("darker_image: depth mismatch.");
  pa= a->channel[0]; pb= b->channel[0];
  for (i= 0; i < w * h; i++) {
    if (*pa > *pb) { *pa= *pb; };
    pa++; pb++;
  }
}

void calc_statistics(image *im, int verbose) {
  int depth= im->depth;
  if (depth != 1) error(": invalid depth");
  // threshold histogram
  vector *thr= make_vector(256);
  thr->len= thr->size;
  real *pt= thr->data;
  // border histogram
  vector *hb= make_vector(256);
  hb->len= hb->size;
  real *pb= hb->data;
  // area histogram
  vector *ha= make_vector(256);
  ha->len= ha->size;
  real *pa= ha->data;
  real area, border, thickness, black, graythr, white;
  int i, x, y, t;
  int h= im->height, w= im->width;
  gray *pi= im->channel[0];
  gray *px= pi + 1;
  gray *py= pi + w;
  short a, b, c;
  uint d;
  // histograms
  for (y= 0; y < h; y++) {
    for (x= 0; x < w; x++) {
      a= *pi / KP; b= *px / KP;
      pa[a]++;
      if ((x >= w - 1) || (y >= h - 1)) {continue;} 
      if (a > b) {c= b; b= a; a= c;}
      pb[a]++; pb[b]--;
      d= b - a; d *= d;
      pt[a] += d; pt[b] -= d;
      a= *pi / KP; b= *py / KP;
      if (a > b) {c= b; b= a; a= c;}
      pb[a]++; pb[b]--;
      d= b - a; d *= d;
      pt[a] += d; pt[b] -= d;
      pi++; px++, py++;
    }
    pi++; px++, py++;
  }
  // gray threshold
  cumul_vector(thr);
  cumul_vector(hb);
  // for (i= 0; i < 256; i++) {thr->data[i] /= sqrt(hb->data[i] + 4);}
  t= index_of_max(thr);
  graythr= t / 255.0;
  // border, area, thickness, nchars
  border= hb->data[t] * 0.8;
  cumul_vector(ha);
  area= ha->data[t];
  thickness= 2 * area / border;
  // black
  black= 0;
  for (i= 0; i < t; i++) {
    black += ha->data[i];
  }
  black= (t - (black / area)) / 255.0;
  // white
  white= 255.0 * w * h - (area * t);
  for (i= t + 1; i < 256; i++) {
    white -= ha->data[i];
  }
  white /= (w * h - area) * 255.0;
  if (verbose) {printf(
      "black: %g gray: %g white: %g thickness: %g area: %g \n",
      black, graythr, white, thickness, area 
  );}
  im->black= black;
  im->graythr= graythr;
  im->white= white;
  im->thickness= thickness;
  im->area= area;
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

// vim: sw=2 ts=2 sts=2 expandtab:
