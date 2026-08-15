#ifndef SHRK_SETTINGS_H
#define SHRK_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>

#include "config.h"

enum {
    CH_LAYOUT = 1 << 0,
    CH_DSP    = 1 << 1,
    CH_AUDIO  = 1 << 2,
};

typedef struct settings_ui settings_ui;

settings_ui *settings_new(void);
void settings_free(settings_ui *s);
/* edit cfg according to key; set bits in *changed for applied settings */
void settings_key(settings_ui *s, srk_config *cfg, int key, unsigned *changed);
/* render the settings panel into the left panel_width columns (absolute rows) */
void settings_draw(const settings_ui *s, const srk_config *cfg, char *out,
                   size_t *out_len, size_t cap, unsigned rows, int panel_width);

#endif
