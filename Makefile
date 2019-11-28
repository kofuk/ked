# ked -- simple text editor with minimal dependency
# Copyright (C) 2019  Koki Fukuda

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lm
OBJ = buffer.o editcommand.o io.o keybind.o main.o rune.o terminal.o ui.o utilities.o

.PHONY: all
all: $(OBJ)
	$(CC) $(LDFLAGS) -o ked $(OBJ)

.PHONY: debug
debug: CFLAGS = -Wall -Wextra -O0 -g3
debug: all

.PHONY: clean
clean:
	$(RM) $(OBJ) ked

buffer.o: buffer.c buffer.h rune.h
	$(CC) $(CFLAGS)   -c -o $@ $<

ui.o: ui.c buffer.h rune.h
	$(CC) $(CFLAGS)   -c -o $@ $<
