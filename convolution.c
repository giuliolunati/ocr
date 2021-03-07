#include "common.h"

void convolve_3x3(image *im, real a, real b, real c, real d) {
  // d c d
  // b a b  symmetric 3x3 kernel
  // d c d
  int depth= im->depth;
  int w= im->width, h= im->height;
  int z, x, y;
  int len= w * sizeof(gray);

  gray *buf= (gray *) malloc(3 * len);
  if (! buf) error("convolve_3x3: out of memory.");
  for (z= 0; z < depth; z ++) {
    memcpy(buf, im->channel[z], 2 * len);
    gray *i0, *i1, *i2, *o;
    o= im->channel[z] + w;
    for (y=1; y < h-1; y++) {
      i0= buf; i1= i0 + w; i2 = i1 + w;
      memcpy(i2, o + w, len);
      for (x= 0; x < w-2; x++) {
        o++;
        *o= (real) a * *(i1+1) +
          b * ( (*i1) + *(i1+2) ) +
          c * ( *(i0+1) + *(i2+1) ) +
          d * ( *i0 + *i2 + *(i0+2) + *(i2+2) );
        i0++, i1++, i2++;
      }
      o += 2;
      memmove(buf, buf + w, 2 * len);
    }
  }
  free(buf);
}

void deconvolve_3x1(image *im, real a, real b, real c, int border) {
  // a b c
  if (border) border= 1;
  int i, y, n= im->width, h= im->height;
  real *th;
  real t= a + b + c;
  real *aa= malloc(n * sizeof(*aa));
  real *bb= malloc(n * sizeof(*bb));
  real *cc= malloc(n * sizeof(*cc));
  if (!aa || !bb || !cc) error("deconvolve_3x1: out of memory.");
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

void deconvolve_1x3(image *im, real a, real b, real c, int border) {
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
  if (!aa || !bb || !cc) error("deconvolve_1x3: out of memory.");
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

float deconvolve_3x3_step_old(
    image *im, image *om, int z,
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
    convolve_3x3(om, 0,0,0,r);
    // om += im
    for (y= 1; y < h-1; y++) {
      pi= im->channel[0] + y*w + 1;
      po= om->channel[0] + y*w + 1;
      for (x= 1; x < w-1; x++,pi++,po++)
      { *po += *pi; }
    }
    deconvolve_3x1(om, p1, p, p1, 0);
    deconvolve_1x3(om, q1, q, q1, 0);
  }
  assert(! im2);
  im2= copy_image(om);
  convolve_3x3(im2, a,b,c,d);
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

float deconvolve_3x3_step(
    image *im, image *om,
    real a, real b, real c, real d,
    int steps, float maxerr
  ) {
  int z, n, x, y, dx;
  int depth= im->depth, w= im->width, h= im->height;
  float k= a*a / (a*a + 2*b*b + 2*c*c + 4*d*d);
  gray *po, *pi, *pu, *pd;
  double t, err1, err= 0;
  for (z= 0; z < depth; z ++) {
    for (n=0; n!= steps-1; n++) {
      fprintf(stderr, "%c", n%10+48);
      if (n%16 < 2) {
        if (n%16 == 0) err1= 0;
        for (y= 1; y < h-1; y++) {
          dx= (n+y)%2;
          po= om->channel[z] + y*w + 1 + dx;
          pi= im->channel[z] + y*w + 1 + dx;
          pu= po - w; pd= po + w;
          for (x= 1+dx ; x < w-1; x+=2,pi+=2,po+=2,pu+=2,pd+=2) {
            t= ( *pi
            - b * (*(po-1) + *(po+1))
            - c * (*pu + *pd)
            - d * (*(pu+1) + *(pu-1) + *(pd+1) + *(pd-1))
            ) / a - *po;
            err1 += t*t;
          }
        }
        fprintf(stderr, "");
        if (n%16 == 1) {
          err1 /= (w-2) * (h-2);
          err1= sqrt(err1);
          if (err1 <= maxerr) break;
        }
      } else {
        for (y= 1; y < h-1; y++) {
          dx= (n+y)%2;
          po= om->channel[z] + y*w + 1 + dx;
          pi= im->channel[z] + y*w + 1 + dx;
          pu= po - w; pd= po + w;
          for (x= 1+dx ; x < w-1; x+=2,pi+=2,po+=2,pu+=2,pd+=2) {
            t= ( *pi
            - b * (*(po-1) + *(po+1))
            - c * (*pu + *pd)
            - d * (*(pu+1) + *(pu-1) + *(pd+1) + *(pd-1))
            ) / a;
            t -= *po;
            *po += t*k;
            err1 += t*t;
          } 
        }
        fprintf(stderr, "");
      }
    }
    err= MAX(err, err1);
  }
  return err;
}

image *deconvolve_3x3(image *im, real a, real b, real c, real d, int steps, float maxerr) {
  image *him, *hom, *om, *im2, *om2;
  int z, n, x, y;
  int depth= im->depth, w= im->width, h= im->height;
  om= make_image(w, h, depth);
  gray *pi, *po;
  real err, mean;
  // border
  for (z= 0; z < depth; z++) {
    mean= 0;
    pi= im->channel[z];
    po= om->channel[z];
    for (x= 0; x < w; x++,pi++,po++)
    { mean += *po= *pi; }
    for (y= 1; y < h-1; y++) {
      mean += *po= *pi; pi += w-1, po += w-1;
      mean += *po= *pi; pi++, po++;
    }
    for (x= 0; x < w; x++,pi++,po++)
    { mean += *po= *pi; }
    mean /= 2*(w+h) - 4;
    for (y= 1; y < h-1; y++) {
      pi= im->channel[z] + y*w + 1;
      po= om->channel[z] + y*w + 1;
      for (x= 1; x < w-1; x++,pi++,po++) {
        *po= mean;
      }
    }
  }
  // inner
  int l= MAX(w,h);
  if (l > 64) for (n= 3; n>0; n--) {
    fprintf(stderr, "%d", n);
    deconvolve_3x3_step(
        im, om,
        a, b, c, d,
        7, 0
    );
    im2= copy_image(om);
    convolve_3x3(im2, a,b,c,d);
    for (z= 0; z < depth; z++) {
      pi= im->channel[z];
      po= im2->channel[z];
      for (y= 0; y < h; y++)
      for (x= 0; x < w; x++,pi++,po++) *po= *pi - *po;
    }
    him= image_half(im2);
    hom= deconvolve_3x3(
        him,
        a*9/16 + b*3/4 + c*3/4 + d,
        a*3/32 + b*3/8 + c/8 + d/2,
        a*3/32 + b/8 + c*3/8 + d/2,
        a/64 + b/16 + c/16 + d/4,
        steps/2, maxerr*n*0.5
    );
    om2= image_redouble(hom, w%2, h%2);
    for (z= 0; z < depth; z++) {
      // om += om2
      for (y= 1; y < h-1; y++) {
        pi= om2->channel[z] + y*w + 1;
        po= om->channel[z] + y*w + 1;
        for (x= 1; x < w-1; x++,pi++,po++) *po += *pi;
      }
      //fix border
      pi= im->channel[z];
      po= om->channel[z];
      for (x= 0; x < w; x++,pi++,po++) *po= *pi;
      for (y= 1; y < h-1; y++) {
        *po= *pi; pi += w-1, po += w-1;
        *po= *pi; pi++, po++;
      }
      for (x= 0; x < w; x++,pi++,po++) *po= *pi;
    }
    destroy_image(him);
    destroy_image(hom);
    destroy_image(im2);
    destroy_image(om2);
    fprintf(stderr, " ");
  }
  err= deconvolve_3x3_step(
      im, om,
      a, b, c, d,
      steps, maxerr
  );
  return om;
}

void image_laplace(image *im, real k) {
  // d c d
  // b a b  symmetric 3x3 kernel
  // d c d
  int depth= im->depth;
  int w= im->width, h= im->height;
  int z, x, y;
  int len= w * sizeof(gray);
  gray *buf= (gray *) malloc(3 * len);
  if (! buf) error("convolve_3x3: out of memory.");
  for (z= 0; z < depth; z ++) {
    memcpy(buf, im->channel[z], 2 * len);
    gray *i0, *i1, *i2, *o;
    o= im->channel[z] + w;
    for (y=1; y < h-1; y++) {
      i0= buf; i1= i0 + w; i2 = i1 + w;
      memcpy(i2, o + w, len);
      for (x= 0; x < w-2; x++) {
        o++;
        *o= (real) k * (
          + (*i1) + *(i1+2) + *(i0+1) + *(i2+1)
          -4 * *(i1+1)
        );
        i0++, i1++, i2++;
      }
      o += 2;
      memmove(buf, buf + w, 2 * len);
    }
  }
  free(buf);
}

float image_poisson_step(
    image *im, image *om,
    real k,
    int steps, float maxerr
  ) {
  int z, n, x, y, dx;
  int depth= im->depth, w= im->width, h= im->height;
  gray *po, *pi, *pu, *pd;
  double t, err1, err= 0;
  for (z= 0; z < depth; z ++) {
    for (n=0; n!= steps-1; n++) {
      fprintf(stderr, "%c", n%10+48);
      if (n%16 < 2) {
        if (n%16 == 0) err1= 0;
        for (y= 1; y < h-1; y++) {
          dx= (n+y)%2;
          po= om->channel[z] + y*w + 1 + dx;
          pi= im->channel[z] + y*w + 1 + dx;
          pu= po - w; pd= po + w;
          for (x= 1+dx ; x < w-1; x+=2,pi+=2,po+=2,pu+=2,pd+=2) {
            t= ( *pi/k
              - *(po-1) - *(po+1) - *pu - *pd
            ) / -4 - *po;
            err1 += t*t;
          }
        }
        fprintf(stderr, "");
        if (n%16 == 1) {
          err1 /= (w-2) * (h-2);
          err1= sqrt(err1);
          if (err1 <= maxerr) break;
        }
      } else {
        for (y= 1; y < h-1; y++) {
          dx= (n+y)%2;
          po= om->channel[z] + y*w + 1 + dx;
          pi= im->channel[z] + y*w + 1 + dx;
          pu= po - w; pd= po + w;
          for (x= 1+dx ; x < w-1; x+=2,pi+=2,po+=2,pu+=2,pd+=2) {
            t= ( *pi/k
              - *(po-1) - *(po+1) - *pu - *pd
            ) / -4;
            t -= *po;
            *po += t;
            err1 += t*t;
          } 
        }
        fprintf(stderr, "");
      }
    }
    err= MAX(err, err1);
  }
  return err;
}

image *image_poisson(image *im, real k, int steps, float maxerr) {
  image *him, *hom, *om, *im2, *om2;
  int z, n, x, y;
  int depth= im->depth, w= im->width, h= im->height;
  om= make_image(w, h, depth);
  gray *pi, *po;
  real err, mean;
  // border
  for (z= 0; z < depth; z++) {
    mean= 0;
    pi= im->channel[z];
    po= om->channel[z];
    for (x= 0; x < w; x++,pi++,po++)
    { mean += *po= *pi; }
    for (y= 1; y < h-1; y++) {
      mean += *po= *pi; pi += w-1, po += w-1;
      mean += *po= *pi; pi++, po++;
    }
    for (x= 0; x < w; x++,pi++,po++)
    { mean += *po= *pi; }
    mean /= 2*(w+h) - 4;
    for (y= 1; y < h-1; y++) {
      pi= im->channel[z] + y*w + 1;
      po= om->channel[z] + y*w + 1;
      for (x= 1; x < w-1; x++,pi++,po++) {
        *po= mean;
      }
    }
  }
  // inner
  float recur= log2(MAX(w,h)/8.0);
  if (recur > 1) for (n= 2; n>0; n--) {
    fprintf(stderr, "%d", n);
    image_poisson_step(
        im, om,
        k,
        8, 0
    );
    im2= copy_image(om);
    image_laplace(im2, k);
    for (z= 0; z < depth; z++) {
      pi= im->channel[z];
      po= im2->channel[z];
      for (y= 0; y < h; y++)
      for (x= 0; x < w; x++,pi++,po++) *po= *pi - *po;
    }
    him= image_half(im2);
    hom= image_poisson(
        him,
        k/4,
        steps*4,
        n * maxerr * sqrt((recur-1)/recur)
    );
    om2= image_redouble(hom, w%2, h%2);
    for (z= 0; z < depth; z++) {
      // om += om2
      for (y= 1; y < h-1; y++) {
        pi= om2->channel[z] + y*w + 1;
        po= om->channel[z] + y*w + 1;
        for (x= 1; x < w-1; x++,pi++,po++) *po += *pi;
      }
      //fix border
      pi= im->channel[z];
      po= om->channel[z];
      for (x= 0; x < w; x++,pi++,po++) *po= *pi;
      for (y= 1; y < h-1; y++) {
        *po= *pi; pi += w-1, po += w-1;
        *po= *pi; pi++, po++;
      }
      for (x= 0; x < w; x++,pi++,po++) *po= *pi;
    }
    destroy_image(him);
    destroy_image(hom);
    destroy_image(im2);
    destroy_image(om2);
    fprintf(stderr, " ");
  }
  err= image_poisson_step(
      im, om,
      k,
      steps, maxerr
  );
  return om;
}

// vim: sw=2 ts=2 sts=2 expandtab:
