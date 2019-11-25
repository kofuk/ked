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
#include "terminal.h"
#include "ui.h"

Buffer *current_buffer;

static Buffer *buffer_create_existing_file(const char *path,
                                           const char *buf_name,
                                           struct stat stat_buf) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return NULL;

    size_t fsize = (size_t)stat_buf.st_size;
    char *tmp_buf = malloc(sizeof(char) * (fsize + INIT_GAP_SIZE));
    size_t buf_pos = INIT_GAP_SIZE;
    size_t read_size = fsize > 0xffff000 ? 0xffff000 : fsize;
    ssize_t nread;
    while ((nread = read(fd, tmp_buf + buf_pos, read_size)) > 0)
        buf_pos += (size_t)nread;

    close(fd);

    size_t nlf = 0, ncr = 0, ncrlf = 0;
    int prev_cr = 0;
    for (size_t i = INIT_GAP_SIZE; i < fsize + INIT_GAP_SIZE; i++) {
        switch (tmp_buf[i]) {
        case '\n':
            if (prev_cr) {
                ++ncrlf;
                prev_cr = 0;
            } else {
                ++nlf;
            }
            break;
        case '\r':
            if (prev_cr) ++ncr;
            prev_cr = 1;
            break;
        default:
            if (prev_cr) {
                ++ncr;
                prev_cr = 0;
            }
            break;
        }
    }
    if (prev_cr) ++ncr;

    size_t orig_fsize = fsize;
    fsize -= ncrlf;
    char *content_buf = malloc(sizeof(char) * (INIT_GAP_SIZE + fsize));

    buf_pos = INIT_GAP_SIZE;
    prev_cr = 0;
    for (size_t i = INIT_GAP_SIZE; i < orig_fsize + INIT_GAP_SIZE; i++) {
        switch (tmp_buf[i]) {
        case '\n':
            if (!prev_cr) {
                content_buf[buf_pos] = tmp_buf[i];
                prev_cr = 0;
                ++buf_pos;
            }
            break;
        case '\r':
            content_buf[buf_pos] = '\n';
            prev_cr = 1;
            ++buf_pos;
            break;
        default:
            content_buf[buf_pos] = tmp_buf[i];
            prev_cr = 0;
            ++buf_pos;
            break;
        }
    }

    free(tmp_buf);

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
    if (ncr > nlf && ncr > ncrlf)
        result->line_ending = LEND_CR;
    else if (ncrlf > nlf && ncrlf > ncr)
        result->line_ending = LEND_CRLF;
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

Buffer *buffer_create_system(const char *name) {
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

        if (BUFFER_GET_CHAR(this, i) == '\n') {
            cursor_x = 1;
            ++cursor_y;
        } else if (cursor_x >= term_width &&
                   i + 1 < this->buf_size - (this->gap_end - this->gap_start) &&
                   BUFFER_GET_CHAR(this, i + 1) != '\n') {
            cursor_x = 1;
            ++cursor_y;
        }

        ++i;
    }

    this->cursor_x = cursor_x;
    this->cursor_y = cursor_y;
}

static void buffer_expand(Buffer *this, size_t amount) {
    char *new_buf = malloc(sizeof(char) * (this->buf_size + amount));
    memcpy(new_buf, this->content, this->gap_start);
    memcpy(new_buf + this->gap_end + amount, this->content + this->gap_end, this->buf_size - this->gap_end);
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

        memcpy(this->content + this->gap_start, this->content + this->gap_end, n);
        this->gap_start += n;
        this->gap_end += n;
        this->point += n;
    } else {
        if (n > this->gap_start) {
            write_message("Beginning of buffer.");
            n = this->gap_start;
        }

        memcpy(this->content + this->gap_start - n, this->content + this->gap_end - n, 1);
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
    while (point != 0 && BUFFER_GET_CHAR(this, point) != '\n') {
        ++col;
        --point;
    }

    point = this->point;
    size_t n_forward = 1;

    /* Next, search for next '\n' to calculate the number of  remaining
       character. If point is now on the last line, we only move to end of the
       buffer and exit. */
    while (BUFFER_GET_CHAR(this, point) != '\n') {
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
    while (BUFFER_GET_CHAR(this, point) != '\n' && new_col < col) {
        ++n_forward;
        ++new_col;
        ++point;
    }
    buffer_cursor_move(this, n_forward, 1);
}

void buffer_insert(Buffer *this, char c) {
    if (this->gap_end - this->gap_start < MIN_GAP_SIZE) buffer_expand(this, INIT_GAP_SIZE);

    this->content[this->gap_start] = c;

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

    FILE *f = fopen(this->path, "w");
    if (f == NULL) {
        write_message(strerror(errno));

        return 0;
    }

    char buf[2048];

    size_t n_in_buf = 0;
    size_t i = 0;
    int wait_lf = 0;
    while (i < this->buf_size) {
        if (this->gap_start <= i && i < this->gap_end) {
            i = this->gap_end;

            if (i >= this->buf_size) break;

            continue;
        }

        if (wait_lf) {
            buf[n_in_buf] = '\n';
            wait_lf = 0;
        }

        if (this->line_ending != LEND_LF && this->content[i] == '\n') {
            if (this->line_ending == LEND_CR) {
                buf[n_in_buf] = '\r';
                ++n_in_buf;
            } else if (this->line_ending == LEND_CRLF) {
                buf[n_in_buf] = '\r';
                ++n_in_buf;
                if (n_in_buf == 2048) {
                    wait_lf = 1;
                } else {
                    buf[n_in_buf] = '\n';
                    ++n_in_buf;
                }
            }
        } else {
            buf[n_in_buf] = this->content[i];
            ++n_in_buf;
        }

        ++i;

        if (n_in_buf == 2048) {
            if (fwrite(buf, 1, n_in_buf, f) < n_in_buf) {
                write_message(strerror(errno));
                fclose(f);

                return 0;
            }

            n_in_buf = 0;
        }
    }

    if (fwrite(buf, 1, n_in_buf, f) < n_in_buf) {
        write_message(strerror(errno));
        fclose(f);

        return 0;
    }

    fclose(f);

    write_message("Saved");

    return 1;
}
