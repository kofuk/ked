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

#include <asm-generic/ioctl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <ked/rune.h>

#include "libked.h"

static struct termios orig_termios;

size_t term_width;
size_t term_height;

#define IO_BUFFER_LEN 2048

static char *io_buffer;
static size_t io_buffer_i;

void tputc(int c) {
    if (io_buffer_i >= IO_BUFFER_LEN) term_flush_buffer();
    io_buffer[io_buffer_i] = (char)c;
    ++io_buffer_i;
}

void tputc_printable(unsigned char c) { char_write_printable(c); }

void tputrune(Rune r) { rune_write_printable(r); }

void tput(const char *buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        if (io_buffer_i >= IO_BUFFER_LEN) term_flush_buffer();
        io_buffer[io_buffer_i] = buf[i];
        ++io_buffer_i;
    }
}

void tputs(const char *s) { tput(s, strlen(s)); }

int tgetc(void) {
    char buf[1];

    read(1, buf, 1);

    return buf[0];
}

void set_graphic_attrs(unsigned int text, unsigned int fg, unsigned int bg) {
    char *t = itoa(text);
    char *f = itoa(fg);
    char *b = itoa(bg);

    tputs("\e[");
    tputs(t);
    tputs("m\e[38;5;");
    tputs(f);
    tputs("m\e[48;5;");
    tputs(b);
    tputs("m");

    free(t);
    free(f);
    free(b);
}

void reset_graphic_attrs(void) { tputs("\e[0m"); }

void esc_write(char *s) {
    tputc('\e');
    tputs(s);
}

void term_flush_buffer(void) {
    write(1, io_buffer, sizeof(char) * io_buffer_i);
    io_buffer_i = 0;
}

static void init_variables(void) {
    // get terminal window size.
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);

    term_width = (size_t)w.ws_col;
    term_height = (size_t)w.ws_row;

    io_buffer = malloc(sizeof(char) * IO_BUFFER_LEN);
    io_buffer_i = 0;
}

void term_set_up() {
    init_variables();

    // https://invisible-island.net/xterm/xterm.faq.html#xterm_tite
    // enter alternate screen.
    esc_write("7");
    // save cursor position.
    esc_write("[?47h");
    // clear screen.
    esc_write("[2J");

    struct termios new_termios;
    tcgetattr(0, &new_termios);

    // save old attibutes to restore the attributes before exit.
    orig_termios = new_termios;

    new_termios.c_iflag &= ~(IXOFF | IXON | IGNCR);

    // set noncanonical mode, no echo, etc.
    new_termios.c_lflag &= ~(ISIG | ICANON | ECHO);

    // read byte-by-byte with blocking read.
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;

    tcsetattr(0, TCSADRAIN, &new_termios);
}

void term_tear_down() {
    // restore original terminal attributes after all output written.
    tcsetattr(0, TCSADRAIN, &orig_termios);

    // clear screen.
    esc_write("[2J");
    // exit alternate screen.
    esc_write("[?47l");
    // restore cursor position.
    esc_write("8");

    term_flush_buffer();

    free(io_buffer);
    io_buffer = NULL;
}

void move_cursor(unsigned int x, unsigned int y) {
    esc_write("[");
    char *line = itoa(y);
    tputs(line);
    free(line);
    line = NULL;
    tputs(";");
    char *col = itoa(x);
    tputs(col);
    free(col);
    col = NULL;
    tputs("f");
}

void stop_editor(void) {
    // FIXME: Avoid deadlock
    display_buffer_lock();

    term_tear_down();

    kill(0, SIGSTOP);
}
