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

#include "ked/rune.h"
#include <string.h>

#include <ked/buffer.h>
#include <ked/ked.h>

static String *lf;

static size_t current_col = 0;
static char moving;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

DEFINE_EDIT_COMMAND(cursor_forward) { buffer_cursor_move(buf, 1, 1); }

DEFINE_EDIT_COMMAND(cursor_back) { buffer_cursor_move(buf, 1, 0); }

DEFINE_EDIT_COMMAND(cursor_forward_line) {
    struct SearchResult result;
    int s = buffer_search(buf, buf->point, lf, 1, &result);
    size_t rest;
    if (!s) {
        moving = 1;
        buffer_cursor_move(
            buf, buf->buf_size - (buf->gap_end - buf->gap_start) - buf->point,
            1);
        moving = 0;

        return;
    } else
        rest = result.start - buf->point;

    size_t n_start = result.end;

    s = buffer_search(buf, n_start, lf, 1, &result);
    if (!s) {
        moving = 1;
        buffer_cursor_move(buf,
                           rest + buf->buf_size -
                               (buf->gap_end - buf->gap_start) - buf->point,
                           1);
        moving = 0;
    } else {
        size_t len = result.end - n_start;
        if (len > current_col) {
            moving = 1;
            buffer_cursor_move(buf, rest + current_col, 1);
            moving = 0;
        } else {
            moving = 1;
            buffer_cursor_move(buf, rest + len, 1);
            moving = 0;
        }
    }
}

DEFINE_EDIT_COMMAND(cursor_back_line) {
    struct SearchResult result;
    int s = buffer_search(buf, buf->point, lf, 0, &result);
    if (!s) {
        moving = 1;
        buffer_cursor_move(buf, buf->point, 0);
        moving = 0;

        return;
    }

    size_t rest = buf->point - result.end;
    size_t n_end = result.start;
    s = buffer_search(buf, n_end, lf, 0, &result);
    size_t len;
    if (!s)
        len = buf->buf_size - (buf->gap_end - buf->gap_start) - n_end;
    else
        len =  n_end - result.start;

    if (len > current_col) {
        moving = 1;
        buffer_cursor_move(buf, rest + len - current_col + 1, 0);
        moving = 0;
    } else {
        moving = 1;
        buffer_cursor_move(buf, rest + 1, 0);
        moving = 0;
    }
}

DEFINE_EDIT_COMMAND(delete_backward) { buffer_delete_backward(buf); }

DEFINE_EDIT_COMMAND(delete_forward) { buffer_delete_forward(buf); }

DEFINE_EDIT_COMMAND(buffer_save) { buffer_save(buf); }

DEFINE_EDIT_COMMAND(editor_quit) { exit_editor(); }

DEFINE_EDIT_COMMAND(display_way_of_quit) { write_message("Ctrl+Q to quit."); }

DEFINE_EDIT_COMMAND(process_stop) { stop_editor(); }

#pragma GCC diagnostic pop

static void on_cursor_move(Buffer *buf) {
    if (moving) return;

    struct SearchResult result;
    int s = buffer_search(buf, buf->point, lf, 0, &result);
    if (!s)
        current_col = buf->point;
    else
        current_col = buf->point - result.start;
}

static const char *face_name_header = "SystemHeader";
static const char *face_name_footer = "SystemFooter";

static const char *face_header = FACE_ATTR_COLOR_256(1, 16, 231);
static const char *face_footer = FACE_COLOR_256(16, 231);

static void on_buffer_entry_change(Buffer **bufs, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        Buffer *buf = bufs[i];
        if (strcmp(buf->buf_name, "__system_header__") == 0) {
            buf->default_face = face_name_header;
        } else if (strcmp(buf->buf_name, "__system_footer__") == 0) {
            buf->default_face = face_name_footer;
        } else {
            buffer_add_on_cursor_move_listener(buf, on_cursor_move);
        }
    }
}

void extension_on_load(void) {
    add_buffer_entry_change_listener(on_buffer_entry_change);

    add_global_keybind("^[[A", EDIT_COMMAND_PTR(cursor_back_line));
    add_global_keybind("^[[B", EDIT_COMMAND_PTR(cursor_forward_line));
    add_global_keybind("^[[C", EDIT_COMMAND_PTR(cursor_forward));
    add_global_keybind("^[[D", EDIT_COMMAND_PTR(cursor_back));
    add_global_keybind("^B", EDIT_COMMAND_PTR(cursor_back));
    add_global_keybind("^C", EDIT_COMMAND_PTR(display_way_of_quit));
    add_global_keybind("^D", EDIT_COMMAND_PTR(delete_forward));
    add_global_keybind("^H", EDIT_COMMAND_PTR(delete_backward));
    add_global_keybind("^N", EDIT_COMMAND_PTR(cursor_forward_line));
    add_global_keybind("^P", EDIT_COMMAND_PTR(cursor_back_line));
    add_global_keybind("^Q", EDIT_COMMAND_PTR(editor_quit));
    add_global_keybind("^X^C", EDIT_COMMAND_PTR(editor_quit));
    add_global_keybind("^X^S", EDIT_COMMAND_PTR(buffer_save));
    add_global_keybind("^Z", EDIT_COMMAND_PTR(process_stop));
    add_global_keybind("^F", EDIT_COMMAND_PTR(cursor_forward));
    add_global_keybind("\x7f", EDIT_COMMAND_PTR(delete_backward));

    face_add(face_name_header, face_header);
    face_add(face_name_footer, face_footer);

    lf = string_create("\n");

    moving = 0;
    current_col = 1;
}

void extension_on_unload(void) {
    string_destruct(lf);
    lf = NULL;
}
