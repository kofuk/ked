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

#ifndef KED_H
#define KED_H

#include <stddef.h>

#include <ked/buffer.h>

#define EDIT_COMMAND_ARG_LIST Buffer *buf

#define DEFINE_EDIT_COMMAND(name) void ec_##name(EDIT_COMMAND_ARG_LIST)
#define EDIT_COMMAND_PTR(name) ec_##name

typedef void (*EditCommand)(EDIT_COMMAND_ARG_LIST);

// extension.c

/* Add listener to be called just after buffer entry changed. */
void add_buffer_entry_change_listener(void (*func)(Buffer **, size_t));

/* Remove buffer change listener. */
void remove_buffer_entry_change_listener(void (*func)(Buffer **, size_t));

// face.c

/* Adds or replaces face. */
void face_add(const char *name, const char *face);

#define FACE_COLOR_256(fg, bg) "\e[0m\e[38;5;" #fg "m\e[48;5;" #bg "m"
#define FACE_ATTR_COLOR_256(attr, fg, bg) "\e[" #attr "m\e[38;5;" #fg "m\e[48;5;" #bg "m"

/* Set face used when specified face not found in storage. */
void face_set_default(const char *face);

// keybind.c

/* Register specified EditCommand as a key handler for specified key sequence.
 * If the same key sequence is registered, this replaces it. */
void add_global_keybind(const char *, EditCommand);

#endif
