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
  // srgb < 0.04045: lin= srgb / 12.92
  // srgb > 0.04045: lin= [(srgb + 0.055) / 1.055]^2.4

short lin_from_srgb[256];
uchar srgb_from_lin[MAXVAL + 1];

void init_srgb() {
  int i, l0, s;
  real l;
  for (s= 0; s <= 255; s++) {
    l= (s + 0.5) / 255.5;
    if (l < 0.04045) l /= 12.92;
    else {
      l= (l + 0.055) / 1.055;
      l= pow(l, 2.4);
    }
    l= roundf(l * MAXVAL);
    lin_from_srgb[s]= l;
    if (s == 0) { l0= 0; continue; }
    for (i= l0; i <= (l0 + l) / 2; i++) {
      srgb_from_lin[i]= s - 1;
    }
    for (; i <= l; i++) {
      srgb_from_lin[i]= s;
    }
    l0= l;
  }
}

//// VECTORS ////

vector *make_vector(uint size) {
  vector *v;;
  if (! (v= malloc(sizeof(vector))))
    error("Can't alloc vector.");
  v->size= size;
  if ( ! (
    v->data= calloc(size, sizeof(v->data))
  )) error("Can't alloc vector data.");
  v->len= 0;
  v->x0= 0.0;
  v->dx= 1.0;
  v->type= 'V';
  return v;
}

void clear_vector(vector *v) {
  memset(v->data, 0, v->size * sizeof(v->data)); 
}

void destroy_vector(vector *h) {
  if (! h) return;
  if (h->data) free(h->data);
  free(h);
}

vector *copy_vector(vector *v0) {
  vector *v= make_vector(v0->size);
  v->len= v0->len;
  v->x0= v0->x0;
  v->dx= v0->dx;
  memcpy(v->data, v0->data, v->len * sizeof(*(v->data)));
  return v;
}

void cumul_vector(vector *v) {
  uint i;
  real *p= v->data;
  for (i= 1; i < v->len; i++) {
    p[i] += p[i - 1];
  }
}

void diff_vector(vector *v, uint d) {
  uint i;
  real *p= v->data;
  for (i= v->len - 1; i >= d; i--) {
    p[i] -= p[i - d];
  }
}

void write_vector(vector *v, FILE *f) {
  uint i;
  real *p= v->data;
  real x= v->x0;
  for (i= 0; i < v->len; i++, x += v->dx) {
    fprintf(f, "%f %f\n", x, p[i]);
  }
}

uint index_of_max(vector *v) {
  uint i, j= 0;
  real *p= v->data;
  real m= p[0];
  for (p++, i= 1; i < v->len; i++, p++) {
    if (*p > m) {m= *p; j= i;}
  }
  return j;
}

//// IMAGES ////

real default_ex= 25;

image *make_image(int width, int height) {
  image *im;
  short *data;
  if (height < 1 || width < 1) return NULL;
  data= calloc(width * height, sizeof(short));
  if (! data) return NULL;
  im= malloc(sizeof(image));
  if (! im) return NULL;
  im->type= 'I';
  im->data= data;
  im->width= width;
  im->height= height;
  im->ex= default_ex;
  im->pag= 0;
  im->black= im->gray= im->white= -1;
  im->area= im->thickness= -1;
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
  if (1 > fscanf(file, "%d ", &width)) {
    error("Not a PNM file - wrong width.");
  }
  if (1 > fscanf(file, "%d ", &height)) {
    error("Not a PNM file - wrong height.");
  }
  if (1 > fscanf(file, "%d", &type)) {
    error("Not a PNM file - wrong type.");
  }
  if (1 > fscanf(file, "%c", &c) || 
    (c != ' ' && c != '\t' && c != '\n')
  ) {
    error("Not a PNM file - no w/space after type");
  }
  switch (depth) {
    case 5: depth= 1; break;
    case 6: depth= 3; break;
    default: error("Invalid depth.");
  }
  switch (type) {
    case 255: break;
    default: error("Invalid bits.");
  }
  if (width < 1 || height < 1) error("Invalid dimensions.");
  im= make_image(width, height);
  if (!im) error("Cannot make image.");;
  buf= malloc(width * depth);
  pt= im->data;
  for (y= 0; y < height; y++) {
    if (width > fread(buf, depth, width, file)) {
      error("Unexpected EOF");
    }
    ps= buf + layer;
    for (x= 0; x < width; x++, pt++, ps += depth) {
      // *pt= *ps * K + K_2; // linear
      *pt= lin_from_srgb[*ps]; // sRGB
    }
  }
  free(buf);
  return im;
}

void write_image(image *im, FILE *file) {
  int x, y;
  uchar *buf, *pt;
  short v, *ps;
  assert(file);
  fprintf(file, "P5\n%d %d\n255\n", im->width, im->height);
  buf= malloc(im->width * sizeof(*buf));
  assert(buf);
  ps= im->data;
  assert(ps);
  for (y= 0; y < im->height; y++) {
    pt= buf;
    for (x= 0; x < im->width; x++, pt++, ps++) {
      v= *(ps);
      if (v < 0) v= 0;
      else
      if (v > MAXVAL) v= MAXVAL;
      // *pt= v / K; // linear
      *pt= srgb_from_lin[v]; // sRGB
    }
    if (im->width > fwrite(buf, 1, im->width, file)) error("Error writing file.");
  }
  free(buf);
}

image *rotate_90_image(image *im, int angle) {
  int w= im->width, h= im->height;
  int x, y, dx, dy;
  image *om= make_image(h, w);
  om->ex= im->ex;
  om->pag= im->pag;
  short *i, *o;
  i= im->data; o= om->data;
  switch ((int)angle) {
    case 90:
    case -270:
      o += h - 1; dx= h; dy= -1;
      break;
    case 180:
    case -180:
      om->width= w; om->height= h;
      o += h * w - 1; dx= -1; dy= -w;
      break;
    case 270:
    case -90:
      o += h * w - h; dx= -h; dy= 1;
      break;
    default: assert(0);
  }
  for (y= 0; y < h; y++) {
    for (x= 0; x < w; x++) {
      *o= *i; i++; o += dx;
    }
    o += dy - w * dx;
  }
  return om;
}

image *rotate_image(image *im, float angle) {
  float r= angle / 90;
  int n= roundf(r);
  r= angle - (90 * n);
  n= (n % 4) * 90;
  return rotate_90_image(im, n);
}

void splitx_image(void **out1, void **out2, image *im, float x) {
  if (x <= 0) error("x must be > 0.");
  if (x == 1) error("x must be != 1.");
  if (x > 1) x= 1/x;
  short *p, *p1, *p2;
  uint y, h= im->height;
  uint w= im->width;
  uint w1= w * x;
  uint w2= w - w1;
  image* im1= make_image(w1, h);
  image* im2= make_image(w2, h);
  p= im->data;
  p1= im1->data;
  p2= im2->data;
  for (y= 0; y < h; y++, p += w, p1 += w1, p2 += w2) {
    memcpy(p1, p, w1 * sizeof(short));
    memcpy(p2, p + w1, w2 * sizeof(short));
  }
  im1->pag= im->pag;
  im2->pag= im->pag + 1;
  *out1= im1;
  *out2= im2;
}

void splity_image(void **out1, void **out2, image *im, float y) {
  if (y <= 0) error("y must be > 0.");
  if (y == 1) error("y must be != 1.");
  if (y > 1) y= 1/y;
  uint w= im->width;
  ulong l1, l2; 
  uint h1= im->height * y;
  uint h2= im->height - h1;
  image* im1= make_image(w, h1);
  image* im2= make_image(w, h2);
  l1= w * h1;
  l2= w * h2;
  memcpy(im1->data, im->data, l1 * sizeof(short));
  memcpy(im2->data, im->data + l1, l2 * sizeof(short));
  im1->pag= im->pag;
  im2->pag= im->pag + 1;
  *out1= im1;
  *out2= im2;
}

image *image_background(image *im) {
  // find light background
  real d= im->ex;
  if (d <= 0) d= default_ex;
  d= 0.333 / d;
  d= exp(-d);
  int x, y, h= im->height, w= im->width;
  image *om= make_image(w, h);
  om->ex= im->ex;
  real t, *v0, *v1;
  v0= malloc(w * sizeof(*v0));
  v1= malloc(w * sizeof(*v1));
  short *pi= im->data;
  short *po= om->data;
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
  assert(pi == im->data + w * h);
  assert(po == om->data + w * h);
  for (y= 1; y < h; y++) {
    po -= w;
    for (x= w - 1; x >= 0; x--) v1[x]= *(--po);
    for (x= 0; x < w; x++) {
      t= v0[x] * d;
      if (v1[x] < t) v1[x]= t;
    }
    for (x= 0; x < w; x++) {
      *(po++)= round(v1[x]);
    }
    memcpy(v0, v1, w * sizeof(*v0));
  }
  assert(po == om->data + w);
  free(v0); free(v1);
  return om;
}

void divide_image(image *a, image *b) {
  int h= a->height;
  int w= a->width;
  int i;
  short *pa, *pb;
  if (b->height != h || b->width != w) error("Dimensions mismatch.");
  pa= a->data; pb= b->data;
  for (i= 0; i < w * h; i++) {
    *pa= (real)*pa / *pb * MAXVAL;
    pa++; pb++;
  }
}

vector *histogram_of_image(image *im) {
  vector *hi= make_vector(256);
  hi->len= hi->size;
  short *p= im->data;
  real *d= hi->data;
  int x, y, h= im->height, w= im->width;
  for (y= 0; y < h; y++) {
    for (x= 0; x < w; x++) {
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

void contrast_image(image *im, real black, real white) {
  short *end= im->data + (im->width * im->height);
  short *p;
  black *= MAXVAL;
  white *= MAXVAL;
  if (white == black) {
    for (p= im->data; p < end; p++) {
      if (*p <= black) *p= 0;
      else *p= MAXVAL;
    }
    return;
  }
  real a= MAXVAL / (white - black) ;
  real b= -a * black;
  if (black < white) {
    for (p= im->data; p < end; p++) {
      if (*p <= black) *p= 0;
      else if (*p >= white) *p= MAXVAL;
      else *p= *p * a + b;
    }
    return;
  }
  else { // white < black
    for (p= im->data; p < end; p++) {
      if (*p >= black) *p= 0;
      else if (*p <= white) *p= MAXVAL;
      else *p= *p * a + b;
    }
    return;
  }
}

void normalize_image(image *im, real strength) {
  vector *d, *v= histogram_of_image(im);
  real *p= v->data;
  int i, b, w;
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

void shearx_image(image *im, real t) {
  // t= tan(angle * M_PI / 180);
  assert(fabs(t) <= 1);
  uint h= im->height;
  uint w= im->width;
  uint y;
  int di;
  real dr, df, ca, cb, cc, cd;;
  short *end, *p, *a, *b;
  short *buf= malloc(w * sizeof(*buf));
  for (y= 0; y < h; y++) {
    memcpy(buf, im->data + (w * y), w * sizeof(*buf));
    dr= ((int)y - (int)h/2) * t;
    di= floor(dr);
    df= dr - di;
    if (di > 0) {
      cb= df; ca= 1 - cb;
      p= im->data + (w * y);
      end= p + w;
      b= buf + di;
      a= b - 1;
      for (; p + di < end; p++, a++, b++) *p= cb * *b + ca * *a;
      for (; p < end; p++) *p= MAXVAL;
    } else {
      cb= df; ca= 1 - cb;
      p= im->data + (w * y) + w - 1;
      end= p - w;
      b= buf + w - 1 + di;
      a= b - 1;
      for (; p + di - 1 > end; p--, a--, b--) *p= cb * *b + ca * *a;
      for (; p > end; p--) *p= MAXVAL;
    }
  }
  free(buf);
}

void sheary_image(image *im, real t) {
  // t= tan(angle * M_PI / 180);
  assert(fabs(t) <= 1);
  uint w= im->width;
  uint h= im->height;
  short *p, *end;
  short *buf= malloc(w * sizeof(*buf));
  int d, *di= malloc(w * sizeof(*di));
  real f, dr, *df= malloc(w * sizeof(*df));
  int a, b, x, y;
  for (x= 0; x < w; x++) {
    dr= ((int)w/2 - (int)x) * t;
    di[x]= floor(dr) * w;
    df[x]= dr - floor(dr);
  }
  end= im->data + (w * h);
  // down
  if (t > 0) {a= 0; b= w/2;}
  else  {a= w/2; b= w;}
  for (y= 0; y < h; y++) {
    p= im->data + (y * w) + a;
    for (x= a; x < b; x++, p++) {
      d= di[x];
      f= df[x];
      if (p + d + w < end) {
        buf[x]= (1-f) * *(p + d) + f * *(p + d + w);
      } else {
        buf[x]= MAXVAL;
      }
    }
    memcpy(
      im->data + y * w + a,
      buf + a ,
      (b - a) * sizeof(*buf)
    );
  }
  if (t > 0) {a= w/2; b= w;}
  else  {a= 0; b= w/2;}
  for (y= h - 1; y >= 0; y--) { // y MUST be signed!
    p= im->data + (y * w) + a;
    for (x= a; x < b; x++, p++) {
      d= di[x];
      f= df[x];
      if (p + d + w >= end) {
        assert(d == 0);
        buf[x]= *p;
      }
      else
      if (p + d >= im->data) {
        buf[x]= (1-f) * *(p + d) + f * *(p + d + w);
      }
      else {
        buf[x]= MAXVAL;
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
  real b= sin(angle);
  real a= b / (1 + cos(angle));
  shearx_image(im, a);
  sheary_image(im, b);
  shearx_image(im, a);
}

void mean_y(image *im, uint d) {
  uint w= im->width;
  uint h= im->height;
  real *v= calloc(w * (d + 1), sizeof(*v));
  real *r1, *r, *rd;
  int y, i;
  short *p, *q, *end;
  for (y= 0; y < h; y++) {
    i= (y+1) % (d+1);
    rd= v + w * i;
    i= (i+d) % (d+1);
    r= v + w * i;
    i= (i+d) % (d+1);
    r1= v + w * i;
    p= im->data + y * w;
    i= y - d/2;
    if (y >= d) q= im->data + i * w;
    else q= 0;
    end= p + w;
    for (; p < end; p++, r1++, r++, rd++) {
      *r= *r1 + *p;
      if (q) *(q++)= (*r - *rd) / d;
    }
  }
  free(v);
}

real skew_score(int d, image *test, vector *v) {
  int i, j, x, y;
  int  w= test->width, h= test->height;
  real t, *p, *end;
  short *pt;
  clear_vector(v);
  for (y= 0; y < h; y++) {
    pt= test->data + y * w;
    x= 0;
    for (i= 0; i <= abs(d); i++) {
      if (d >= 0) {j= y + i;} else {j= y + w - i;} 
      for (; x < w * (i + 1) / (abs(d) + 1); x++, pt++) {if (*pt) {v->data[j] += abs(*pt);}}
    }
  }
  t= 0;
  for (p= v->data, end= p + v->size; p < end; p++) { t += *p * *p; }
  return t;
}

real detect_skew(image *im) {
  int i, y, w1, w= im->width;
  int h= im->height;
  real t, s= 0;
  // create test image
  image *test= make_image(w, h - 1);
  short *p1, *p2, *pt, *end;
  for (y= 0; y < h - 1; y++) {
    p1= im->data + y * w;
    end= p2= p1 + w;
    pt= test->data + y * w;
    for (; p1 < end; p1++, p2++, pt++) {
      t= abs(*p1 - *p2);
      *pt= t;
      s += t * t;
    }
  }
  s= sqrt(s / w / (h - 1));
  for (y= 0; y < h - 1; y++) {
    pt= test->data + y * w;
    end= pt + w;
    for (; pt < end; pt++) {
      if (*pt < s) {*pt= 0;}
    }
  }
  vector *v=  make_vector(h + w);
  int a, b;
  real sa, sb;
  a= w / 10; b= -a;
  sa= skew_score(a, test, v); sb= skew_score(b, test, v);
  while (abs(a - b) > 1) {
    if (sa > sb) {
      b= (5 * a - 2 * b) / 3;
      sb= skew_score(b, test, v);
    } else {
      a= (5 * b - 2 * a) / 3;
      sa= skew_score(a, test, v);
    }
  }
  if (sb > sa) {sa= sb; a= b;}
  if (a == 0) t= 0;
  if (a > 0) t= a + 1;
  if (a < 0) t= a - 1;
  return atan(t / w) * 180 / M_PI;
}

image *crop(image *im, int x1, int y1, int x2, int y2) {
  // 0 <= x1 < x2 <= im->width  
  // 0 <= y1 < y2 <= im->height  
  int w= im->width;
  int h= im->height;
  int w1= x2 - x1;
  int h1= y2 - y1;
  int i;
  short *s, *t;
  if (x1 < 0 || x2 <= x1 || x2 > w) error("crop: wrong x parameters\n");
  if (y1 < 0 || y2 <= y1 || y2 > h) error("crop: wrong y parameters\n");
  image *out= make_image(w1, h1);
  out->ex= im->ex;
  out->pag= im->pag;
  for (i= 0; i < h1; i++) {
    s= im->data + (i + y1) * w + x1; 
    t= out->data + i * w1;
    memcpy(t, s, w1 * sizeof(*s));
  }
  return out;
}

int find_margin(vector *v, int w) {
  real t, t1, n1;
  real *p= v->data;
  int i, j, a, b;
  int l= v->len;
  vector *v1;
  if (w < 0 || w > l) error("find_margin: invalid width");
  t= 0;
  // -> logarithmic scale
  for (i= 0; i < l; i++) {
    p[i]= log(p[i] + 1);
    t += p[i];
  }
  // threshold:
  t /= l;
  n1= t1= 0;
  for (i= 0; i < l; i++) {
    if (p[i] <= t) {n1++; t1 += p[i];}
  }
  t= (t + t1/n1) / 2;
  // calc scores for margins
  // (> l: forbidden)
  j= 0;
  for (i= 0; i < l; i++) {
    if (p[i] > t) {j= 0; p[i]= l + 1;}
    else {p[i]= ++ j;}
  }
  j= 0;
  for (i= l - 1; i >=0; i--) {
    if (p[i] > l) j= 0;
    else {p[i] -= ++ j;}
  }
  t= -l;
  for (i= 0; i + w + 1 < l; i++) {
    a= p[i];
    if (a > l) continue;
    b= p[i + w + 1];
    if (b > l) continue;
    p[i]= a -= b;
    if (a > t) {t= a; j= i;}
  }
  i= j; while (p[i] == t) i++;
  return (j + i) / 2;
}

image *autocrop(image *im, int width, int height) {
  int w= im->width;
  int h= im->height;
  vector *vx= make_vector(w); // x-histogram
  vx->len= vx->size;
  vector *vy= make_vector(h); // y-histogram
  vy->len= vy->size;
  int i, x1, x2, y1, y2;
  short *p, *end;
  real *px, *py, t, t1;
  int k= (MAXVAL + 1) /2;

  // calc vx
  for (i= 0; i < h - 1; i++, py++) {
    p= im->data + i * w;
    end= p + w - 1;
    px= vx->data;
    for (; p < end; p++, px++) {
      if (*p / k != *(p + w) / k) (*px)++;
    }
  }

  x1= find_margin(vx, width);
  x2= x1 + width - 1;

  // calc vy
  py= vy->data;
  for (i= 0; i < h - 1; i++, py++) {
    p= im->data + i * w + x1;
    end= p + width - 1;
    for (; p < end; p++, px++) {
      if (*p / k != *(p + 1) / k) (*py)++;
    }
  }

  y1= find_margin(vy, height);
  y2= y1 + height - 1;

  destroy_vector(vx);
  destroy_vector(vy);

  return crop(im, x1, y1, x2, y2);
}

void draw_grid(image *im, int stepx, int stepy) {
  int sbigx= stepx * 10;
  int step4x= stepx / 4;
  int step2x= stepx / 2;
  int sbigy= stepy * 10;
  int step4y= stepy / 4;
  int step2y= stepy / 2;
  int w= im->width;
  int h= im->height;
  int x, y;
  short *p;

  for (y= 0; y < h; y++) {
    p= im->data + y * w;
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

image *double_size(image *im, real k /*hardness*/) {
  int w= im->width, h= im->height;
  int x, y, dx, dy;
  image *om= make_image(2 * w, 2 * h);
  om->ex= 2 * im->ex;
  om->pag= im->pag;
  short *i1, *i2, *i3, *i4, *o;
  real v;
  real a= 9.0/16, b= 3.0/16, c= 1.0/16; 
  real a1= 8.0/15, b1= 2.0/15, c1= 3.0/15; 
  a= a * (1 - k) + a1 * k;
  b= b * (1 - k) + b1 * k;
  c= c * (1 - k) + c1 * k;

  for (y= 0; y < h; y++) {
    i4= i3= im->data + (w * y);
    if (y == 0) {i1= i3; i2= i4;}
    else {i1= i3 - w; i2= i4 - w;}
    o= om->data + (4 * w * y);
    for (x= 0; x < w; x++) {
      v= a * *i4
        + b * (*i3 + *i2)
        + c * *i1 ;
      *o= v; o++;
      if (x > 0) {i1++; i3++;}
      if (x < w - 1) {i2++; i4++;}
      v= a * *i3
        + b * (*i4 + *i1)
        + c * *i2 ;
      *o= v; o++;
    }
    i1= i2= im->data + (w * y);
    if (y == h - 1) {i3= i1; i4= i2;}
    else {i3= i1 + w; i4= i2 + w;}
    o= om->data + (4 * w * y) + (2 * w);
    for (x= 0; x < w; x++) {
      v= a * *i2
        + b * (*i1 + *i4)
        + c * *i3 ;
      *o= v; o++;
      if (x > 0) {i1++; i3++;}
      if (x < w - 1) {i2++; i4++;}
      v= a * *i1
        + b * (*i2 + *i3)
        + c * *i4 ;
      *o= v; o++;
    }
  }
  return om;
}

void darker_image(image *a, image *b) {
  int h= a->height;
  int w= a->width;
  int i;
  short *pa, *pb;
  if (b->height != h || b->width != w) error("Dimensions mismatch.");
  pa= a->data; pb= b->data;
  for (i= 0; i < w * h; i++) {
    if (*pa > *pb) { *pa= *pb; };
    pa++; pb++;
  }
}

void calc_statistics(image *im, int verbose) {
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
  real area, border, thickness, black, gray, white;
  int i, x, y, t;
  int h= im->height, w= im->width;
  short *pi= im->data;
  short *px= pi + 1;
  short *py= pi + w;
  short a, b, c;
  uint d;
  // histograms
  for (y= 0; y < h; y++) {
    for (x= 0; x < w; x++) {
      a= *pi / K; b= *px / K;
      pa[a]++;
      if ((x >= w - 1) || (y >= h - 1)) {continue;} 
      if (a > b) {c= b; b= a; a= c;}
      pb[a]++; pb[b]--;
      d= b - a; d *= d;
      pt[a] += d; pt[b] -= d;
      a= *pi / K; b= *py / K;
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
  gray= t / 255.0;
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
      black, gray, white, thickness, area 
  );}
  im->black= black;
  im->gray= gray;
  im->white= white;
  im->thickness= thickness;
  im->area= area;
}

image *half_size(image *im) {
  int w= im->width, h= im->height;
  int x, y, dx, dy;
  int w2= w / 2, h2= h / 2;
  image *om= make_image(w2, h2);
  om->ex= im->ex / 2;
  om->pag= im->pag;
  short *i1, *i2, *i3, *i4, *o;
  real v;

  for (y= 0; y < h; y += 2) {
    i1= im->data + (w * y); i2= i1 + 1;
    if (y == h - 1) {i3= i1; i4= i2;}
    else {i3= i1 + w; i4= i2 + w;}
    o= om->data + (w2 * y / 2);
    for (x= 0; x < w; x += 2) {
      v= (*i1 + *i2 + *i3 + *i4) / 4;
      *o= v; o++;
      i1 += 2; i3 += 2;
      if (x < w - 1) {i2 += 2; i4 += 2;}
    }
  }
  return om;
}

// vim: sw=2 ts=2 sts=2 expandtab:
