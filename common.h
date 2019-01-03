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
#define K 16
#define K_2 8 // K/2
#define MAXVAL 4095 // 256 * K - 1
#define MAXSHORT 32767

// TYPES -- alphabetical order //

typedef struct { // image
  char type;
  short *data;
  uint width;
  uint height;
  real ex; // height of x in pixels
  uint pag;
} image;

typedef struct { // vector
  char type;
  real *data;
  uint len;
  uint size;
  real x0;
  real dx;
} vector;

// PROTOTYPES -- alphabetical order //

image *autocrop(image *im, int width, int height);
void contrast_image(image *im, real black, real white);
vector *copy_vector(vector *v0);
image *crop(image *im, int x1, int y1, int x2, int y2);
void cumul_vector(vector *v);
real default_ex;
void destroy_image(image *im);
void destroy_vector(vector *h);
real detect_skew(image *im);
void diff_vector(vector *v, uint d);
void divide_image(image *a, image *b);
image *double_size(image *im, real k /*hardness*/);
void draw_grid(image *im, int stepx, int stepy);
void error1(const char *msg, const char *param);
void error(const char *msg);
vector *histogram_of_image(image *im);
image *image_background(image *im);
void *image_from_srgb(image *im);
uint index_of_max(vector *v);
void init_srgb();
short lin_from_srgb[256];
image *make_image(int width, int height);
vector *make_vector(uint size);
void mean_y(image *im, uint d);
void normalize_image(image *im, real strength);
image *read_image(FILE *file, int layer);
image *rotate_90_image(image *im, int angle);
image *rotate_image(image *im, float angle);
void shearx_image(image *im, real t);
void sheary_image(image *im, real t);
void skew(image* im, real angle);
void splitx_image(void **out1, void **out2, image *im, float x);
void splity_image(void **out1, void **out2, image *im, float y);
vector *threshold_histogram(image *im);
int write_image(image *im, FILE *file);
void write_vector(vector *v, FILE *f);

// vim: sw=2 ts=2 sts=2 expandtab:
