#include "common.h"

void laplacian(image *im, real k) {
  int w= im->width, h= im->height;
  int x, y, z;
  int len= w * sizeof(gray);
  gray *buf= (gray *) malloc(3 * len);
  if (! buf) error("image_laplace: out of memory.");
  for (z= 1; z < 4; z++) {
    if (! im->chan[z]) continue;
    gray *i0, *i1, *i2, *o;
    o= im->chan[z];
    memcpy(buf, o, 2 * len);
    // y=0
    *o= 128;
    o++;
    i1= buf+1;
    for (x=1; x < w-1; x++,i1++,o++) {
      *o= 128 + (real) k * (
        *(i1-1) - *i1*2 + *(i1+1)
      );
    }
    *o= 128;
    //
    for (y=1; y < h-1; y++) {
      i0= buf; i1= i0 + w; i2 = i1 + w;
      o= im->chan[z] + w*y;
      memcpy(i2, o + w, len);
      *o= 128 + (real) k * (*i0 - *i1*2 + *i2);
      i0++, i1++, i2++, o++;
      for (x=1; x < w-1; x++,i0++,i1++,i2++,o++) {
        *o= 128 + (real) k * (
          *(i1-1) + *(i1+1) + *i0 + *i2 - *i1 * 4
        );
      }
      *o= 128 + (real) k * (*i0 - *i1*2 + *i2);
      o++;
      memmove(buf, buf + w, 2 * len);
    }
    // y=h-1
    o= im->chan[z]+w*(h-1);
    *o= 128;
    o++;
    i1= buf+2*w+1;
    for (x=1; x < w-1; x++,i1++,o++) {
      *o= 128 + (real) k * (
        *(i1-1) - *i1*2 + *(i1+1)
      );
    }
    *o= 128;
  }
  free(buf);
}

float image_poisson_step(
    // laplacian(guess) =~ target
    image *target, image *guess,
    real k,
    int steps, float maxerr
  ) {
  int z, n, x, y, dx;
  int w= target->width, h= target->height;
  assert(guess->width == w);
  assert(guess->height == h);
  gray *mask, *pg, *pt, *pm;
  mask= target->ALPHA;
  double t, err1, err= 0;
  unsigned long int count= 0;
  for (z= 1; z < 4; z++) {
    if (! target->chan[z]) continue;
    for (n=0; n!= steps-1; n++) {
      fprintf(stderr, "%c", n%10+48);
      if (n%16 == 0) {
        n++; // preserve parity 
        err1= count= 0;
        pg= guess->chan[z] + 1;
        pt= target->chan[z] + 1;
        pm= mask + 1;
        for (x= 1 ; x < w-1; x++,pt++,pg++,pm++) {
          if (mask && *pm < 1) continue;
          t= (
            *(pg-1) + *(pg+1) - (*pt-128)/k
          ) / 2 - *pg;
          err1 += t*t;
          count ++;
        }
        for (y= 1; y < h-1; y++) {
          pg= guess->chan[z] + y*w;
          pt= target->chan[z] + y*w;
          pm= mask + y*w;
          if (!mask || *pm >= 1) {
            t= (
              *(pg-w) + *(pg+w) - (*pt-128)/k
            ) / 2 - *pg;
            err1 += t*t;
            count ++;
          }
          pg++; pt++; pm++;
          for (x= 1 ; x < w-1; x++,pt++,pg++,pm++) {
            if (mask && *pm < 1) continue;
            t= (
              *(pg-1) + *(pg+1) + *(pg-w) + *(pg+w) - (*pt-128)/k
            ) / 4 - *pg;
            err1 += t*t;
            count ++;
          }
          if (!mask || *pm >= 1) {
            t= (
              *(pg-w) + *(pg+w) - (*pt-128)/k
            ) / 2 - *pg;
            err1 += t*t;
            count ++;
          }
        }
        pg= guess->chan[z] + w*(h-1) + 1;
        pt= target->chan[z] + w*(h-1) + 1;
        pm= mask + w*(h-1) + 1;
        for (x= 1; x < w-1; x++,pt++,pg++,pm++) {
          if (mask && *pm < 1) continue;
          t= (
            *(pg-1) + *(pg+1) - (*pt-128)/k
          ) / 2 - *pg;
          err1 += t*t;
          count ++;
        }
        fprintf(stderr, "");
        if (count == 0) error("===");
        err1 /= count;
        err1= sqrt(err1);
        if (err1 <= maxerr) break;
      } else {
        dx= n%2;
        pg= guess->chan[z] + 1+dx;
        pt= target->chan[z] + 1+dx;
        pm= mask + 1+dx;
        for (x= 1+dx ; x < w-1; x+=2,pt+=2,pg+=2,pm+=2) {
          if (mask && *pm < 1) continue;
          *pg= (
            *(pg-1) + *(pg+1) - (*pt-128)/k
          ) / 2;
        }
        for (y= 1; y < h-1; y++) {
          dx= (n+y)%2;
          pg= guess->chan[z] + y*w;
          pt= target->chan[z] + y*w;
          pm= mask + y*w;
          if (dx && (!mask || *pm >= 1)) {
            *pg= (
              *(pg-w) + *(pg+w) - (*pt-128)/k
            ) / 2;
          }
          pg += 1+dx; pt += 1+dx; pm += 1+dx;
          for (x= 1+dx ; x < w-1; x+=2,pt+=2,pg+=2,pm+=2) {
            if (mask && *pm < 1) continue;
            *pg= (
              *(pg-1) + *(pg+1) + *(pg-w) + *(pg+w) - (*pt-128)/k
            ) / 4;
          }
          if (x == w-1 && (!mask || *pm >= 1)) {
            *pg= (
              *(pg-w) + *(pg+w) - (*pt-128)/k
            ) / 2;
          }
        }
        dx= (n+h-1)%2;
        pg= guess->chan[z] + w*(h-1) + 1+dx;
        pt= target->chan[z] + w*(h-1) + 1+dx;
        pm= mask + w*(h-1) + 1+dx;
        for (x= 1+dx ; x < w-1; x+=2,pt+=2,pg+=2,pm+=2) {
          if (mask && *pm < 1) continue;
          *pg= (
            *(pg-1) + *(pg+1) - (*pt-128)/k
          ) / 2;
        }
        fprintf(stderr, "");
      }
    }
    err= MAX(err, err1);
  }
  return err;
}

void solve_poisson(image *guess, image *target, real k, int steps, float maxerr) {
  image *ta1, *gu1, *ta2, *gu2;
  int z, n, x, y;
  assert(guess);
  int w= target->width, h= target->height;
  long int i, l= w*h;
  if (guess->width != w || guess->height != h)
  { error("solve_poisson: size mismatch."); }
  gray *mask, *pt, *pg, *pm;
  mask= target->ALPHA;
  real err, mean;
  // inner
  float recur= log2(MAX(w,h)/8.0);
  if (recur > 1) for (n= 2; n>0; n--) {
    fprintf(stderr, "%d", n);
    image_poisson_step(
        target, guess,
        k,
        8, 0
    );
    ta1= image_copy(guess);
    laplacian(ta1, k);
    for (z= 1; z < 4; z++) {
      if (! target->chan[z]) continue;
      pt= target->chan[z];
      pg= ta1->chan[z];
      if (mask) {
        pm= mask;
        for (i= 0; i < l; i++,pt++,pg++,pm++) {
          if (*pm >= 1) *pg= *pt - *pg + 128;
          else *pg= 128;
        }
      } else {
        for (i= 0; i < l; i++,pt++,pg++) *pg= *pt - *pg + 128;
      }
    }
    if (ta1->ALPHA) free(ta1->ALPHA);
    ta1->ALPHA= target->ALPHA;
    ta2= image_half(ta1);
    ta1->ALPHA= NULL;
    gu2= image_clone(
        guess, 0, (w + 2 - w%2)/2, (h + 2 - h%2)/2
    );
    fill_selection(gu2, NAN, 128, 128, 128);
    solve_poisson(
        gu2,
        ta2,
        k/4,
        steps*4,
        n * maxerr * sqrt((recur-1)/recur)
    );
    gu1= image_redouble(gu2, w%2, h%2);
    for (z= 1; z < 4; z++) {
      if (! target->chan[z]) continue;
      // guess += gu1
      pt= gu1->chan[z];
      pg= guess->chan[z];
      *pt= *(pt+w-1)= *(pt+w*(h-1))= *(pt+w*h-1)= 128;
      if (mask) {
        pm= mask;
        for (i= 0; i < l; i++,pt++,pg++,pm++) {
          if (*pm >= 1) *pg += *pt - 128;
        }
      } else {
        for (i= 0; i < l; i++,pt++,pg++) *pg += *pt - 128;
      }
    }
    destroy_image(ta2);
    destroy_image(gu2);
    destroy_image(ta1);
    destroy_image(gu1);
    fprintf(stderr, " ");
  }
  err= image_poisson_step(
      target, guess,
      k,
      steps, maxerr
  );
}

// vim: sw=2 ts=2 sts=2 expandtab:
