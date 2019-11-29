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
CFLAGS = -fPIC -Wall -Wextra -I../include
OBJS = buffer.o extension.o io.o keybind.o rune.o terminal.o ui.o utilities.o
LDFLAGS = -shared -lm -ldl

.PHONY: all
all: $(OBJS)
	$(CC) -o libked.so $(OBJS) $(LDFLAGS)

.PHON: debug
debug: CFLAGS = -fPIC -Wall -Wextra -O0 -g3 -I../include
debug: all

.PHONY: clean
clean:
	$(RM) $(OBJS) libked.so
