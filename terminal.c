// This file is part of `ked', licensed under the GPLv3 or later,
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "terminal.h"
#include "utilities.h"

static struct termios orig_termios;

size_t term_width;
size_t term_height;

static char *line_buffer;
static size_t line_buffer_pos;

void tputc(int c)
{
    char buf[1];

    buf[0] = (char)c;
    write(1, buf, 1);
}

void tputs(char *s) { write(1, s, strlen(s)); }

size_t append_to_line(char *s)
{
    unsigned int len = (unsigned int)strlen(s);

    if (len + line_buffer_pos > term_width)
    {
        size_t ncopy = term_width - line_buffer_pos;
        strncpy(line_buffer + line_buffer_pos, s, ncopy);
        line_buffer_pos = term_width;

        return ncopy;
    }

    strcpy(line_buffer + line_buffer_pos, s);
    line_buffer_pos += len;

    return len;
}

void flush_line(void)
{
    // fill remaining columns with space to make background coloring work fine.
    for (size_t i = line_buffer_pos; i < term_width; i++)
        line_buffer[i] = ' ';

    tputs(line_buffer);

    line_buffer_pos = 0;
}

void new_line(void)
{
    tputs("\r\n");
}

static int tgetc(void)
{
    char buf[1];

    read(1, buf, 1);

    return buf[0];
}

void esc_write(char *s)
{
    tputc('\e');
    tputs(s);
}

static int editor_exited;

static void init_variables(void)
{
    // get terminal window size.
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);

    term_width = (size_t)w.ws_col;
    term_height = (size_t)w.ws_row;

    line_buffer = malloc(sizeof(char) * (term_width + 1));
    line_buffer[term_width] = 0;
    line_buffer_pos = 0;
}

static void tear_down_variable(void)
{
    clear_buffer(line_buffer, term_width + 1);
    free(line_buffer);
    line_buffer = NULL;
}

void term_set_up()
{
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

    // set noncanonical mode, no echo, etc.
    new_termios.c_lflag &= ~(ISIG | ICANON | ECHO | FLUSHO);

    // read byte-by-byte with blocking read.
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;

    tcsetattr(0, TCSADRAIN, &new_termios);

    init_variables();
}

void term_tear_down()
{
    // restore original terminal attributes after all output written.
    tcsetattr(0, TCSADRAIN, &orig_termios);

    // clear screen.
    esc_write("[2J");
    // exit alternate screen.
    esc_write("[?47l");
    // restore cursor position.
    esc_write("8");

    tear_down_variable();
}

void exit_editor()
{
    editor_exited = 1;
}

void editor_main_loop()
{
    for (;;)
    {
        if (editor_exited) break;

        if (tgetc() == 'q') exit_editor();
    }
}

void move_cursor(unsigned int x, unsigned int y)
{
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
