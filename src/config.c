#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *trim(char *s) {
    while (isspace((unsigned char)*s))
        s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = '\0';
    return s;
}

static long geti(const char *v, long def) {
    errno = 0;
    char *end;
    long r = strtol(v, &end, 10);
    if (errno != 0 || end == v)
        return def;
    return r;
}

static double getf(const char *v, double def) {
    errno = 0;
    char *end;
    double r = strtod(v, &end);
    if (errno != 0 || end == v)
        return def;
    return r;
}

void config_default(srk_config *c) {
    c->bars = 0;
    c->bar_width = 2;
    c->bar_spacing = 1;
    c->framerate = 60;
    c->sensitivity = 100.0;
    c->autosens = true;
    c->lower_cutoff = 50;
    c->higher_cutoff = 8000;
    c->noise_reduction = 0.2;
    c->source = strdup("auto");
    c->sample_rate = 48000;
    c->channels = 2;
    c->gradient = false;
}

char *config_default_path(void) {
    const char *env = getenv("SHARKVIS_CONFIG");
    if (env && env[0])
        return strdup(env);
    const char *home = getenv("HOME");
    if (home) {
        size_t n = strlen(home);
        char *p = malloc(n + 24);
        snprintf(p, n + 24, "%s/.config/SharkVis/config", home);
        if (access(p, F_OK) == 0)
            return p;
        free(p);
    }
    if (access("config", F_OK) == 0)
        return strdup("config");
    if (home) {
        size_t n = strlen(home);
        char *p = malloc(n + 24);
        snprintf(p, n + 24, "%s/.config/SharkVis/config", home);
        return p;
    }
    return strdup("config");
}

void config_free(srk_config *c) {
    free(c->source);
}

static void mkdir_p(const char *path) {
    char tmp[1024];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

bool config_save(const srk_config *c, const char *path) {
    char dir[1024];
    snprintf(dir, sizeof dir, "%s", path);
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
        if (dir[0])
            mkdir_p(dir);
    }

    FILE *f = fopen(path, "w");
    if (!f)
        return false;

    fprintf(f, "[general]\n");
    fprintf(f, "bars = %zu\n", c->bars);
    fprintf(f, "bar_width = %zu\n", c->bar_width);
    fprintf(f, "bar_spacing = %zu\n", c->bar_spacing);
    fprintf(f, "framerate = %u\n", c->framerate);
    fprintf(f, "sensitivity = %.0f\n", c->sensitivity);
    fprintf(f, "autosens = %d\n", c->autosens ? 1 : 0);
    fprintf(f, "lower_cutoff_freq = %u\n", c->lower_cutoff);
    fprintf(f, "higher_cutoff_freq = %u\n", c->higher_cutoff);
    fprintf(f, "\n[smoothing]\n");
    fprintf(f, "noise_reduction = %.2f\n", c->noise_reduction);
    fprintf(f, "\n[input]\n");
    fprintf(f, "method = pulse\n");
    fprintf(f, "source = %s\n", c->source ? c->source : "auto");
    fprintf(f, "sample_rate = %u\n", c->sample_rate);
    fprintf(f, "channels = %u\n", c->channels);
    fprintf(f, "\n[color]\n");
    fprintf(f, "gradient = %d\n", c->gradient ? 1 : 0);

    return fclose(f) == 0;
}

bool config_load(srk_config *c, const char *path) {
    FILE *f = fopen(path, "r");
    if (!f)
        return false;

    char line[512];
    char section[64] = "general";
    while (fgets(line, sizeof line, f)) {
        char *s = trim(line);
        if (*s == '\0' || *s == ';' || *s == '#')
            continue;
        if (*s == '[') {
            char *end = strchr(s, ']');
            if (end)
                *end = '\0';
            snprintf(section, sizeof section, "%s", trim(s + 1));
            for (char *p = section; *p; p++)
                *p = (char)tolower((unsigned char)*p);
            continue;
        }
        char *eq = strchr(s, '=');
        if (!eq)
            continue;
        *eq = '\0';
        char *key = trim(s);
        char *val = trim(eq + 1);
        char *semi = strchr(val, ';');
        if (semi)
            *semi = '\0';
        val = trim(val);
        for (char *p = key; *p; p++)
            *p = (char)tolower((unsigned char)*p);

        if (strcmp(section, "general") == 0) {
            if (strcmp(key, "bars") == 0)
                c->bars = (size_t)geti(val, (long)c->bars);
            else if (strcmp(key, "bar_width") == 0)
                c->bar_width = (size_t)geti(val, (long)c->bar_width);
            else if (strcmp(key, "bar_spacing") == 0)
                c->bar_spacing = (size_t)geti(val, (long)c->bar_spacing);
            else if (strcmp(key, "framerate") == 0)
                c->framerate = (unsigned)geti(val, (long)c->framerate);
            else if (strcmp(key, "sensitivity") == 0)
                c->sensitivity = getf(val, c->sensitivity);
            else if (strcmp(key, "autosens") == 0)
                c->autosens = geti(val, 1) != 0;
            else if (strcmp(key, "lower_cutoff_freq") == 0)
                c->lower_cutoff = (unsigned)geti(val, (long)c->lower_cutoff);
            else if (strcmp(key, "higher_cutoff_freq") == 0)
                c->higher_cutoff = (unsigned)geti(val, (long)c->higher_cutoff);
        } else if (strcmp(section, "smoothing") == 0) {
            if (strcmp(key, "noise_reduction") == 0)
                c->noise_reduction = getf(val, c->noise_reduction);
        } else if (strcmp(section, "input") == 0) {
            if (strcmp(key, "method") == 0) {
                if (*val && strcmp(val, "pulse") != 0 && strcmp(val, "pipewire") != 0 &&
                    strcmp(val, "auto") != 0)
                    fprintf(stderr, "SharkVis: input method '%s' not supported, using pulse\n",
                            val);
            } else if (strcmp(key, "source") == 0) {
                free(c->source);
                c->source = strdup(val);
            } else if (strcmp(key, "sample_rate") == 0) {
                c->sample_rate = (unsigned)geti(val, (long)c->sample_rate);
            } else if (strcmp(key, "channels") == 0) {
                c->channels = (unsigned)geti(val, (long)c->channels);
            }
        } else if (strcmp(section, "color") == 0) {
            if (strcmp(key, "gradient") == 0)
                c->gradient = geti(val, 1) != 0;
        }
    }

    fclose(f);
    return true;
}
