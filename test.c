#include "common.h"

int main(int argc, char **args) {
  char *input= args[1];
  //int n= atoi(args[2]);
  char *output= args[2];

  image *im= image_read(input);
  image *im2= image_copy(im),
        *im3= image_copy(im);
  laplacian(im, -0.25);
  //dither_floyd_bidir(im, 1);
  //dither_blue_noise(im, 1);
  dither_cumulative(im, 1);
  solve_poisson(im, im2, -0.25, 0, 0.001);
  diff_image(im2, im3);
  contrast_image(im2, 116, 140);
  write_image(im2, output);
}
// vim: sw=2 ts=2 sts=2 expandtab:
