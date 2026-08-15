#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "audio.h"
#include "config.h"
#include "dsp.h"
#include "render.h"
#include "settings.h"
#include "term.h"

static volatile sig_atomic_t g_sig = 0;
static volatile sig_atomic_t g_resize = 0;

static void on_signal(int sig) {
    (void)sig;
    g_sig = 1;
}

static void on_winch(int sig) {
    (void)sig;
    g_resize = 1;
}

static void usage(void) {
    printf("usage: sharkvis [-p config_file]\n");
    printf("  g - settings, q - quit\n");
}

static int panel_width_for(unsigned cols) {
    int pw = (int)(cols / 3);
    if (pw < 28)
        pw = 28;
    if (pw > 44)
        pw = 44;
    if (pw >= cols)
        pw = cols > 2 ? (int)cols / 2 : 1;
    if (pw < 1)
        pw = 1;
    return pw;
}

/* apply cfg to the running dsp/renderer/audio; x_off is the bar-area offset
 * (panel width while settings are open, 0 after closing) */
static void apply_settings(dsp_t *dsp, renderer_t *rnd, audio_t *audio,
                           srk_config *cfg, size_t *bars, double **heights,
                           unsigned rows, unsigned cols, unsigned chmask,
                           bool audio_reinit, size_t x_off) {
    size_t new_bars = cfg->bars
        ? cfg->bars
        : (size_t)(cols / (cfg->bar_width + cfg->bar_spacing));
    if (new_bars < 1)
        new_bars = 1;

    if ((chmask & (CH_DSP | CH_AUDIO)) || new_bars != *bars) {
        double saved_sens = dsp->sens;
        bool saved_sens_init = dsp->sens_init;
        dsp_free(dsp);
        dsp_init(dsp, new_bars, cfg->sample_rate, cfg->autosens,
                 cfg->noise_reduction, cfg->lower_cutoff, cfg->higher_cutoff);
        dsp->sens = saved_sens;
        dsp->sens_init = saved_sens_init;
    }

    if (new_bars != *bars) {
        free(*heights);
        *heights = malloc(new_bars * sizeof **heights);
        *bars = new_bars;
        renderer_resize(rnd, rows, cols, new_bars);
    }

    rnd->bar_width = cfg->bar_width;
    rnd->bar_spacing = cfg->bar_spacing;
    rnd->gradient = cfg->gradient;
    renderer_set_offset(rnd, x_off);
    renderer_clear(rnd);

    if (audio_reinit) {
        audio_stop(audio);
        audio_init(audio, dsp->input_buffer_size);
        audio_start(audio, cfg->source, cfg->sample_rate, cfg->channels);
    }
}

int main(int argc, char **argv) {
    const char *cfgpath = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            cfgpath = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage();
            return 0;
        } else {
            fprintf(stderr, "SharkVis: unknown option '%s'\n", argv[i]);
            usage();
            return 1;
        }
    }

    srk_config cfg;
    config_default(&cfg);

    char *save_path = NULL;
    if (cfgpath) {
        save_path = strdup(cfgpath);
        if (!config_load(&cfg, save_path)) {
            fprintf(stderr, "SharkVis: error loading config %s\n", save_path);
            free(save_path);
            config_free(&cfg);
            return 1;
        }
    } else {
        save_path = config_default_path();
        if (access(save_path, F_OK) == 0 && !config_load(&cfg, save_path))
            fprintf(stderr, "SharkVis: error loading config %s, using defaults\n",
                    save_path);
    }

    if (cfg.bar_width < 1)
        cfg.bar_width = 1;
    if (cfg.framerate < 1)
        cfg.framerate = 1;
    if (cfg.framerate > 240)
        cfg.framerate = 240;
    if (cfg.sensitivity < 0.1)
        cfg.sensitivity = 0.1;
    if (cfg.noise_reduction < 0.0)
        cfg.noise_reduction = 0.0;
    if (cfg.noise_reduction > 1.0)
        cfg.noise_reduction = 1.0;
    if (cfg.lower_cutoff < 1)
        cfg.lower_cutoff = 1;
    if (cfg.higher_cutoff < cfg.lower_cutoff)
        cfg.higher_cutoff = cfg.lower_cutoff + 1;

    unsigned rows, cols;
    if (!term_winsize(1, &rows, &cols)) {
        rows = 24;
        cols = 80;
    }

    size_t bars = cfg.bars ? cfg.bars
                           : (size_t)(cols / (cfg.bar_width + cfg.bar_spacing));
    if (bars < 1)
        bars = 1;

    dsp_t dsp;
    dsp_init(&dsp, bars, cfg.sample_rate, cfg.autosens, cfg.noise_reduction,
             cfg.lower_cutoff, cfg.higher_cutoff);

    audio_t audio;
    audio_init(&audio, dsp.input_buffer_size);
    audio_start(&audio, cfg.source, cfg.sample_rate, cfg.channels);

    if (!term_raw_enter(0)) {
        fprintf(stderr, "SharkVis: not a terminal\n");
        audio_stop(&audio);
        dsp_free(&dsp);
        config_free(&cfg);
        return 1;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    struct sigaction win;
    memset(&win, 0, sizeof win);
    win.sa_handler = on_winch;
    sigaction(SIGWINCH, &win, NULL);

    printf("\x1b[2J\x1b[H\x1b[?25l");
    fflush(stdout);

    renderer_t rnd;
    renderer_init(&rnd, rows, cols, cfg.bar_width, cfg.bar_spacing, bars, cfg.gradient);

    double *heights = malloc(bars * sizeof *heights);
    char *out = malloc((size_t)1 << 20);

    settings_ui *st = settings_new();
    bool in_settings = false;
    bool clear_render = false;
    unsigned chmask = 0;

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    int rc = 0;
    while (!g_sig) {
        int key = term_read_key(0);

        if (in_settings) {
            if (key == 'g' || key == 'G' || key == KEY_ESC) {
                in_settings = false;
                printf("\x1b[2J\x1b[H");
                fflush(stdout);
                apply_settings(&dsp, &rnd, &audio, &cfg, &bars, &heights, rows,
                               cols, chmask, !!(chmask & CH_AUDIO), 0);
                chmask = 0;
                if (!config_save(&cfg, save_path))
                    fprintf(stderr, "SharkVis: could not save config to %s\n",
                            save_path);
            } else if (key == 'q' || key == 'Q' || key == 3) {
                break;
            } else {
                settings_key(st, &cfg, key, &chmask);
                if (chmask) {
                    apply_settings(&dsp, &rnd, &audio, &cfg, &bars, &heights,
                                   rows, cols, chmask, !!(chmask & CH_AUDIO),
                                   (size_t)panel_width_for(cols));
                    clear_render = true;
                    chmask = 0;
                }
            }
        } else {
            if (key == 'g' || key == 'G') {
                in_settings = true;
                chmask = 0;
                printf("\x1b[2J\x1b[H");
                fflush(stdout);
                renderer_set_offset(&rnd, (size_t)panel_width_for(cols));
            } else if (key == 'q' || key == 'Q' || key == 3) {
                break;
            }
        }

        if (g_resize) {
            g_resize = 0;
            unsigned nr, nc;
            if (term_winsize(1, &nr, &nc) && nr > 0 && nc > 0 &&
                (nr != rows || nc != cols)) {
                size_t new_bars = cfg.bars
                    ? cfg.bars
                    : (size_t)(nc / (cfg.bar_width + cfg.bar_spacing));
                if (new_bars < 1)
                    new_bars = 1;
                cols = nc;
                rows = nr;
                bars = new_bars;
                double saved_sens = dsp.sens;
                bool saved_sens_init = dsp.sens_init;
                dsp_free(&dsp);
                dsp_init(&dsp, bars, cfg.sample_rate, cfg.autosens,
                         cfg.noise_reduction, cfg.lower_cutoff, cfg.higher_cutoff);
                dsp.sens = saved_sens;
                dsp.sens_init = saved_sens_init;
                free(heights);
                heights = malloc(bars * sizeof *heights);
                renderer_resize(&rnd, rows, cols, bars);
                if (in_settings)
                    renderer_set_offset(&rnd, (size_t)panel_width_for(cols));
                printf("\x1b[2J\x1b[H");
                fflush(stdout);
            }
        }

        const double *samples = NULL;
        size_t n = audio_consume(&audio, &samples);
        if (n > 0)
            dsp_execute(&dsp, samples, n, heights);
        if (audio_failed(&audio)) {
            fprintf(stderr, "\nSharkVis: audio input failed: %s\n", audio_error(&audio));
            rc = 1;
            break;
        }

        double sens = cfg.sensitivity / 100.0;
        for (size_t i = 0; i < bars; i++)
            heights[i] *= sens;

        size_t olen = 0;
        if (clear_render) {
            memcpy(out, "\x1b[2J\x1b[H", 7);
            olen = 7;
            clear_render = false;
        }
        if (in_settings)
            settings_draw(st, &cfg, out, &olen, (size_t)1 << 20, rows,
                          panel_width_for(cols));
        renderer_draw(&rnd, heights, out, &olen, (size_t)1 << 20);
        if (olen) {
            fwrite(out, 1, olen, stdout);
            fflush(stdout);
        }

        long frame_ns = (long)(1e9 / (cfg.framerate ? cfg.framerate : 1));
        next.tv_nsec += frame_ns;
        next.tv_sec += next.tv_nsec / 1000000000L;
        next.tv_nsec %= 1000000000L;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    }

    if (!config_save(&cfg, save_path))
        fprintf(stderr, "SharkVis: could not save config to %s\n", save_path);

    printf("\x1b[?25h\x1b[0m\x1b[J");
    fflush(stdout);
    term_raw_restore(0);

    audio_stop(&audio);
    renderer_free(&rnd);
    dsp_free(&dsp);
    settings_free(st);
    free(heights);
    free(out);
    free(save_path);
    config_free(&cfg);
    return rc;
}
