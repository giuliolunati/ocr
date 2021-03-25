#include "common.h"
#include <errno.h>

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

image *image_read_pnm(FILE *file) {
  int x, y, prec, magic, depth, height, width, binary;
  image *im;
  uchar *buf, *ps;
  int z;
  char c, *tok;

  assert(file);
  if (1 > fscanf(file, "P%d ", &magic))
  { error("image_read_pnm: wrong magic."); }
  if (magic < 7) { // PNM
    if (0 >= (width= atoi(read_next_token(file))))
    { error("image_read_pnm: wrong width."); }
    if (0 >= (height= atoi(read_next_token(file))))
    { error("image_read_pnm: wrong height."); }
    if (0 >= (prec= atoi(read_next_token(file))))
    { error("image_read_pnm: wrong precision"); }
    if (1 > fscanf(file, "%c", &c) || 
      (c != ' ' && c != '\t' && c != '\n')
    ) {
      error("image_read_pnm: no w/space after precision");
    }
    switch (magic) {
      case 5: depth= 1; break;
      case 6: depth= 3; break;
      default: error("image_read_pnm: Invalid depth.");
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
        else error("image_read_pnm: unknown TUPLTYPE");
        c= '#';
        while (c != '\n') fscanf(file, "%c", &c);
      }
    }
    fscanf(file, "%c", &c);
    if (c != '\n') 
    { error("image_read_pnm: missing newline after ENDHDR"); }
  }
  if (prec != 255)
  { error("image_read_pnm: Precision != 255.");
  }
  assert(width > 0 && height > 0);
  assert(1 <= depth && depth <= 4);
  im= make_image(width, height, depth);
  int opaque= depth % 2;
  buf= malloc(width * depth);
  gray *p[4];
  // for grayscale, RGB, RGBA 
  p[0]= im->chan[1];
  p[1]= im->chan[2];
  p[2]= im->chan[3];
  p[3]= im->chan[0];
  // for gray+alpha
  if (depth == 2) p[1]= p[3]; // alpha
  //
  for (y= 0; y < height; y++) {
    if (width > fread(buf, depth, width, file)) {
      error("image_read_pnm: Unexpected EOF");
    }
    ps= buf;
    for (x= 0; x < width; x++) {
      for (z= 0; z < depth; z++, ps++) {
        *p[z]= ((int)*ps - 128) * KP;
        p[z]++;
      }
    }
  }
  free(buf);
  return im;
}

void image_write_pnm(image *im, FILE *file) {
  int x, y;
  int width= im->width, height= im->height;
  uchar *buf, *pt;
  float v;

  int z, depth= im->depth, opaque= depth % 2;
  assert(file);
  if (opaque) {
    if (depth == 1) fprintf(file, "P5\n");
    else
    if (depth == 3) fprintf(file, "P6\n");
    else error("image_write_pnm: unsupported depth");
    fprintf(file, "%d %d\n255\n", width, height);
  } else {
    fprintf(file, "P7\n");
    fprintf(file, "WIDTH %d\n", width);
    fprintf(file, "HEIGHT %d\n", height);
    fprintf(file, "DEPTH %d\n", depth);
    fprintf(file, "MAXVAL 255\n");
    fprintf(file, "TUPLTYPE ");
    if (depth == 2) fprintf(file, "GRAYSCALE_ALPHA\n");
    else
    if (depth == 4) fprintf(file, "RGB_ALPHA\n");
    else error("image_write_pnm: unsupported depth");
    fprintf(file, "ENDHDR\n");
  }
  buf= malloc(width * depth * sizeof(*buf));
  assert(buf);
  gray *p[4];
  // for grayscale, RGB, RGBA 
  p[0]= im->chan[1];
  p[1]= im->chan[2];
  p[2]= im->chan[3];
  p[3]= im->chan[0];
  // for gray+alpha
  if (depth == 2) p[1]= p[3]; // alpha
  //
  for (y= 0; y < height; y++) {
    pt= buf;
    for (x= 0; x < width; x++) {
      for (z= 0; z < depth; z++, pt++) {
        v= *p[z]; p[z]++;
        if (v < -MAXVAL) *pt= 0;
        else if (v > MAXVAL) *pt= 255;
        else *pt= v / KP + 128;
      }
    }
    if (width > fwrite(buf, depth, width, file)) error("Error writing file.");
  }
  free(buf);
}

#define CMDLEN 256
char cmd[CMDLEN];
char *jpegtopnm= "jpegtopnm -quiet ";
char *pngtopam= "pngtopam -quiet -alphapam ";
char *pngtopnm= "pngtopam -quiet ";
char *pnmtojpeg= "pnmtojpeg -quiet -quality 90 > ";
char *pnmtopng= "pamtopng -quiet > ";

image *image_read(char *fname) {
  FILE *f;
  image *img;
  if (! fname) return image_read_pnm(stdin);
  int l= strlen(fname);
  char *ext= fname + l - 4;
  char *filter= NULL;
  unsigned char type;
  if (l >= 4) {
    if (EQ(ext, ".jpg")) filter= jpegtopnm;
    else if (EQ(ext, ".png")) {
      f= fopen(fname, "rb");
      if (! f) error1("image_read: can't read file ", fname);
      fseek(f, 25, SEEK_SET);
      fscanf(f, "%c", &type);
      if (type & 4) filter= pngtopam;
      else filter= pngtopnm;
    }
  }
  if (filter) {
    assert(CMDLEN > strlen(filter));
    strcpy(cmd, filter);
    if (strlen(filter) + l >= CMDLEN) {
      error1("image_read: name too long: %s", fname);
    }
    strcat(cmd, fname);
    f= popen(cmd, "r");
    if (! f) error1("image_read: popen failed:", strerror(errno));
    img= image_read_pnm(f);
    pclose(f);
  } else {
    f= fopen(fname, "rb");
    if (! f) error1("image_read: can't read file ", fname);
    img= image_read_pnm(f);
    fclose(f);
  }
  return img;
}

void image_write(image *im, char *fname) {
  FILE *f;
  if (! fname)
  { image_write_pnm(im, stdout); return; }
  int l= strlen(fname);
  char *ext= fname + l - 4;
  char *filter= NULL;
  if (l >= 4) {
    if (EQ(ext, ".jpg")) filter= pnmtojpeg;
    else if (EQ(ext, ".png")) filter= pnmtopng;
  }
  if (filter) {
    assert(CMDLEN > strlen(filter));
    strcpy(cmd, filter);
    if (strlen(filter) + strlen(fname) >= CMDLEN) {
      error1("image_write: name too long: %s", fname);
    }
    strcat(cmd, fname);
    f= popen(cmd, "w");
    if (! f) error1("image_write: popen failed:", strerror(errno));
    image_write_pnm(im, f);
    pclose(f);
  } else {
    f= fopen(fname, "wb");
    if (! f) error1("image_write: can't write file", fname);
    image_write_pnm(im, f);
    fclose(f);
  }
}
// vim: set et sw=2:
