#include "common.h"

#define EQ(a, b) (0 == strcmp((a), (b)))
#define IS_IMAGE(p) (((image *)p)->type == 'I')
#define IS_VECTOR(p) (((vector *)p)->type == 'V')

void help(char **arg0, char *topic) {
#define TOPIC(x) EQ((x), topic)
if (! topic) {
  printf("\nUSAGE: %s COMMANDS...\n\n", *arg0);
  printf("COMMANDS:\n\
+ FILENAME.EXT: ------------ load a PNM image\n\
+ -: ------------------------ load from STDIN\n\
+ autocrop WIDTH HEIGHT: ----------- autocrop\n\
- bg: ----------------------- find background\n\
- contrast BLACK WHITE: ---- enhance contrast\n\
+ cropx LEFT RIGHT: ------- crop horizontally\n\
+ cropy TOP BOTTOM: --------- crop vertically\n\
- deskew: ---------------------- deskew image\n\
- div: --------------------- divide im2 / im1\n\
- ex HEIGHT: ----------- set lowercase height\n\
- fix-bg: -------------------- fix background\n\
- grid STEP: ----------- draw grid over image\n\
- histo: -------------------------- histogram\n\
- norm STRENGTH: --------- normalize contrast\n\
- pag NUM: ------------------ set page number\n\
- quit: ------------------- quit w/out output\n\
+ rot ANGLE: ----- rotate (only +-90 180 270)\n\
- skew ANGLE: ----------- rotate (-45 ... 45)\n\
+ splitx {X | N}: ---------- split vertically\n\
+ splity {Y | N}: -------- split horizontally\n\
- bin {auto | THRESHOLD} ------to black&white\n\
+ w FILENAME: ----------------- write to file\n\
- -h, --help: ------------------ this summary\n\
- -h, --help COMMAND: ------- help on COMMAND\n\
  \n");
} else printf("No help for %s.\n", topic);
exit(0);
}

int get_page_number(char *s) {
  int n;
  for (n = 0; *s; s++) {
    if ('.' == *s && n) break; 
    if ('0' <= *s && *s <= '9') {
      n = n * 10 + (*s - '0');
    }
    else n = 0;
  }
  return n;
}

//// STACK ////
#define STACK_SIZE 256
void *stack[STACK_SIZE];
#define SP stack[sp]
#define SP1 stack[sp - 1]
#define SP2 stack[sp - 2]
#define SP3 stack[sp - 3]

int sp = 0;

void *swap() {
  void *p;
  if (sp < 2) error("Stack underflow");
  p = SP2; SP2 = SP1; SP1 = p;
}

void *pop() {
  if (sp < 1) error("Stack underflow");
  sp--;
  return SP;
}

void push(void *p) {
  if (sp >= STACK_SIZE) error("Stack overflow");
  if (SP) free(SP);
  SP = p;
  sp++;
}

image *im(int i) {
  if (sp < i) error("Missing image");
  return (image*)stack[sp - i];
}

//// MAIN ////
int main(int argc, char **args) {
  #define ARG_IS(x) EQ((x), *arg)
  char **arg = args;
  char buf[256];
  image *img;
  vector *v;
  void *p;
  FILE *f;
  real t, x, y;
  int i, w, h;

  init_srgb();
  if (argc < 2) help(args, NULL);
  while (*(++arg)) {
    if (ARG_IS("-h") || ARG_IS("--help")) {
      help(args, *(++arg));
    }
    else
    if (ARG_IS("autocrop")) { // FLOAT FLOAT
      img = im(1);
      if (! *(++arg)) error("autocrop: missing WIDTH parameter");
      x = atof(*arg);
      if (x <= 1) x *= img->width;
      if (x <= 0 || x > img->width) error("autocrop: invalid WIDTH parameter");
      if (! *(++arg)) error("autocrop: missing HEIGHT parameter");
      y = atof(*arg);
      if (y <= 1) y *= img->height;
      if (y <= 0 || y > img->height) error("autocrop: invalid HEIGHT parameter");
      push(autocrop(img, x, y));
      swap(); pop();
    }
    else
    if (ARG_IS("bg")) { // FLOAT
      push(image_background(im(1)));
    }
    else
    if (ARG_IS("contrast")) { // BLACK WHITE
      if (! *(++arg)) error("contrast: missing BLACK parameter");
      if (! *(++arg)) error("contrast: missing WHITE parameter");
      contrast_image(im(1), atof(*(arg-1)), atof(*arg));
    }
    else
    if (ARG_IS("cropx")) { // FLOAT FLOAT
      img = im(1);
      if (! *(++arg)) error("cropx: missing LEFT parameter");
      x = atof(*arg);
      if (x <= 1) x *= img->width;
      if (x < 0 || x >= img->width) error("cropx: invalid LEFT parameter");
      if (! *(++arg)) error("cropx: missing RIGHT parameter");
      y = atof(*arg);
      if (y <= 1) y *= img->width;
      if (y <= x || y > img->width) error("cropx: invalid RIGHT parameter");
      push(crop(img, x, 0, y, img->height));
      swap(); pop();
    }
    else
    if (ARG_IS("cropy")) { // FLOAT FLOAT
      img = im(1);
      if (! *(++arg)) error("cropy: missing TOP parameter");
      x = atof(*arg);
      if (x <= 1) x *= img->height;
      if (x < 0 || x >= img->height) error("cropy: invalid TOP parameter");
      if (! *(++arg)) error("cropy: missing BOTTOM parameter");
      y = atof(*arg);
      if (y <= 1) y *= img->height;
      if (y <= x || y > img->height) error("cropx: invalid BOTTOM parameter");
      push(crop(img, 0, x, img->width, y));
      swap(); pop();
    }
    else
    if (ARG_IS("deskew")) {
      img = im(1);
      skew(img, detect_skew(img) );
    }
    else
    if (ARG_IS("div")) {
      divide_image(im(2), im(1));
      pop();
    }
    else
    if (ARG_IS("ex")) { // FLOAT
      if (! *(++arg)) error("ex: missing parameter");
      t = atof(*arg);
      if (t <= 0) error("ex param must be > 0.");
      if (sp) {
        img = im(1);
        if (t < 1) t *= img->height;
        img->ex = t;
      }
      default_ex = t;
    }
    else
    if (ARG_IS("fix-bg")) {
      img = im(1);
      push(image_background(img));
      divide_image(im(2), img);
      pop();
    }
    else
    if (ARG_IS("grid")) {
      if (! *(++arg)) error("grid: missing STEP parameter");
      draw_grid(im(1), atof(*arg));
    }
    else
    if (ARG_IS("histo")) {
      v = histogram_of_image(im(2));
      push(v);
    }
    else
    if (ARG_IS("norm")) { // FLOAT
      if (! *(++arg)) error("norm: missing parameter");
      normalize_image(im(1), atof(*arg));
    }
    else
    if (ARG_IS("pag")) { // INT
      if (! *(++arg)) error("pag: missing parameter");
      im(1)->pag = atoi(*arg);
    }
    else
    if (ARG_IS("quit")) exit(0);
    else
    if (ARG_IS("rot")) { // +-90, 180, 270
      if (! *(++arg)) error("rot: missing parameter");
      push(rotate_image(im(1), atof(*arg)));
      swap(); pop();
    }
    else
    if (ARG_IS("skew")) { // FLOAT
      if (! *(++arg)) error("skew: missing parameter");
      skew(im(1), atof(*arg));
    }
    else
    if (ARG_IS("splitx")) { // FLOAT
      if (! *(++arg)) error("splitx: missing parameter");
      img = im(1);
      push(0); swap();
      push(0); swap();
      splitx_image(&SP2, &SP3, img, atof(*arg));
      pop();
    }
    else
    if (ARG_IS("splity")) { // FLOAT
      if (! *(++arg)) error("splity: missing parameter");
      img = im(1);
      push(0); swap();
      push(0); swap();
      splity_image(&SP2, &SP3, img, atof(*arg));
      pop();
    }
    else
    if (ARG_IS("bin")) { // auto | FLOAT
      if (! *(++arg)) error("bin: missing parameter");
      if (ARG_IS("auto")) {
        v = threshold_histogram(im(1));
        x = index_of_max(v) / 255.0;
        destroy_vector(v);
      }
      else x = atof(*arg);
      contrast_image(im(1), x, x);
    }
    else
    if (ARG_IS("w")) { // FILENAME
      if (! *(++arg)) error("w: missing parameter");
      p = pop();
      if (IS_IMAGE(p)) {
        img = p;
        if (strlen(*arg) > 200) {
          error1("page name too long:", *arg);
        }
        sprintf(buf, *arg, img->pag);
        f = fopen(buf, "wb");
        write_image(p, f);
      }
      else
      if (IS_VECTOR(p)) {
        f = fopen(*arg, "wb");
        write_vector(p, f);
      }
    }
    else
    if (ARG_IS("-")) {
      push(read_image(stdin, 0));
    }
    else
    if (strchr(*arg, '.')) { // FILENAME.EXT
      i = get_page_number(*arg);
      f = fopen(*arg, "rb");
      if (! f) error1("File not found:", *arg);
      img = read_image(f, 0);
      img->pag = i;
      push(img);
    }
    else error1("Command not found:", *arg);
  }
  if (sp > 0) {
    p = pop();
    f = stdout;
    if (IS_IMAGE(p)) write_image(p, f);
    else
    if (IS_VECTOR(p)) write_vector(p, f);
    else assert(0);
  }
}

// vim: sw=2 ts=2 sts=2 expandtab:
