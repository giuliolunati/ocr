#include "common.h"

image *n_laplacian(image *im) {
  int depth= im->depth;
  if (depth != 1) error("laplacian: invalid depth");
  // negative laplacian
  int w= im->width, h= im->height;
  int x, y= 0;
  image *om= make_image(w, h, im->depth);
  om->ex= im->ex;
  om->pag= im->pag;
  short *i, *iu, *id, *il, *ir, *o, *end;
  real v;
  
  i= im->channel[0];
  end= i + (w * h);
  o= om->channel[0];
  iu= i - w;
  il= i - 1; ir= i + 1;
  id= i + w;
  // y = 0 
  for (x=0; x<w; x++,i++,iu++,id++,il++,ir++,o++) {
    if (x == 0 || x == w - 1) {
      *o= 0;
    } else *o= (real) 2 * *i - *il - *ir;
  }
  for (y=1; y < h-1; y++) { 
    for (x=0; x<w; x++,i++,iu++,id++,il++,ir++,o++) {
      if (x == 0 || x == w - 1) {
        *o= (real) 2 * *i - *iu - *id;
      } else *o= (real) 4 * *i - *iu - *id - *il - *ir;
    }
  }
  // y = h-1
  for (x=0; x<w; x++,i++,iu++,id++,il++,ir++,o++) {
    if (x == 0 || x == w - 1) {
      *o= 0;
    } else *o= (real) 2 * *i - *il - *ir;
  }
  return om;
}

double poisson_step(image *im, image *nlap) {
  int depth= im->depth;
  int w= im->width;
  int h= im->height;
  short *data= im->channel[0];
  short *nl= nlap->channel[0];
  int x, y;
  short *p, *pl;
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

  short *data= im->channel[0];
  short *nl= nlap->channel[0];

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
  short *data= im->channel[0];
  short *nl= nlap->channel[0];

  if (depth != 1) error("poisson_image: invalid depth");
  if (nlap->depth != depth) error("poisson_image: depth mismatch");
  if (nlap->width != w || nlap->height != h)
  { error("poisson_image: size mismatch"); }

  poisson_border(im, nlap);
  poisson_inner(im, nlap);
}


// vim: sw=2 ts=2 sts=2 expandtab:
