#ifndef SHARK_RENDER_H
#define SHARK_RENDER_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    RENDER_BARS,
    RENDER_BALL,
} render_mode;

typedef struct {
    unsigned rows;
    unsigned cols;
    size_t bar_width;
    size_t bar_spacing;
    size_t num_bars;
    bool gradient;
    unsigned color;
    unsigned grad_lo;
    unsigned grad_hi;
    render_mode mode;
    double ball_amp;
    size_t x_off;
    unsigned char *prev;
} renderer_t;

void renderer_init(renderer_t *r, unsigned rows, unsigned cols, size_t bar_width,
                   size_t bar_spacing, size_t num_bars, bool gradient);
void renderer_resize(renderer_t *r, unsigned rows, unsigned cols, size_t num_bars);
void renderer_set_offset(renderer_t *r, size_t x_off);
void renderer_set_mode(renderer_t *r, render_mode m);
render_mode renderer_mode_parse(const char *name);
void renderer_clear(renderer_t *r);
void renderer_free(renderer_t *r);
void renderer_draw(renderer_t *r, const double *values, char *out, size_t *out_len,
                   size_t cap);
void renderer_draw_stereo(renderer_t *r, const double *left, const double *right,
                          size_t per_ch_l, char *out, size_t *out_len, size_t cap);

#endif
