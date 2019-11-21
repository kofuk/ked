#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "buffer.h"
#include "keybind.h"
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

Buffer *current_buffer;
Buffer **displayed_buffers;

char **display_buffer;

void set_buffer(Buffer *buf, enum BufferPosition pos)
{
    switch (pos) {
    case BUF_HEADER:
        buf->display_range_y_start = 1;
        buf->display_range_y_end = 2;

        displayed_buffers[0] = buf;

        break;
    case BUF_MAIN:
        buf->display_range_y_start = 2;
        buf->display_range_y_end = term_height;

        displayed_buffers[1] = buf;

        break;
    case BUF_FOOTER:
        buf->display_range_y_start = term_height;
        buf->display_range_y_end = term_height + 1;

        displayed_buffers[2] = buf;

        break;
    }
}

void init_system_buffers(void)
{
    Buffer *header = buffer_create_system("Header");
    set_buffer(header, BUF_HEADER);

    Buffer *footer = buffer_create_system("Footer");
    set_buffer(footer, BUF_FOOTER);
}

static void update_header(char *fname)
{
    Buffer *header = displayed_buffers[0];

    if (fname == NULL)
        fname = "UNTITLED";

    size_t len = strlen(fname);
    for (size_t i = 0; i < len; i++)
        buffer_insert(header, fname[i]);

    buffer_insert(header, ' ');

    char *progname_section = "- KED";
    len = strlen(progname_section);
    for (size_t i = 0; i < len; i++)
        buffer_insert(header, progname_section[i]);
}

void select_buffer(Buffer *buf)
{
    current_buffer = buf;
    update_header(buf->buf_name);
}

void write_message(char *msg)
{
    Buffer *footer = displayed_buffers[2];
    //TODO: Don't touch struct member here.
    footer->point = 0;
    footer->gap_start = 0;
    footer->gap_end = footer->buf_size;

    buffer_insert(footer, ' ');
    size_t len = strlen(msg);
    for (size_t i = 0; i < len; i++)
        buffer_insert(footer, msg[i]);
}

void ui_set_up(void)
{
    display_buffer = malloc(sizeof(char*) * term_height);
    for (size_t i = 0; i < term_height; i++)
    {
        display_buffer[i] = malloc(sizeof(char) * term_width);

        for (size_t j = 0; j < term_width; j++)
            display_buffer[i][j] = ' ';
    }

    displayed_buffers = malloc(sizeof(Buffer*) * 4);
    memset(displayed_buffers, 0, sizeof(Buffer*) * 4);
}

void exit_editor()
{
    editor_exited = 1;
}

void redraw_editor(void)
{
    unsigned int x = 1;
    unsigned int y = 1;

    size_t i;
    int c;
    for (size_t b = 0; b < 3; b++)
    {
        Buffer *buf = displayed_buffers[b];

        y = (unsigned int)buf->display_range_y_start;
        i = 0;
        while (i < buf->buf_size)
        {
            if (buf->gap_start <= i && i < buf->gap_end)
            {
                i = buf->gap_end;

                if (i >= buf->buf_size) break;

                continue;
            }

            c = buf->content[i];

            if (c == '\n')
            {
                for (unsigned int j = x; j <= term_width; j++) DRAW_CHAR(' ', j, y);

                ++y;

                if (y > buf->display_range_y_end) break;

                x = 1;
            }
            else
            {
                DRAW_CHAR((char)c, x, y);

                ++x;
            }

            ++i;
        }

        for (unsigned int j = y; j < buf->display_range_y_end; j++)
        {
            for (unsigned int k = x; k <= term_width; k++) DRAW_CHAR(' ', k, j);
            x = 1;
        }
    }

    move_cursor_editor(current_buffer->cursor_x, current_buffer->cursor_y);
}

void editor_main_loop()
{
    for (;;)
    {
        if (editor_exited) break;

//        update_header(current_buffer->buf_name);
//        update_footer();

        redraw_editor();

        handle_key((char)tgetc());
    }
}

void move_cursor_editor(unsigned int x, unsigned int y)
{
    move_cursor(x, y + 1);
}
