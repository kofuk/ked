#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "buffer.h"
#include "string.h"
#include "terminal.h"
#include "ui.h"
#include "utilities.h"

#define TANONE "0"
#define TABOLD "1"
#define TAUNDERSCORE "4"
#define TABLINK "5"
#define TAREVVIDEO "7"
#define TACONCEALED "8"

#define TCFG "3"
#define TCBG "4"

#define TCBLACK "0"
#define TCRED "1"
#define TCGREEN "2"
#define TCYELLOW "3"
#define TCBLUE "4"
#define TCMAGENTA "5"
#define TCCYAN "6"
#define TCWHITE "7"

#define set_bgcolor(c) esc_write("[" TCBG c "m")
#define set_fgcolor(c) esc_write("[" TCFG c "m")
#define set_color(f, b) esc_write("[" TCFG f ";" TCBG b "m")
#define set_attr(a) esc_write("[" a "m")
#define set_style(a, f, g) esc_write("[" a ";" TCFG f ";" TCBG b "m")

#define KEY_ESC 0x1b

static int editor_exited;

static void write_header(char *fname)
{
    // move cursor to top-left.
    // http://ascii-table.com/ansi-escape-sequences.php
    move_cursor(1, 1);

    if (fname != NULL)
    {
        append_to_line(" ");
        append_to_line(fname);
        append_to_line(" -");
    }

    set_color(TCBLACK, TCWHITE);
    append_to_line(" KED");
    flush_line();
    set_attr(TANONE);
    new_line();
}

static void write_footer(char *msg)
{
    move_cursor(1, (unsigned int)term_height);

    set_color(TCBLACK, TCWHITE);

    if (msg != NULL)
    {
        append_to_line(" ");
        append_to_line(msg);
    }
    flush_line();
    set_attr(TANONE);
}

void ui_set_up(void)
{
    write_header(NULL);
    write_footer(NULL);
}

void exit_editor()
{
    editor_exited = 1;
}

void redraw_editor(void)
{
    Range *r;

    for (unsigned int i = 1; i <= UINT_MAX; i++)
    {
        r = buffer_line(current_buffer, (size_t)i);
        if (r == NULL) break;

        String *line = buffer_string_range(current_buffer, r);
        move_cursor(1, i + 1);
        append_to_line(line->buf);
        flush_line();
        string_destruct(line);
    }

    move_cursor_editor(current_buffer->cursor_x, current_buffer->cursor_y);
}

void editor_main_loop()
{
    write_header(current_buffer->buf_name);
    redraw_editor();

    for (;;)
    {
        if (editor_exited) break;

        char c = (char)tgetc();
        if (c == KEY_ESC)
        {
            c = (char)tgetc();
            if (c == '[')
            {
                c = (char)tgetc();
                if (c == 'A') move_cursor(cursor_x, cursor_y - 1);
                else if (c == 'B') move_cursor(cursor_x, cursor_y + 1);
                else if (c == 'C') buffer_cursor_forward(current_buffer);
                else if (c == 'D') buffer_cursor_back(current_buffer);
            }
        }
        else {
            if (c == 'q') exit_editor();

            buffer_insert(current_buffer, c);
        }
    }
}

void move_cursor_editor(unsigned int x, unsigned int y)
{
    move_cursor(x, y + 1);
}
