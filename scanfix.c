#include "common.h"

#define EQ(a, b) (0 == strcmp((a), (b)))
#define IS_IMAGE(p) (((image *)p)->type == 'I')
#define IS_VECTOR(p) (((vector *)p)->type == 'V')

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
  real t;

  init_srgb();
  if (argc < 2) help(args, NULL);
  while (*(++arg)) {
    if (ARG_IS("-h") || ARG_IS("--help")) { // -h,--help
      help(args, *(++arg));
    }
    else
    if (ARG_IS("bg")) { // bg FLOAT
      push(image_background((image*)SP_1));
    }
    else
    if (ARG_IS("contrast")) { // norm BLACK WHITE
      if (! *(++arg)) error("norm: missing parameter");
      if (! *(++arg)) error("norm: missing parameter");
      contrast_image((image*)SP_1, atof(*(arg-1)), atof(*arg));
    }
    if (ARG_IS("deskew")) { // deskew
      skew( (image*)SP_1, detect_skew((image*)SP_1) );
    }
    else
    if (ARG_IS("div")) { // div
      divide_image((image*)SP_2, (image*)SP_1);
      pop();
    }
    else
    if (ARG_IS("ex")) { // ex FLOAT
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
    if (ARG_IS("fix-bg")) { // fix-bg
      push(image_background((image*)SP_1));
      divide_image((image*)SP_2, (image*)SP_1);
      pop();
    }
    else
    if (ARG_IS("histo")) { // histo
      v = histogram_of_image((image*)SP_1);
      push(v);
    }
    else
    if (ARG_IS("norm")) { // norm FLOAT
      if (! *(++arg)) error("norm: missing parameter");
      normalize_image((image*)SP_1, atof(*arg));
    }
    else
    if (ARG_IS("quit")) exit(0); // quit
    else
    if (ARG_IS("rot")) { // rot +-90, 180, 270
      if (! *(++arg)) error("rot: missing parameter");
      push(rotate_image((image*)SP_1, atof(*arg)));
      swap(); pop();
    }
    else
    if (ARG_IS("skew")) { // skew FLOAT
      if (! *(++arg)) error("skew: missing parameter");
      skew((image*)SP_1, atof(*arg));
    }
    else
    if (ARG_IS("splitx")) { // splitx FLOAT
      if (! *(++arg)) error("splitx: missing parameter");
      push(0); swap();
      push(0); swap();
      splitx_image(&SP_2, &SP_3, (image*)SP_1, atof(*arg));
      pop();
    }
    else
    if (ARG_IS("splity")) { // splity FLOAT
      if (! *(++arg)) error("splity: missing parameter");
      push(0); swap();
      push(0); swap();
      splity_image(&SP_2, &SP_3, (image*)SP_1, atof(*arg));
      pop();
    }
    else
    if (ARG_IS("thr")) { // thr
      v = threshold_histogram((image*)SP_1);
      push(v);
      printf("thr: %d\n", index_of_max(v));
    }
    else
    if (ARG_IS("w")) { // w FILENAME
      if (! *(++arg)) error("w: missing parameter");
      p = pop();
      f = fopen(*arg, "wb");
      if (IS_IMAGE(p)) write_image(p, f);
      else
      if (IS_VECTOR(p)) write_vector(p, f);
    }
    else {
    if (ARG_IS("-")) { // -
      push(read_image(stdin, 0));
    }
    else
      push(read_image(fopen(*arg, "rb"), 0)); // FILENAME
    }
  }
  if (sp > 0) {
    p = pop();
    f = stdout;
    if (IS_IMAGE(p)) write_image(p, f);
    else
    if (IS_VECTOR(p)) write_vector(p, f);
  }
}

// vim: sw=2 ts=2 sts=2 expandtab:
