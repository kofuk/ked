// This file is part of `ked', licensed under the GPLv3 or later.
#ifndef TERMINAL_H
#define TERMINAL_H

#include <stddef.h>

void term_set_up(void);
void term_tear_down(void);

extern size_t term_width;
extern size_t term_height;

extern unsigned int cursor_x;
extern unsigned int cursor_y;

void tputc(int);
void tputs(char *);
int tgetc(void);
size_t append_to_line(char*);
void flush_line(void);
void new_line(void);
void esc_write(char*);

void move_cursor(unsigned int, unsigned int);

#endif
