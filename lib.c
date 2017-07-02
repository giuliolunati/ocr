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
  for (i = 1; i < v->len; i++) {
    p[i] += p[i - 1];
  }
}

void diff_vector(vector *v, uint d) {
  uint i;
  real *p = v->data;
  for (i = v->len - 1; i >= d; i--) {
    p[i] -= p[i - d];
  }
}

void write_vector(vector *v, FILE *f) {
  uint i;
  real *p = v->data;
  real x = v->x0;
  for (i = 0; i < v->len; i++, x += v->dx) {
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
  im->ex = default_ex;
  im->pag = 0;
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

image *rotate_90_image(image *im, int angle) {
  int w = im->width, h = im->height;
  int x, y, dx, dy;
  image *om = make_image(h, w);
  om->ex = im->ex;
  om->pag = im->pag;
  short *i, *o;
  i = im->data; o = om->data;
  switch ((int)angle) {
    case 90:
    case -270:
      o += h - 1; dx = h; dy = -1;
      break;
    case 180:
    case -180:
      om->width = w; om->height = h;
      o += h * w - 1; dx = -1; dy = -w;
      break;
    case 270:
    case -90:
      o += h * w - h; dx = -h; dy = 1;
      break;
    default: assert(0);
  }
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      *o = *i; i++; o += dx;
    }
    o += dy - w * dx;
  }
  return om;
}

image *rotate_image(image *im, float angle) {
  float r = angle / 90;
  int n = roundf(r);
  r = angle - (90 * n);
  n = (n % 4) * 90;
  return rotate_90_image(im, n);
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
  im1->pag = im->pag;
  im2->pag = im->pag + 1;
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
  image* im1 = make_image(w, h1);
  image* im2 = make_image(w, h2);
  l1 = w * h1;
  l2 = w * h2;
  memcpy(im1->data, im->data, l1 * sizeof(short));
  memcpy(im2->data, im->data + l1, l2 * sizeof(short));
  im1->pag = im->pag;
  im2->pag = im->pag + 1;
  *out1 = im1;
  *out2 = im2;
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
      p++;
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
      pi++; px++, py++;
    }
    pi++; px++, py++;
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
  for (i = 0; i < v1->len; i++, p++) *p *= i;
  cumul_vector(v0);
  cumul_vector(v1);
  max0 = v0->data[255];
  max1 = v1->data[255];
  c = max1 / max0;
  for (i = 0; i < 100; i++) {
    b = v1->data[c] / v0->data[c];
    w = (max1 - v1->data[c]) / (max0 -v0->data[c]);
    c1 = (b + w + 1) / 2;
    if (c1 == c) break;
    c = c1;
  }
  //printf("%f %f\n", b, w);

  uint x1, x2;
  real y1, y2, m, q, mb, qb, mw, qw, yb, yw;
  for (j = 0; j < 9; j++) {
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
    for (i = 0; i < 256; i++) {
      x1 = i;
      if (v0->data[i] <= y1) continue;
      break;
    }
    for (i = x1; i < 256; i++) {
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
    for (i = 255; i >= 0 ; i--) {
      x2 = i;
      if (v0->data[i] >= y2) continue;
      break;
    }
    for (i = x2; i >=0; i--) {
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

void shearx_image(image *im, real t) {
  // t = tan(angle * M_PI / 180);
  assert(fabs(t) <= 1);
  uint h = im->height;
  uint w = im->width;
  uint y;
  int di;
  real dr, df, ca, cb, cc, cd;;
  short *end, *p, *a, *b;
  short *buf = malloc(w * sizeof(*buf));
  for (y = 0; y < h; y++) {
    memcpy(buf, im->data + (w * y), w * sizeof(*buf));
    dr = ((int)y - (int)h/2) * t;
    di = floor(dr);
    df = dr - di;
    if (di > 0) {
      cb = df; ca = 1 - cb;
      p = im->data + (w * y);
      end = p + w;
      b = buf + di;
      a = b - 1;
      for (; p + di < end; p++, a++, b++) *p = cb * *b + ca * *a;
      for (; p < end; p++) *p = MAXVAL;
    } else {
      cb = df; ca = 1 - cb;
      p = im->data + (w * y) + w - 1;
      end = p - w;
      b = buf + w - 1 + di;
      a = b - 1;
      for (; p + di - 1 > end; p--, a--, b--) *p = cb * *b + ca * *a;
      for (; p > end; p--) *p = MAXVAL;
    }
  }
  free(buf);
}

void sheary_image(image *im, real t) {
  // t = tan(angle * M_PI / 180);
  assert(fabs(t) <= 1);
  uint w = im->width;
  uint h = im->height;
  short *p, *end;
  short *buf = malloc(w * sizeof(*buf));
  int d, *di = malloc(w * sizeof(*di));
  real f, dr, *df = malloc(w * sizeof(*df));
  int a, b, x, y;
  for (x = 0; x < w; x++) {
    dr = ((int)w/2 - (int)x) * t;
    di[x] = floor(dr) * w;
    df[x] = dr - floor(dr);
  }
  end = im->data + (w * h);
  // down
  if (t > 0) {a = 0; b = w/2;}
  else  {a = w/2; b = w;}
  for (y = 0; y < h; y++) {
    p = im->data + (y * w) + a;
    for (x = a; x < b; x++, p++) {
      d = di[x];
      f = df[x];
      if (p + d + w < end) {
        buf[x] = (1-f) * *(p + d) + f * *(p + d + w);
      } else {
        buf[x] = MAXVAL;
      }
    }
    memcpy(
      im->data + y * w + a,
      buf + a ,
      (b - a) * sizeof(*buf)
    );
  }
  if (t > 0) {a = w/2; b = w;}
  else  {a = 0; b = w/2;}
  for (y = h - 1; y >= 0; y--) { // y MUST be signed!
    p = im->data + (y * w) + a;
    for (x = a; x < b; x++, p++) {
      d = di[x];
      f = df[x];
      if (p + d + w >= end) {
        assert(d == 0);
        buf[x] = *p;
      }
      else
      if (p + d >= im->data) {
        buf[x] = (1-f) * *(p + d) + f * *(p + d + w);
      }
      else {
        buf[x] = MAXVAL;
      }
    }
    memcpy(
      im->data + y * w + a,
      buf + a ,
      (b - a) * sizeof(*buf)
    );
  }
  free(buf);
  free(di);
  free(df);
}

void skew(image* im, real angle) {
  angle *= M_PI /180;
  if (fabs(angle) > 45) error("skew: angle must be between -45 and 45.");
  real b = sin(angle);
  real a = b / (1 + cos(angle));
  shearx_image(im, a);
  sheary_image(im, b);
  shearx_image(im, a);
}

void mean_y(image *im, uint d) {
  uint w = im->width;
  uint h = im->height;
  real *v = calloc(w * (d + 1), sizeof(*v));
  real *r1, *r, *rd;
  int y, i;
  short *p, *q, *end;
  for (y = 0; y < h; y++) {
    i = (y+1) % (d+1);
    rd = v + w * i;
    i = (i+d) % (d+1);
    r = v + w * i;
    i = (i+d) % (d+1);
    r1 = v + w * i;
    p = im->data + y * w;
    i = y - d/2;
    if (y >= d) q = im->data + i * w;
    else q = 0;
    end = p + w;
    for (; p < end; p++, r1++, r++, rd++) {
      *r = *r1 + *p;
      if (q) *(q++) = (*r - *rd) / d;
    }
  }
  free(v);
}

real detect_skew(image *im) {
  uint d, i, y, w1, w = im->width;
  uint h = im->height;
  // create test image
  d = im->ex;
  if (d <= 0) error("detect_skew: im->ex <= 0");
  w1 = floor(w / d);
  image *test = make_image(w1, h);
  real dx, dy, n, t;
  short *p, *q, *end;
  for (y = 0; y < h; y++) {
    p = test->data + y * w1;
    end = p + w1;
    q = im->data + y * w;
    for (; p < end; p++) {
      t = 0;
      for (i = 0; i < d; i++, q++) t += *q;
      *p = t / d;
    }
  }
  mean_y(test, d);
  for (y = d; y < h; y++) {
    p = test->data + y * w1;
    q = p - d * w1;
    end = p + w1;
    for (; p < end; p++, q++) {
      *q -= *p - MAXVAL/2;
    }
  }
  // calc skew
  n = t = 0;
  for (y = 1; y < h - d; y++) {
    q = test->data + y * w1;
    p = q - w1;
    end = q + w1 - 1;
    for (; q < end; p++, q++) {
      dy = *p + *(p + 1) - *q - *(q + 1);
      dx = *(p + 1) + *(q + 1) - *p - *q;
      if ( fabs(dy) < 4 *
        fabs(*p + *(q + 1) - *q - *(p + 1))
      ) {continue;}
      if (fabs(dx) > fabs(dy) * d) {continue;}
      if (dy > 0) {t -= dx; n += dy;}
      else {t += dx; n -= dy;}
    }
  }
  if (n == 0) error("detect_skew: can't detect skew :-(\n"); //)
  t /= n * d;
  return atan(t) * 180 / M_PI;
}

image *crop(image *im, int x1, int y1, int x2, int y2) {
  int w = im->width;
  int h = im->height;
  int w1 = x2 - x1 + 1;
  int h1 = y2 - y1 + 1;
  int i;
  short *s, *t;
  if (x1 < 0 || x2 <= x1 || x2 >= w) error("cropx: wrong x parameters\n");
  if (y1 < 0 || y2 <= y1 || y2 >= h) error("cropx: wrong y parameters\n");
  image *out = make_image(w1, h1);
  out->ex = im->ex;
  out->pag = im->pag;
  for (i = 0; i <= h1; i++) {
    s = im->data + (i + y1) * w + x1; 
    t = out->data + i * w1;
    memcpy(t, s, w1 * sizeof(*s));
  }
  return out;
}

int find_margin(vector *v, int w) {
  real t, t1, n1;
  real *p = v->data;
  int i, j, a, b;
  int l = v->len;
  vector *v1;
  if (w < 0 || w > l) error("find_margin: invalid width");
  t = 0;
  // -> logarithmic scale
  for (i = 0; i < l; i++) {
    p[i] = log(p[i] + 1);
    t += p[i];
  }
  // threshold:
  t /= l;
  n1 = t1 = 0;
  for (i = 0; i < l; i++) {
    if (p[i] <= t) {n1++; t1 += p[i];}
  }
  t = (t + t1/n1) / 2;
  // calc scores for margins
  // (> l: forbidden)
  j = 0;
  for (i = 0; i < l; i++) {
    if (p[i] > t) {j = 0; p[i] = l + 1;}
    else {p[i] = ++ j;}
  }
  j = 0;
  for (i = l - 1; i >=0; i--) {
    if (p[i] > l) j = 0;
    else {p[i] -= ++ j;}
  }
  t = -l;
  for (i = 0; i + w + 1 < l; i++) {
    a = p[i];
    if (a > l) continue;
    b = p[i + w + 1];
    if (b > l) continue;
    p[i] = a -= b;
    if (a > t) {t = a; j = i;}
  }
  i = j; while (p[i] == t) i++;
  return (j + i) / 2;
}

image *autocrop(image *im, int width, int height) {
  int w = im->width;
  int h = im->height;
  vector *vx = make_vector(w); // x-histogram
  vx->len = vx->size;
  vector *vy = make_vector(h); // y-histogram
  vy->len = vy->size;
  int i, x1, x2, y1, y2;
  short *p, *end;
  real *px, *py, t, t1;
  int k = (MAXVAL + 1) /2;

  // calc vx
  for (i = 0; i < h - 1; i++, py++) {
    p = im->data + i * w;
    end = p + w - 1;
    px = vx->data;
    for (; p < end; p++, px++) {
      if (*p / k != *(p + w) / k) (*px)++;
    }
  }

  x1 = find_margin(vx, width);
  x2 = x1 + width - 1;

  // calc vy
  py = vy->data;
  for (i = 0; i < h - 1; i++, py++) {
    p = im->data + i * w + x1;
    end = p + width - 1;
    for (; p < end; p++, px++) {
      if (*p / k != *(p + 1) / k) (*py)++;
    }
  }

  y1 = find_margin(vy, height);
  y2 = y1 + height - 1;

  destroy_vector(vx);
  destroy_vector(vy);

  return crop(im, x1, y1, x2, y2);
}

void draw_grid(image *im, int step) {
  int sbig = step * 10;
  int step4 = step / 4;
  int step2 = step / 2;
  int w = im->width;
  int h = im->height;
  int x, y;
  short *p;

  for (y = 0; y < h; y++) {
    p = im->data + y * w;
    for(x = 0; x < w; x++, p++) {
      if (
        y % sbig == 0 ||
        x % sbig == 0 ||
        (x % step == 0 && (y + step4) % step < step2) ||
        (y % step == 0 && (x + step4) % step < step2)
      ) *p = 0;

    }
  }
}

// vim: sw=2 ts=2 sts=2 expandtab:
