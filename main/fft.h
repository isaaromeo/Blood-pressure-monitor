#ifndef _FFT_H_
#define _FFT_H_

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


typedef float real;
typedef struct{real Re; real Im;} complex;

#ifndef PI
#define PI	3.14159265358979323846264338327950288
#endif


void print_vector(const char *title, complex *x, int n);
void print_magnitude_vector(const char *title, complex *x, int n);
void fft( complex *v, int n, complex *tmp );
void ifft( complex *v, int n, complex *tmp );
float get_highest_harmonic(complex *v, int n, float fs);

#endif // _FFT_H_
