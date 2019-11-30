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

#ifndef KED_UI_H
#define KED_UI_H

#include <stddef.h>
#include <string.h>

#include "buffer.h"
#include "terminal.h"

/* Main routine of ked. Wait for input and refresh the screen. */
void editor_main_loop(void);
/* Requests to exit main loop. */
void exit_editor(void);

/* Requests to write message to message area. */
void write_message(char *);

/* Redraws the display area that differs from previous draw. */
void redraw_editor(void);

/* Clear display buffer and redraw editor. */
void force_redraw_editor(void);

void move_cursor_editor(unsigned int, unsigned int);

/* Creates buffer for header line and message line. */
void init_system_buffers(void);

/* Forcused buffer. */
extern Buffer *current_buffer;
/* All entries of displayed buffer. */
extern Buffer **displayed_buffers;

/* Buffer position to be displayed. */
enum BufferPosition { BUF_HEADER, BUF_MAIN, BUF_FOOTER };

/* Adds buffer to display. */
void set_buffer(Buffer *, enum BufferPosition);
/* Forcuses buffer. */
void select_buffer(Buffer *);

#endif
