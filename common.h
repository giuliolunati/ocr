#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#define uchar unsigned char
#define uint unsigned int
#define ulong unsigned long int
#define real float

#define MAXSHORT 32767

// used for encoding in read/write_image
#define KP 32
// MAXVAL= 128 * KP - 1
#define MAXVAL 4095
// KS= 128 * (MAXSHORT+1)/ (MAXSHORT-MAXVAL)
#define KS 146.28571428571428
// KSKP= KS * KP
#define KSKP 4681.142857142857

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// TYPES -- alphabetical order //

typedef struct { // image
  char magic;
  short *channel[4];
  uint width;
  uint height;
  int depth;
  real ex; // height of x in pixels
  uint pag;
  real black;
  real gray;
  real white;
  real area;
  real thickness;
} image;

typedef struct { // vector
  char magic;
  real *data;
  uint len;
  uint size;
  real x0;
  real dx;
} vector;

// PROTOTYPES -- alphabetical order //

image *autocrop(image *im, int width, int height);
void calc_statistics(image *im, int verbose);
void clear_vector(vector *v);
void contrast_image(image *im, real black, real white);
vector *copy_vector(vector *v0);
image *crop(image *im, int x1, int y1, int x2, int y2);
void cumul_vector(vector *v);
void darker_image(image *a, image *b);
real default_ex;
void destroy_image(image *im);
void destroy_vector(vector *h);
real detect_skew(image *im);
void diff_vector(vector *v, uint d);
void divide_image(image *a, image *b);
image *double_size(image *im, real k /*hardness*/);
void draw_grid(image *im, int stepx, int stepy);
void ensure_init_srgb();
void ensure_init_sigma();
void error(const char *msg);
void error1(const char *msg, const char *param);
void fill_image(image *im, real v);
void export_vector(vector *v, short *data, int len, int step);
image *half_size(image *im);
vector *histogram_of_image(image *im, int chan);
image *image_background(image *im);
void import_vector(vector *v, short *data, int len, int step);
uint index_of_max(vector *v);
image *make_image(int width, int height, int depth);
vector *make_vector(uint size);
void mean_y(image *im, uint d);
void normalize_image(image *im, real strength);
image *n_laplacian(image *im);
void poisson_image(image *im, image *nlap);
image *read_image(FILE *file, int sigma);
image *rotate_90_image(image *im, int angle);
image *rotate_image(image *im, float angle);
void shearx_image(image *im, real t);
void sheary_image(image *im, real t);
void skew(image* im, real angle);
void splitx_image(void **out1, void **out2, image *im, float x);
void splity_image(void **out1, void **out2, image *im, float y);
short *srgb_to_lin;
void write_image(image *im, FILE *file, int sigma);
void write_vector(vector *v, FILE *f);

// vim: sw=2 ts=2 sts=2 expandtab:
