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

#include <ked/buffer.h>
#include <ked/internal.h>
#include <ked/ked.h>
#include <ked/rune.h>

#include "libked.h"

Buffer *current_buffer;

static Buffer *buffer_create_empty(void) {
    Buffer *result = malloc(sizeof(Buffer));
    memset(result, 0, sizeof(Buffer));
    result->cursor_x = 1;
    result->cursor_y = 1;

    result->listener = malloc(sizeof(struct BufferListeners));
    result->listener->on_cursor_move =
        malloc(sizeof(struct BufferListenerList));
    memset(result->listener->on_cursor_move, 0,
           sizeof(struct BufferListenerList));
    result->listener->on_content_change =
        malloc(sizeof(struct BufferListenerList));
    memset(result->listener->on_cursor_move, 0,
           sizeof(struct BufferListenerList));

    return result;
}

static Buffer *buffer_create_existing_file(const char *path,
                                           const char *buf_name,
                                           struct stat stat_buf) {
    FILE *f = fopen(path, "r");
    if (f == NULL) return NULL;

    size_t size = (size_t)stat_buf.st_size;

    enum LineEnding lend;
    AttrRune *content_buf =
        create_content_buffer(f, &size, INIT_GAP_SIZE, &lend);
    fclose(f);

    size_t path_len = strlen(path);
    char *path_buf = malloc(sizeof(char) * (path_len + 1));
    memcpy(path_buf, path, path_len + 1);

    size_t buf_name_len = strlen(buf_name);
    char *buf_name_buf = malloc(sizeof(char) * (buf_name_len + 1));
    memcpy(buf_name_buf, buf_name, buf_name_len + 1);

    Buffer *result = buffer_create_empty();
    result->buf_name = buf_name_buf;
    result->path = path_buf;
    result->content = content_buf;
    result->buf_size = size;
    result->gap_end = INIT_GAP_SIZE;
    result->line_ending = lend;

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

    Buffer *result = buffer_create_empty();
    result->buf_name = buf_name_buf;
    result->path = path_buf;
    result->content = content_buf;
    result->buf_size = INIT_GAP_SIZE;
    result->gap_end = INIT_GAP_SIZE;

    return result;
}

Buffer *buffer_create_system(const char *name) {
    AttrRune *content_buf = malloc(sizeof(AttrRune) * INIT_GAP_SIZE);

    Buffer *result = buffer_create_empty();
    result->buf_name = strdup(name);
    result->system = 1;
    result->content = content_buf;
    result->buf_size = INIT_GAP_SIZE;
    result->gap_end = INIT_GAP_SIZE;

    return result;
}

void buffer_destruct(Buffer *this) {
    if (this == NULL) return;

    free(this->buf_name);
    free(this->path);
    free(this->content);

    free(this->listener->on_content_change);
    free(this->listener->on_cursor_move);
    free(this->listener);

    free(this);
}

static void buffer_call_listener(Buffer *this,
                                 struct BufferListenerList *listeners) {
    if (listeners->listner_calling) return;

    listeners->listner_calling = 1;
    for (size_t i = 0; i < listeners->i; ++i)
        (*(listeners->listeners[i]))(this);
    listeners->listner_calling = 0;
}

static void buffer_update_cursor_position(Buffer *this) {
    unsigned int cursor_x = 1;
    unsigned int cursor_y = 1;

    if (this->point < this->visible_start_point) {
        this->cursor_y = 0;

        return;
    }

    AttrRune r;
    size_t i = this->visible_start_point;
    while (i < this->buf_size - (this->gap_end - this->gap_start) &&
           i < this->point) {
        r = buffer_get_rune(this, i);
        cursor_x += r.display_width;

        if (rune_is_lf(r)) {
            cursor_x = 1;
            ++cursor_y;
        } else if (i + 1 < this->buf_size - (this->gap_end - this->gap_start) &&
                   i + 1 < this->point) {
            AttrRune next = buffer_get_rune(this, i + 1);
            if (cursor_x + next.display_width >= term_width &&
                !rune_is_lf(next)) {
                cursor_x = 1;
                ++cursor_y;
            }
        }

        ++i;
    }

    this->cursor_x = cursor_x;
    this->cursor_y = cursor_y;

    buffer_call_listener(this, this->listener->on_cursor_move);
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

static void buffer_scroll_in_need(Buffer *this) {
    if (this->cursor_y >
        this->display_range_y_end - this->display_range_y_start) {
        buffer_scroll(this, 1, 1);

        buffer_update_cursor_position(this);
    } else if (this->cursor_y == 0) {
        buffer_scroll(this, 1, 0);

        buffer_update_cursor_position(this);
    }
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

    buffer_scroll_in_need(this);

    buffer_call_listener(this, this->listener->on_cursor_move);
}

void buffer_insert(Buffer *this, Rune r) {
    if (this->gap_end - this->gap_start < MIN_GAP_SIZE)
        buffer_expand(this, INIT_GAP_SIZE);

    memcpy(this->content[this->gap_start].c, r, sizeof(Rune));
    this->content[this->gap_start].face = this->default_face;
    attr_rune_set_width(this->content + this->gap_start);

    ++(this->gap_start);
    ++(this->point);

    this->modified = 1;

    buffer_update_cursor_position(this);

    buffer_scroll_in_need(this);

    buffer_call_listener(this, this->listener->on_content_change);
    buffer_call_listener(this, this->listener->on_cursor_move);
}

void buffer_delete_backward(Buffer *this) {
    if (this->point == 0) {
        write_message("Beginning of buffer.");

        return;
    }

    if (RUNE_PROTECTED(buffer_get_rune(this, this->point - 1).attrs)) {
        write_message("Write protected.");

        return;
    }

    --(this->gap_start);
    --(this->point);

    buffer_update_cursor_position(this);

    buffer_scroll_in_need(this);

    buffer_call_listener(this, this->listener->on_content_change);
    buffer_call_listener(this, this->listener->on_cursor_move);
}

void buffer_delete_forward(Buffer *this) {
    if (this->point >= this->buf_size - (this->gap_end - this->gap_start)) {
        write_message("End of buffer.");

        return;
    }

    if (RUNE_PROTECTED(buffer_get_rune(this, this->point).attrs)) {
        write_message("Write protected.");

        return;
    }

    ++(this->gap_end);

    buffer_update_cursor_position(this);

    buffer_call_listener(this, this->listener->on_content_change);
}

void buffer_scroll(Buffer *this, size_t n, char forward) {
    if (forward) {
        size_t point = this->visible_start_point;
        while (point < this->buf_size - (this->gap_end - this->gap_start) &&
               n != 0) {
            if (rune_is_lf(buffer_get_rune(this, point))) --n;

            ++point;
        }

        this->visible_start_point = point;
    } else {
        if (this->visible_start_point == 0) return;

        ++n;
        size_t point = this->visible_start_point - 1;
        while (point != 0 && n != 0) {
            if (rune_is_lf(buffer_get_rune(this, point))) {
                --n;
                if (n == 0) {
                    ++point;

                    break;
                }
            }

            --point;
        }

        this->visible_start_point = point;
    }
}

int buffer_search(Buffer *this, size_t start_point, String *search,
                  char forward, struct SearchResult *result) {
    if (search->len == 0) {
        result->start = start_point;
        result->end = start_point + 1;

        return 1;
    }

    if (forward) {
        size_t search_i = 0;
        for (size_t i = start_point;
             i < this->buf_size - (this->gap_end - this->gap_start); ++i) {
            if (rune_eq(search->str[search_i], buffer_get_rune(this, i).c)) {
                ++search_i;

                if (search_i >= search->len) {
                    result->start = i - (search->len - 1);
                    result->end = i + 1;

                    return 1;
                }
            } else if (search_i != 0) {
                i -= (search_i - 1);
                search_i = 0;
            }
        }
    } else {
        size_t search_i = search->len - 1;
        size_t i = start_point;
        for (; i != 0; --i) {
            if (rune_eq(search->str[search_i], buffer_get_rune(this, i).c)) {
                if (search_i == 0) {
                    result->start = i;
                    result->end = i + search->len;

                    return 1;
                }

                --search_i;
            } else if (search_i != search->len - 1) {
                i += (search->len - search_i);
                search_i = search->len - 1;
            }
        }

        if (rune_eq(search->str[search_i], buffer_get_rune(this, i).c)) {
            if (search_i == 0) {
                result->start = i;
                result->end = i + search->len;

                return 1;
            } else {
                return 0;
            }
        }
    }

    return 0;
}

int buffer_save(Buffer *this) {
    if (!this->modified) {
        write_message("Buffer is not modified.");

        return 0;
    }

    return save_buffer_utf8(this);
}

static void set_buffer_listener_internal(struct BufferListenerList *list,
                                         BufferListener l) {
    for (size_t i = 0; i < list->i; ++i)
        if (list->listeners[i] == l) return;

    if (list->i >= list->len) {
        if (list->len == 0) {
            list->len = 1;
        } else if (list->len <= 8) {
            list->len = list->len << 1;
        } else {
            list->len += 8;
        }
        list->listeners = realloc(list->listeners, list->len);
    }

    list->listeners[list->i] = l;
    ++(list->i);
}

void buffer_add_on_cursor_move_listener(Buffer *this, BufferListener l) {
    if (this->system) return;

    set_buffer_listener_internal(this->listener->on_cursor_move, l);
}

void buffer_add_on_content_change_listener(Buffer *this, BufferListener l) {
    if (this->system) return;

    set_buffer_listener_internal(this->listener->on_content_change, l);
}
