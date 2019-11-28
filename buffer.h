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

#ifndef KED_BUFFER_H
#define KED_BUFFER_H

#include <stddef.h>

#include "rune.h"

/* Amont of buffer allocate once.  */
#define INIT_GAP_SIZE 1024
/* Allocates additional buffer when gap is smaller than MIN_GAP_SIZE. */
#define MIN_GAP_SIZE 32

enum LineEnding { LEND_LF, LEND_CR, LEND_CRLF };

/* Buffer struct defines editing buffer for each file. every edit event take
 * place on buffer, rather than UI. */
typedef struct {
    /* Buffer name to be displayed. */
    char *buf_name;
    /* Buffer file path to be saved. */
    char *path;
    /* The content of buffer includes gap. */
    AttrRune *content;
    /* Cursor position in this buffer excluding gap area. */
    size_t point;
    /* Buffer size including gap. */
    size_t buf_size;
    /* Start index of gap in this buffer. Inclusive. */
    size_t gap_start;
    /* End index of gap in this buffer. Exclusive */
    size_t gap_end;
    /* Line ending character for this file. */
    enum LineEnding line_ending;
    /* Beginning of display area that this buffer can be displayed. Inclusive.
     */
    size_t display_range_y_start;
    /* End of display area that this buffer can be displayed. Exclusive. */
    size_t display_range_y_end;
    /* Whether this buffer is modified of not. */
    int modified;
    /* Cursor X position in display area. */
    unsigned int cursor_x;
    /* Cursor Y position in display area. */
    unsigned int cursor_y;
} Buffer;

/* Creates buffer for the path, specified in 1st argument, with name of 2nd
 * argument. If file is not exisiting, ked creates the file when saved. */
Buffer *buffer_create(const char *, const char *);
/* Creates system buffer with name of 2nd argument. */
Buffer *buffer_create_system(const char *);
/* Frees the buffer. */
void buffer_destruct(Buffer *);

/* Move cursor forward of backward in specified size. */
void buffer_cursor_move(Buffer *, size_t, int);

/* Move cursor forward in 1 line. */
void buffer_cursor_forward_line(Buffer *);

/* Insertes character to buffer point position. */
void buffer_insert(Buffer *, Rune);

static inline void buffer_insert_char(Buffer *this, unsigned char c) {
    Rune r = {c, 0, 0, 0};
    buffer_insert(this, r);
}

/* Deletes 1 character backward. */
void buffer_delete_backward(Buffer *);
/* Deletes 1 character forward. */
void buffer_delete_forward(Buffer *);

/* Saves buffer to buffer file path. On success, this returns 1, and on failure,
 * returns 0. */
int buffer_save(Buffer *);

/* Returnes AttrRune on specified point. */
static inline AttrRune buffer_get_rune(const Buffer *buf, const size_t point) {
    return buf->gap_start <= point
               ? buf->content[point + (buf->gap_end - buf->gap_start)]
               : buf->content[point];
}

#endif
