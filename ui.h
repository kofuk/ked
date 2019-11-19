#ifndef UI_H
#define UI_H

#include <stddef.h>

void editor_main_loop(void);
void exit_editor(void);

void ui_set_up(void);

void redraw_editor(void);

void move_cursor_editor(unsigned int, unsigned int);

extern char **display_buffer;
extern size_t display_width;
extern size_t display_height;

#include "terminal.h"

#define DRAW_CHAR(c, x, y)                                              \
    do                                                                  \
    {                                                                   \
        if (x > display_width || y > display_height                     \
            || display_buffer[y - 1][x - 1] == c) break;                \
                                                                        \
        move_cursor_editor(x, y);                                       \
        tputc(c);                                                       \
        display_buffer[y - 1][x - 1] = c;                               \
    } while(0);

#endif
