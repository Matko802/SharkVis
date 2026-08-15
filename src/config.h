#ifndef SHRK_CONFIG_H
#define SHRK_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    size_t bars;
    size_t bar_width;
    size_t bar_spacing;
    unsigned framerate;
    double sensitivity;
    bool autosens;
    unsigned lower_cutoff;
    unsigned higher_cutoff;
    double noise_reduction;
    char *source;
    unsigned sample_rate;
    unsigned channels;
    bool gradient;
} srk_config;

void config_default(srk_config *c);
/* returns false if the file could not be read; defaults kept otherwise */
bool config_load(srk_config *c, const char *path);
char *config_default_path(void);
void config_free(srk_config *c);

#endif
