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

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <ked/buffer.h>
#include <ked/keybind.h>
#include <ked/rune.h>
#include <ked/ui.h>
#include <ked/ked.h>

#include "ked/terminal.h"
#include "libked.h"
#include "terminal.h"
#include "utilities.h"

static int editor_exited;

Buffer *current_buffer;
Buffer **displayed_buffers;

static AttrRune **display_buffer;

static const char *current_face;

/* Draws char to the terminal if needed. */
static inline void ui_draw_char(unsigned char c, const char *face,
                                unsigned int x, unsigned int y) {
    if (x > term_width || y > term_height ||
        (display_buffer[y - 1][x - 1].c[0] == c &&
         font_attr_eq(face, display_buffer[y - 1][x - 1].face)) ||
        c == '\n')
        return;

    if (!font_attr_eq(face, current_face)) {
        tputs(face_lookup(face));

        current_face = face;
    }

    move_cursor(x, y);
    tputc_printable(c);
    display_buffer[y - 1][x - 1].c[0] = c;
    display_buffer[y - 1][x - 1].face = face;
    for (int i = 1; i < 4; ++i) {
        display_buffer[y - 1][x - 1].c[i] = 0;
    }
}

/* Draws AttrRune with its attrubutes to the termianl if needed. */
static inline void ui_draw_rune(AttrRune r, const char *default_face,
                                unsigned int x, unsigned int y) {
    const char *face = r.face;
    if (face == NULL) {
        face = default_face;
    }

    if (x > term_width || y > term_height ||
        (rune_eq(display_buffer[y - 1][x - 1].c, r.c) &&
         font_attr_eq(face, display_buffer[y - 1][x - 1].face)) ||
        r.c[0] == '\n')
        return;

    if (!font_attr_eq(r.face, current_face)) {
        tputs(face_lookup(r.face));

        current_face = r.face;
    }

    move_cursor(x, y);
    tputrune(r.c);
    memcpy(&(display_buffer[y - 1][x - 1]), &r, sizeof(AttrRune));
    display_buffer[y - 1][x - 1].face = face;
}

/* Make next drawing in the position to be redrawn. */
static inline void ui_invalidate_point(unsigned int x, unsigned int y) {
    if (x > term_width || y >= term_height) return;

    /* \n will never drawn. */
    display_buffer[y - 1][x - 1].c[0] = '\n';
}

static void (**buffer_change_listeners)(Buffer **, size_t);
static size_t buffer_change_listeners_i;
static size_t buffer_change_listeners_len;

void call_buffer_entry_change_listeners(void) {
    size_t n_buffer = 0;
    for (size_t i = 0; i < 3; ++i)
        if (displayed_buffers[i] != NULL) ++n_buffer;

    Buffer **bufs = malloc(sizeof(Buffer *) * n_buffer);
    size_t bufs_i = 0;
    for (size_t i = 0; i < 3; ++i) {
        if (displayed_buffers[i] != NULL) {
            bufs[bufs_i] = displayed_buffers[i];
            ++bufs_i;
        }
    }

    for (size_t i = 0; i < buffer_change_listeners_i; ++i)
        (buffer_change_listeners[i])(bufs, n_buffer);
}

void add_buffer_entry_change_listener(void (*func)(Buffer **, size_t)) {
    for (size_t i = 0; i < buffer_change_listeners_i; ++i)
        if (buffer_change_listeners[i] == func) return;

    if (buffer_change_listeners_i >= buffer_change_listeners_len) {
        if (buffer_change_listeners_len <= 64) {
            buffer_change_listeners_len = buffer_change_listeners_len << 2;
        } else {
            buffer_change_listeners_len += 64;
        }

        buffer_change_listeners =
            realloc(buffer_change_listeners, buffer_change_listeners_len);
    }

    buffer_change_listeners[buffer_change_listeners_i] = func;
    ++buffer_change_listeners_i;

    call_buffer_entry_change_listeners();
}

void remove_buffer_entry_change_listener(void (*func)(Buffer **, size_t)) {
    for (size_t i = 0; i < buffer_change_listeners_i; ++i) {
        if (func == buffer_change_listeners[i]) {
            memmove(buffer_change_listeners + i,
                    buffer_change_listeners + i + 1,
                    buffer_change_listeners_len - i - 1);
            --buffer_change_listeners_i;

            return;
        }
    }
}

void set_buffer(Buffer *buf, enum BufferPosition pos) {
    switch (pos) {
    case BUF_HEADER:
        buf->display_range_y_start = 1;
        buf->display_range_y_end = 2;

        displayed_buffers[0] = buf;

        break;
    case BUF_MAIN:
        buf->display_range_y_start = 2;
        buf->display_range_y_end = term_height;

        displayed_buffers[1] = buf;

        break;
    case BUF_FOOTER:
        buf->display_range_y_start = term_height;
        buf->display_range_y_end = term_height + 1;

        displayed_buffers[2] = buf;

        break;
    }

    call_buffer_entry_change_listeners();
}

void init_system_buffers(void) {
    Buffer *header = buffer_create_system("__system_header__");
    set_buffer(header, BUF_HEADER);

    Buffer *footer = buffer_create_system("__system_footer__");
    set_buffer(footer, BUF_FOOTER);
}

static void update_header(char *fname) {
    Buffer *header = displayed_buffers[0];

    if (fname == NULL) fname = "UNTITLED";

    String *fname_section = string_create(fname);
    for (size_t i = 0; i < fname_section->len; ++i)
        buffer_insert(header, fname_section->str[i]);
    string_destruct(fname_section);

    buffer_insert_char(header, ' ');

    String *progname_section = string_create("- KED");
    for (size_t i = 0; i < progname_section->len; i++)
        buffer_insert(header, progname_section->str[i]);
    string_destruct(progname_section);
}

void select_buffer(Buffer *buf) {
    current_buffer = buf;
    update_header(buf->buf_name);
}

void write_message(char *msg) {
    Buffer *footer = displayed_buffers[2];
    // TODO: Don't touch struct member here.
    footer->point = 0;
    footer->gap_start = 0;
    footer->gap_end = footer->buf_size;

    buffer_insert_char(footer, ' ');
    String *msg_str = string_create(msg);
    for (size_t i = 0; i < msg_str->len; ++i)
        buffer_insert(footer, msg_str->str[i]);
    string_destruct(msg_str);
}

void ui_set_up(void) {
    display_buffer = malloc(sizeof(AttrRune *) * term_height);
    for (size_t i = 0; i < term_height; ++i) {
        display_buffer[i] = malloc(sizeof(AttrRune) * term_width);
        memset(display_buffer[i], 0, term_width);

        for (size_t j = 0; j < term_width; ++j)
            display_buffer[i][j].c[0] = '\n';
    }

    displayed_buffers = malloc(sizeof(Buffer *) * 4);
    memset(displayed_buffers, 0, sizeof(Buffer *) * 4);

    buffer_change_listeners_len = 8;
    buffer_change_listeners_i = 0;
    buffer_change_listeners =
        malloc(sizeof(void (*)(void)) * buffer_change_listeners_len);
}

void ui_tear_down(void) {
    for (size_t i = 0; i < 3; ++i)
        buffer_destruct(displayed_buffers[i]);
    free(displayed_buffers);

    for (size_t i = 0; i < term_height; ++i) {
        free(display_buffer[i]);
    }
    free(display_buffer);

    free(buffer_change_listeners);
}

void exit_editor() { editor_exited = 1; }

void redraw_editor(void) {
    unsigned int x = 1;
    unsigned int y = 1;

    size_t i;
    AttrRune c;
    for (size_t b = 0; b < 3; b++) {
        Buffer *buf = displayed_buffers[b];

        y = (unsigned int)buf->display_range_y_start;
        i = buf->visible_start_point;
        while (i < buf->buf_size - (buf->gap_end - buf->gap_start)) {
            if (y >= buf->display_range_y_end) break;

            c = buffer_get_rune(buf, i);

            if (rune_is_lf(c)) {
                for (unsigned int j = x; j <= term_width; j++)
                    ui_draw_char(' ', buf->default_face, j, y);

                ++y;

                x = 1;
            } else {
                ui_draw_rune(c, buf->default_face, x, y);

                for (unsigned int j = x + 1; j < x + c.display_width; ++j)
                    ui_invalidate_point(j, y);

                x += c.display_width;

                if (i + 1 < buf->buf_size - (buf->gap_end - buf->gap_start)) {
                    AttrRune next_rune = buffer_get_rune(buf, i + 1);
                    if (!rune_is_lf(next_rune) &&
                        x + next_rune.display_width >= term_width) {
                        if (x + next_rune.display_width == term_width) {
                            ui_draw_char('\\', buf->default_face, x, y);
                        } else {
                            ui_draw_char(' ', buf->default_face, x, y);
                            ++x;
                            ui_draw_char('\\', buf->default_face, x, y);
                        }

                        x = 1;
                        ++y;
                    }
                }
            }

            ++i;
        }

        for (unsigned int j = y; j < buf->display_range_y_end; j++) {
            for (unsigned int k = x; k <= term_width; k++)
                ui_draw_char(' ', buf->default_face, k, j);
            x = 1;
        }
    }

    move_cursor_editor(current_buffer->cursor_x, current_buffer->cursor_y);
}

void ui_invalidate(void) {
    for (size_t i = 0; i < term_height; ++i) {
        for (size_t j = 0; j < term_width; ++j) {
            display_buffer[i][j].c[0] = '\n';
            display_buffer[i][j].face = NULL;
        }
    }

    redraw_editor();
}

void editor_main_loop() {
    Rune buf;
    memset(buf, 0, sizeof(buf));
    unsigned int n_byte;
    char broken;
    for (;;) {
        if (editor_exited) break;

        redraw_editor();

        unsigned char c = (unsigned char)tgetc();
        if ((c >> 7 & 1) == 0)
            n_byte = 1;
        else if ((c >> 5 & 0b111) == 0b110)
            n_byte = 2;
        else if ((c >> 4 & 0b1111) == 0b1110)
            n_byte = 3;
        else if ((c >> 3 & 0b11111) == 0b11110)
            n_byte = 4;
        else {
            n_byte = 0;
        }

        if (n_byte == 1)
            handle_key(c);
        else if (1 < n_byte && n_byte <= 4) {
            buf[0] = c;

            broken = 0;
            for (--n_byte; n_byte > 0; --n_byte) {
                c = (unsigned char)tgetc();
                if ((c >> 6 & 0b11) != 0b10) {
                    broken = 1;

                    break;
                }

                buf[sizeof(Rune) - n_byte - 1] = c;
            }

            if (!broken) handle_rune(buf);
            memset(buf, 0, sizeof(Rune));
        }
    }
}

void move_cursor_editor(unsigned int x, unsigned int y) {
    move_cursor(x, y + 1);
}
