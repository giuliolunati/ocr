#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#define uchar unsigned char
#define uint unsigned int
#define ulong unsigned long int
#define real float
#define K 16
#define K_2 8 // K/2
#define MAXVAL 4095 // 256 * K - 1
#define MAXSHORT 32767
#define EQ(a, b) (0 == strcmp((a), (b)))
#define IS_IMAGE(p) (((char *)p)->type == 'I')
#define IS_VECTOR(p) (((char *)p)->type == 'V')

void help(char **arg0, char *topic) {
#define TOPIC(x) EQ((x), topic)
if (! topic) {
  printf("\nUSAGE: %s COMMANDS...\n\n", *arg0);
  printf("COMMANDS:\n\
  FILENAME:       load a PNM image\n\
  -:              load from STDIN\n\
  bg FLOAT:       find background light\n\
  div:            divide im2 / im1\n\
  fix-bg:         fix background light\n\
  histo:          histogram\n\
  lpp FLOAT:      set lines-per-page\n\
  quit:           quit w/out output\n\
  rot ANGLE:      rotate\n\
  splitx X:       split vertically\n\
  splity Y:       split horizontally\n\
  w FILENAME:     write to file\n\
  -h, --help:     this summary\n\
  \n\
  -h, --help COMMAND: help on COMMAND\n\
  \n");
} else if (TOPIC("bg")) {;
  printf("Coming soon...\n");
} else if (TOPIC("div")) {;
  printf("Coming soon...\n");
} else if (TOPIC("fix-bg")) {;
  printf("Coming soon...\n");
} else if (TOPIC("histo")) {;
  printf("Coming soon...\n");
} else if (TOPIC("lpp")) {;
  printf("Coming soon...\n");
} else if (TOPIC("quit")) {;
  printf("Coming soon...\n");
} else if (TOPIC("rot")) {;
  printf("Coming soon...\n");
} else if (TOPIC("splitx")) {;
  printf("Coming soon...\n");
} else if (TOPIC("splity")) {;
  printf("Coming soon...\n");
} else if (TOPIC("w")) {;
  printf("Coming soon...\n");
}
exit(0);
}

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
  real l;
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

//// VECTORS ////

typedef struct { // vector
  real *data;
  uint len;
  uint size;
  real x0;
  real dx;
  char type;
} vector;

vector *make_vector(uint size) {
  vector *v;;
  if (! (v = malloc(sizeof(vector))))
    error("Can't alloc vector.");
  v->size = size;
  if ( ! (
    v->data = calloc(size, sizeof(v->data))
  )) error("Can't alloc vector data.");
  v->len = 0;
  v->x0 = 0.0;
  v->dx = 1.0;
  v->type = 'V';
  return v;
}

void destroy_vector(vector *h) {
  if (! h) return;
  if (h->data) free(h->data);
  free(h);
}

void cumul_vector(vector *v) {
  uint i;
  real *p = v->data;
  for (i = 1; i < v->len; i ++) {
    p[i] += p[i - 1];
  }
}

void diff_vector(vector *v) {
  uint i;
  real *p = v->data;
  for (i = v->len - 1; i > 0; i --) {
    p[i] -= p[i - 1];
  }
}

void dump_vector(FILE *f, vector *v) {
  uint i;
  real *p = v->data;
  fprintf(f, "# x0: %f dx: %f len: %d\n",
      v->x0, v->dx, v->len
  );
  for (i = 0; i < v->len; i ++) {
    fprintf(f, "%f\n", p[i]);
  }
}

//// IMAGES ////

typedef struct { // image
  short *data;
  uint width;
  uint height;
  real lpp; // lines-per-page: height / line-height
  char type;
} image;

real default_lpp = 40;

image *make_image(int width, int height) {
  image *im;
  short *data;
  if (height < 1 || width < 1) return NULL;
  data = calloc(width * height, sizeof(short));
  if (! data) return NULL;
  im = malloc(sizeof(image));
  if (! im) return NULL;
  im->height = height;
  im->width = width;
  im->data = data;
  im->lpp = 0;
  im->type = 'I';
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
  char c;
  if (! file) error("File not found.");
  if (1 > fscanf(file, "P%d ", &depth)) {
    error("Not a PNM file - wrong magic.");
  }
  while (1 > fscanf(file, "%d ", &width)) {
    x = fscanf(file, "%c", &c);
    if (c != '#') error("Not a PNM file - wrong width.");
    while (1) {
      x = fscanf(file, "%c", &c);
      if (c == '\n') break;
    }
  }
  while (1 > fscanf(file, "%d ", &height)) {
    x = fscanf(file, "%c", &c);
    if (c != '#') error("Not a PNM file - wrong height.");
    while (1) {
      x = fscanf(file, "%c", &c);
      if (c == '\n') break;
    }
  }
  while (1 > fscanf(file, "%d ", &type)) {
    x = fscanf(file, "%c", &c);
    if (c != '#') error("Not a PNM file - wrong type.");
    while (1) {
      x = fscanf(file, "%c", &c);
      if (c == '\n') break;
    }
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
    for (x = 0; x < width; x++, pt++, ps += depth) {
      *pt = *ps * K + K_2;
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
  short v, *ps;
  assert(file);
  fprintf(file, "P5\n%d %d\n255\n", im->width, im->height);
  buf = malloc(im->width * sizeof(*buf));
  assert(buf);
  ps = im->data;
  assert(ps);
  for (y = 0; y < im->height; y++) {
    pt = buf;
    for (x = 0; x < im->width; x++) {
      v = *(ps++);
      if (v < 0) v = 0;
      else
      if (v > MAXVAL) v = MAXVAL;
      *pt++ = v / K; 
    }
    if (im->width > fwrite(buf, 1, im->width, file)) error("Error writing file.");
  }
  free(buf);
}

image *rotate_image(image *im, float angle) {
  int w = im->width, h = im->height;
  int x, y, dx, dy;
  image *om = make_image(h, w);
  om->lpp = im->lpp;
  short *i, *o;
  i = im->data; o = om->data;
  switch ((int)angle) {
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

void splitx_image(void **out1, void **out2, image *im, float x) {
  if (x <= 0) error("x must be > 0.");
  if (x == 1) error("x must be != 1.");
  if (x > 1) x = 1/x;
  short *p, *p1, *p2;
  uint y, h = im->height;
  uint w = im->width;
  uint w1 = w * x;
  uint w2 = w - w1;
  image* im1 = make_image(w1, h);
  image* im2 = make_image(w2, h);
  p = im->data;
  p1 = im1->data;
  p2 = im2->data;
  for (y = 0; y < h; y++, p += w, p1 += w1, p2 += w2) {
    memcpy(p1, p, w1 * sizeof(short));
    memcpy(p2, p + w1, w2 * sizeof(short));
  }
  *out1 = im1;
  *out2 = im2;
}

void splity_image(void **out1, void **out2, image *im, float y) {
  if (y <= 0) error("y must be > 0.");
  if (y == 1) error("y must be != 1.");
  if (y > 1) y = 1/y;
  uint w = im->width;
  ulong l1, l2; 
  uint h1 = im->height * y;
  uint h2 = im->height - h1;
  *out1 = make_image(w, h1);
  *out2 = make_image(w, h2);
  l1 = w * h1;
  l2 = w * h2;
  memcpy(((image*)(*out1))->data, im->data, l1 * sizeof(short));
  memcpy(((image*)(*out2))->data, im->data + l1, l2 * sizeof(short));
}

image *image_background(image *im) {
  // find light background
  real d = im->lpp;
  if (d <= 0) d = default_lpp;
  d /= im->height;
  d = exp(-d);
  int x, y, h = im->height, w = im->width;
  image *om = make_image(w, h);
  om->lpp = im->lpp;
  real t, *v0, *v1;
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
    *pa = (real)*pa / *pb * MAXVAL;
    pa++; pb++;
  }
}

vector *histogram_of_image(image *im) {
  vector *hi = make_vector(256);
  hi->len = hi->size;
  short *p = im->data;
  real *d = hi->data;
  int x, y, h = im->height, w = im->width;
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      if (*p < 0) d[0] += 1;
      else
      if (*p > MAXVAL) d[255] += 1;
      else
      d[*p / K] += 1;
      p ++;
    }
  }
  return hi;
}

//// STACK ////
#define STACK_SIZE 256
void *stack[STACK_SIZE];
#define SP stack[sp]
#define SP_1 stack[sp - 1]
#define SP_2 stack[sp - 2]
#define SP_3 stack[sp - 3]
int sp = 0;

void *swap() {
  void *p;
  if (sp < 2) error("Stack underflow");
  p = SP_2; SP_2 = SP_1; SP_1 = p;
}

void *pop() {
  if (sp < 1) error("Stack underflow");
  sp--;
  return SP;
}

void push(void *p) {
  if (sp >= STACK_SIZE) error("Stack overflow");
  if (SP) free(SP);
  SP = p;
  sp++;
}

//// MAIN ////
int main(int argc, char **args) {
  #define ARG_IS(x) EQ((x), *arg)
  int i, c, d;
  init_srgb();
  char **arg = args;

  image *im;
  if (argc < 2) help(args, NULL);
  while (*(++arg)) {
    if (ARG_IS("-h") || ARG_IS("--help")) { // -h, --help
      help(args, *(++arg));
    }
    else
    if (ARG_IS("bg")) { // bg FLOAT
      push(image_background((image*)SP_1));
    }
    else
    if (ARG_IS("div")) { // div
      divide_image((image*)SP_2, (image*)SP_1);
      pop();
    }
    else
    if (ARG_IS("fix-bg")) { // fix-bg
      push(image_background((image*)SP_1));
      divide_image((image*)SP_2, (image*)SP_1);
      pop();
    }
    else
    if (ARG_IS("histo")) { // histo
      vector *v = histogram_of_image((image*)SP_1);
      dump_vector(stdout, v);
    }
    else
    if (ARG_IS("lpp")) { // lpp FLOAT
      if (! *(++arg)) break;
      default_lpp = atof(*arg);
      if (sp) ((image*)SP_1)->lpp = default_lpp;
    }
    else
    if (ARG_IS("quit")) exit(0); // quit
    else
    if (ARG_IS("rot")) { // rot +-90, 180, 270
      if (! *(++arg)) break;
      push(rotate_image((image*)SP_1, atof(*arg)));
      swap(); pop();
    }
    else
    if (ARG_IS("splitx")) { // splitx FLOAT
      if (! *(++arg)) break;
      push(0); swap();
      push(0); swap();
      splitx_image(&SP_2, &SP_3, (image*)SP_1, atof(*arg));
      pop();
    }
    else
    if (ARG_IS("splity")) { // splity FLOAT
      if (! *(++arg)) break;
      push(0); swap();
      push(0); swap();
      splity_image(&SP_2, &SP_3, (image*)SP_1, atof(*arg));
      pop();
    }
    else
    if (ARG_IS("w")) { // w FILENAME
      if (! *(++arg)) break;
      if (sp > 0) {
        write_image(pop(), fopen(*arg, "wb"));
      }
    }
    else {
    if (ARG_IS("-")) { // -
      push(read_image(stdin, 0));
    }
    else
      push(read_image(fopen(*arg, "rb"), 0)); // FILENAME
    }
  }
  if (sp > 0) write_image(pop(), stdout);
}

// vim: sw=2 ts=2 sts=2 expandtab:
