#include <stdlib.h>

#include "terminal.h"
#include "ui.h"

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
