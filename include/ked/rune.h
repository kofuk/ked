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
    /* Rune attributes.
     * | MSB    LSB |
     * | protected  |
     * |------------|
     * | 1 bit      | */
    unsigned int attrs;
    /* Face name this Rune should use. */
    const char *face;
} AttrRune;

/* Whether the rune is write protected or not. */
#define RUNE_PROTECTED(attrs) (((attrs) >> 21) & 0x1)

/* Check if given rune is ASCII \n. */
static inline int rune_is_lf(AttrRune rune) {
    return rune.c[0] == '\n' && rune.c[1] == 0 && rune.c[2] == 0 &&
           rune.c[3] == 0;
}

/* Compares two AttrRune's. If r1 == r2, returns 1 otherwise returns 0. */
static inline int attr_rune_eq(AttrRune r1, AttrRune r2) {
    if ((r1.face == NULL && r2.face != NULL) ||
        (r1.face != NULL && r2.face == NULL))
        return 0;
    else if (r1.face == NULL && r2.face == NULL)
        return 1;
    else
        return r1.c[0] == r2.c[0] && r1.c[1] == r2.c[1] && r1.c[2] == r2.c[2] &&
               r1.c[3] == r2.c[3] && strcmp(r1.face, r2.face) == 0;
}

/* Compares two Rune's. If r1 -- r2, returns 1 otherwise returns 0. */
static inline int rune_eq(Rune r1, Rune r2) {
    return r1[0] == r2[0] && r1[1] == r2[1] && r1[2] == r2[2] && r1[3] == r2[3];
}

/* Returns whether to attrs are equal except of protected state. */
static inline int font_attr_eq(const char *face_name_1,
                               const char *face_name_2) {
    if ((face_name_1 == NULL && face_name_2 != NULL) ||
        (face_name_1 != NULL && face_name_2 == NULL)) {
        return 0;
    } else if (face_name_1 == NULL && face_name_2 == NULL)
        return 1;
    else
        return strcmp(face_name_1, face_name_2) == 0;
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
