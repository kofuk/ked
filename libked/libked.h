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

#ifndef LIBKED_H
#define LIBKED_H

#include <pthread.h>
#include <stdio.h>

#include <ked/buffer.h>
#include <ked/rune.h>

// face.c

/* Finds face associated with the name. */
const char *face_lookup(const char *name);

// io.c

/* Reads up to given length of file and create an array of AttrRune. Length
 * pointer will be updated to represent length of array of AttrRune. */
AttrRune *create_content_buffer(FILE *, size_t *, size_t, enum LineEnding *);

/* Saves buffer as UTF-8 text file. */
int save_buffer_utf8(Buffer *);

// ui.c

/* Aquire lock for diplay buffer. */
void display_buffer_lock(void);

void display_buffer_unlock(void);


// utilities.c

/* converts unsigned int to string. */
char *itoa(unsigned int);

char *cstr_dup(const char *str);

// terminal.c

/* Write 1 byte to the terminal. */
void tputc(int);
/* Put specified character in printable form. */
void tputc_printable(unsigned char);
void tputrune(Rune);
void tputs(const char *);
void tput(const char *buf, size_t len);
/* Reads 1 byte from stdin and return the value casting to int. */
int tgetc(void);
/* Writes escape sequence to the terminal. */
void esc_write(char *);

void move_cursor(unsigned int, unsigned int);

/* Set terminal graphic mode to specified value. */
void set_graphic_attrs(unsigned int text, unsigned int fg, unsigned int bg);

/* Reset terminal graphic mode. */
void reset_graphic_attrs(void);

/* Flushes terminal IO buffer. */
void term_flush_buffer(void);

// extension.c

/* Loads library from specified path and executes initialize routine if it
 * exists. */
int load_extension(const char *);

// rune.c

void char_write_printable(unsigned char);

/* Write the rune in printable form. */
void rune_write_printable( Rune);

#endif
