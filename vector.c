#include "common.h"

//// VECTORS ////

vector *make_vector(uint size) {
  vector *v;;
  if (! (v= malloc(sizeof(vector))))
    error("Can't alloc vector.");
  v->size= size;
  if ( ! (
    v->data= calloc(size, sizeof(*(v->data)))
  )) error("Can't alloc vector data.");
  v->len= 0;
  v->x0= 0.0;
  v->dx= 1.0;
  v->magic= 'V';
  return v;
}

void clear_vector(vector *v) {
  memset(v->data, 0, v->size * sizeof(*(v->data))); 
}

void destroy_vector(vector *v) {
  if (! v) return;
  if (v->data) free(v->data);
  free(v);
}

vector *copy_vector(vector *v0) {
  vector *v= make_vector(v0->size);
  v->len= v0->len;
  v->x0= v0->x0;
  v->dx= v0->dx;
  memcpy(v->data, v0->data, v->len * sizeof(*(v->data)));
  return v;
}

void import_vector(vector *v, short *data, int len, int step) {
  int i;
  real *pv;
  short *pd;

  assert(v->size >= len);
  v->len= len;
  for (
    i= 0, pv= v->data, pd= data;
    i < len;
    i++, pv++, pd += step
  ) { *pv= *pd; }
}

void export_vector(vector *v, short *data, int len, int step) {
  int i;
  real *pv;
  short *pd;
  assert(v->len == len);
  for (
    i= 0, pv= v->data, pd= data;
    i < len;
    i++, pv++, pd += step
  ) { *pd= *pv; }
}

void write_vector(vector *v, FILE *f) {
  uint i;
  real *p= v->data;
  real x= v->x0;
  for (i= 0; i < v->len; i++, x += v->dx) {
    fprintf(f, "%f %f\n", x, p[i]);
  }
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

