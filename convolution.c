#include "common.h"

void convolution_3x3(image *im, real a, real b, real c, real d, int border) {
  // border = 2 => border + corners
  // d c d
  // b a b  symmetric 3x3 kernel
  // d c d
  int depth= im->depth;
  int w= im->width, h= im->height, x, y;
  int len= w * sizeof(gray);

  gray *buf= (gray *) malloc(3 * len);
  if (! buf) error("convolution_3x3: out of memory.");
  memcpy(buf, im->channel[0], 2 * len);
  gray *i0, *i1, *i2, *o;
  if (border > 0) {
    o= im->channel[0];
    i0= buf; i1= i0 + 1; i2 = i1 + 1;
    if (border > 1) {
      *o= *i0 * (a + 2 * (b + c) + 4 * d);
    }
    for (x= 0; x < w-2; x++) {
      o++; 
      *o= (real) *i1 * (2 * c + a) +
        (*i0 + *i2) * (2 * d + b);
      i0++; i1++; i2++;
    }
    if (border > 1) {
      *(o+1)= *i2 * (a + 2 * (b + c) + 4 * d);
    }
  }
  o= im->channel[0] + w;
  for (y=1; y < h-1; y++) {
    i0= buf; i1= i0 + w; i2 = i1 + w;
    memcpy(i2, o + w, len);
    if (border > 0) {
      *o= *i1 * (2 * b + a) +
        (*i0 + *i2) * (2 * d + c);
    }
    for (x= 0; x < w-2; x++) {
      o++;
      *o= (real) a * *(i1+1) +
        b * ( (*i1) + *(i1+2) ) +
        c * ( *(i0+1) + *(i2+1) ) +
        d * ( *i0 + *i2 + *(i0+2) + *(i2+2) );
      i0++, i1++, i2++;
    }
    if (border > 0) {
      *(o+1)= *i1 * (2 * b + a) +
        (*i0 + *i2) * (2 * d + c);
    }
    o += 2;
    memmove(buf, buf + w, 2 * len);
  }
  if (border > 0) {
    i0= buf + 2*w; i1= i0 + 1; i2 = i1 + 1;
    if (border > 1) {
      *o= *i0 * (a + 2 * (b + c) + 4 * d);
    }
    for (x= 0; x < w-2; x++) {
      o++; 
      *o= (real) *i1 * (2 * c + a) +
        (*i0 + *i2) * (2 * d + b);
      i0++; i1++; i2++;
    }
    if (border > 1) {
      *(o+1)= *i2 * (a + 2 * (b + c) + 4 * d);
    }
  }
  free(buf);
}

void deconvolution_3x1(image *im, real a, real b, real c, int border) {
  // a b c
  if (border) border= 1;
  int i, y, n= im->width, h= im->height;
  real *th;
  real t= a + b + c;
  real *aa= malloc(n * sizeof(*aa));
  real *bb= malloc(n * sizeof(*bb));
  real *cc= malloc(n * sizeof(*cc));
  if (!aa || !bb || !cc) error("deconvolution_3x1: out of memory.");
  for (i= 0; i < n; i++) {
    aa[i]= a; bb[i]= b; cc[i]= c;
  }
  bb[0]= 1; cc[0]=0;
  bb[n-1]= 1; aa[n-1]=0;
  real p,q,r, *v= malloc(n * sizeof(*v));
  gray *s;
  th= solve_tridiagonal(aa, bb, cc, n);
  for (y=1-border ; y < h-1+border; y++) {
    s= im->channel[0] + n*y;
    for (i= 0; i < n; i++) {
      v[i]= s[i];
    }
    for (i= 0; i < n-1; i++) {
      p = sin(th[i]); q = cos(th[i]);
      r= p * v[i] + q * v[i+1];
      v[i] -= 2*r*p;
      v[i+1] -= 2*r*q;
    }
    for (i= n-1; i >= 0; i--) {
      if (i+2 < n) v[i] -= aa[i] * v[i+2];
      if (i+1 < n) v[i] -= cc[i] * v[i+1];
      assert(bb[i] != 0);
      v[i] /= bb[i];
    }
    for (i= 0; i < n; i++) {
      s[i]= v[i];
    }
  }
  free(aa); free(bb); free(cc);
  free(th); free(v);
}

void deconvolution_1x3(image *im, real a, real b, real c, int border) {
  // a b c
  if (border) border= 1;
  int i, x, w= im->width, n= im->height;
  gray *s;
  real *th;
  real p,q,r, *v= malloc(n * sizeof(*v));
  real t= a + b + c;
  real *aa= malloc(n * sizeof(*aa));
  real *bb= malloc(n * sizeof(*bb));
  real *cc= malloc(n * sizeof(*cc));
  if (!aa || !bb || !cc) error("deconvolution_1x3: out of memory.");
  for (i= 0; i < n; i++) {
    aa[i]= a; bb[i]= b; cc[i]= c;
  }
  bb[0]= 1; cc[0]=0;
  bb[n-1]= 1; aa[n-1]=0;
  th= solve_tridiagonal(aa, bb, cc, n);
  for (x= 1-border ; x < w-1+border; x++) {
    s= im->channel[0] + x;
    for (i= 0; i < n; i++) v[i]= s[i*w];
    for (i= 0; i < n-1; i++) {
      p = sin(th[i]); q = cos(th[i]);
      r= p * v[i] + q * v[i+1];
      v[i] -= 2*r*p;
      v[i+1] -= 2*r*q;
    }
    for (i= n-1; i >= 0; i--) {
      if (i+2 < n) v[i] -= aa[i] * v[i+2];
      if (i+1 < n) v[i] -= cc[i] * v[i+1];
      assert(bb[i] != 0);
      v[i] /= bb[i];
    }
    for (i= 0; i < n; i++) s[i*w]= v[i];
  }
  free(aa); free(bb); free(cc);
  free(th); free(v);
}

float deconvolution_3x3_step_old(
    image *im, image *om, int chan,
    real a, real b, real c, real d,
    int steps, float maxerr
  ) {
  int i, x, y, w= im->width, h= im->height;
  fprintf(stderr, "%dx%d \n", w,h);
  double t, err;
  gray *pi, *po;
  image *im2= NULL;
  // y = Ax
  // A = PQ-R
  // y = PQx - Rx
  // (PQ)^-1 (y+Rx) = x
  real p= sqrt(fabs(a)), q= a/p;
  real p1= b/p, q1= c/q;
  real r= p1*q1 - d;
  for (i=steps; i>0; i--) {
    pi= im->channel[0];
    po= om->channel[0];
    for (x= 0; x < w; x++,pi++,po++)
    { *po= *pi; }
    for (y= 1; y < h-1; y++) {
      *po= *pi; pi += w-1, po += w-1;
      *po= *pi; pi++, po++;
    }
    for (x= 0; x < w; x++,pi++,po++)
    { *po= *pi; }
    convolution_3x3(om, 0,0,0,r, 0);
    // om += im
    for (y= 1; y < h-1; y++) {
      pi= im->channel[0] + y*w + 1;
      po= om->channel[0] + y*w + 1;
      for (x= 1; x < w-1; x++,pi++,po++)
      { *po += *pi; }
    }
    deconvolution_3x1(om, p1, p, p1, 0);
    deconvolution_1x3(om, q1, q, q1, 0);
  }
  assert(! im2);
  im2= copy_image(om);
  convolution_3x3(im2, a,b,c,d, 0);
  pi= im2->channel[0];
  po= im->channel[0];
  err= 0;
  for (y= 0; y < h; y++)
  for (x= 0; x < w; x++,pi++,po++) {
    t= *po - *pi;
    err += t*t;
  }
  destroy_image(im2);
  err= sqrt(err / (w*h));
  if (err > 999) {
    fprintf(stderr, "p=%f p1=%f q=%f q2=%f r=%f \n", p,p1,q,q1,r);
    write_image(om, stdout, 0);
    error(".");
  }
  return err;
}

float deconvolution_3x3_step(
    image *im, image *om, int chan,
    real a, real b, real c, real d,
    int steps, float maxerr
  ) {
  //fprintf(stderr, "abcd= %f %f %f %f \n", a,b,c,d);
  int n, x, y, dx, w= im->width, h= im->height;
  float k= a*a / (a*a + 2*b*b + 2*c*c + 4*d*d);
  gray *po, *pi, *pu, *pd;
  double t, err;
  for (n=0; n!= steps-1; n++) {
    if (n%2 == 0) err= 0;
    for (y= 1; y < h-1; y++) {
      dx= (n+y)%2;
      po= om->channel[chan] + y*w + 1 + dx;
      pi= im->channel[chan] + y*w + 1 + dx;
      pu= po - w; pd= po + w;
      for (x= 1+dx ; x < w-1; x+=2,pi+=2,po+=2,pu+=2,pd+=2) {
        t= ( *pi
        - b * (*(po-1) + *(po+1))
        - c * (*pu + *pd)
        - d * (*(pu+1) + *(pu-1) + *(pd+1) + *(pd-1))
        ) / a;
        t -= *po;
        *po += t*1.618;
        err += t*t;
      } 
    }
    if (n%2 == 0) continue;
    err /= (w-2)*(h-2);
    err= sqrt(err);
    if (err < maxerr) break;
  }
  return err;
}
int ID=0;
image *deconvolution_3x3(image *im, real a, real b, real c, real d, int steps, float maxerr) {
  // D-E = a  b   = a b = F
  //       ak bk-e  c d
  // Y = FX = DX - EX
  // D\(Y + EX) = X
  assert(a != 0);
  float err;
  image *him, *hom, *om, *im2, *om2;
  int n,x,y,w= im->width, h= im->height;
  om= make_image(w, h, 1);
  gray *pi= im->channel[0];
  gray *po= om->channel[0];
  // border
  err= 0;
  pi= im->channel[0];
  po= om->channel[0];
  for (x= 0; x < w; x++,pi++,po++)
  { err += *po= *pi; }
  for (y= 1; y < h-1; y++) {
  err += *po= *pi; pi += w-1, po += w-1;
  err += *po= *pi; pi++, po++;
  }
  for (x= 0; x < w; x++,pi++,po++)
  { err += *po= *pi; }
  err /= 2*(w+h) - 4;
  for (y= 1; y < h-1; y++) {
    pi= im->channel[0] + y*w + 1;
    po= om->channel[0] + y*w + 1;
    for (x= 1; x < w-1; x++,pi++,po++) *po= err;
  }
  // inner
  int l= MAX(w,h);
  if (l > 64) for (n=3; n>0; n--) {
    deconvolution_3x3_step(
        im, om, 0,
        a, b, c, d,
        4, 0
    );
    im2= copy_image(om);
    convolution_3x3(im2, a,b,c,d, 0);
    pi= im->channel[0];
    po= im2->channel[0];
    for (y= 0; y < h; y++)
    for (x= 0; x < w; x++,pi++,po++) *po= *pi - *po;
    him= image_half(im2);
    hom= deconvolution_3x3(
      him,
      a*9/16 + b*3/4 + c*3/4 + d,
      a*3/32 + b*3/8 + c/8 + d/2,
      a*3/32 + b/8 + c*3/8 + d/2,
      a/64 + b/16 + c/16 + d/4,
      steps/2, maxerr*n
    );
    om2= image_double(hom, w%2, h%2);
    // om += om2
    for (y= 1; y < h-1; y++) {
      pi= om2->channel[0] + y*w + 1;
      po= om->channel[0] + y*w + 1;
      for (x= 1; x < w-1; x++,pi++,po++)
      { *po += *pi; }
    }
    //fix border
    pi= im->channel[0];
    po= om->channel[0];
    for (x= 0; x < w; x++,pi++,po++)
    { *po= *pi; }
    for (y= 1; y < h-1; y++) {
    *po= *pi; pi += w-1, po += w-1;
    *po= *pi; pi++, po++;
    }
    for (x= 0; x < w; x++,pi++,po++)
    { *po= *pi; }

    destroy_image(him);
    destroy_image(hom);
    destroy_image(im2);
    destroy_image(om2);
  }
  if (MAX(w,h) <= -64) {
    FILE *f;
    f= fopen("im.pnm", "w");
    write_image(im, f, 0);
    fclose(f);
    f= fopen("om.pnm", "w");
    write_image(om, f, 0);
    fclose(f);
    exit(0);
  }
  err= deconvolution_3x3_step(
      im, om, 0,
      a, b, c, d,
      steps, maxerr
  );
  fprintf(stderr, "w=%d err=%f\n", w, err);
  return om;
}

double poisson_step(image *im, image *nlap) {
  int depth= im->depth;
  int w= im->width;
  int h= im->height;
  gray *data= im->channel[0];
  gray *nl= nlap->channel[0];
  int x, y;
  gray *p, *pl;
  double t=0, v;
  assert(depth == 1);
  assert(nlap->depth == depth && nlap->width == w && nlap->height == h);
  assert(w >= 3 && h >= 3);

  // odd step //
  for (y= 1; y < h-1; y++) {
    for (
      x= 2 - y%2, p= data + y*w + x, pl= nl + y*w + x;
      x < w-1;
      x += 2, p += 2, pl += 2
    ) {
      *p= (real)(*pl + *(p+1) + *(p-1) + *(p+w) + *(p-w)) / 4.0;
    }
  }

  // even step //
  for (y= 1; y < h-1; y++) {
    for (
      x= 1 + y%2, p= data + y*w + x, pl= nl + y*w + x;
      x < w-1;
      x += 2, p += 2, pl += 2
    ) {
      v= (real)(*pl + *(p+1) + *(p-1) + *(p+w) + *(p-w)) / 4.0;
      *p += v= (v - *p) / 2;
      t= MAX(t, fabs(v));
    }
  }

  return t;
}

void poisson_border(image *im, image *nlap) {
  int depth= im->depth;
  int w= im->width;
  int h= im->height;
  assert(depth == 1 && nlap->width == w || nlap->height == h);

  gray *data= im->channel[0];
  gray *nl= nlap->channel[0];

  // border //
  vector *v= make_vector(MAX(w, h));
  vector *l= make_vector(MAX(w, h));
  // top
  import_vector(v, data, w, 1);
  import_vector(l, nl + 1, w-2, 1);
  poisson_vector(v, l);
  export_vector(v, data, w, 1);
  // left
  import_vector(v, data, h, w);
  import_vector(l, nl + w, h-2, w);
  poisson_vector(v, l);
  export_vector(v, data, h, w);
  // bottom
  import_vector(v, data+w*(h-1), w, 1);
  import_vector(l, nl+w*(h-1)+1, w-2, 1);
  poisson_vector(v, l);
  export_vector(v, data+w*(h-1), w, 1);
  //right
  import_vector(v, data + w-1, h, w);
  import_vector(l, nl + w-1+w, h-2, w);
  poisson_vector(v, l);
  export_vector(v, data + w-1, h, w);

  destroy_vector(v); destroy_vector(l);
}

void poisson_inner(image *im, image *nlap) {
  // inner
  int i;
  double err;
  for (i=1; i<=100; i++) {
    err= poisson_step(im, nlap);
    printf("err: %f\n", err);
  }
}

void poisson_image(image *im, image *nlap) {
  int depth= im->depth;
  int w= im->width;
  int h= im->height;
  gray *data= im->channel[0];
  gray *nl= nlap->channel[0];

  if (depth != 1) error("poisson_image: invalid depth");
  if (nlap->depth != depth) error("poisson_image: depth mismatch");
  if (nlap->width != w || nlap->height != h)
  { error("poisson_image: size mismatch"); }

  poisson_border(im, nlap);
  poisson_inner(im, nlap);
}


// vim: sw=2 ts=2 sts=2 expandtab:
