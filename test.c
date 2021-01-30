#include "common.h"


int main(int argc, char **args) {
  vector *v= make_vector(0);
  real d[10]= {1, 0, 1, -1, 2, 0, 1, 0, 1};
  v->data= d;
  v->len= 9;
  v->size= 9;
  vector_convolution_3(v, 5, 2, 1);
  write_vector(v, stdout);
}
// vim: sw=2 ts=2 sts=2 expandtab:
