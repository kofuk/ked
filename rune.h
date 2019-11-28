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

#ifndef KED_RUNE_H
#define KED_RUNE_H

#include <string.h>

typedef unsigned char Rune[4];

/* Hold a unicode character with its display attributes. */
typedef struct {
    /* Holds unicode one character. */
    unsigned char c[4];
    /* Width of the character when drawn on terminals */
    unsigned char display_width;
    /* Terminal decoration attributes. */
    int attrs;
} AttrRune;

#define RUNE_FONT_ATTR(rune) (rune->attrs & 0xff)
#define RUNE_FG_ATTR(rune) ((rune->attrs >> 8) & 0xff)
#define RUNE_BF_ATTR(rune) ((rune->attrs >> 16) & 0xff)

/* Check if given rune is ASCII \n. */
static inline int rune_is_lf(AttrRune rune) {
    return rune.c[0] == '\n' && rune.c[1] == 0 && rune.c[2] == 0 &&
           rune.c[3] == 0;
}

/* Compares two AttrRune's. If r1 == r2, returns 1 otherwise returns 0. */
static inline int rune_eq(AttrRune r1, AttrRune r2) {
    return r1.c[0] == r2.c[0] && r1.c[1] == r2.c[1] && r1.c[2] == r2.c[2] &&
           r1.c[3] == r2.c[3] && r1.attrs == r2.attrs;
}

typedef struct {
    Rune *str;
    size_t len;
} String;

/* Converts given char array to array of Rune. */
String *string_create(const char *);

/* Frees geven array of Rune. */
void string_destruct(String *);

/* Set AttrRune.display_width for each given AttrRune. */
void attr_runes_set_width(AttrRune *, size_t);

void char_write_printable(int, unsigned char);

/* Write the rune to the file discriptor in printable form. */
void rune_write_printable(int, Rune);

/* Set AttrRune.display_width. */
void attr_rune_set_width(AttrRune *r);

#endif
