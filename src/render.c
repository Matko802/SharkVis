#include "render.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const GLYPHS[9] = {
    " ",
    "\xe2\x96\x81", /* ▁ */
    "\xe2\x96\x82", /* ▂ */
    "\xe2\x96\x83", /* ▃ */
    "\xe2\x96\x84", /* ▄ */
    "\xe2\x96\x85", /* ▅ */
    "\xe2\x96\x86", /* ▆ */
    "\xe2\x96\x87", /* ▇ */
    "\xe2\x96\x88", /* █ */
};

static const char *color_for(unsigned from_bottom, unsigned rows) {
    unsigned third = rows / 3;
    if (third == 0)
        return "\x1b[32m";
    if (from_bottom >= rows - third)
        return "\x1b[32m";
    if (from_bottom >= rows - 2 * third)
        return "\x1b[33m";
    return "\x1b[31m";
}

void renderer_init(renderer_t *r, unsigned rows, unsigned cols, size_t bar_width,
                   size_t bar_spacing, size_t num_bars, bool gradient) {
    r->rows = rows;
    r->cols = cols;
    r->bar_width = bar_width ? bar_width : 1;
    r->bar_spacing = bar_spacing;
    r->num_bars = num_bars;
    r->gradient = gradient;
    r->x_off = 0;
    r->prev = malloc((size_t)rows * cols);
    memset(r->prev, 0xFF, (size_t)rows * cols);
}

void renderer_resize(renderer_t *r, unsigned rows, unsigned cols, size_t num_bars) {
    free(r->prev);
    r->rows = rows;
    r->cols = cols;
    r->num_bars = num_bars;
    r->prev = malloc((size_t)rows * cols);
    memset(r->prev, 0xFF, (size_t)rows * cols);
}

void renderer_set_offset(renderer_t *r, size_t x_off) {
    if (r->x_off == x_off)
        return;
    r->x_off = x_off;
    free(r->prev);
    r->prev = malloc((size_t)r->rows * r->cols);
    memset(r->prev, 0xFF, (size_t)r->rows * r->cols);
}

void renderer_clear(renderer_t *r) {
    if (r->prev)
        memset(r->prev, 0xFF, (size_t)r->rows * r->cols);
}

void renderer_free(renderer_t *r) {
    free(r->prev);
    r->prev = NULL;
}

static void draw_bars(renderer_t *r, const double *left, const double *right,
                      size_t nbars, size_t per_ch_l, size_t x_start,
                      size_t region_w, char *out, size_t *out_len, size_t cap) {
    unsigned rows = r->rows;
    size_t cols = r->cols;
    if (rows == 0 || region_w == 0)
        return;

    size_t bw = r->bar_width ? r->bar_width : 1;
    size_t step = bw + r->bar_spacing;
    if (step == 0)
        step = 1;

    for (size_t b = 0; b < nbars; b++) {
        const double *src;
        size_t vi;
        if (b < per_ch_l) {
            src = left;
            vi = per_ch_l - 1 - b; /* left half is mirrored toward centre */
        } else {
            src = right;
            vi = b - per_ch_l;
        }
        double v = src[vi];
        if (!(v > 0.0))
            v = 0.0;
        else if (v > 1.0)
            v = 1.0;
        double h = v * (double)rows;
        if (h < 1.0)
            h = 0.1;

        size_t base = b * step;
        if (base >= region_w)
            break;

        for (size_t w = 0; w < bw; w++) {
            size_t col = x_start + base + w;
            if (col >= cols || col >= x_start + region_w)
                break;
            for (unsigned y = 0; y < rows; y++) {
                unsigned fb = rows - 1 - y;
                double frac = h - (double)fb;
                if (!(frac > 0.0))
                    frac = 0.0;
                else if (frac > 1.0)
                    frac = 1.0;
                int gi = (int)(frac * 8.0 + 0.9999);
                if (gi < 0)
                    gi = 0;
                if (gi > 8)
                    gi = 8;

                size_t idx = (size_t)y * cols + col;
                if (gi == r->prev[idx])
                    continue;
                r->prev[idx] = (unsigned char)gi;

                const char *color = r->gradient ? color_for(fb, rows) : "\x1b[37m";
                int n = snprintf(out + *out_len, cap - *out_len,
                                 "\x1b[%u;%zuH%s%s\x1b[0m", y + 1, col + 1, color,
                                 GLYPHS[gi]);
                if (n > 0)
                    *out_len += (size_t)n;
            }
        }
    }

    for (size_t col = 0; col < region_w; col++) {
        size_t base = col / step;
        bool in_bar = base < nbars && col - base * step < bw;
        if (in_bar)
            continue;
        size_t abs_col = x_start + col;
        for (unsigned y = 0; y < rows; y++) {
            size_t idx = (size_t)y * cols + abs_col;
            if (r->prev[idx] == 0xFF)
                continue;
            r->prev[idx] = 0;
            int n = snprintf(out + *out_len, cap - *out_len, "\x1b[%u;%zuH ",
                             y + 1, abs_col + 1);
            if (n > 0)
                *out_len += (size_t)n;
        }
    }
}

void renderer_draw(renderer_t *r, const double *values, char *out, size_t *out_len,
                   size_t cap) {
    size_t region = r->cols - r->x_off;
    if (region == 0)
        return;
    draw_bars(r, values, NULL, r->num_bars, r->num_bars, r->x_off, region, out,
              out_len, cap);
}

void renderer_draw_stereo(renderer_t *r, const double *left, const double *right,
                          size_t per_ch_l, char *out, size_t *out_len, size_t cap) {
    size_t region = r->cols - r->x_off;
    if (region == 0)
        return;
    draw_bars(r, left, right, r->num_bars, per_ch_l, r->x_off, region, out,
              out_len, cap);
}
