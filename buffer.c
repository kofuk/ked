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

static Buffer *buffer_create_existing_file(const char *path, const char *buf_name, struct stat stat_buf)
{
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

    close(fd);

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

Buffer *buffer_create(const char *path, const char *buf_name)
{
    struct stat stat_buf;
    if (stat(path, &stat_buf) == 0)
    {
        return buffer_create_existing_file(path, buf_name, stat_buf);
    }

    char *content_buf = malloc(sizeof(char) * INIT_GAP_SIZE);

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
    result->buf_size = INIT_GAP_SIZE;
    result->gap_start = 0;
    result->gap_end = INIT_GAP_SIZE;
    result->n_lines = 0;
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

String *buffer_string_range(Buffer *this, Range *r)
{
    // check if `r' is not point out of range.
    if (r->start >= this->buf_size || r->end >= this->buf_size)
        return NULL;

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

static void buffer_update_cursor_position(Buffer *this)
{
    unsigned int cursor_x = 1;
    unsigned int cursor_y = 1;

    size_t cursor_index;
    POINT_TO_INDEX(this, this->point, cursor_index);

    size_t i = 0;
    while (i < this->buf_size && i < cursor_index)
    {
        if (this->gap_start <= i && i < this->gap_end)
        {
            i = this->gap_end + 1;

            if (i >= this->buf_size) break;

            continue;
        }

        ++cursor_x;

        if (this->content[i] == '\n')
        {
            cursor_x = 1;
            ++cursor_y;
        }

        ++i;
    }

    this->cursor_x = cursor_x;
    this->cursor_y = cursor_y;
}

void buffer_cursor_forward(Buffer *this)
{
    if (this->point >= this->buf_size - (this->gap_end - this->gap_start) - 1) return;

    ++(this->point);

    this->content[this->gap_start] = this->content[this->gap_end];
    ++(this->gap_start);
    ++(this->gap_end);

    buffer_update_cursor_position(this);
    buffer_flush_cursor_position(this);
}

void buffer_cursor_back(Buffer *this)
{
    if (this->point == 0) return;

    --(this->point);

    this->content[this->gap_end - 1] = this->content[this->gap_start - 1];
    --(this->gap_start);
    --(this->gap_end);

    buffer_update_cursor_position(this);
    buffer_flush_cursor_position(this);
}

void buffer_insert(Buffer *this, char c)
{
    if (this->gap_end - this->gap_start < MIN_GAP_SIZE)
    {
        char *new_buf = malloc(sizeof(char) * (this->buf_size + INIT_GAP_SIZE));
        memcpy(new_buf, this->content, this->gap_start);
        memcpy(new_buf + this->gap_end + INIT_GAP_SIZE,
               this->content + this->gap_end, this->buf_size - this->gap_end);

        char *old_buf = this->content;
        size_t old_size = this->buf_size;

        this->content = new_buf;

        this->gap_end += INIT_GAP_SIZE;
        this->buf_size += INIT_GAP_SIZE;

        clear_buffer(old_buf, old_size);
        free(old_buf);
        old_buf = NULL;
    }

    this->content[this->gap_start] = c;

    ++(this->gap_start);
    ++(this->point);

    if (c == '\n') ++(this->n_lines);

    buffer_update_cursor_position(this);
    redraw_editor();
}

void buffer_delete_backward(Buffer *this)
{
    if (this->point == 0) return;

    --(this->gap_start);
    --(this->point);

    buffer_update_cursor_position(this);
    redraw_editor();
}

void set_buffer(Buffer *buf)
{
    current_buffer = buf;
}
