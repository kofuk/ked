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

#include <ked/ked.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

DEFINE_EDIT_COMMAND(cursor_forward) { KED_FUNCALL(buffer_cursor_move)(buf, 1, 1); }

DEFINE_EDIT_COMMAND(cursor_back) { KED_FUNCALL(buffer_cursor_move)(buf, 1, 0); }

DEFINE_EDIT_COMMAND(cursor_forward_line) { KED_FUNCALL(buffer_cursor_forward_line)(buf); };

DEFINE_EDIT_COMMAND(delete_backward) { KED_FUNCALL(buffer_delete_backward)(buf); }

DEFINE_EDIT_COMMAND(delete_forward) { KED_FUNCALL(buffer_delete_forward)(buf); }

DEFINE_EDIT_COMMAND(buffer_save) { KED_FUNCALL(buffer_save)(buf); }

DEFINE_EDIT_COMMAND(editor_quit) { KED_FUNCALL(exit_editor)(); }

DEFINE_EDIT_COMMAND(display_way_of_quit) { KED_FUNCALL(write_message)("Ctrl+Q to quit."); }

DEFINE_EDIT_COMMAND(process_stop) { KED_FUNCALL(stop_editor)(); }

#pragma GCC diagnostic pop

EXPORT_KEYBINDS = {{EDIT_COMMAND_PTR(cursor_forward_line), "^[[B"},
                   {EDIT_COMMAND_PTR(cursor_forward), "^[[C"},
                   {EDIT_COMMAND_PTR(cursor_back), "^[[D"},
                   {EDIT_COMMAND_PTR(cursor_back), "^B"},
                   {EDIT_COMMAND_PTR(display_way_of_quit), "^C"},
                   {EDIT_COMMAND_PTR(delete_forward), "^D"},
                   {EDIT_COMMAND_PTR(delete_backward), "^H"},
                   {EDIT_COMMAND_PTR(cursor_forward_line), "^N"},
                   {EDIT_COMMAND_PTR(editor_quit), "^Q"},
                   {EDIT_COMMAND_PTR(editor_quit), "^X^C"},
                   {EDIT_COMMAND_PTR(buffer_save), "^X^S"},
                   {EDIT_COMMAND_PTR(process_stop), "^Z"},
                   {EDIT_COMMAND_PTR(cursor_forward), "^F"},
                   {EDIT_COMMAND_PTR(delete_backward), "\x7f"},
                   KEYBINDS_END};
