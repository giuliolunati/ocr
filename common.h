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

typedef struct {
  char type;
  real *data;
  uint len;
  uint size;
  real x0;
  real dx;
} vector;

typedef struct {
  char type;
  short *data;
  uint width;
  uint height;
  real ex; // height of x in pixels
} image;

void error(const char *msg);
void error1(const char *msg, const char *param);
short lin_from_srgb[256];
void init_srgb();
vector *make_vector(uint size);
void destroy_vector(vector *h);
vector *copy_vector(vector *v0);
void cumul_vector(vector *v);
void diff_vector(vector *v, uint d);
void write_vector(vector *v, FILE *f);
uint index_of_max(vector *v);
real default_ex;
image *make_image(int width, int height);
void destroy_image(image *im);
image *read_image(FILE *file, int layer);
void *image_from_srgb(image *im);
int write_image(image *im, FILE *file);
image *rotate_90_image(image *im, int angle);
image *rotate_image(image *im, float angle);
void splitx_image(void **out1, void **out2, image *im, float x);
void splity_image(void **out1, void **out2, image *im, float y);
image *image_background(image *im);
void divide_image(image *a, image *b);
vector *histogram_of_image(image *im);
vector *threshold_histogram(image *im);
void contrast_image(image *im, real black, real white);
void normalize_image(image *im, real strength);
void shearx_image(image *im, real t);
void sheary_image(image *im, real t);
void skew(image* im, real angle);
void mean_y(image *im, uint d);
real detect_skew(image *im);
image *crop(image *im, int x1, int y1, int x2, int y2);
image *autocrop(image *im, int width, int height);

// vim: sw=2 ts=2 sts=2 expandtab:
