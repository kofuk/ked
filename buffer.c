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

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#include "buffer.h"
#include "terminal.h"
#include "ui.h"

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

    size_t path_len = strlen(path);
    char *path_buf = malloc(sizeof(char) * (path_len + 1));
    memcpy(path_buf, path, path_len + 1);

    size_t buf_name_len = strlen(buf_name);
    char *buf_name_buf = malloc(sizeof(char) * (buf_name_len + 1));
    memcpy(buf_name_buf, buf_name, buf_name_len + 1);

    Buffer *result = malloc(sizeof(Buffer));
    memset(result, 0, sizeof(Buffer));
    result->buf_name = buf_name_buf;
    result->path = path_buf;
    result->content = content_buf;
    result->point = 0;
    result->buf_size = fsize + INIT_GAP_SIZE;
    result->gap_start = 0;
    result->gap_end = INIT_GAP_SIZE;
    result->modified = 0;
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

    write_message("You opened nonexisiting file.");

    char *content_buf = malloc(sizeof(char) * INIT_GAP_SIZE);

    size_t path_len = strlen(path);
    char *path_buf = malloc(sizeof(char) * (path_len + 1));
    memcpy(path_buf, path, path_len + 1);

    size_t buf_name_len = strlen(buf_name);
    char *buf_name_buf = malloc(sizeof(char) * (buf_name_len + 1));
    memcpy(buf_name_buf, buf_name, buf_name_len + 1);

    Buffer *result = malloc(sizeof(Buffer));
    memset(result, 0, sizeof(Buffer));
    result->buf_name = buf_name_buf;
    result->path = path_buf;
    result->content = content_buf;
    result->point = 0;
    result->buf_size = INIT_GAP_SIZE;
    result->gap_start = 0;
    result->gap_end = INIT_GAP_SIZE;
    result->cursor_x = 1;
    result->cursor_y = 1;

    return result;
}

Buffer *buffer_create_system(const char *name)
{
    char *content_buf = malloc(sizeof(char) * INIT_GAP_SIZE);

    Buffer *result = malloc(sizeof(Buffer));
    memset(result, 0, sizeof(Buffer));
    result->buf_name = strdup(name);
    result->content = content_buf;
    result->buf_size = INIT_GAP_SIZE;
    result->gap_start = 0;
    result->gap_end = INIT_GAP_SIZE;
    result->cursor_x = 1;
    result->cursor_y = 1;

    return result;
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
            i = this->gap_end;

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
    if (this->point >= this->buf_size - (this->gap_end - this->gap_start) - 1)
    {
        write_message("End of buffer.");

        return;
    }

    ++(this->point);

    this->content[this->gap_start] = this->content[this->gap_end];
    ++(this->gap_start);
    ++(this->gap_end);

    buffer_update_cursor_position(this);
}

void buffer_cursor_back(Buffer *this)
{
    if (this->point == 0)
    {
        write_message("Beginning of buffer.");

        return;
    }

    --(this->point);

    this->content[this->gap_end - 1] = this->content[this->gap_start - 1];
    --(this->gap_start);
    --(this->gap_end);

    buffer_update_cursor_position(this);
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

        this->content = new_buf;

        this->gap_end += INIT_GAP_SIZE;
        this->buf_size += INIT_GAP_SIZE;

        free(old_buf);
        old_buf = NULL;
    }

    this->content[this->gap_start] = c;

    ++(this->gap_start);
    ++(this->point);

    this->modified = 1;

    buffer_update_cursor_position(this);
}

void buffer_delete_backward(Buffer *this)
{
    if (this->point == 0)
    {
        write_message("Beginning of buffer.");

        return;
    }

    --(this->gap_start);
    --(this->point);

    buffer_update_cursor_position(this);
}

int buffer_save(Buffer *this)
{
    if (!this->modified)
    {
        write_message("Buffer is not modified.");

        return 0;
    }

    FILE *f = fopen(this->path, "w");
    if (f == NULL)
    {
        write_message(strerror(errno));

        return 0;
    }

    char buf[2048];

    size_t n_in_buf = 0;
    size_t i = 0;
    while (i < this->buf_size)
    {
        if (this->gap_start <= i && i < this->gap_end)
        {
            i = this->gap_end;

            if (i >= this->buf_size) break;

            continue;
        }

        buf[n_in_buf] = this->content[i];
        ++n_in_buf;
        ++i;

        if (n_in_buf == 2048)
        {
            if (fwrite(buf, 1, n_in_buf, f) < n_in_buf)
            {
                write_message(strerror(errno));
                fclose(f);

                return 0;
            }

            n_in_buf = 0;
        }
    }

    if (fwrite(buf, 1, n_in_buf, f) < n_in_buf)
    {
        write_message(strerror(errno));
        fclose(f);

        return 0;
    }

    fclose(f);

    write_message("Saved");

    return 1;
}
