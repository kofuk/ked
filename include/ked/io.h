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

#ifndef KED_IO_H
#define KED_IO_H

#include <stdio.h>

#include "buffer.h"

/* Reads up to given length of file and create an array of AttrRune. Length
 * pointer will be updated to represent length of array of AttrRune. */
AttrRune *create_content_buffer(FILE *, size_t *, size_t, enum LineEnding *);

/* Saves buffer as UTF-8 text file. */
int save_buffer_utf8(Buffer *);

#endif
