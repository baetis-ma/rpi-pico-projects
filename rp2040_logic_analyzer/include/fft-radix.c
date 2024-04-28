/*
  This code was written by [Robin Scheibler](http://www.robinscheibler.org) during rainy days in October 2017.
*/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "fft.h"

#define TWO_PI 6.28318530

fft_config_t *fft_init(int size, fft_type_t type, fft_direction_t direction, float *input, float *output)
//fft_config_t *fft_init(int size, fft_type_t type, fft_direction_t direction, fixp15 *input, fixp15 *output)
{
  //Prepare an FFT of correct size and types.
  int k,m;

  fft_config_t *config = (fft_config_t *)malloc(sizeof(fft_config_t));

  // Check if the size is a power of two
  if ((size & (size-1)) != 0)  // tests if size is a power of two
    return NULL;

  // start configuration
  config->flags = 0;
  config->type = type;
  config->direction = direction;
  config->size = size;

  // Allocate and precompute twiddle factors
  config->twiddle_factors = (float *)malloc(2 * config->size * sizeof(float));
  //config->twiddle_factors = (fixp15 *)malloc(2 * config->size * sizeof(fixp15));

  float two_pi_by_n = TWO_PI / config->size;

  for (k = 0, m = 0 ; k < config->size ; k++, m+=2)
  {
    //config->twiddle_factors[m] =   float2fisp15(cosf(two_pi_by_n * k));    // real
    //config->twiddle_factors[m+1] = float2fisp15(sinf(two_pi_by_n * k));  // imag
    config->twiddle_factors[m] =   cosf(two_pi_by_n * k);    // real
    config->twiddle_factors[m+1] = sinf(two_pi_by_n * k);  // imag
  }

  // Allocate input buffer
  if (input != NULL)
    config->input = input;
  //else 
  //{
  //  if (config->type == FFT_REAL)
  //    config->input = (float *)malloc(config->size * sizeof(float));
  //  else if (config->type == FFT_COMPLEX)
  //    config->input = (float *)malloc(2 * config->size * sizeof(float));
  //
  //  config->flags |= FFT_OWN_INPUT_MEM;
  //}

  if (config->input == NULL)
    return NULL;

  // Allocate output buffer
  if (output != NULL)
    config->output = output;
  //else
  //{
  //  if (config->type == FFT_REAL)
  //    config->output = (float *)malloc(config->size * sizeof(float));
  //  else if (config->type == FFT_COMPLEX)
  //    config->output = (float *)malloc(2 * config->size * sizeof(float));
  //
  //  config->flags |= FFT_OWN_OUTPUT_MEM;
  //}

  if (config->output == NULL) return NULL;
  return config;
}

void fft_destroy(fft_config_t *config)
{
  if (config->flags & FFT_OWN_INPUT_MEM) free(config->input);
  if (config->flags & FFT_OWN_OUTPUT_MEM) free(config->output);
  free(config->twiddle_factors);
  free(config);
}

void fft_execute(fft_config_t *config)
{
    fft(config->input, config->output, config->twiddle_factors, config->size);
}

void fft(float *input, float *output, float *twiddle_factors, int n)
//void fft(fixp15 *input, fixp15 *output, fixp15 *twiddle_factors, int n)
{
  /*
   * Forward fast Fourier transform
   * DIT, radix-2, out-of-place implementation
   *
   * Parameters
   * ----------
   *  input (float *)
   *    The input array containing the complex samples with
   *    real/imaginary parts interleaved [Re(x0), Im(x0), ..., Re(x_n-1), Im(x_n-1)]
   *  output (float *)
   *    The output array containing the complex samples with
   *    real/imaginary parts interleaved [Re(x0), Im(x0), ..., Re(x_n-1), Im(x_n-1)]
   *  n (int)
   *    The FFT size, should be a power of 2
   */

  split_radix_fft(input, output, n, 2, twiddle_factors, 2);
}

void split_radix_fft(float *x, float *y, int n, int stride, float *twiddle_factors, int tw_stride)
//void split_radix_fft(fixp15 *x, fixp15 *y, int n, int stride, fixp15 *twiddle_factors, int tw_stride)
{
  /*
   * This code will compute the FFT of the input vector x
   *
   * The input data is assumed to be real/imag interleaved
   *
   * The size n should be a power of two
   *
   * y is an output buffer of size 2n to accomodate for complex numbers
   *
   * Forward fast Fourier transform
   * Split-Radix
   * DIT, radix-2, out-of-place implementation
   *
   * For a complex FFT, call first stage as:
   * fft(x, y, n, 2, 2);
   *
   * Parameters
   * ----------
   *  x (float *)
   *    The input array containing the complex samples with
   *    real/imaginary parts interleaved [Re(x0), Im(x0), ..., Re(x_n-1), Im(x_n-1)]
   *  y (float *)
   *    The output array containing the complex samples with
   *    real/imaginary parts interleaved [Re(x0), Im(x0), ..., Re(x_n-1), Im(x_n-1)]
   *  n (int)
   *    The FFT size, should be a power of 2
   *  stride (int)
   *    The number of elements to skip between two successive samples
   *  twiddle_factors (float *)
   *    The array of twiddle factors
   *  tw_stride (int)
   *    The number of elements to skip between two successive twiddle factors
   */
  int k;

  // End condition, stop at n=2 to avoid one trivial recursion
  if (n == 8) { fft8(x, stride, y, 2); return; }
  else if (n == 4) { fft4(x, stride, y, 2); return; }

  // Recursion -- Decimation In Time algorithm
  split_radix_fft(x, y, n / 2, 2 * stride, twiddle_factors, 2 * tw_stride);
  split_radix_fft(x + stride, y + n, n / 4, 4 * stride, twiddle_factors, 4 * tw_stride);
  split_radix_fft(x + 3 * stride, y + n + n / 2, n / 4, 4 * stride, twiddle_factors, 4 * tw_stride);

  // Stitch together the output
  //fixp15 u1r, u1i, u2r, u2i, x1r, x1i, x2r, x2i;
  //fixp15 t;
  float u1r, u1i, u2r, u2i, x1r, x1i, x2r, x2i;
  float t;

  // We can save a few multiplications in the first step
  u1r = y[0];
  u1i = y[1];
  u2r = y[n / 2];
  u2i = y[n / 2 + 1];

  x1r = y[n];
  x1i = y[n + 1];
  x2r = y[n / 2 + n];
  x2i = y[n / 2 + n + 1];

  t = x1r + x2r;
  y[0] = u1r + t;
  y[n]     = u1r - t;

  t = x1i + x2i;
  y[1] = u1i + t;
  y[n + 1] = u1i - t;

  t = x2i - x1i;
  y[n / 2]     = u2r - t;
  y[n + n / 2]     = u2r + t;

  t = x1r - x2r;
  y[n / 2 + 1] = u2i - t;
  y[n + n / 2 + 1] = u2i + t;

  for (k = 1 ; k < n / 4 ; k++)
  {
    float u1r, u1i, u2r, u2i, x1r, x1i, x2r, x2i, c1, s1, c2, s2;
    //fix15 u1r, u1i, u2r, u2i, x1r, x1i, x2r, x2i, c1, s1, c2, s2;
    c1 = twiddle_factors[k * tw_stride];
    s1 = twiddle_factors[k * tw_stride + 1];
    c2 = twiddle_factors[3 * k * tw_stride];
    s2 = twiddle_factors[3 * k * tw_stride + 1];

    u1r = y[2 * k];
    u1i = y[2 * k + 1];
    u2r = y[2 * k + n / 2];
    u2i = y[2 * k + n / 2 + 1];

    //non integer math
    //x1r = multifixp15(  c1, y[n + 2 * k])         + multificp15( s1, y[n + 2 * k + 1]);
    //x1i = multifixp15( -s1, y[n + 2 * k])         + multificp15( c1, y[n + 2 * k + 1]);
    //x2r = multifixp15(  c2, y[n / 2 + n + 2 * k]) + multificp15( s2, y[n / 2 + n + 2 * k + 1]);
    //x2i = multifixp15( -s2, y[n / 2 + n + 2 * k]) + multificp15( c2, y[n / 2 + n + 2 * k + 1]);
    x1r =  c1 * y[n + 2 * k] + s1 * y[n + 2 * k + 1];
    x1i = -s1 * y[n + 2 * k] + c1 * y[n + 2 * k + 1];
    x2r =  c2 * y[n / 2 + n + 2 * k] + s2 * y[n / 2 + n + 2 * k + 1];
    x2i = -s2 * y[n / 2 + n + 2 * k] + c2 * y[n / 2 + n + 2 * k + 1];

    t = x1r + x2r;
    y[2 * k]     = u1r + t;
    y[2 * k + n]     = u1r - t;

    t = x1i + x2i;
    y[2 * k + 1] = u1i + t;
    y[2 * k + n + 1] = u1i - t;

    t = x2i - x1i;
    y[2 * k + n / 2]     = u2r - t;
    y[2 * k + n + n / 2]     = u2r + t;

    t = x1r - x2r;
    y[2 * k + n / 2 + 1] = u2i - t;
    y[2 * k + n + n / 2 + 1] = u2i + t;
  }

}

inline void fft8(float *input, int stride_in, float *output, int stride_out)
//inline void fft8(fixp15 *input, int stride_in, fixp15 *output, int stride_out)
{
  /*
   * Unrolled implementation of FFT8 for a little more performance
   */
  float a0r, a1r, a2r, a3r, a4r, a5r, a6r, a7r;
  float a0i, a1i, a2i, a3i, a4i, a5i, a6i, a7i;
  float b0r, b1r, b2r, b3r, b4r, b5r, b6r, b7r;
  float b0i, b1i, b2i, b3i, b4i, b5i, b6i, b7i;
  float t;
  float sin_pi_4 = 0.7071067812;

  a0r = input[0];
  a0i = input[1];
  a1r = input[stride_in];
  a1i = input[stride_in+1];
  a2r = input[2*stride_in];
  a2i = input[2*stride_in+1];
  a3r = input[3*stride_in];
  a3i = input[3*stride_in+1];
  a4r = input[4*stride_in];
  a4i = input[4*stride_in+1];
  a5r = input[5*stride_in];
  a5i = input[5*stride_in+1];
  a6r = input[6*stride_in];
  a6i = input[6*stride_in+1];
  a7r = input[7*stride_in];
  a7i = input[7*stride_in+1];

  // Stage 1

  b0r = a0r + a4r;
  b0i = a0i + a4i;

  b1r = a1r + a5r;
  b1i = a1i + a5i;

  b2r = a2r + a6r;
  b2i = a2i + a6i;

  b3r = a3r + a7r;
  b3i = a3i + a7i;

  b4r = a0r - a4r;
  b4i = a0i - a4i;

  b5r = a1r - a5r;
  b5i = a1i - a5i;
  // W_8^1 = 1/sqrt(2) - j / sqrt(2)
  t = b5r + b5i;
    //non integer math
  //b5i = multifixp15( (b5i - b5r), sin_pi_4);
  //b5r = multifixp15( t, sin_pi_4);
  b5i = (b5i - b5r) * sin_pi_4;
  b5r = t * sin_pi_4;

  // W_8^2 = -j
  b6r = a2i - a6i;
  b6i = a6r - a2r;

  b7r = a3r - a7r;
  b7i = a3i - a7i;
  // W_8^3 = -1 / sqrt(2) + j / sqrt(2)
    //non integer math
  //t = multifixp15(sin_pi_4, (b7i - b7r));
  //b7i = multifixp15( - (b7r + b7i), sin_pi_4);
  t = sin_pi_4 * (b7i - b7r);
  b7i = - (b7r + b7i) * sin_pi_4;
  b7r = t;

  // Stage 2

  a0r = b0r + b2r;
  a0i = b0i + b2i;

  a1r = b1r + b3r;
  a1i = b1i + b3i;

  a2r = b0r - b2r;
  a2i = b0i - b2i;

  // * j
  a3r = b1i - b3i;
  a3i = b3r - b1r;

  a4r = b4r + b6r;
  a4i = b4i + b6i;

  a5r = b5r + b7r;
  a5i = b5i + b7i;

  a6r = b4r - b6r;
  a6i = b4i - b6i;

  // * j
  a7r = b5i - b7i;
  a7i = b7r - b5r;

  // Stage 3

  // X[0]
  output[0] = a0r + a1r;
  output[1] = a0i + a1i;

  // X[4]
  output[4*stride_out] = a0r - a1r;
  output[4*stride_out+1] = a0i - a1i;

  // X[2]
  output[2*stride_out] = a2r + a3r;
  output[2*stride_out+1] = a2i + a3i;

  // X[6]
  output[6*stride_out] = a2r - a3r;
  output[6*stride_out+1] = a2i - a3i;

  // X[1]
  output[stride_out] = a4r + a5r;
  output[stride_out+1] = a4i + a5i;

  // X[5]
  output[5*stride_out] = a4r - a5r;
  output[5*stride_out+1] = a4i - a5i;

  // X[3]
  output[3*stride_out] = a6r + a7r;
  output[3*stride_out+1] = a6i + a7i;

  // X[7]
  output[7*stride_out] = a6r - a7r;
  output[7*stride_out+1] = a6i - a7i;

}

inline void fft4(float *input, int stride_in, float *output, int stride_out)
//inline void fft4(fixp15 *input, int stride_in, fixp15 *output, int stride_out)
{
  /*
   * Unrolled implementation of FFT4 for a little more performance
   */
  float t1, t2;
  //fixp15 t1, t2;

  t1 = input[0] + input[2*stride_in];
  t2 = input[stride_in] + input[3*stride_in];
  output[0] = t1 + t2;
  output[2*stride_out] = t1 - t2;

  t1 = input[1] + input[2*stride_in+1];
  t2 = input[stride_in+1] + input[3*stride_in+1];
  output[1] = t1 + t2;
  output[2*stride_out+1] = t1 - t2;

  t1 = input[0] - input[2*stride_in];
  t2 = input[stride_in+1] - input[3*stride_in+1];
  output[stride_out] = t1 + t2;
  output[3*stride_out] = t1 - t2;

  t1 = input[1] - input[2*stride_in+1];
  t2 = input[3*stride_in] - input[stride_in];
  output[stride_out+1] = t1 + t2;
  output[3*stride_out+1] = t1 - t2;
}
