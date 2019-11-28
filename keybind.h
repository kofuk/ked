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

#ifndef KEYBIND_H
#define KEYBIND_H

#include "rune.h"

/* Initializes local variables that needed for key bind handling, and global key
 * bind. */
void keybind_set_up(void);
/* Frees allocated for key bind handling. */
void keybind_tear_down(void);

/* Handles key input or buffers the key for next input. */
void handle_key(int c);

/* Clear queued key sequence and insert given Rune to buffer. */
void handle_rune(Rune r);

#endif
