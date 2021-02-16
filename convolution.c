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
    for (i= 0; i < n; i++) v[i]= s[i];
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
    for (i= 0; i < n; i++) s[i]= v[i];
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


float deconvolution_3x3_step(
    image *im, image *om, int chan,
    real a, real b, real c, real d,
    int steps, float maxerr
) {
  int n, x, y, dx, w= im->width, h= im->height;
  float k= a*a;
  k /= k + 4*d*d + 2*b*b + 2*c*c;
  fprintf(stderr, "k=%f\n", k);
  gray *pi, *po, *pu, *pd;
  float del, err;
  for (n=0; n<steps; n++) {
    if (n%2 == 0) err= 0;
    for (y= 1; y < h-1; y++) {
      dx= (n+y)%2;
      pi= im->channel[chan] + y*w + 1 + dx;
      po= om->channel[chan] + y*w + 1 + dx;
      pu= po - w; pd= po + w;
      for (x= 1+dx ; x < w-1; x+=2,pi+=2,po+=2,pu+=2,pd+=2) {
        del= ((float) *pi
        - b * (*(po-1) + *(po+1))
        - c * (*pu + *pd)
        - d * (*(pu+1) + *(pu-1) + *(pd+1) + *(pd-1))
        ) / a - *po;
        *po += k*del;
        del= a * fabs(del);
        if (del > err) err= del;
      } 
    }
    if (err < maxerr) break;
  }
  return err;
}

image *deconvolution_3x3(image *im, real a, real b, real c, real d, int steps) {
  // D-E = a  b   = a b = F
  //       ak bk-e  c d
  // Y = FX = DX - EX
  // D\(Y + EX) = X
int border= 0;
  assert(a != 0);
  int n,x,y,w= im->width, h= im->height;
fprintf(stderr, "%dx%d \n", w,h);
  int dx;
  image *him, *hom, *om, *im2, *om2;
  vector *v= make_vector(MAX(w,h));
  gray *pi= im->channel[0];
  gray *po, *pu, *pd;
  float mean, err;
  om= make_image(w, h, 1);
  po= om->channel[0];
  // deconvolve border
  // top
assert(border==0);
  mean= 0;
  import_vector(v,pi,w,1);
  if (border) vector_deconvolution_3(v, 2*d+b, 2*c+a, 2*d+b, 0);
  export_vector(v,po,w,1);
  // left
  import_vector(v,pi,h,w);
  if (border) vector_deconvolution_3(v, 2*d+c, 2*b+a, 2*d+c, 0);
  export_vector(v,po,h,w);
  // bottom
  import_vector(v,pi+h*w-w,w,1);
  if (border) vector_deconvolution_3(v, 2*d+b, 2*c+a, 2*d+b, 0);
  export_vector(v,po+h*w-w,w,1);
  // right
  import_vector(v,pi+w-1,h,w);
  if (border) vector_deconvolution_3(v, 2*d+c, 2*b+a, 2*d+c, 0);
  export_vector(v,po+w-1,h,w);
  // inner
  mean= 0;
  po= om->channel[0]+1;
  for (x= 1; x < w-1; x++, po++) mean += *po;
  po= om->channel[0]+w;
  for (y= 1; y < h-1; y++, po+=w)
  { mean += *(po) + *(po+w-1); }
  po= om->channel[0]+ w*h-w+1;
  for (x= 1; x < w-1; x++, po++) mean += *po;
  mean /= 2*(w+h-4);
  po= om->channel[0]+w+1;
  for (y=1; y<h-1; y++, po+=2)
  { for(x=1; x<w-1; x++, po++) *po= mean; }
  
  fprintf(stderr, "mean=%f\n", mean);
  if (MAX(w,h) > 32) {
    deconvolution_3x3_step(
        im, om, 0,
        a, b, c, d,
        4, 0.5
    );
    im2= copy_image(om);
    convolution_3x3(im2, a, b, c, d, 0);
    po= im2->channel[0];
    pi= im->channel[0];
    for (x=w; x>0; x--) for (y=h; y>0; y--)
    { *po= *pi - *po; po++, pi++;}
    if (w > h) {
      him= image_half_x(im2);
      hom= deconvolution_3x3(
          him, b+a*3/4 , b/2+a/8, d+c*3/4, d/2+c/8, steps * 2
      );
      om2= image_double_x(hom, w%2);
    } else { // h >= w
      him= image_half_y(im2);
      hom= deconvolution_3x3(
          him, c+a*3/4 , d+b*3/4, c/2+a/8, d/2+b/8, steps * 2
      );
      om2= image_double_y(hom, h%2);
    }
    err= deconvolution_3x3_step(
        im2, om2, 0,
        a, b, c, d,
        steps, 0.5
    );
    fprintf(stderr, "err=%f\n", err);
    destroy_image(him);
    destroy_image(hom);
    destroy_image(im2);
    po= om->channel[0];
    pi= om2->channel[0];
    for (x=w; x>0; x--) for (y=h; y>0; y--)
    { *po += *pi; po++; pi++;}
    destroy_image(om2);
  }
    err= deconvolution_3x3_step(
        im, om, 0,
        a, b, c, d,
        steps, 0.5
    );
  fprintf(stderr, "* err=%f\n", err);
  destroy_vector(v);
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
