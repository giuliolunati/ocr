#include "common.h"

image *double_size(image *im, real k /*hardness*/) {
  int depth= im->depth;
  int w= im->width, h= im->height;
  int x, y, z;
  image *om= make_image(2 * w, 2 * h, im->depth);
  om->ex= 2 * im->ex;
  om->pag= im->pag;
  gray *i1, *i2, *i3, *i4, *o;
  real v;
  real a= 9.0/16, b= 3.0/16, c= 1.0/16; 
  real a1= 8.0/15, b1= 2.0/15, c1= 3.0/15; 
  a= a * (1 - k) + a1 * k;
  b= b * (1 - k) + b1 * k;
  c= c * (1 - k) + c1 * k;
  for (z= 0; z < depth; z ++) {
    for (y= 0; y < h; y++) {
      i4= i3= im->channel[z] + (w * y);
      if (y == 0) {i1= i3; i2= i4;}
      else {i1= i3 - w; i2= i4 - w;}
      o= om->channel[z] + (4 * w * y);
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
      i1= i2= im->channel[z] + (w * y);
      if (y == h - 1) {i3= i1; i4= i2;}
      else {i3= i1 + w; i4= i2 + w;}
      o= om->channel[z] + (4 * w * y) + (2 * w);
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
  }
  return om;
}

image *half_size(image *im) {
  int depth= im->depth;
  int w= im->width, h= im->height;
  int x, y, z;
  int w2= w / 2, h2= h / 2;
  image *om= make_image(w2, h2, im->depth);
  om->ex= im->ex / 2;
  om->pag= im->pag;
  gray *i1, *i2, *i3, *i4, *o;
  real v;
  for (z= 0; z < depth; z ++) {
    for (y= 0; y < h; y += 2) {
      i1= im->channel[z] + (w * y); i2= i1 + 1;
      if (y == h - 1) {i3= i1; i4= i2;}
      else {i3= i1 + w; i4= i2 + w;}
      o= om->channel[z] + (w2 * y / 2);
      for (x= 0; x < w; x += 2) {
        v= (*i1 + *i2 + *i3 + *i4) / 4;
        *o= v; o++;
        i1 += 2; i3 += 2;
        if (x < w - 1) {i2 += 2; i4 += 2;}
      }
    }
  }
  return om;
}

image *image_half_x(image *im) {
  int depth= im->depth;
  int wi= im->width;
  int h= im->height;
  image *om;
  int wo, x, y, z;
  gray *pi, *po;
  wo= (wi - wi%1) / 2; 
  om= make_image(wo, h, depth);
  for (z= 0; z < depth; z ++) {
    if (wi % 2) { // odd
      for (y= 0; y < h; y ++) {
        pi= im->channel[z] + (y * wi);
        po= om->channel[z] + (y * wo);
        for (x=0; x < wo; x++,po++,pi+=2) {
          *po= (int)(*pi + *(pi+1)*2 + *(pi+2)) / 4;
        }
      }
    } else { // even
      for (y= 0; y < h; y ++) {
        pi= im->channel[z] + (y * wi);
        po= om->channel[z] + (y * wo);
        for (x=0; x < wo; x++,po++,pi+=2) {
          *po= (int)(*pi + *(pi+1)) / 2;
        }
      }
    }
  }
  return om;
}

image *image_half_y(image *im) {
  int depth= im->depth;
  int w= im->width;
  int hi= im->height;
  image *om;
  int ho, x, y, z;
  gray *pi, *po;
  ho= (hi - hi%2) / 2; 
  om= make_image(w, ho, depth);
  for (z= 0; z < depth; z ++) {
    if (hi % 2) { // odd
      for (y= 0; y < ho; y ++) {
        po= om->channel[z] + w * y;
        pi= im->channel[z] + w * (y*2 +1);
        for (x=0; x < w; x++, po++, pi++) {
          *po= (int)(*(pi-w) + (*pi)*2 + *(pi+w)) / 4;
        }
      }
    } else { // even
      for (y= 0; y < ho; y ++) {
        po= om->channel[z] + w * y;
        pi= im->channel[z] + w * (y*2 +1);
        for (x=0; x < w; x++, po++, pi++) {
          *po= (int)(*(pi-w) + (*pi)) / 2;
        }
      }
    }
  }
  return om;
}

image *image_half(image *im) {
  image *t= image_half_x(im);
  im= image_half_y(t);
  destroy_image(t);
  return im;
}

image *image_double_x(image *im, int odd) {
  int depth= im->depth;
  if (odd) odd= 1;
  int wi= im->width;
  int h= im->height;
  image *om;
  int wo, x, y, z;
  gray *pi, *po;
  wo= wi*2 + odd; 
  om= make_image(wo, h, depth);
  for (z= 0; z < depth; z ++) {
    pi= im->channel[z];
    po= om->channel[z];
    if (odd) {
      // i:   0   1   2   3
      // o: 0 1 2 3 4 5 6 7 8
      for (y= 0; y < h; y ++) {
        *po= *pi; po++;
        // i=0, o=1
        for (x=0; x<wi-1; x++) {
          *po= *pi; po++;
          *po= (int)(*pi + *(pi+1)) / 2;
          po++; pi++;
        }
        *po= *pi; po++;
        *po= *pi;
        po++; pi++;
      }
    } else { // even
      // i:  0   1   2   3
      // o: 0 1 2 3 4 5 6 7
      for (y= 0; y < h; y++) {
        *po= *pi; po++;
        // i=0, o=1
        for (x=0; x < wi-1; x++) {
          *po= (int)(*pi * 3 + *(pi+1)) / 4;
          po++;
          *po= (int)(*pi + *(pi+1) * 3) / 4;
          po++; pi++;
        }
        *po= *pi;
        po++; pi++;
      }
    }
  }
  return om;
}

image *image_double_y(image *im, int odd) {
  int depth= im->depth;
  if (odd) odd= 1;
  int w= im->width;
  int hi= im->height;
  image *om;
  int ho, x, i, z;
  gray *pi, *po;
  ho= hi*2 +odd;
  om= make_image(w, ho, depth);
  for (z= 0; z < depth; z ++) {
    po= om->channel[z];
    pi= im->channel[z];
    // i= o= 0
    memcpy(po, pi, w * sizeof(*po));
    po += w;
    // i=0, o=1
    if (odd) {
      // i:   0   1   2   3
      // o: 0 1 2 3 4 5 6 7 8
      for (i=0; i<hi-1; i++) {
        memcpy(po, pi, w * sizeof(*po));
        po += w;
        for (x=0; x<w; x++,po++,pi++) {
          *po= (int)(*pi + *(pi+w)) / 2;
        }
      }
      memcpy(po, pi, w * sizeof(*po));
      po += w;
    } else { // even
      // i:  0   1   2   3
      // o: 0 1 2 3 4 5 6 7
      // i=0, o=1
      for (i=0; i<hi-1; i++,po+=w) {
        for (x=0; x<w; x++,po++,pi++) {
          *po= (int)(*pi * 3 + *(pi+w)) / 4;
          *(po+w)= (int)(*pi + *(pi+w)*3) / 4;
        }
      }
    }
    // i=hi-1, o=ho-1
    memcpy(po, pi, w * sizeof(*po));
  }
  return om;
}

image *image_double(image *im, int oddx, int oddy) {
  image *t= image_double_x(im, oddx);
  im= image_double_y(t, oddy);
  destroy_image(t);
  return im;
}

// vim: sw=2 ts=2 sts=2 expandtab:
