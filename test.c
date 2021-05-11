#include "common.h"

int main(int argc, char **args) {
  char *input= args[1];
  //int n= atoi(args[2]);
  char *output= args[2];

  image *im= image_read(input);
  image *im2= image_copy(im);
  laplacian(im2, -0.25);
  fill_selection(im, NAN, 128, 128, 128);
  solve_poisson(im, im2, -0.25, 0, 0.01);
  //diff_image(im2, im);
  //contrast_image(im2, 116, 140);
  write_image(im, output);
}
// vim: sw=2 ts=2 sts=2 expandtab:
