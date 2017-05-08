#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#define EQ(a, b) 0 == strcmp((a), (b))
#define uchar unsigned char
#define uint unsigned int
#define ulong unsigned long int
#define MAXVAL 6552 
#define MAXSHORT 32767

//// ERRORS ////

void error(const char *msg) {
  fprintf(stderr, "ERROR: ");
  fprintf(stderr, "%s", msg);
  fprintf(stderr, "\n");
  exit(1);
}

//// SRGB ////
  // 0 <= srgb <= 1  0 <= lin <= 1
  // srgb < 0.04045: lin = srgb / 12.92
  // srgb > 0.04045: lin = [(srgb + 0.055) / 1.055]^2.4

short lin_from_srgb[256];
uchar srgb_from_lin[MAXVAL + 1];

void init_srgb() {
  int i, l0, s;
  float l;
  for (s = 0; s <= 255; s++) {
    l = (s + 0.5) / 255.5;
    if (l < 0.04045) l /= 12.92;
    else {
      l = (l + 0.055) / 1.055;
      l = pow(l, 2.4);
    }
    l = roundf(l * MAXVAL);
    lin_from_srgb[s] = l;
    if (s == 0) { l0 = 0; continue; }
    for (i = l0; i <= (l0 + l) / 2; i++) {
      srgb_from_lin[i] = s - 1;
    }
    for (; i <= l; i++) {
      srgb_from_lin[i] = s;
    }
    l0 = l;
  }
}

//// IMAGES ////

typedef struct { // image
  short *data;
  uint width;
  uint height;
  float lpp; // lines-per-page: height / line-height
} image;

float default_lpp = 40;

image *make_image(int height, int width) {
  image *im;
  short *data;
  if (height < 1 || width < 1) return NULL;
  data = malloc(width * height * sizeof(short));
  if (! data) return NULL;
  im = malloc(sizeof(image));
  if (! im) return NULL;
  im->height = height;
  im->width = width;
  im->data = data;
  im->lpp = 0;
  return im;
}

void destroy_image(image *im) {
  if (! im) return;
  if (im->data) free(im->data);
  free(im);
}

image *read_image(FILE *file, int layer) {
  int x, y, type, depth, height, width, binary;
  image *im;
  uchar *buf, *ps;
  short *pt;
  if (! file) error("File not found.");
  if (4 > fscanf(file, "P%d %d %d %d\n", &depth, &width, &height, &type)) {
    error("Not a PNM file.");
  }
  switch (depth) {
    case 5: depth = 1; break;
    case 6: depth = 3; break;
    default: error("Invalid depth.");
  }
  switch (type) {
    case 255: break;
    default: error("Invalid bits.");
  }
  if (width < 1 || height < 1) error("Invalid dimensions.");
  im = make_image(width, height);
  if (!im) error("Cannot make image.");;
  buf = malloc(width * depth);
  pt = im->data;
  for (y = 0; y < height; y++) {
    if (width > fread(buf, depth, width, file))  error("Unexpected EOF");
    ps = buf + layer;
    for (x = 0; x < width; x++) {
      *(pt++) = *(ps += depth);
    }
  }
  free(buf);
  return im;
}

void *image_from_srgb(image *im) {
  short *p, *end;
  short i;
  end = im->data + (im->height * im->width);
  for (p = im->data; p < end; p++) {
    i = *p;
    if (i > 255) *p = MAXVAL;
    else
    if (i < 0) *p = 0;
    else
    *p = lin_from_srgb[*p];
  }
}

int write_image(image *im, FILE *file) {
  int x, y;
  uchar *buf, *pt;
  short i, *ps;
  assert(file);
  fprintf(file, "P5\n%d %d\n255\n", im->width, im->height);
  buf = malloc(im->width * sizeof(*buf));
  assert(buf);
  ps = im->data;
  assert(ps);
  for (y = 0; y < im->height; y++) {
    pt = buf;
    for (x = 0; x < im->width; x++) {
      i = *(ps++);
      if (i < 1) {*(pt++) = 0; continue;}
      else
      if (i > MAXVAL) {*(pt++) = 255; continue;}
      else
      *(pt++) = srgb_from_lin[i];
    }
    if (im->width > fwrite(buf, 1, im->width, file)) error("Error writing file.");
  }
  free(buf);
}

image *rotate_image(image *im, int angle) {
  int w = im->width, h = im->height;
  int x, y, dx, dy;
  image *om = make_image(h, w);
  om->lpp = im->lpp;
  short *i, *o;
  i = im->data; o = om->data;
  switch (angle) {
    case 90:
      o += h - 1; dx = h; dy = -1;
      break;
    case 180:
      om->width = w; om->height = h;
      o += h * w - 1; dx = -1; dy = -w;
      break;
    case 270:
    case -90:
      o += h * w - h; dx = -h; dy = 1;
      break;
    default: error("rotate: only +/-90, 180, 270");
  }
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      *o = *i; i++; o += dx;
    }
    o += dy - w * dx;
  }
  return om;
}

image *image_background(image *im) {
  // find light background
  float d = im->lpp;
  if (d <= 0) d = default_lpp;
  d /= im->height;
  d = exp(-d);
  int x, y, h = im->height, w = im->width;
  image *om = make_image(w, h);
  om->lpp = im->lpp;
  float t, *v0, *v1;
  v0 = malloc(w * sizeof(*v0));
  v1 = malloc(w * sizeof(*v1));
  short *pi = im->data;
  short *po = om->data;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) v1[x] = *(pi++);
    for (x = 1; x < w; x++) {
      t = v1[x - 1] * d;
      if (v1[x] < t) v1[x] = t;
    }
    for (x = w - 2; x >= 0; x--) {
      t = v1[x + 1] * d;
      if (v1[x] < t) v1[x] = t;
    }
    if (y > 0) for (x = 0; x < w; x++) {
      t = v0[x] * d;
      if (v1[x] < t) v1[x] = t;
    }
    for (x = 0; x < w; x++) {
      *(po++) = round(v1[x]);
    }
    memcpy(v0, v1, w * sizeof(*v0));
  }
  assert(pi == im->data + w * h);
  assert(po == om->data + w * h);
  for (y = 1; y < h; y++) {
    po -= w;
    for (x = w - 1; x >= 0; x--) v1[x] = *(--po);
    for (x = 0; x < w; x++) {
      t = v0[x] * d;
      if (v1[x] < t) v1[x] = t;
    }
    for (x = 0; x < w; x++) {
      *(po++) = round(v1[x]);
    }
    memcpy(v0, v1, w * sizeof(*v0));
  }
  assert(po == om->data + w);
  free(v0); free(v1);
  return om;
}

void *divide_image(image *a, image *b) {
  int h = a->height;
  int w = a->width;
  int i;
  short *pa, *pb;
  if (b->height != h || b->width != w) error("Dimensions mismatch.");
  pa = a->data; pb = b->data;
  for (i = 0; i < w * h; i++) {
    *pa = (float)*pa / *pb * MAXVAL;
    pa++; pb++;
  }
}

//// STACK ////
#define STACK_SIZE 256
image *stack[STACK_SIZE];
#define SP stack[sp]
#define SP_1 stack[sp - 1]
#define SP_2 stack[sp - 2]

int sp = 0;
void *swap() {
  image *im;
  if (sp < 2) error("Stack underflow");
  im = SP_2; SP_2 = SP_1; SP_1 = im;
}
image *pop() {
  if (sp < 1) error("Stack underflow");
  sp--;
  return SP;
}
void push(image *im) {
  if (sp >= STACK_SIZE) error("Stack overflow");
  if (SP) free(SP);
  SP = im;
  sp++;
}

//// MAIN ////
#define ARG_IS(x) EQ((x), *arg)
int main(int argc, char **arg) {
  int i, c, d;
  init_srgb();

  image * im;
  while (*(++arg)) { // -
    if (ARG_IS("-")) {
      push(read_image(stdin, 0));
      image_from_srgb(SP_1);
    }
    else // bg FLOAT
    if (ARG_IS("bg")) {
      push(image_background(SP_1));
    }
    else // div
    if (ARG_IS("div")) {
      divide_image(SP_2, SP_1);
      pop();
    }
    else // fix-bg
    if (ARG_IS("fix-bg")) {
      push(image_background(SP_1));
      divide_image(SP_2, SP_1);
      pop();
    }
    else // lpp float
    if (ARG_IS("lpp")) {
      if (! *(++arg)) break;
      default_lpp = atof(*arg);
      if (sp) SP_1->lpp = default_lpp;
    }
    else // quit
    if (ARG_IS("quit")) exit(0);
    else // rot +-90, 180, 270
    if (ARG_IS("rot")) {
      if (! *(++arg)) break;
      push(rotate_image(SP_1, atoi(*arg)));
      swap(); pop();
    }
    else { // STRING
      push(read_image(fopen(*arg, "rb"), 0));
      image_from_srgb(SP_1);
    }
  }
  if (sp > 0) write_image(pop(), stdout);
}

// vim: sw=2 ts=2 sts=2 expandtab:
