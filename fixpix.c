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
#define IS_IMAGE(p) (((image *)p)->type == 'I')
#define IS_VECTOR(p) (((vector *)p)->type == 'V')

void help(char **arg0, char *topic) {
#define TOPIC(x) EQ((x), topic)
if (! topic) {
  printf("\nUSAGE: %s COMMANDS...\n\n", *arg0);
  printf("COMMANDS:\n\
  FILENAME:       load a PNM image\n\
  -:              load from STDIN\n\
  bg FLOAT:       find background light\n\
  contrast BLACK WHITE:\n\
                  enhance contrast\n\
  div:            divide im2 / im1\n\
  fix-bg:         fix background light\n\
  histo:          histogram\n\
  ex FLOAT:       set height-of-x\n\
  norm STRENGTH:  normalize contrast\n\
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
} else if (TOPIC("contrast")) {;
  printf("Coming soon...\n");
} else if (TOPIC("div")) {;
  printf("Coming soon...\n");
} else if (TOPIC("fix-bg")) {;
  printf("Coming soon...\n");
} else if (TOPIC("histo")) {;
  printf("Coming soon...\n");
} else if (TOPIC("ex")) {;
  printf("Coming soon...\n");
} else if (TOPIC("norm")) {;
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
  char type;
  real *data;
  uint len;
  uint size;
  real x0;
  real dx;
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

vector *copy_vector(vector *v0) {
  vector *v = make_vector(v0->size);
  v->len = v0->len;
  v->x0 = v0->x0;
  v->dx = v0->dx;
  memcpy(v->data, v0->data, v->len * sizeof(*(v->data)));
  return v;
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

void write_vector(vector *v, FILE *f) {
  uint i;
  real *p = v->data;
  real x = v->x0;
  for (i = 0; i < v->len; i ++, x += v->dx) {
    fprintf(f, "%f %f\n", x, p[i]);
  }
}

uint index_of_max(vector *v) {
  uint i, j = 0;
  real *p = v->data;
  real m = p[0];
  for (p++, i = 1; i < v->len; i++, p++) {
    if (*p > m) {m = *p; j = i;}
  }
  return j;
}

//// IMAGES ////

typedef struct { // image
  char type;
  short *data;
  uint width;
  uint height;
  real ex; // height of x in pixels
} image;

real default_ex = 25;

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
  im->ex = 0;
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
  om->ex = im->ex;
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
  real d = im->ex;
  if (d <= 0) d = default_ex;
  d = 0.333 / d;
  d = exp(-d);
  int x, y, h = im->height, w = im->width;
  image *om = make_image(w, h);
  om->ex = im->ex;
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

void divide_image(image *a, image *b) {
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

vector *threshold_histogram(image *im) {
  vector *hi = make_vector(256);
  hi->len = hi->size;
  real *v = hi->data;
  int x, y, h = im->height, w = im->width;
  short *pi = im->data;
  short *px = pi + 1;
  short *py = pi + w;
  short a, b, c;
  uint d;
  for (y = 0; y < h - 1; y++) {
    for (x = 0; x < w - 1; x++) {
      a = *pi / K; b = *px / K;
      if (a > b) {c = b; b = a; a = c;}
      d = b - a;
      v[a] += d; v[b] -= d;
      a = *pi / K; b = *py / K;
      if (a > b) {c = b; b = a; a = c;}
      d = b - a;
      v[a] += d; v[b] -= d;
      pi ++; px ++, py ++;
    }
    pi ++; px ++, py ++;
  }
  cumul_vector(hi);
  return hi;
}

void contrast_image(image *im, real black, real white) {
  vector *v = make_vector(256);
  v->len = v->size;
  real m = 255.0 * K / (white - black);
  real q = K_2 - black * m;
  real t;
  uint i;
  for (i = 0; i < v->len; i++) {
    t = m * i + q;
    if (t < 0) t = 0;
    else
    if (t > MAXVAL) t = MAXVAL;
    v->data[i] = t;
  }
  ulong l = im->width * im->height;
  short *p;
  for (p = im->data; p < im->data + l; p++) {
    *p = v->data[*p / K];
  }
  destroy_vector(v);
}

void normalize_image(image *im, real strength) {
  uint i, j;
  vector *v0 = histogram_of_image(im);
  vector *v1 = copy_vector(v0);
  real w, b, max0, max1, *p = v1->data;
  uint c, c1;
  for (i = 0; i < v1->len; i ++, p ++) *p *= i;
  cumul_vector(v0);
  cumul_vector(v1);
  max0 = v0->data[255];
  max1 = v1->data[255];
  c = max1 / max0;
  for (i = 0; i < 100; i ++) {
    b = v1->data[c] / v0->data[c];
    w = (max1 - v1->data[c]) / (max0 -v0->data[c]);
    c1 = (b + w + 1) / 2;
    if (c1 == c) break;
    c = c1;
  }
  //printf("%f %f\n", b, w);

  uint x1, x2;
  real y1, y2, m, q, mb, qb, mw, qw, yb, yw;
  for (j = 0; j < 9; j ++) {
    // gray zone
    x1 = (3*b + w) / 4;
    x2 = (b + 3*w) / 4;
    y1 = v0->data[x1];
    y2 = v0->data[x2];
    m = (y2 - y1) / (x2 - x1);
    q = (y1 - m * x1) ;

    // black zone
    yb = m * b + q;
    y1 = yb * 0.25;
    y2 = yb * 0.75;
    for (i = 0; i < 256; i ++) {
      x1 = i;
      if (v0->data[i] <= y1) continue;
      break;
    }
    for (i = x1; i < 256; i ++) {
      x2 = i;
      if (v0->data[i] < y2) continue;
      break;
    }
    y1 = v0->data[x1];
    y2 = v0->data[x2];
    mb = (y2 - y1) / (x2 - x1);
    qb = (y1 - mb * x1);

    // white zone
    yw = m * w + q;
    y1 = (3*yw + max0) / 4;
    y2 = (yw + 3*max0) / 4;
    for (i = 255; i >= 0 ; i --) {
      x2 = i;
      if (v0->data[i] >= y2) continue;
      break;
    }
    for (i = x2; i >=0; i --) {
      x1 = i;
      if (v0->data[i] >= y1) continue;
      break;
    }
    y1 = v0->data[x1];
    y2 = v0->data[x2];
    mw = (y2 - y1) / (x2 - x1);
    qw = (y2 - mw * x2) ;

    y1 = (q - qb) / (mb - m);
    y2 = (q - qw) / (mw - m);

    if (hypot(y1-b, y2-w) < 0.1) break;
    b = y1; w = y2;

    //printf("b: %f w: %f \n", b, w);
  }

  // black zone: [x1..b]
  // white zone: [w..x2]
  x1 = - qb / mb;
  x2 = (max0 - qw) / mw;
  c = (b + w + x1 + x2) / 4;
  // choose black and white
  b += (x1 - b) * (1 - strength);
  w += (x2 - w) * (1 - strength);
  if (w <= b) {
    b = c;
    w = b + 1;
  }

  contrast_image(im, b, w);

  destroy_vector(v0);
  destroy_vector(v1);
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
  char **arg = args;
  image *im;
  vector *v;
  void *p;
  FILE *f;
  real t;

  init_srgb();
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
    if (ARG_IS("contrast")) { // norm BLACK WHITE
      if (! *(++arg)) error("norm: missing parameter");
      if (! *(++arg)) error("norm: missing parameter");
      contrast_image((image*)SP_1, atof(*(arg-1)), atof(*arg));
    }
    if (ARG_IS("div")) { // div
      divide_image((image*)SP_2, (image*)SP_1);
      pop();
    }
    else
    if (ARG_IS("ex")) { // ex FLOAT
      if (! *(++arg)) error("ex: missing parameter");
      t = atof(*arg);
      if (t <= 0) error("ex param must be > 0.");
      if (sp) {
        im = (image*)SP_1;
        if (t < 1) t *= im->height;
        im->ex = t;
      }
      default_ex = t;
    }
    else
    if (ARG_IS("fix-bg")) { // fix-bg
      push(image_background((image*)SP_1));
      divide_image((image*)SP_2, (image*)SP_1);
      pop();
    }
    else
    if (ARG_IS("histo")) { // histo
      v = histogram_of_image((image*)SP_1);
      push(v);
    }
    else
    if (ARG_IS("norm")) { // norm FLOAT
      if (! *(++arg)) error("norm: missing parameter");
      normalize_image((image*)SP_1, atof(*arg));
    }
    else
    if (ARG_IS("quit")) exit(0); // quit
    else
    if (ARG_IS("rot")) { // rot +-90, 180, 270
      if (! *(++arg)) error("rot: missing parameter");
      push(rotate_image((image*)SP_1, atof(*arg)));
      swap(); pop();
    }
    else
    if (ARG_IS("splitx")) { // splitx FLOAT
      if (! *(++arg)) error("splitx: missing parameter");
      push(0); swap();
      push(0); swap();
      splitx_image(&SP_2, &SP_3, (image*)SP_1, atof(*arg));
      pop();
    }
    else
    if (ARG_IS("splity")) { // splity FLOAT
      if (! *(++arg)) error("splity: missing parameter");
      push(0); swap();
      push(0); swap();
      splity_image(&SP_2, &SP_3, (image*)SP_1, atof(*arg));
      pop();
    }
    else
    if (ARG_IS("thr")) { // thr
      v = threshold_histogram((image*)SP_1);
      push(v);
      printf("thr: %d\n", index_of_max(v));
    }
    else
    if (ARG_IS("w")) { // w FILENAME
      if (! *(++arg)) error("w: missing parameter");
      p = pop();
      f = fopen(*arg, "wb");
      if (IS_IMAGE(p)) write_image(p, f);
      else
      if (IS_VECTOR(p)) write_vector(p, f);
    }
    else {
    if (ARG_IS("-")) { // -
      push(read_image(stdin, 0));
    }
    else
      push(read_image(fopen(*arg, "rb"), 0)); // FILENAME
    }
  }
  if (sp > 0) {
    p = pop();
    f = stdout;
    if (IS_IMAGE(p)) write_image(p, f);
    else
    if (IS_VECTOR(p)) write_vector(p, f);
  }
}

// vim: sw=2 ts=2 sts=2 expandtab:
