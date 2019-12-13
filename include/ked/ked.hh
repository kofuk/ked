/*
 * ked -- simple text editor with minimal dependency
 * Copyright (C) 2019  Koki Fukuda
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KED_HH
#define KED_HH

#include "Buffer.hh"
#include "Ui.hh"

#define EDITOR_COMMAND_ARG_LIST Ked::Ui &ui, Ked::Buffer &buf

#define DEFINE_EDITOR_COMMAND(name) void ec_##name(EDITOR_COMMAND_ARG_LIST)
#define EDITOR_COMMAND_PTR(name) &ec_##name

#define FACE_COLOR_256(fg, bg) "\e[0m\e[38;5;" #fg "m\e[48;5;" #bg "m"
#define FACE_ATTR_COLOR_256(attr, fg, bg) \
    "\e[" #attr "m\e[38;5;" #fg "m\e[48;5;" #bg "m"
#define FACE_NONE "\e[0m"

#endif
