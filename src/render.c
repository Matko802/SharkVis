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

static void bar_color(const renderer_t *r, unsigned from_bottom, unsigned rows,
                      char *buf, size_t n) {
    unsigned cr = (r->color >> 16) & 0xff, cg = (r->color >> 8) & 0xff,
             cb = r->color & 0xff;
    if (r->gradient) {
        long lo_r = (r->grad_lo >> 16) & 0xff, lo_g = (r->grad_lo >> 8) & 0xff,
             lo_b = r->grad_lo & 0xff;
        long hi_r = (r->grad_hi >> 16) & 0xff, hi_g = (r->grad_hi >> 8) & 0xff,
             hi_b = r->grad_hi & 0xff;
        double t = rows > 1 ? (double)from_bottom / (double)(rows - 1) : 0.0;
        cr = (unsigned)(lo_r + (hi_r - lo_r) * t + 0.5);
        cg = (unsigned)(lo_g + (hi_g - lo_g) * t + 0.5);
        cb = (unsigned)(lo_b + (hi_b - lo_b) * t + 0.5);
        if (cr > 255) cr = 255;
        if (cg > 255) cg = 255;
        if (cb > 255) cb = 255;
    }
    snprintf(buf, n, "\x1b[38;2;%u;%u;%um", cr, cg, cb);
}

void renderer_init(renderer_t *r, unsigned rows, unsigned cols, size_t bar_width,
                   size_t bar_spacing, size_t num_bars, bool gradient) {
    r->rows = rows;
    r->cols = cols;
    r->bar_width = bar_width ? bar_width : 1;
    r->bar_spacing = bar_spacing;
    r->num_bars = num_bars;
    r->gradient = gradient;
    r->color = 0xffffffu;
    r->grad_lo = 0xff0000u;
    r->grad_hi = 0x00ff00u;
    r->mode = RENDER_BARS;
    r->x_off = 0;
    r->wave_buf = NULL;
    r->wave_cap = 0;
    r->wave_pos = 0;
    r->wave_filled = 0;
    r->wave_spc = 1;
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

void renderer_set_mode(renderer_t *r, render_mode m) {
    if (r->mode == m)
        return;
    r->mode = m;
    renderer_clear(r);
}

render_mode renderer_mode_parse(const char *name) {
    if (name && strcmp(name, "wave") == 0)
        return RENDER_WAVE;
    return RENDER_BARS;
}

void renderer_set_wave(renderer_t *r, unsigned sample_rate) {
    size_t cap = sample_rate ? (size_t)sample_rate : 48000;
    if (cap < 4096)
        cap = 4096;
    size_t spc = sample_rate / 2000;
    if (spc < 1)
        spc = 1;
    if (r->wave_buf && r->wave_cap == cap) {
        r->wave_spc = spc;
        return;
    }
    free(r->wave_buf);
    r->wave_buf = calloc(cap, sizeof *r->wave_buf);
    r->wave_cap = cap;
    r->wave_pos = 0;
    r->wave_filled = 0;
    r->wave_spc = spc;
}

void renderer_feed(renderer_t *r, const double *left, const double *right,
                   size_t n) {
    if (!r->wave_buf || r->wave_cap == 0 || n == 0)
        return;
    for (size_t i = 0; i < n; i++) {
        double v = left ? left[i] : 0.0;
        if (right)
            v = (v + right[i]) * 0.5;
        r->wave_buf[r->wave_pos] = v;
        r->wave_pos = (r->wave_pos + 1) % r->wave_cap;
        if (r->wave_filled < r->wave_cap)
            r->wave_filled++;
    }
}

void renderer_clear(renderer_t *r) {
    if (r->prev)
        memset(r->prev, 0xFF, (size_t)r->rows * r->cols);
}

void renderer_free(renderer_t *r) {
    free(r->prev);
    r->prev = NULL;
    free(r->wave_buf);
    r->wave_buf = NULL;
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

                char colr[40];
                bar_color(r, fb, rows, colr, sizeof colr);
                int n = snprintf(out + *out_len, cap - *out_len,
                                 "\x1b[%u;%zuH%s%s\x1b[0m", y + 1, col + 1, colr,
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

static void draw_cell(renderer_t *r, unsigned y, size_t x, int gi, char *out,
                      size_t *out_len, size_t cap) {
    size_t idx = (size_t)y * r->cols + x;
    if ((unsigned char)gi == r->prev[idx])
        return;
    r->prev[idx] = (unsigned char)gi;
    if (gi == 0) {
        int n = snprintf(out + *out_len, cap - *out_len, "\x1b[%u;%zuH ",
                         y + 1, x + 1);
        if (n > 0)
            *out_len += (size_t)n;
    } else {
        char colr[40];
        bar_color(r, r->rows - 1 - y, r->rows, colr, sizeof colr);
        int n = snprintf(out + *out_len, cap - *out_len,
                         "\x1b[%u;%zuH%s%s\x1b[0m", y + 1, x + 1, colr,
                         GLYPHS[gi]);
        if (n > 0)
            *out_len += (size_t)n;
    }
}

static void draw_wave(renderer_t *r, size_t x_start, size_t region_w,
                      char *out, size_t *out_len, size_t cap) {
    if (!r->wave_buf || r->rows < 3 || region_w == 0)
        return;
    long yrow[4096];
    size_t ncol = region_w < 4096 ? region_w : 4096;
    if (ncol == 0)
        return;

    size_t spc = r->wave_spc ? r->wave_spc : 1;
    double center = (double)(r->rows - 1) * 0.5;
    double height = (double)(r->rows - 2) * 0.5;

    for (size_t c = 0; c < ncol; c++) {
        size_t off = (region_w - 1 - c) * spc;
        if (off >= r->wave_filled) {
            yrow[c] = -1;
            continue;
        }
        size_t idx = (r->wave_pos + r->wave_cap - 1 - off) % r->wave_cap;
        double v = r->wave_buf[idx];
        if (v < -1.0)
            v = -1.0;
        else if (v > 1.0)
            v = 1.0;
        yrow[c] = (long)(center - v * height + 0.5);
    }

    for (size_t c = 0; c < ncol; c++) {
        size_t x = x_start + c;
        long cur = yrow[c];
        if (cur < 0) {
            for (unsigned y = 0; y < r->rows; y++)
                draw_cell(r, y, x, 0, out, out_len, cap);
            continue;
        }
        long lo = cur, hi = cur;
        if (c + 1 < ncol && yrow[c + 1] >= 0) {
            long nxt = yrow[c + 1];
            if (nxt < lo)
                lo = nxt;
            if (nxt > hi)
                hi = nxt;
        }
        for (unsigned y = 0; y < (unsigned)lo; y++)
            draw_cell(r, y, x, 0, out, out_len, cap);
        for (long y = lo; y <= hi; y++)
            draw_cell(r, (unsigned)y, x, 8, out, out_len, cap);
        for (unsigned y = (unsigned)hi + 1; y < r->rows; y++)
            draw_cell(r, y, x, 0, out, out_len, cap);
    }
}

void renderer_draw(renderer_t *r, const double *values, char *out, size_t *out_len,
                   size_t cap) {
    size_t region = r->cols - r->x_off;
    if (region == 0)
        return;
    if (r->mode == RENDER_WAVE)
        draw_wave(r, r->x_off, region, out, out_len, cap);
    else
        draw_bars(r, values, NULL, r->num_bars, r->num_bars, r->x_off, region, out,
                  out_len, cap);
}

void renderer_draw_stereo(renderer_t *r, const double *left, const double *right,
                          size_t per_ch_l, char *out, size_t *out_len, size_t cap) {
    size_t region = r->cols - r->x_off;
    if (region == 0)
        return;
    if (r->mode == RENDER_WAVE)
        draw_wave(r, r->x_off, region, out, out_len, cap);
    else
        draw_bars(r, left, right, r->num_bars, per_ch_l, r->x_off, region, out,
                  out_len, cap);
}
