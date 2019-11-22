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

#include "buffer.h"
#include "editcommand.h"
#include "ui.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

void ec_cursor_forward(EDIT_COMMAND_ARG_LIST) { buffer_cursor_forward(buf); }

void ec_cursor_back(EDIT_COMMAND_ARG_LIST) { buffer_cursor_back(buf); }

void ec_delete_backward(EDIT_COMMAND_ARG_LIST) { buffer_delete_backward(buf); }

void ec_buffer_save(EDIT_COMMAND_ARG_LIST) { buffer_save(buf); }

void ec_editor_quit(EDIT_COMMAND_ARG_LIST) { exit_editor(); }

#pragma GCC diagnostic pop
