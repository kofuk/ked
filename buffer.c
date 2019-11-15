#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "buffer.h"
#include "string.h"
#include "terminal.h"
#include "ui.h"
#include "utilities.h"

#define POINT_TO_INDEX(buf, point, result)                                     \
    do {                                                                       \
        if (point >= buf->gap_start)                                           \
            result = point + (buf->gap_end - buf->gap_start);                  \
        else                                                                   \
            result = point;                                                    \
    } while (0)

#define IS_OUT_OF_BOUNDS(buf, index) index >= buf->buf_size

Buffer *current_buffer;

Buffer *buffer_create(const char *path, const char *buf_name)
{
    struct stat stat_buf;
    if (stat(path, &stat_buf) != 0)
    {
        return NULL;
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        return NULL;
    }

    size_t fsize = (size_t)stat_buf.st_size;
    char *content_buf = malloc(sizeof(char) * (fsize + INIT_GAP_SIZE));
    size_t buf_pos = INIT_GAP_SIZE;
    size_t read_size = fsize > 0xffff000 ? 0xffff000 : fsize;
    ssize_t nread;
    while ((nread = read(fd, content_buf + buf_pos, read_size)) > 0)
    {
        buf_pos += (size_t)nread;
    }

    size_t n_lines = 0;
    for (size_t i = INIT_GAP_SIZE; i < fsize + INIT_GAP_SIZE; i++)
        if (content_buf[i] == '\n') n_lines++;

    size_t path_len = strlen(path);
    char *path_buf = malloc(sizeof(char) * (path_len + 1));
    memcpy(path_buf, path, path_len + 1);

    size_t buf_name_len = strlen(buf_name);
    char *buf_name_buf = malloc(sizeof(char) * (buf_name_len + 1));
    memcpy(buf_name_buf, buf_name, buf_name_len + 1);

    Buffer *result = malloc(sizeof(Buffer));
    result->buf_name = buf_name_buf;
    result->path = path_buf;
    result->content = content_buf;
    result->point = 0;
    result->buf_size = fsize + INIT_GAP_SIZE;
    result->gap_start = 0;
    result->gap_end = INIT_GAP_SIZE;
    result->n_lines = n_lines;
    result->cursor_x = 1;
    result->cursor_y = 1;

    return result;
}

Range *buffer_line(Buffer *this, size_t lineno)
{
    if (lineno > this->n_lines) return NULL;

    Range *result = malloc(sizeof(Range));
    int searching_start = 1;
    size_t l = 1;

    for (size_t i = 0; i < this->buf_size; i++)
    {
        if (this->gap_start <= i && i <= this->gap_end) i = this->gap_end;

        if (searching_start && l == lineno)
        {
            result->start = i;

            searching_start = 0;
        }

        if (this->content[i] == '\n') ++l;

        if (l > lineno)
        {
            result->end = i;
            goto END;
        }
    }

    result->end = this->buf_size;

END:
    return result;
}

static Range *buffer_line_at_point(Buffer *this, size_t point)
{
    size_t p = point;

    size_t index;
    POINT_TO_INDEX(this, p, index);

    if (IS_OUT_OF_BOUNDS(this, index)) return NULL;

    Range *result = malloc(sizeof(Range));

    size_t tmp = p;

    if (tmp > 0)
    {
        do {
            --tmp;
            POINT_TO_INDEX(this, tmp, index);

            if (this->content[index] == '\n')
            {
                // to cope with the condition that point is on in beginning of buffer.
                ++index;

                break;
            }
        } while (index > 0 && tmp > 0);
    }

    result->start = index - 1;

    while (!(IS_OUT_OF_BOUNDS(this, index)))
    {
        POINT_TO_INDEX(this, p, index);
        if (this->content[index] == '\n') break;

        ++p;
    }

    result->end = index;

    return result;
}

String *buffer_string_range(Buffer *this, Range *r)
{
    String *result;

    if (r->start < this->gap_start && r->end > this->gap_end)
    {
        size_t size = r->end - r->start - (this->gap_end - this->gap_start);
        result = string_create(NULL, size);
        memcpy(result->buf, this->content + r->start, this->gap_start - r->start);
        memcpy(result->buf + (this->gap_start + r->start), this->content + this->gap_end, r->end - this->gap_end);
    }
    else
    {
        size_t size = r->end - r->start;
        result = string_create(NULL, size);
        memcpy(result->buf, this->content + r->start, size);
    }

    return result;
}

static void buffer_flush_cursor_position(Buffer *this)
{
    move_cursor_editor(this->cursor_x, this->cursor_y);
}

void buffer_cursor_forward(Buffer *this)
{
    size_t old_point = this->point;
    size_t new_point = old_point + 1;
    size_t old_i;
    POINT_TO_INDEX(this, old_point, old_i);
    size_t new_i;
    POINT_TO_INDEX(this, new_point, new_i);
    if (IS_OUT_OF_BOUNDS(this, new_i)) return;

    if (this->content[old_i] == '\n')
    {
        this->cursor_x = 1;
        ++(this->cursor_y);
    }
    else
    {
        ++(this->cursor_x);
    }

    this->point = new_point;

    buffer_flush_cursor_position(this);
}

void buffer_cursor_back(Buffer *this)
{
    size_t index;
    POINT_TO_INDEX(this, this->point, index);
    if (this->point == 0 || index == 0) return;

    size_t new_point = this->point - 1;
    POINT_TO_INDEX(this, new_point, index);

    if (this->cursor_x == 1)
    {
        --(this->cursor_y);

        Range *r = buffer_line_at_point(this, new_point);
        String *line = buffer_string_range(this, r);
        clear_buffer((char*)r, sizeof(Range));
        free(r);
        r = NULL;

        this->cursor_x = (unsigned int)line->length;

        string_destruct(line);
    }
    else
        --(this->cursor_x);

    this->point = new_point;

    buffer_flush_cursor_position(this);
}

void buffer_insert(Buffer *this, char c)
{
    if (this->gap_end - this->gap_start < 32)
    {
        //TODO: allocate buffer.
    }

    this->content[this->gap_start] = c;

    ++(this->gap_start);
    ++(this->point);

    if (c == '\n')
    {
        ++(this->n_lines);

        this->cursor_x = 1;
        ++(this->cursor_y);
    }
    else
    {
        ++(this->cursor_x);
    }

    redraw_editor();
}

void set_buffer(Buffer *buf)
{
    current_buffer = buf;
}
