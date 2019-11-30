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

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <ked/rune.h>
#include <ked/terminal.h>
#include <ked/ui.h>

#include "utilities.h"

static struct termios orig_termios;

size_t term_width;
size_t term_height;

void tputc(int c) { write(1, &c, 1); }

void tputc_printable(unsigned char c) { char_write_printable(1, c); }

void tputrune(Rune r) { rune_write_printable(1, r); }

void tputs(char *s) { write(1, s, strlen(s)); }

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
    tputs(";3");
    tputs(f);
    tputs(";4");
    tputs(b);
    tputs("m");

    free(t);
    free(f);
    free(b);
}

void reset_graphic_attrs(void) {
    tputs("\e[0m");
}

void esc_write(char *s) {
    tputc('\e');
    tputs(s);
}

static void init_variables(void) {
    // get terminal window size.
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);

    term_width = (size_t)w.ws_col;
    term_height = (size_t)w.ws_row;
}

void term_set_up() {
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

    init_variables();
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void resume_editor(int signal) {
    term_set_up();

    force_redraw_editor();
}

#pragma GCC diagnostic pop

void stop_editor(void) {
    term_tear_down();
    signal(SIGCONT, resume_editor);

    kill(0, SIGSTOP);
}
