#ifndef UI_H
#define UI_H

#include <stddef.h>

#include "buffer.h"
#include "terminal.h"

void editor_main_loop(void);
void exit_editor(void);

void ui_set_up(void);

void write_message(char *);

void redraw_editor(void);

void move_cursor_editor(unsigned int, unsigned int);

void init_system_buffers(void);

extern Buffer *current_buffer;
extern Buffer **displayed_buffers;

enum BufferPosition { BUF_HEADER, BUF_MAIN, BUF_FOOTER };

void set_buffer(Buffer *, enum BufferPosition);
void select_buffer(Buffer *);

extern char **display_buffer;

#include "terminal.h"

#define DRAW_CHAR(c, x, y)                                                     \
    do {                                                                       \
        if (x > term_width || y > term_height ||                               \
            display_buffer[y - 1][x - 1] == c)                                 \
            break;                                                             \
                                                                               \
        move_cursor(x, y);                                                     \
        tputc(c);                                                              \
        display_buffer[y - 1][x - 1] = c;                                      \
    } while (0);

#endif
