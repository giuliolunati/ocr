#include "common.h"

//// VECTORS ////

vector *make_vector(uint size) {
  vector *v;;
  if (! (v= malloc(sizeof(vector))))
    error("Can't alloc vector.");
  v->size= size;
  if (size) {
    if ( ! (
      v->data= calloc(size, sizeof(*(v->data)))
    )) error("Can't alloc vector data.");
  } else v->data= NULL;
  v->step= 1;
  v->len= 0;
  v->magic= 'V';
  return v;
}

void clear_vector(vector *v) {
  memset(v->data, 0, v->size * sizeof(*(v->data))); 
}

void destroy_vector(vector *v) {
  if (! v) return;
  if (v->size && v->data) free(v->data);
  free(v);
}

vector *copy_vector(vector *v0) {
  vector *v= make_vector(v0->size);
  v->len= v0->len;
  memcpy(v->data, v0->data, v->len * sizeof(*(v->data)));
  return v;
}

void import_vector(vector *v, short *data, int len, int step) {
  int i, d= v->step;
  real *pv;
  short *pd;

  assert(v->size >= len);
  v->len= len;
  for (
    i= 0, pv= v->data, pd= data;
    i < len;
    i++, pv += d, pd += step
  ) { *pv= *pd; }
}

void export_vector(vector *v, short *data, int len, int step) {
  int i, d= v->step;
  real *pv;
  short *pd;
  assert(v->len == len);
  for (
    i= 0, pv= v->data, pd= data;
    i < len;
    i++, pv += d, pd += step
  ) { *pd= *pv; }
}

void write_vector(vector *v, FILE *f) {
  uint i, d= v->step;
  real *p= v->data;
  for (i= 0; i < v->len; i++, p += d) {
    fprintf(f, "%d %f\n", i, *p);
  }
}

void cumul_vector(vector *v) {
  uint i, s= v->step;
  real *p= v->data;
  for (i= 0; i < v->len - 1; i++, p+=s) {
    *(p+s) += *p;
  }
}

void diff_vector(vector *v, uint d) {
  uint i, s= v->step;
  real *p= v->data + s*(v->len - 1);
  for (i= v->len - 1; i >= d; i--, p-=s) {
    *p -= *(p-s);
  }
}

void vector_convolution_3(vector *v, real a, real b, real c, int border) {
  int i, s= v->step, len= v->len;
  real x1, x2, x3;
  real *o= v->data;
  real *end= o + v->len - 1;
  x2= *o; x3= *(o+1);
  if (border) {
    *o *= a + b + c;
  }
  for (i=0; i<len-2; i++) {
    o+=s;
    x1= x2; x2= x3; x3= *(o+s);
    *o= a * x1 + b * x2 + c * x3;
  }
  if (border) {
    *(o+s) *= a + b + c;
  }
}

real *solve_tridiagonal(real *aa, real *bb, real *cc, int n) {
  /*
  b0 c0          b0 c0 a0   
  a1 b1 c1    =>    b1 c1 a1
     a2 b2 c2          b2 c2
        a3 b3             b3
  USAGE:
  th= solve_tridiagonal(aa, bb, cc, n);
  for (i= 0; i < n-1; i++) {
    p = sin(th[i]); q = cos(th[i]);
    r= p*y[i] + q*y[i+1];
    y[i] -= 2*r*p;
    y[i+1] -= 2*r*q;
  }
  for (i= n-1; i >= 0; i--) {
    if (i+2 < n) y[i] -= aa[i] * y[i+2];
    if (i+1 < n) y[i] -= cc[i] * y[i+1];
    assert(bb[i] != 0);
    y[i] /= bb[i];
  }
  */
  int i;
  real p, q, r, *t= malloc((n-1) * sizeof(*t));
  cc[n-1]= 0;
  for (i= 0; i < n-1; i++) {
    p= bb[i]; q= aa[i+1];
    if (q == 0) r= 0;
    else {
      r= hypot(p,q);
      bb[i]= r; aa[i+1]= 0;
      p= r - p; q= -q;
      r= atan2(p,q);
    }
    t[i]= r;
    p= sin(r); q= cos(r);
    // |(p,q)| = 1
    // (a,b) -> (a,b) - 2<(a,b).(p,q)>(p,q)
    r= p * cc[i] + q * bb[i+1];
    cc[i] -= 2*r*p;
    bb[i+1] -= 2*r*q;
    if (i >= n-2) continue;
    r= q*cc[i+1];
    aa[i]= -2*r*p;
    cc[i+1] -= 2*r*q;
  }
  return t;
}

void vector_deconvolution_3(vector *v, real a, real b, real c, int border) {
  int i, n= v->len;
  real *aa= malloc(n * sizeof(*aa));
  real *bb= malloc(n * sizeof(*bb));
  real *cc= malloc(n * sizeof(*cc));
  real *th;
  if (!aa || !bb || !cc) error("vector_deconvolution_3: out of memory.");
  for (i= 0; i < n; i++) {
    aa[i]= a; bb[i]= b; cc[i]= c;
  }
  real *y= v->data;
  real t= a + b + c;
  if (border == -1) {
    if (t == 0) error("vector_deconvolution_3: a+b+c = 0 && border");
    bb[0]= t; cc[0]=0;
    bb[n-1]= t; aa[n-1]=0;
  } else if (border == 1) {
    bb[0]= a + b;
    bb[n-1]= b + c;
  } else {
    bb[0]= 1; cc[0]=0;
    bb[n-1]= 1; aa[n-1]=0;
  }

  real p,q,r;
  th= solve_tridiagonal(aa, bb, cc, n);
  for (i= 0; i < n-1; i++) {
    p = sin(th[i]); q = cos(th[i]);
    r= p*y[i] + q*y[i+1];
    y[i] -= 2*r*p;
    y[i+1] -= 2*r*q;
  }
  for (i= n-1; i >= 0; i--) {
    if (i+2 < n) y[i] -= aa[i] * y[i+2];
    if (i+1 < n) y[i] -= cc[i] * y[i+1];
    assert(bb[i] != 0);
    y[i] /= bb[i];
  }
  
  free(aa); free(bb); free(cc); free(th);
}

void poisson_vector(vector *target, vector *nlap) {
  int len= target->len;
  if (nlap->len != len - 2) error("poisson_vector: len mismatch");
  real t;
  real *d= target->data;
  real *l= nlap->data;
  int i;
  cumul_vector(nlap);
  l[0] -= d[0];
  cumul_vector(nlap);
  t= d[len-1];
  d[1]= d[0];
  for (i=2; i<len; i++) { d[i]= -l[i-2]; }
  t= (t - d[len-1]) / (len-1);
  for (i=1; i<len; i++) { d[i] += i * t; }
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

// vim: sw=2 ts=2 sts=2 expandtab:
