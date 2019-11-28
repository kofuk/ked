#ifndef RUNE_H
#define RUNE_H

#include <string.h>

typedef char Rune[4];

/* Hold a unicode character with its display attributes. */
typedef struct {
    /* Holds unicode one character. */
    char c[4];
    /* Width of the character when drawn on terminals */
    char display_width;
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
    return memcmp(&r1, &r2, sizeof(AttrRune)) == 0;
}

typedef struct {
    Rune *str;
    size_t len;
} String;

/* Converts given char array to array of Rune. */
String *string_create(const char *);

/* Frees geven array of Rune. */
void string_destruct(String *);

#endif
