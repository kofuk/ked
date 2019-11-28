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

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "buffer.h"
#include "io.h"
#include "rune.h"
#include "terminal.h"
#include "ui.h"

Buffer *current_buffer;

static Buffer *buffer_create_existing_file(const char *path,
                                           const char *buf_name,
                                           struct stat stat_buf) {
    FILE *f = fopen(path, "r");
    if (f == NULL) return NULL;

    size_t size = (size_t)stat_buf.st_size;

    enum LineEnding lend;
    AttrRune *content_buf = create_content_buffer(f, &size, INIT_GAP_SIZE, &lend);
    fclose(f);

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
    result->buf_size = size;
    result->gap_start = 0;
    result->gap_end = INIT_GAP_SIZE;
    result->line_ending = lend;
    result->modified = 0;
    result->cursor_x = 1;
    result->cursor_y = 1;

    return result;
}

Buffer *buffer_create(const char *path, const char *buf_name) {
    struct stat stat_buf;
    if (stat(path, &stat_buf) == 0) {
        return buffer_create_existing_file(path, buf_name, stat_buf);
    }

    write_message("You opened nonexisiting file.");

    AttrRune *content_buf = malloc(sizeof(AttrRune) * INIT_GAP_SIZE);

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

Buffer *buffer_create_system(const char *name) {
    AttrRune *content_buf = malloc(sizeof(AttrRune) * INIT_GAP_SIZE);

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

void buffer_destruct(Buffer *this) {
    free(this->buf_name);
    free(this->path);
    free(this->content);

    free(this);
}

static void buffer_update_cursor_position(Buffer *this) {
    unsigned int cursor_x = 1;
    unsigned int cursor_y = 1;

    size_t i = 0;
    while (i < this->buf_size - (this->gap_end - this->gap_start) &&
           i < this->point) {
        ++cursor_x;

        if (rune_is_lf(buffer_get_rune(this, i))) {
            cursor_x = 1;
            ++cursor_y;
        } else if (cursor_x >= term_width &&
                   i + 1 < this->buf_size - (this->gap_end - this->gap_start) &&
                   !rune_is_lf(buffer_get_rune(this, i + 1))) {
            cursor_x = 1;
            ++cursor_y;
        }

        ++i;
    }

    this->cursor_x = cursor_x;
    this->cursor_y = cursor_y;
}

static void buffer_expand(Buffer *this, size_t amount) {
    AttrRune *new_buf = malloc(sizeof(AttrRune) * (this->buf_size + amount));
    memcpy(new_buf, this->content, sizeof(AttrRune) * this->gap_start);
    memcpy(new_buf + this->gap_end + amount, this->content + this->gap_end,
           sizeof(AttrRune) * (this->buf_size - this->gap_end));
    free(this->content);

    this->content = new_buf;
    this->gap_end += amount;
    this->buf_size += amount;
}

void buffer_cursor_move(Buffer *this, size_t n, int forward) {
    if (n > this->gap_end - this->gap_start)
        buffer_expand(this, n - this->gap_end - this->gap_start);

    if (forward) {
        if (n > this->buf_size - this->gap_end) {
            write_message("End of buffer.");
            n = this->buf_size - this->gap_end;
        }

        memcpy(this->content + this->gap_start, this->content + this->gap_end,
               sizeof(AttrRune) * n);
        this->gap_start += n;
        this->gap_end += n;
        this->point += n;
    } else {
        if (n > this->gap_start) {
            write_message("Beginning of buffer.");
            n = this->gap_start;
        }

        memcpy(this->content + this->gap_end - n,
               this->content + this->gap_start - n, sizeof(AttrRune) * n);
        this->gap_start -= n;
        this->gap_end -= n;
        this->point -= n;
    }

    buffer_update_cursor_position(this);
}

void buffer_cursor_forward_line(Buffer *this) {
    size_t col = 0;

    size_t point = this->point;

    /* First, search for '\n' to obtain current column number. */
    while (point != 0 && !rune_is_lf(buffer_get_rune(this, point))) {
        ++col;
        --point;
    }

    point = this->point;
    size_t n_forward = 1;

    /* Next, search for next '\n' to calculate the number of  remaining
       character. If point is now on the last line, we only move to end of the
       buffer and exit. */
    while (!rune_is_lf(buffer_get_rune(this, point))) {
        if (point >= this->buf_size - (this->gap_end - this->gap_start)) {
            buffer_cursor_move(this, n_forward, 1);

            return;
        }

        ++n_forward;
        ++point;
    }

    ++point;

    size_t new_col = 1;
    /* Finally, move to target column unless reach to the end of the buffer. */
    while (!rune_is_lf(buffer_get_rune(this, point)) && new_col < col) {
        ++n_forward;
        ++new_col;
        ++point;
    }
    buffer_cursor_move(this, n_forward, 1);
}

void buffer_insert(Buffer *this, Rune r) {
    if (this->gap_end - this->gap_start < MIN_GAP_SIZE)
        buffer_expand(this, INIT_GAP_SIZE);

    memcpy(this->content[this->gap_start].c, r, 4);

    ++(this->gap_start);
    ++(this->point);

    this->modified = 1;

    buffer_update_cursor_position(this);
}

void buffer_delete_backward(Buffer *this) {
    if (this->point == 0) {
        write_message("Beginning of buffer.");

        return;
    }

    --(this->gap_start);
    --(this->point);

    buffer_update_cursor_position(this);
}

int buffer_save(Buffer *this) {
    if (!this->modified) {
        write_message("Buffer is not modified.");

        return 0;
    }

    return save_buffer_utf8(this);
}
