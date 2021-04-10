// INCLUDE'S //
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <math.h>
#include <string.h>


// DEFINE'S //

#define uchar unsigned char
#define uint unsigned int
#define ulong unsigned long int
#define real float
#define gray float

#define ASSERT(cond) assert(cond)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// TYPES //

typedef struct { // image
  char magic;
  gray *chan[5];
  uint width;
  uint height;
  int depth;
  real ex; // height of x in pixels
  uint pag;
  real black;
  real graythr;
  real white;
  real area;
  real thickness;
} image;

#define ALPHA chan[0]
#define SEL chan[4]

typedef struct { // vector
  char magic;
  real *data;
  int step;
  uint len;
  uint size;
} vector;


// PROTOTYPES //

// convolution.c
void convolve_3x3(image *im, real a, real b, real c, real d);
void deconvolve_3x1(image *im, real a, real b, real c, int border);
void deconvolve_1x3(image *im, real a, real b, real c, int border);
image *deconvolve_3x3(image *im, real a, real b, real c, real d, int steps, float maxerr);
void image_laplace(image *im, real k);
void image_poisson(image *target, image *guess, real k, int steps, float maxerr);

// draw.c
void draw_grid(image *im, int stepx, int stepy);
void image_sel_fill(image *im, real v0, real v1, real v2, real v3);
void poke(image *im, int x, int y, int chan, gray v);

// image.c
extern real default_ex;
image *image_make(int width, int height, int depth);
void image_destroy(image *im);
image *copy_image(image *im);
image *image_read(char *fname);
void image_write(image *im, char *fname);

// misc.c
void error(const char *msg);
void error1(const char *msg, const char *param);
image *image_background(image *im);
void divide_image(image *a, image *b);
vector *histogram_of_image(image *im, int chan);
void contrast_image(image *im, real black, real white);
void normalize_image(image *im, real strength);
void mean_y(image *im, uint d);
void darker_image(image *a, image *b);
void calc_statistics(image *im, int verbose);
void diff_image(image *a, image *b);
void patch_image(image *a, image *b);
void image_quantize(image *im, float step);
void image_dither(image *im, int step, int border);

// scale.c
image *image_double(image *im, real k /* hardness */);
image *image_half_x(image *im);
image *image_half_y(image *im);
image *image_half(image *im);
image *image_redouble_x(image *im, int odd);
image *image_redouble_y(image *im, int odd);
image *image_redouble(image *im, int oddx, int oddy);

// select.c
void image_alpha_to_sel(image *im);
void image_sel_make(image *im, gray v);
void image_sel_rect(image *im, real mode, int x0, int y0, int w, int h);
void image_sel_fill(image *im, real v0, real v1, real v2, real v3);

// transform.c
image *rotate_90_image(image *im, int angle);
image *rotate_image(image *im, float angle);
void splitx_image(void **out1, void **out2, image *im, float x);
void splity_image(void **out1, void **out2, image *im, float y);
image *crop(image *im, int x1, int y1, int x2, int y2);
void skew(image* im, real angle);
real detect_skew(image *im);
void shearx_image(image *im, real t);
void sheary_image(image *im, real t);

// vector.c
vector *make_vector(uint size);
void clear_vector(vector *v);
void destroy_vector(vector *h);
vector *copy_vector(vector *v0);
void import_vector(vector *v, gray *data, int len, int step);
void export_vector(vector *v, gray *data, int len, int step);
void write_vector(vector *v, FILE *f);
void cumul_vector(vector *v);
void diff_vector(vector *v, uint d);
real *solve_tridiagonal(real *a, real *b, real *c, int n);
void poisson_vector(vector *target, vector *nlap);
uint index_of_max(vector *v);

// vim: sw=2 ts=2 sts=2 expandtab:
