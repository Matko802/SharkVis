#ifndef SHARK_TERM_H
#define SHARK_TERM_H

#include <stdbool.h>

enum {
    KEY_NONE = -1,
    KEY_ESC = 0x1b,
    KEY_UP = 0x1001,
    KEY_DOWN = 0x1002,
    KEY_LEFT = 0x1003,
    KEY_RIGHT = 0x1004,
    KEY_ENTER = 0x1005,
};

bool term_winsize(int fd, unsigned *rows, unsigned *cols);
bool term_raw_enter(int fd);
void term_raw_restore(int fd);
/* returns a byte value, a KEY_* code, or KEY_NONE */
int term_read_key(int fd);

#endif
