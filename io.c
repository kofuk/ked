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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "rune.h"

/* Converts given buffer's line ending character to lf, and returnes converted
 * buffer. If returned buffer and given buffer is not equal, this function frees
 * old buffer. */
static char *convert_to_lf(char *buf, size_t *len, size_t n_crlf) {
    /* If there's no CRLF, it's not needed to allocate new buffer. */
    if (n_crlf == 0) {
        /* Simply replace \r with \n. */
        for (size_t i = 0; i < *len; ++i) {
            if (buf[i] == '\r') buf[i] = '\n';
        }

        return buf;
    }

    size_t orig_len = *len;
    *len -= n_crlf;
    char *result = malloc(sizeof(char) * (*len));

    size_t buf_pos = 0;
    char prev_cr = 0;
    for (size_t i = 0; i < orig_len; ++i) {
        switch (buf[i]) {
        case '\n':
            if (!prev_cr) {
                result[buf_pos] = buf[i];
                prev_cr = 0;
                ++buf_pos;
            }
            break;
        case '\r':
            result[buf_pos] = '\n';
            prev_cr = 1;
            ++buf_pos;
            break;
        default:
            result[buf_pos] = buf[i];
            prev_cr = 0;
            ++buf_pos;
            break;
        }
    }

    free(buf);

    return result;
}

/* Converts char array to an array of AttrRune. */
static AttrRune *convert_to_rune_array(char *buf, size_t *len,
                                       size_t gap_size) {
    size_t n_rune = 0;
    for (size_t i = 0; i < *len; ++i) {
        if (buf[i] >> 7 == 0 || (buf[i] >> 6 & 0b11) == 0b11) ++n_rune;
    }

    AttrRune *result = malloc(sizeof(AttrRune) * (n_rune + gap_size));
    memset(result, 0, sizeof(AttrRune) * n_rune);

    size_t res_i = gap_size;
    char rune_buf[4];
    memset(rune_buf, 0, sizeof(rune_buf));
    int rune_i = 0;
    for (size_t i = 0; i < *len; ++i) {
        if (buf[i] >> 7 == 0 || (buf[i] >> 6 & 0b11) == 0b11) {
            if (rune_i != 0) {
                // FIXME: check if the rune is valid or not.
                memcpy(result[res_i].c, rune_buf, sizeof(rune_buf));
                memset(rune_buf, 0, sizeof(rune_buf));
                rune_i = 0;
                ++res_i;
            }

            rune_buf[rune_i] = buf[i];
            ++rune_i;
        } else if (1 <= rune_i && rune_i < (int)sizeof(rune_buf) &&
                   (buf[i] >> 6 & 0b11) == 0b10) {
            rune_buf[rune_i] = buf[i];
            ++rune_i;
        } else {
            // FIXME: broken utf-8 buffer.
        }
    }
    if (rune_i != 0) memcpy(result[res_i].c, rune_buf, sizeof(rune_buf));

    *len = n_rune + gap_size;

    return result;
}

/* Reads file f, and returns array of AttrRune. Original line ending is saved to
 * lend. */
AttrRune *create_content_buffer(FILE *f, size_t *len, size_t gap_size,
                                enum LineEnding *lend) {
    char *tmp_buf = malloc(sizeof(char) * (*len));
    size_t buf_pos = 0;
    size_t read_size = (*len) > 0xffff000 ? 0xffff000 : (*len);
    unsigned long nread;
    while ((nread = fread(tmp_buf + buf_pos, 1, read_size, f)) > 0)
        buf_pos += (size_t)nread;

    size_t nlf = 0, ncr = 0, ncrlf = 0;
    int prev_cr = 0;
    for (size_t i = 0; i < (*len); i++) {
        switch (tmp_buf[i]) {
        case '\n':
            if (prev_cr) {
                ++ncrlf;
                prev_cr = 0;
            } else {
                ++nlf;
            }
            break;
        case '\r':
            if (prev_cr) ++ncr;
            prev_cr = 1;
            break;
        default:
            if (prev_cr) {
                ++ncr;
                prev_cr = 0;
            }
            break;
        }
    }
    if (prev_cr) ++ncr;

    tmp_buf = convert_to_lf(tmp_buf, len, ncrlf);
    AttrRune *result = convert_to_rune_array(tmp_buf, len, gap_size);

    if (ncr > nlf && ncr > ncrlf)
        *lend = LEND_CR;
    else if (ncrlf > nlf && ncrlf > ncr)
        *lend = LEND_CRLF;
    else
        *lend = LEND_LF;

    free(tmp_buf);

    return result;
}

#define WRITE_OUT(file, buf, counter, c)             \
    do {                                             \
        if (counter >= 4096) {                       \
            if (fwrite(buf, 1, 4096, file) < 4096) { \
                fclose(file);                        \
                return 0;                            \
            }                                        \
            counter = 0;                             \
        }                                            \
        buf[counter] = c;                            \
        ++counter;                                   \
    } while (0)

int save_buffer_utf8(Buffer *buf) {
    if (buf->path == NULL) return 0;

    FILE *f = fopen(buf->path, "w");
    if (f == NULL) return 0;

    char b[4096];
    size_t i = 0;
    for (size_t p = 0; p < buf->buf_size - (buf->gap_end - buf->gap_start);
         ++p) {
        AttrRune r = buffer_get_rune(buf, p);
        if (rune_is_lf(r) && buf->line_ending != LEND_LF) {
            switch (buf->line_ending) {
            case LEND_CR:
                WRITE_OUT(f, b, i, '\r');
                break;
            case LEND_CRLF:
                WRITE_OUT(f, b, i, '\r');
                WRITE_OUT(f, b, i, '\n');
                break;
            default:
                break;
            }
        } else {
            WRITE_OUT(f, b, i, r.c[0]);
            for (int j = 0; i < 4; ++i) {
                if (r.c[j] >> 6 != 0b11) break;
                WRITE_OUT(f, b, i, r.c[j]);
            }
        }
    }
    if (fwrite(b, 1, i, f) < i) {
        fclose(f);

        return 0;
    }

    fclose(f);

    return 1;
}
