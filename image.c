#include "common.h"

#define EQ(a, b) (0 == strcmp((a), (b)))

//// SRGB ////
  // 0 <= srgb <= 1  0 <= lin <= 1
  // srgb < 0.04045: lin= srgb / 12.92
  // srgb > 0.04045: lin= [(srgb + 0.055) / 1.055]^2.4

gray *srgb_to_lin= NULL;
uchar *srgb_from_lin= NULL;

void ensure_init_srgb() {
  if (srgb_to_lin && srgb_from_lin) return;
  srgb_to_lin= realloc(srgb_to_lin, 256);
  srgb_from_lin= realloc(srgb_from_lin, MAXVAL + 1);
  float x, x0= 0;
  int l0= 0, l= 0;
  int s;
  for (s= 1; s <= 256; s ++) {
    x= s / 256.0;
    // sRGB map [0, 1] -> [0, 1]
    if (x < 0.04045) x /= 12.92;
    else x= pow( (x + 0.055) / 1.055, 2.4 );
    //
    while (l + 0.5 < x * (MAXVAL + 1)) {
      srgb_from_lin[l]= s-1;
      l ++;
    }
    srgb_to_lin[s-1]= l0= (l0 + l - 1) / 2;
    x0= x; l0= l;
  }
  while (l <= MAXVAL) {
    srgb_from_lin[l]= 255;
    l ++;
  }
  assert(srgb_to_lin[255] <= MAXVAL);
  assert(srgb_from_lin[MAXVAL] == 255);
  for (s= 0; s <= 255; s ++) assert(
    s == (srgb_from_lin[(int)srgb_to_lin[s]])
  );
}

//// IMAGES ////

real default_ex= 25;

image *make_image(int width, int height, int depth) {
  assert(1 <= depth && depth <= 4);
  if (height < 1 || width < 1) error("make_image: size <= 0");
  int i;
  int opaque= depth % 2;
  gray *p;
  image *im= malloc(sizeof(image));
  if (! im) error("make_image: can't alloc memory");
  for (i= opaque; i < depth+opaque; i ++) {
    p= calloc (width * height, sizeof(*p));
    if (! p) error("make_image: can't alloc memory");
    im->chan[i]= p;
  } for (; i < 4; i ++) im->chan[i]= NULL;
  if (opaque) im->chan[0]= NULL;
  im->magic= 'I';
  im->width= width;
  im->height= height;
  im->depth= depth;
  im->ex= default_ex;
  im->pag= 0;
  im->black= im->graythr= im->white= -1;
  im->area= im->thickness= -1;
  return im;
}

void destroy_image(image *im) {
  if (! im) return;
  int i;
  for (i= 0; i < 4; i ++) {
    if (im->chan[i]) free(im->chan[i]);
  }
  free(im);
}

image *copy_image(image *im) {
  image *om= malloc(sizeof(*om));
  uint len= im->width * im->height * sizeof(gray);
  memcpy(om, im, sizeof(*om));
  gray *pi, *po;
  int i;
  for (i=0; i<4; i++) {
    pi= im->chan[i];
    if (! pi) continue;
    om->chan[i]= po = malloc(len);
    memcpy(po, pi, len);
  }
  return om;
}

char *read_next_token(FILE *file) {
  char c;
  int i;
  char *buf= malloc(16);
  unsigned long int p0, p1;
  while (1) {
    p0= ftell(file);
    fscanf(file, " ");
    if (p0 < (p1= ftell(file))) continue;
    fscanf(file, "#");
    if (p0 < (p1= ftell(file))) {
      c= '#';
      while (c != '\n') fscanf(file, "%c", &c);
      continue;
    }
    break;
  }
  if ( ! fscanf(file, "%15s", buf) ) return "";
  return buf;
}

image *read_image(FILE *file, int sigma) {
  int x, y, prec, magic, depth, height, width, binary;
  image *im;
  uchar *buf, *ps;
  ulong i;
  int z;
  char c, *tok;
  gray *p, v;

  assert(file);
  if (1 > fscanf(file, "P%d ", &magic))
  { error("read_image: wrong magic."); }
  if (magic < 7) { // PNM
    if (0 >= (width= atoi(read_next_token(file))))
    { error("read_image: wrong width."); }
    if (0 >= (height= atoi(read_next_token(file))))
    { error("read_image: wrong height."); }
    if (0 >= (prec= atoi(read_next_token(file))))
    { error("read_image: wrong precision"); }
    if (1 > fscanf(file, "%c", &c) || 
      (c != ' ' && c != '\t' && c != '\n')
    ) {
      error("read_image: no w/space after precision");
    }
    switch (magic) {
      case 5: depth= 1; break;
      case 6: depth= 3; break;
      default: error("read_image: Invalid depth.");
    }
  } else if (magic == 7) { // PAM
    while (1) {
      tok= read_next_token(file);
      if (EQ(tok, "DEPTH"))
      { depth= atoi(read_next_token(file)); }
      else
      if (EQ(tok, "ENDHDR")) break;
      else
      if (EQ(tok, "HEIGHT"))
      { height= atoi(read_next_token(file)); }
      else
      if (EQ(tok, "MAXVAL"))
      { prec= atoi(read_next_token(file)); }
      else
      if (EQ(tok, "WIDTH"))
      { width= atoi(read_next_token(file)); }
      else
      if (EQ(tok, "TUPLTYPE")) {
        tok= read_next_token(file);
        if (EQ(tok, "GRAYSCALE")) depth= 1;
        else
        if (EQ(tok, "GRAYSCALE_ALPHA")) depth= 2;
        else
        if (EQ(tok, "RGB")) depth= 3;
        else
        if (EQ(tok, "RGB_ALPHA")) depth= 4;
        else error("read_image: unknown TUPLTYPE");
        c= '#';
        while (c != '\n') fscanf(file, "%c", &c);
      }
    }
  }
  //fprintf(stderr, "%d x %d x %d \n", depth, width, height); exit(1);
  if (prec != 255)
  { error("read_image: Precision != 255.");
  }
  assert(width > 0 && height > 0);
  assert(1 <= depth && depth <= 4);
  im= make_image(width, height, depth);
  int opaque= depth % 2;
  buf= malloc(width * depth);

  if (sigma) {
    i= 0;
    for (y= 0; y < height; y++) {
      if (width > fread(buf, depth, width, file)) {
        error("read_image: Unexpected EOF");
      }
      ps= buf;
      for (x= 0; x < width; x++, i++) {
        for (z= opaque; z < depth+opaque; z++, ps++) {
          v= *ps - 128;
          *(im->chan[z] + i)= KSKP * v / (KS - fabs(v));
        }
      }
    }
  } else {
    i= 0;
    for (y= 0; y < height; y++) {
      if (width > fread(buf, depth, width, file)) {
        error("read_image: Unexpected EOF");
      }
      ps= buf;
      for (x= 0; x < width; x++, i++) {
        for (z= opaque; z < depth+opaque; z++, ps++) {
          *(im->chan[z] + i)= ((int)*ps - 128) * KP;
        }
      }
    }
  }
  free(buf);
  return im;
}

void write_image(image *im, FILE *file, int sigma) {
  int x, y;
  int width= im->width, height= im->height;
  uchar *buf, *pt;
  float v;
  gray *p;
  ulong i;
  int z, depth= im->depth, opaque= depth % 2;
  assert(file);
  if (! opaque) error("write_image: alpha unsupported");
  if (depth == 1) fprintf(file, "P5\n");
  else
  if (depth == 3) fprintf(file, "P6\n");
  else error("write_image: unsupported depth");
  fprintf(file, "%d %d\n255\n", width, height);
  buf= malloc(width * depth * sizeof(*buf));
  assert(buf);
  // TODO: don't modify im!
  image_dither(im,1,1);
  if (sigma) {
    i= 0;
    for (y= 0; y < height; y++) {
      pt= buf;
      for (x= 0; x < width; x++, i++) {
        for (z= 1; z <= depth; z++, pt++) {
          v= *(im->chan[z] + i);
          v= KS * v / (KSKP + fabs(v));
          *pt= v + 128;
        }
      }
      if (width > fwrite(buf, depth, width, file)) error("Error writing file.");
    }
  } else {
    i= 0;
    for (y= 0; y < height; y++) {
      pt= buf;
      for (x= 0; x < width; x++, i++) {
        for (z= 1; z <= depth; z++, pt++) {
          v= *(im->chan[z] + i);
          if (v < -MAXVAL) *pt= 0;
          else if (v > MAXVAL) *pt= 255;
          else *pt= v / KP + 128;
        }
      }
      if (width > fwrite(buf, depth, width, file)) error("Error writing file.");
    }
  }
  free(buf);
}

// vim: set et sw=2:
