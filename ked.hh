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

#define PROGRAM_NAME "ked"

/* Load user preference and initialize the editor with it and returns 1 if
 * success and 0 if fail. If debug is true then it loads etc/kedrc as
 * preference else loads $HOME/.kedrc. Unless the preference system extension
 * with loadSystemExtension(string path) first, it fails. */
bool userpref_load(bool debug);

#endif
