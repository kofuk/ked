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

#include "editcommand.h"
#include "buffer.h"
#include "terminal.h"
#include "ui.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

DEFINE_EDIT_COMMAND(cursor_forward) { buffer_cursor_move(buf, 1, 1); }

DEFINE_EDIT_COMMAND(cursor_back) { buffer_cursor_move(buf, 1, 0); }

DEFINE_EDIT_COMMAND(cursor_forward_line) { buffer_cursor_forward_line(buf); };

DEFINE_EDIT_COMMAND(delete_backward) { buffer_delete_backward(buf); }

DEFINE_EDIT_COMMAND(buffer_save) { buffer_save(buf); }

DEFINE_EDIT_COMMAND(editor_quit) { exit_editor(); }

DEFINE_EDIT_COMMAND(display_way_of_quit) { write_message("Ctrl+Q to quit."); }

DEFINE_EDIT_COMMAND(process_stop) { stop_editor(); }

#pragma GCC diagnostic pop
