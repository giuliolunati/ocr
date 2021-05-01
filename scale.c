#include "common.h"

image *image_half_x(image *im) {
  int wi= im->width;
  int h= im->height;
  real a, b, c, s;
  image *om;
  int wo, x, y, z;
  gray *pi, *po;
  wo= (wi + wi%2) / 2; 
  om= image_clone(im, 0, wo, h);
  for (z= 0; z < 5; z++) {
    if (! im->chan[z]) continue;
    if (wi % 2) { // odd
      a= 1.0/16; b= 4.0/16; c= 6.0/16;
      s=a+b+c+b+a;
      for (y= 0; y < h; y++) {
        pi= im->chan[z] + (y * wi);
        po= om->chan[z] + (y * wo);
        *po= *pi * s;
        po++; pi +=2;
        for (x=1; x < wo-1; x++,po++,pi+=2) {
          *po= c * *pi
            + b * (*(pi-1) + *(pi+1))
            + a * (*(pi-2) + *(pi+2))
          ;
        }
        *po= *pi * s;
      }
      ASSERT(po+1 == om->chan[z] + h*wo);
    } else { // even
      a= 1.0/8; b= 3.0/8;;
      for (y= 0; y < h; y++) {
        pi= im->chan[z] + (y * wi);
        po= om->chan[z] + (y * wo);
        *po= b * (*pi + *(pi+1))
          + a * (*pi*2 - *(pi+1) + *(pi+2));
        po++; pi += 2;
        for (x=1; x < wo-1; x++,po++,pi+=2) {
          *po= b * (*pi + *(pi+1))
            + a * (*(pi-1) + *(pi+2));
        }
        *po= b * (*pi + *(pi+1))
          + a * (*(pi-1) + *(pi+1)*2 - *pi);
      }
      ASSERT(po+1 == om->chan[z] + h*wo);
    }
  }
  return om;
}

image *image_half_y(image *im) {
  int w= im->width;
  int hi= im->height;
  image *om;
  int ho, x, y, z;
  real a, b, c, s;
  gray *pi, *po;
  ho= (hi + hi%2) / 2; 
  om= image_clone(im, 0, w, ho);
  for (z= 0; z < 5; z++) {
    if (! im->chan[z]) continue;
    if (hi % 2) { // odd
      a= 1.0/16; b= 4.0/16; c= 6.0/16;
      s=a+b+c+b+a;
      po= om->chan[z];
      pi= im->chan[z];
      for (x=0; x < w; x++, po++, pi++) *po= *pi * s;
      pi += w;
      for (y= 1; y < ho-1; y++) {
        for (x=0; x < w; x++, po++, pi++) {
          *po= c * *pi
            + b * (*(pi-w) + *(pi+w))
            + a * (*(pi-2*w) + *(pi+2*w))
          ;
        }
        pi += w;
      }
      for (x=0; x < w; x++, po++, pi++) *po= *pi * s;
      ASSERT(pi == im->chan[z] + hi*w);
      ASSERT(po == om->chan[z] + ho*w);
    } else { // even
      a= 1.0/8; b= 3.0/8;;
      po= om->chan[z];
      pi= im->chan[z];
      for (x=0; x < w; x++, po++, pi++) {
        *po= b * (*pi + *(pi+w))
          + a * (*pi*2 - *(pi+w) + *(pi+2*w));
      }
      pi += w;
      for (y= 1; y < ho-1; y++) {
        for (x=0; x < w; x++, po++, pi++) {
          *po= b * (*pi + *(pi+w))
            + a * (*(pi-w) + *(pi+2*w));
        }
        pi += w;
      }
      for (x=0; x < w; x++, po++, pi++) {
        *po= b * (*pi + *(pi+w))
          + a * (*(pi-w) + *(pi+w)*2 - *pi);
      }
      pi += w;
      ASSERT(pi == im->chan[z] + hi*w);
      ASSERT(po == om->chan[z] + ho*w);
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

image *image_redouble_x(image *im, int odd) {
  odd= odd%2;
  int wi= im->width;
  assert(wi >= 3);
  int h= im->height;
  image *om;
  int wo, x, y, z;
  real a, b, c, d, s;
  gray *pi, *po;
  wo= wi*2 - odd; 
  om= image_clone(im, 0, wo, h);
  for (z= 0; z < 5; z++) {
    if (! im->chan[z]) continue;
    pi= im->chan[z];
    po= om->chan[z];
    if (odd) {
      a= -1.0/8; b= 10.0/8;
      s= a+b+a;
      c= -3.0/16; d= 11.0/16;
      // i: 0   1   2   3
      // o: 0 1 2 3 4 5 6
      for (y= 0; y < h; y++) {
        *po= *pi * s;
        po++; pi++;
        // i=1, o=1
        *po= d * (*(pi-1) + *pi)
          + c * (*(pi-1)*2 - *pi + *(pi+1));
        po++;
        // i=1, o=2
        *po= *pi*b + (*(pi-1) + *(pi+1))*a;
        for (x=2; x<wi-1; x++) {
          po++; pi++;
          // i=x, o=2x-1
          *po= d * (*(pi-1) + *pi)
            + c * (*(pi-2) + *(pi+1));
          po++;
          // i=x, o=2x
          *po= *pi*b + (*(pi-1) + *(pi+1))*a;
        }
        // i=wi-2, o= 2wi-4= wo-3
        po++; pi++;
        // i=wi-1, o=wo-2
        *po= d * (*(pi-1) + *pi)
          + c * (*(pi-2) + *pi*2 - *(pi-1));
        po++;
        *po= *pi * s;
        po++; pi++;
      }
      ASSERT(pi == im->chan[z] + h*wi);
      ASSERT(po == om->chan[z] + h*wo);
    } else { // even
      // i:  0   1   2   3
      // o: 0 1 2 3 4 5 6 7
      a= 1.0/16; b= 18.0/16; c= -3.0/16;
      for (y= 0; y < h; y++) {
        *po= (*pi*2-*(pi+1))*a + *pi*b + *(pi+1)*c;
        po++;
        *po= (*pi*2-*(pi+1))*c + *pi*b + *(pi+1)*a;
        po++; pi++;
        // i=1, o=2
        for (x=1; x < wi-1; x++) {
          // i=x, o=2x
          *po= *(pi-1)*a + *pi*b + *(pi+1)*c;
          po++;
          *po= *(pi-1)*c + *pi*b + *(pi+1)*a;
          po++; pi++;
        }
        // i= wi-1, o= 2wi-2 = wo-2
        *po= *(pi-1)*a + *pi*b + (*pi*2 - *(pi-1))*c;
        po++;
        *po= *(pi-1)*c + *pi*b + (*pi*2 - *(pi-1))*a;
        po++; pi++;
      }
      ASSERT(pi == im->chan[z] + h*wi);
      ASSERT(po == om->chan[z] + h*wo);
    }
  }
  return om;
}

image *image_redouble_y(image *im, int odd) {
  odd= odd%2;
  int w= im->width;
  int hi= im->height;
  assert(hi >= 3);
  image *om;
  int ho, x, y, z;
  real a, b, c, d, s;
  gray *pi, *po;
  ho= hi*2 - odd;
  om= image_clone(im, 0, w, ho);
  for (z= 0; z < 5; z++) {
    if (! im->chan[z]) continue;
    po= om->chan[z];
    pi= im->chan[z];
    if (odd) {
      a= -1.0/8; b= 10.0/8;
      s= a+b+a;
      c= -3.0/16; d= 11.0/16;
      // i: 0   1   2   3
      // o: 0 1 2 3 4 5 6
      for (x=0; x<w; x++,po++,pi++) *po= *pi * s;
      // i=1, o=1
      for (x=0; x<w; x++,po++,pi++) {
        *po= d * (*(pi-w) + *pi)
          + c * (*(pi-w)*2 - *pi + *(pi+w));
      }
      pi -= w;
      // i=1, o=2
      for (x=0; x<w; x++,po++,pi++) {
        *po= *pi*b + (*(pi-w) + *(pi+w))*a;
      }
      for (y=2; y<hi-1; y++) {
        // i=y, o=2y-1
        for (x=0; x<w; x++,po++,pi++) {
          *po= d * (*(pi-w) + *pi)
            + c * (*(pi-2*w) + *(pi+w));
        }
        pi -= w;
        for (x=0; x<w; x++,po++,pi++) {
          *po= *pi*b + (*(pi-w) + *(pi+w))*a;
        }
      }
      for (x=0; x<w; x++,po++,pi++) {
        *po= d * (*(pi-w) + *pi)
          + c * (*(pi-2*w) + *pi*2 - *(pi-w));
      }
      pi -= w;
      for (x=0; x<w; x++,po++,pi++) *po= *pi * s;
      ASSERT(pi == im->chan[z] + (hi)*w);
      ASSERT(po == om->chan[z] + (ho)*w);
    } else { // even
      // i:  0   1   2   3
      // o: 0 1 2 3 4 5 6 7
      a= 1.0/16; b= 18.0/16; c= -3.0/16;
      for (x=0; x<w; x++,po++,pi++) {
        *po= (*pi*2 - *(pi+w))*a + *pi*b + *(pi+w)*c;
      }
      pi -= w;
      for (x=0; x<w; x++,po++,pi++) {
        *po= (*pi*2 - *(pi+w))*c + *pi*b + *(pi+w)*a;
      }
      // i=1, o=2
      for (y=1; y<hi-1; y++) {
        // i=y, o=2y
        for (x=0; x<w; x++,po++,pi++) {
          *po= *(pi-w)*a + *pi*b + *(pi+w)*c;
        }
        pi -= w;
        for (x=0; x<w; x++,po++,pi++) {
          *po= *(pi-w)*c + *pi*b + *(pi+w)*a;
        }
      }
      // i= hi-1, o= 2hi-2 = ho-2
      for (x=0; x<w; x++,po++,pi++) {
        *po= *(pi-w)*a + *pi*b + (*pi*2 - *(pi-w))*c;
      }
      pi -= w;
      for (x=0; x<w; x++,po++,pi++) {
        *po= *(pi-w)*c + *pi*b + (*pi*2 - *(pi-w))*a;
      }
      ASSERT(pi == im->chan[z] + (hi)*w);
      ASSERT(po == om->chan[z] + (ho)*w);
    }
  }
  return om;
}

image *image_redouble(image *im, int oddx, int oddy) {
  image *t= image_redouble_x(im, oddx);
  im= image_redouble_y(t, oddy);
  destroy_image(t);
  return im;
}

image *image_double(image *im, real k /*sharpness*/) {
  int w= im->width, h= im->height;
  int x, y, z;
  image *om= image_clone(im, 0, 2*w, 2*h);
  om->ex= 2 * im->ex;
  om->pag= im->pag;
  gray *i1, *i2, *i3, *i4, *o;
  real v;
  real a= 9.0/16, b= 3.0/16, c= 1.0/16; 
  real a1= 8.0/15, b1= 2.0/15, c1= 3.0/15; 
  a= a * (1 - k) + a1 * k;
  b= b * (1 - k) + b1 * k;
  c= c * (1 - k) + c1 * k;
  for (z= 0; z < 5; z++) {
    if (! im->chan[z]) continue;
    for (y= 0; y < h; y++) {
      i4= i3= im->chan[z] + (w * y);
      if (y == 0) {i1= i3; i2= i4;}
      else {i1= i3 - w; i2= i4 - w;}
      o= om->chan[z] + (4 * w * y);
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
      i1= i2= im->chan[z] + (w * y);
      if (y == h - 1) {i3= i1; i4= i2;}
      else {i3= i1 + w; i4= i2 + w;}
      o= om->chan[z] + (4 * w * y) + (2 * w);
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

// vim: sw=2 ts=2 sts=2 expandtab:
