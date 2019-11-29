/* ked -- simple text editor with minimal dependency */
/* Copyright (C) 2019  Koki Fukuda */

/* This program is free software: you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or */
/* (at your option) any later version. */

/* This program is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
/* GNU General Public License for more details. */

/* You should have received a copy of the GNU General Public License */
/* along with this program.  If not, see <https://www.gnu.org/licenses/>. */

#ifndef KED_TERMINAL_H
#define KED_TERMINAL_H

#include <stddef.h>

#include "rune.h"

extern size_t term_width;
extern size_t term_height;

/* Write 1 byte to the terminal. */
void tputc(int);
/* Put specified character in printable form. */
void tputc_printable(unsigned char);
void tputrune(Rune);
/* Reads 1 byte from stdin and return the value casting to int. */
int tgetc(void);
/* Writes escape sequence to the terminal. */
void esc_write(char *);

void move_cursor(unsigned int, unsigned int);

/* Send SIGSTOP to process. */
void stop_editor(void);

#endif
