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

#include "buffer.h"
#include "keybind.h"
#include "rune.h"
#include "terminal.h"
#include "ui.h"
#include "utilities.h"

#define KEY_ESC 0x1b

static int editor_exited;

Buffer *current_buffer;
Buffer **displayed_buffers;

AttrRune **display_buffer;

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
}

void init_system_buffers(void) {
    Buffer *header = buffer_create_system("Header");
    set_buffer(header, BUF_HEADER);

    Buffer *footer = buffer_create_system("Footer");
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
            display_buffer[i][j].c[0] = ' ';
    }

    displayed_buffers = malloc(sizeof(Buffer *) * 4);
    memset(displayed_buffers, 0, sizeof(Buffer *) * 4);
}

void ui_tear_down(void) {
    for (size_t i = 0; i < 3; ++i)
        buffer_destruct(displayed_buffers[i]);
    free(displayed_buffers);

    for (size_t i = 0; i < term_height; ++i) {
        free(display_buffer[i]);
    }
    free(display_buffer);
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
        i = 0;
        while (i < buf->buf_size - (buf->gap_end - buf->gap_start)) {
            if (y > buf->display_range_y_end) break;

            c = buffer_get_rune(buf, i);

            if (rune_is_lf(c)) {
                for (unsigned int j = x; j <= term_width; j++)
                    ui_draw_char(' ', j, y);

                ++y;

                x = 1;
            } else {
                ui_draw_rune(c, x, y);

                for (unsigned int j = x; j < x + c.display_width; ++j)
                    ui_invalidate_point(j, y);

                x += c.display_width;

                if (i + 1 < buf->buf_size - (buf->gap_end - buf->gap_start)) {
                    AttrRune next_rune = buffer_get_rune(buf, i + 1);
                    if (!rune_is_lf(next_rune) &&
                        x + next_rune.display_width >= term_width) {
                        if (x + next_rune.display_width == term_width) {
                            ui_draw_char('\\', x, y);
                        } else {
                            ui_draw_char(' ', x, y);
                            ++x;
                            ui_draw_char('\\', x, y);
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
                ui_draw_char(' ', k, j);
            x = 1;
        }
    }

    move_cursor_editor(current_buffer->cursor_x, current_buffer->cursor_y);
}

void force_redraw_editor(void) {
    for (size_t i = 0; i < term_height; ++i) {
        memset(display_buffer[i], ' ', term_width);
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
