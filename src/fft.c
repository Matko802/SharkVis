#include "fft.h"

#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void fft_init(fft_t *f, size_t n) {
    size_t bits = 0;
    while (((size_t)1 << bits) < n)
        bits++;

    f->n = n;
    f->bits = bits;
    f->rev = malloc(n * sizeof *f->rev);
    f->cos = malloc((n / 2) * sizeof *f->cos);
    f->sin = malloc((n / 2) * sizeof *f->sin);
    f->re = malloc(n * sizeof *f->re);
    f->im = malloc(n * sizeof *f->im);

    for (size_t i = 0; i < n; i++) {
        size_t v = 0, x = i;
        for (size_t b = 0; b < bits; b++) {
            v = (v << 1) | (x & 1);
            x >>= 1;
        }
        f->rev[i] = v;
    }
    for (size_t k = 0; k < n / 2; k++) {
        double a = -2.0 * M_PI * (double)k / (double)n;
        f->cos[k] = cos(a);
        f->sin[k] = sin(a);
    }
}

void fft_free(fft_t *f) {
    free(f->rev);
    free(f->cos);
    free(f->sin);
    free(f->re);
    free(f->im);
    f->rev = NULL;
    f->cos = NULL;
    f->sin = NULL;
    f->re = NULL;
    f->im = NULL;
}

void fft_process(fft_t *f, const double *input, double *out_mag) {
    size_t n = f->n;
    for (size_t i = 0; i < n; i++) {
        size_t r = f->rev[i];
        f->re[r] = input[i];
        f->im[r] = 0.0;
    }
    for (size_t size = 2; size <= n; size <<= 1) {
        size_t half = size / 2;
        size_t step = n / size;
        for (size_t i = 0; i < n; i += size) {
            for (size_t j = 0; j < half; j++) {
                size_t k = j * step;
                double tre = f->cos[k] * f->re[i + j + half] -
                             f->sin[k] * f->im[i + j + half];
                double tim = f->cos[k] * f->im[i + j + half] +
                             f->sin[k] * f->re[i + j + half];
                double ure = f->re[i + j];
                double uim = f->im[i + j];
                f->re[i + j] = ure + tre;
                f->im[i + j] = uim + tim;
                f->re[i + j + half] = ure - tre;
                f->im[i + j + half] = uim - tim;
            }
        }
    }
    for (size_t b = 0; b <= n / 2; b++) {
        double r = f->re[b], im = f->im[b];
        out_mag[b] = sqrt(r * r + im * im);
    }
}
