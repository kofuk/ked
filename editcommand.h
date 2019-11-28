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

#ifndef EDITCOMMAND_H
#define EDITCOMMAND_H

#include "buffer.h"

#define EDIT_COMMAND_ARG_LIST Buffer *buf

#define DEFINE_EDIT_COMMAND(name) void ec_##name(EDIT_COMMAND_ARG_LIST)
#define EDIT_COMMAND_PTR(name) ec_##name

typedef void (*EditCommand)(EDIT_COMMAND_ARG_LIST);

DEFINE_EDIT_COMMAND(cursor_forward);
DEFINE_EDIT_COMMAND(cursor_back);
DEFINE_EDIT_COMMAND(cursor_forward_line);
DEFINE_EDIT_COMMAND(delete_backward);
DEFINE_EDIT_COMMAND(delete_forward);
DEFINE_EDIT_COMMAND(buffer_save);
DEFINE_EDIT_COMMAND(editor_quit);
DEFINE_EDIT_COMMAND(display_way_of_quit);
DEFINE_EDIT_COMMAND(process_stop);

#endif
