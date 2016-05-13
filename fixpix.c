#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <math.h>

#define EQ(a, b) 0 == strcmp((a), (b))
#define uchar unsigned char
#define uint unsigned int

typedef struct {
  short *data;
  uint height;
  uint width;
} image;

//// ... ////

void error(char *msg) {
  fprintf(stderr, "ERROR: ");
  fprintf(stderr, msg);
  fprintf(stderr, "\n");
  exit(1);
}

//// SRGB ////
// 0 <= srgb <= 1  0 <= lin <= 1
// srgb < 0.04045: lin = srgb / 12.92
// srgb > 0.04045: lin = [(srgb + 0.055) / 1.055]^2.4

#define MAXVAL 3276

short lin_from_srgb[256];
uchar srgb_from_lin[MAXVAL + 1];

void init_srgb() {
  int i, l0, s;
  float l;
  for (s = 0; s <= 255; s++) {
    l = s / 255.0;
    if (l < 0.04045) l /= 12.92;
    else {
      l = (l + 0.055) / 1.055;
      l = pow(l, 2.4);
    }
    l = roundf(l * MAXVAL)
    lin_from_srgb[s] = l;
    if (s == 0) { l0 = 0; continue; }
    for (i = l0; i <= (l0 + l) / 2; i++) {
      srgb_from_lin[i] = s - 1;
    }
    for (; i < l; i++) {
      srgb_from_lin[i] = s;
    }
    l0 = l;
  }
}

//// IMAGES ////

image *image_make(int height, int width) {
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
  return im;
}

void image_destroy(image *im) {
  if (! im) return;
  if (im->data) free(im->data);
  free(im);
}

image *image_read(FILE *file) {
  int x, y, type, depth, height, width, binary;
  image *im;
  uchar *buf, *ps;
  short *pt;
  if (! file) error("File not found.");
  if (4 > fscanf(file, "P%d %d %d %d\n", &depth, &width, &height, &type)) {
    error("Not PNM file.");
  }
  switch (depth) {
    case 5: break;
    default: return NULL;
  }
  switch (type) {
    case 255: break;
    default: return NULL;
  }
  if (width < 1 || height < 1) return NULL;
  im = image_make(height, width);
  if (!im) return NULL;
  buf = malloc(width);
  pt = im->data;
  for (y = 0; y < height; y++) {
    if (width > fread(buf, 1, width, file))  error("Unexpected EOF");
    ps = buf;
    for (x = 0; x < width; x++) {
      *(pt++) = lin_from_srgb[*(ps++)];
    }
  }
  free(buf);
  return im;
}

int image_write(image *im, FILE *file) {
  int x, y;
  uchar *buf, *pt;
  short *ps;
  assert(file);
  fprintf(file, "P5\n%d %d\n255\n", im->width, im->height);
  buf = malloc(im->width);
  ps = im->data;
  for (y = 0; y < im->height; y++) {
    pt = buf;
    for (x = 0; x < im->width; x++) {
      *(pt++) = srgb_from_lin[*(ps++)];
    }
    if (im->width > fwrite(buf, 1, im->width, file)) error("Error writing file.");
  }
}

//// MAIN ////
image *IM;
#define ARG_IS(x) EQ((x), *arg)
int main(int argc, char **arg) {
  int i, c, d;
  init_srgb();

  image * im;
  while (*(++arg)) {
    if (ARG_IS("-")) {
      IM = image_read(stdin);
    }
    else {
      IM = image_read(fopen(*arg, "rb"));
    }
  }
  if (IM) image_write(IM, stdout);
  exit(0);
}
/**/
// vim: sw=2 ts=2 sts=2:
