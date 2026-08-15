#ifndef SHARK_FFT_H
#define SHARK_FFT_H

#include <stddef.h>

typedef struct {
    size_t n;
    size_t bits;
    size_t *rev;
    double *cos;
    double *sin;
    double *re;
    double *im;
} fft_t;

/* n must be a power of two */
void fft_init(fft_t *fft, size_t n);
void fft_free(fft_t *fft);
/* out_mag must have at least n/2 + 1 slots; fills bins 0..=n/2 with |X[k]| */
void fft_process(fft_t *fft, const double *input, double *out_mag);

#endif
