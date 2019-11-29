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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ked/rune.h>

String *string_create(const char *str) {
    size_t n_rune = 0;
    size_t len = strlen(str);
    for (size_t i = 0; i < len; ++i) {
        if (str[i] >> 7 == 0 || (str[i] >> 6 & 0b11) == 0b11) ++n_rune;
    }

    Rune *ra = malloc(sizeof(Rune) * n_rune);
    size_t ra_i = 0;
    char rune_buf[4];
    size_t buf_i = 0;
    memset(rune_buf, 0, sizeof(rune_buf));
    for (size_t i = 0; i < len; ++i) {
        if (str[i] >> 7 == 0 || (str[i] >> 6 & 0b11) == 0b11) {
            if (buf_i != 0) {
                memcpy(ra + ra_i, rune_buf, sizeof(rune_buf));
                memset(rune_buf, 0, sizeof(rune_buf));
                buf_i = 0;
                ++ra_i;
            }
            rune_buf[buf_i] = str[i];
            ++buf_i;
        } else if (buf_i != 0 && (str[i] >> 6 & 0b11) == 0b10) {
            if (buf_i >= 4) {
                free(ra);

                return NULL;
            }

            rune_buf[buf_i] = str[i];
            ++buf_i;
        } else {
            free(ra);

            return NULL;
        }
    }
    if (buf_i != 0) {
        memcpy(ra + ra_i, rune_buf, sizeof(rune_buf));
    }

    String *result = malloc(sizeof(String));
    result->str = ra;
    result->len = n_rune;

    return result;
}

void string_destruct(String *this) {
    free(this->str);
    free(this);
}

void attr_runes_set_width(AttrRune *r, size_t len) {
    // FIXME: temporary implementation
    for (size_t i = 0; i < len; ++i)
        attr_rune_set_width(r + i);
}

void attr_rune_set_width(AttrRune *r) {
    if ((r->c[0] >> 7 & 1) == 0) {
        unsigned char c = r->c[0];

        if (c == '\t')
            r->display_width = 8;
        else if (c <= 0x1f)
            r->display_width = 2;
        else
            r->display_width = 1;
    } else
        r->display_width = 2;
}

void char_write_printable(int fd, unsigned char c) {
    if (c == '\t')
        write(fd, "        ", 8);
    else if (c <= 0x1f) {
        char buf[2] = {'^', (char)(c + '@')};
        write(fd, buf, 2);
    } else
        write(fd, &c, 1);
}

void rune_write_printable(int fd, Rune r) {
    if ((r[0] >> 7 & 1) == 0) {
        char_write_printable(fd, r[0]);
    } else {
        size_t len = 1;
        for (; len < 4; ++len)
            if ((r[len] >> 6 & 0b11) != 0b10) break;

        write(fd, r, len);
    }
}
