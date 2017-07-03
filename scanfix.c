#include "common.h"

#define EQ(a, b) (0 == strcmp((a), (b)))
#define IS_IMAGE(p) (((image *)p)->type == 'I')
#define IS_VECTOR(p) (((vector *)p)->type == 'V')

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

void help(char **arg0, char *topic) {
#define TOPIC(x) EQ((x), topic)
if (! topic) {
  printf("\nUSAGE: %s COMMANDS...\n\n", *arg0);
  printf("COMMANDS:\n\
  FILENAME:       load a PNM image\n\
  -:              load from STDIN\n\
  bg FLOAT:       find background light\n\
  contrast BLACK WHITE:\n\
                  enhance contrast\n\
  deskew:         deskew image\n\
  div:            divide im2 / im1\n\
  fix-bg:         fix background light\n\
  histo:          histogram\n\
  ex FLOAT:       set height-of-x\n\
  norm STRENGTH:  normalize contrast\n\
  quit:           quit w/out output\n\
  rot ANGLE:      rotate (only +-90 180 270)\n\
  skew ANGLE:     rotate (-45 ... 45)\n\
  splitx X:       split vertically\n\
  splity Y:       split horizontally\n\
  w FILENAME:     write to file\n\
  -h, --help:     this summary\n\
  \n\
  -h, --help COMMAND: help on COMMAND\n\
  \n");
} else if (TOPIC("bg")) {
  printf("Coming soon...\n");
} else if (TOPIC("contrast")) {
  printf("Coming soon...\n");
} else if (TOPIC("deskew")) {
  printf("Coming soon...\n");
} else if (TOPIC("div")) {
  printf("Coming soon...\n");
} else if (TOPIC("fix-bg")) {
  printf("Coming soon...\n");
} else if (TOPIC("histo")) {
  printf("Coming soon...\n");
} else if (TOPIC("ex")) {
  printf("Coming soon...\n");
} else if (TOPIC("norm")) {
  printf("Coming soon...\n");
} else if (TOPIC("quit")) {
  printf("Coming soon...\n");
} else if (TOPIC("rot")) {
  printf("Coming soon...\n");
} else if (TOPIC("skew")) {
  printf("Coming soon...\n");
} else if (TOPIC("splitx")) {
  printf("Coming soon...\n");
} else if (TOPIC("splity")) {
  printf("Coming soon...\n");
} else if (TOPIC("w")) {
  printf("Coming soon...\n");
} else printf("Unknown topic %s.\n", topic);
exit(0);
}

//// STACK ////
#define STACK_SIZE 256
void *stack[STACK_SIZE];
#define SP stack[sp]
#define SP_1 stack[sp - 1]
#define SP_2 stack[sp - 2]
#define SP_3 stack[sp - 3]
int sp = 0;

void *swap() {
  void *p;
  if (sp < 2) error("Stack underflow");
  p = SP_2; SP_2 = SP_1; SP_1 = p;
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

//// MAIN ////
int main(int argc, char **args) {
  #define ARG_IS(x) EQ((x), *arg)
  char **arg = args;
  image *im;
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
    if (ARG_IS("bg")) { // FLOAT
      push(image_background((image*)SP_1));
    }
    else
    if (ARG_IS("contrast")) { // BLACK WHITE
      if (! *(++arg)) error("contrast: missing BLACK parameter");
      if (! *(++arg)) error("contrast: missing WHITE parameter");
      contrast_image((image*)SP_1, atof(*(arg-1)), atof(*arg));
    }
    else
    if (ARG_IS("crop")) { // FLOAT FLOAT
      im = (image*)SP_1;
      if (! *(++arg)) error("crop: missing WIDTH");
      x = atof(*arg);
      if (x <= 0 || x >= im->width) error("crop: invalid WIDTH");
      if (x < 1) x *= im->width;
      if (! *(++arg)) error("crop: missing HEIGHT parameter");
      y = atof(*arg);
      if (y <= 0 || y >= im->height) error("crop: invalid HEIGHT");
      if (y < 1) y *= im->height;
      push(autocrop(im, x, y));
      swap(); pop();
    }
    else
    if (ARG_IS("deskew")) {
      skew( (image*)SP_1, detect_skew((image*)SP_1) );
    }
    else
    if (ARG_IS("div")) {
      divide_image((image*)SP_2, (image*)SP_1);
      pop();
    }
    else
    if (ARG_IS("grid")) {
      if (! *(++arg)) error("grid: missing STEP");
      im = (image*)SP_1;
      draw_grid(im, atof(*arg));
    }
    else
    if (ARG_IS("ex")) { // FLOAT
      if (! *(++arg)) error("ex: missing parameter");
      t = atof(*arg);
      if (t <= 0) error("ex param must be > 0.");
      if (sp) {
        im = (image*)SP_1;
        if (t < 1) t *= im->height;
        im->ex = t;
      }
      default_ex = t;
    }
    else
    if (ARG_IS("fix-bg")) {
      push(image_background((image*)SP_1));
      divide_image((image*)SP_2, (image*)SP_1);
      pop();
    }
    else
    if (ARG_IS("histo")) {
      v = histogram_of_image((image*)SP_1);
      push(v);
    }
    else
    if (ARG_IS("norm")) { // FLOAT
      if (! *(++arg)) error("norm: missing parameter");
      normalize_image((image*)SP_1, atof(*arg));
    }
    else
    if (ARG_IS("pag")) { // INT
      if (! *(++arg)) error("pag: missing parameter");
      ((image*)SP_1)->pag = atoi(*arg);
    }
    else
    if (ARG_IS("quit")) exit(0);
    else
    if (ARG_IS("rot")) { // +-90, 180, 270
      if (! *(++arg)) error("rot: missing parameter");
      push(rotate_image((image*)SP_1, atof(*arg)));
      swap(); pop();
    }
    else
    if (ARG_IS("skew")) { // FLOAT
      if (! *(++arg)) error("skew: missing parameter");
      skew((image*)SP_1, atof(*arg));
    }
    else
    if (ARG_IS("splitx")) { // FLOAT
      if (! *(++arg)) error("splitx: missing parameter");
      push(0); swap();
      push(0); swap();
      splitx_image(&SP_2, &SP_3, (image*)SP_1, atof(*arg));
      pop();
    }
    else
    if (ARG_IS("splity")) { // FLOAT
      if (! *(++arg)) error("splity: missing parameter");
      push(0); swap();
      push(0); swap();
      splity_image(&SP_2, &SP_3, (image*)SP_1, atof(*arg));
      pop();
    }
    else
    if (ARG_IS("thr")) {
      v = threshold_histogram((image*)SP_1);
      push(v);
      printf("thr: %d\n", index_of_max(v));
    }
    else
    if (ARG_IS("w")) { // FILENAME
      if (! *(++arg)) error("w: missing parameter");
      p = pop();
      if (IS_IMAGE(p)) {
        im = p;
        i = strlen(*arg);
        if (i < sprintf(*arg, *arg, im->pag)) {
          error1("page number too long:", *arg);
        }
        f = fopen(*arg, "wb");
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
      im = read_image(f, 0);
      im->pag = i;
      push(im);
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
