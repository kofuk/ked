/*
 * ked -- simple text editor with minimal dependency
 * Copyright (C) 2019  Koki Fukuda
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <cstring>
#include <fstream>
#include <functional>
#include <string>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <ked/Buffer.hh>

#include "libked.hh"

namespace Ked {
    void Buffer::BufferListener::call(Buffer &buf) {
        /* Drop if calling to avoid infinite loop. */
        if (calling) return;

        calling = true;
        for (auto itr = std::begin(listener); itr != std::end(listener);
             ++itr) {
            (*itr)(buf);
        }
        calling = false;
    }

    Buffer::Buffer()
        : point(0), buf_size(0), gap_start(0), gap_end(0), lend(LEND_LF),
          visible_start_point(0), display_range_x_start(0),
          display_range_x_end(0), display_range_y_start(0),
          display_range_y_end(0), modified(false), cursor_x(1), cursor_y(1) {}

    Buffer::Buffer(std::string const &name) : Buffer() {
        buf_name = name;

        buf_size = INIT_GAP_SIZE;
        gap_end = INIT_GAP_SIZE;
        content = new AttrRune[buf_size];
    }

    Buffer::Buffer(std::string const &name, std::string const &file_path)
        : Buffer() {
        buf_name = name;
        path = file_path;

        struct stat stat_buf;
        if (stat(file_path.c_str(), &stat_buf) != 0) {
            buf_size = INIT_GAP_SIZE;
            gap_end = INIT_GAP_SIZE;
            content = new AttrRune[buf_size];

            return;
        }

        std::ifstream f(file_path);
        if (!f) {
            buf_size = INIT_GAP_SIZE;
            gap_end = INIT_GAP_SIZE;
            content = new AttrRune[buf_size];
        }
        buf_size = stat_buf.st_size;
        gap_end = INIT_GAP_SIZE;
        content = IO::create_content_buffer(f, &buf_size, INIT_GAP_SIZE, &lend);
    }

    Buffer::~Buffer() {
        delete[] content;
        content = nullptr;
    }

    void Buffer::update_cursor_position() {
        unsigned int new_cursor_x = 1;
        unsigned int new_cursor_y = 1;

        if (point < visible_start_point) {
            cursor_y = 0;

            return;
        }

        AttrRune *r;
        size_t i = visible_start_point;
        while (i < buf_size - (gap_end - gap_start) && i < point) {
            r = get_rune_ptr(i);
            new_cursor_x += r->display_width;

            if (r->is_lf()) {
                new_cursor_x = 1;
                ++new_cursor_y;
            } else if (i + 1 < buf_size - (gap_end - gap_start) &&
                       i + 1 < point) {
                AttrRune *next = get_rune_ptr(i + 1);
                if (!next->is_lf() &&
                    new_cursor_x + next->display_width >
                        (display_range_x_end - display_range_x_start)) {
                    new_cursor_x = 1;
                    ++new_cursor_y;
                }
            }

            ++i;
        }

        cursor_x = new_cursor_x;
        cursor_y = new_cursor_y;
    }

    void Buffer::expand(std::size_t amount) {
        AttrRune *new_buf = new AttrRune[buf_size + amount];
        for (std::size_t i = 0; i < gap_start; ++i)
            new_buf[i] = content[i];
        for (std::size_t i = gap_end; i < buf_size - gap_end; ++i)
            new_buf[i + amount] = content[i];
        delete[] content;

        content = new_buf;
        gap_end += amount;
        buf_size += amount;
    }

    void Buffer::scroll_in_need() {
        if (cursor_y > display_range_y_end - display_range_y_start) {
            scroll(1, true);

            update_cursor_position();
        } else if (cursor_y == 0) {
            scroll(1, false);

            update_cursor_position();
        }
    }

    void Buffer::cursor_move(std::size_t n, bool forward) {
        if (n == 0) return;

        if (forward) {
            if (n > buf_size - gap_end) {
                n = buf_size - gap_end;
            }

            for (std::size_t i = 0; i < n; ++i)
                content[gap_start + i] = content[gap_end + i];
            gap_start += n;
            gap_end += n;
            point += n;
        } else {
            if (n > gap_start) {
                n = gap_start;
            }

            for (std::size_t i = 0; i < n; ++i)
                content[gap_end - i - 1] = content[gap_start - i - 1];
            gap_start -= n;
            gap_end -= n;
            point -= n;
        }

        update_cursor_position();

        scroll_in_need();

        on_cursor_move_listeners.call(*this);
    }

    void Buffer::insert(Rune const &r) {
        if (gap_end - gap_start < MIN_GAP_SIZE) expand(INIT_GAP_SIZE);

        content[gap_start].c = r;
        content[gap_start].face_name = default_face_name;
        content[gap_start].calculate_width();

        ++gap_start;
        ++point;

        modified = true;

        update_cursor_position();

        scroll_in_need();

        on_cursor_move_listeners.call(*this);
    }

    void Buffer::insert(char const c) {
        Rune r;
        r.fill(0);
        r[0] = c;
        insert(r);
    }

    void Buffer::delete_backward() {
        if (point == 0) return;

        if (get_rune(point - 1).is_protected()) return;

        --gap_start;
        --point;

        update_cursor_position();

        scroll_in_need();

        on_cursor_move_listeners.call(*this);
    }

    void Buffer::delete_forward() {
        if (point >= buf_size - (gap_end - gap_start)) return;

        if (get_rune(point).is_protected()) return;

        ++gap_end;

        update_cursor_position();

        /* Do NOT call on_cursor_move_listener here because forward deleting do
         * not change cursor point. */
    }

    void Buffer::scroll(std::size_t n, bool forward) {
        if (forward) {
            std::size_t new_point = visible_start_point;
            while (new_point < buf_size - (gap_end - gap_start) && n != 0) {
                if (get_rune(new_point).is_lf()) --n;

                ++new_point;
            }

            visible_start_point = new_point;
        } else {
            if (visible_start_point == 0) return;

            ++n;
            std::size_t new_point = visible_start_point - 1;
            while (new_point != 0 && n != 0) {
                if (get_rune(new_point).is_lf()) {
                    --n;
                    if (n == 0) {
                        ++new_point;

                        break;
                    }
                }

                --new_point;
            }

            visible_start_point = new_point;
        }
    }

    SearchResult *Buffer::search(std::size_t start_point, String const &search,
                                 bool forward) const {
        SearchResult *result = new SearchResult;
        if (search.str.size() == 0) {
            result->start = start_point;
            result->end = start_point + 1;

            return result;
        }

        if (forward) {
            std::size_t search_i = 0;
            for (std::size_t i = start_point;
                 i < buf_size - (gap_end - gap_start); ++i) {
                if (search.str[search_i] == get_rune(i).c) {
                    ++search_i;

                    if (search_i >= search.str.size()) {
                        result->start = i - (search.str.size() - 1);
                        result->end = i + 1;

                        return result;
                    }
                } else if (search_i != 0) {
                    i -= (search_i - 1);
                    search_i = 0;
                }
            }
        } else {
            std::size_t search_i = search.str.size() - 1;

            if (start_point == 0) {
                return 0;
            }

            size_t i = start_point - 1;
            for (; i != 0; --i) {
                if (search.str[search_i] == get_rune(i).c) {
                    if (search_i == 0) {
                        result->start = i;
                        result->end = i + search.str.size();

                        return result;
                    }

                    --search_i;
                } else if (search_i != search.str.size() - 1) {
                    i += (search.str.size() - search_i);
                    search_i = search.str.size() - 1;
                }
            }

            if (search.str[search_i] == get_rune(i).c) {
                if (search_i == 0) {
                    result->start = i;
                    result->end = i + search.str.size();

                    return result;
                } else {
                    delete result;

                    return 0;
                }
            }
        }

        delete result;

        return 0;
    }

    bool Buffer::save() {
        if (!modified) return false;

        bool success = IO::save_buffer_utf8(*this);
        if (success) modified = false;

        return success;
    }

    AttrRune &Buffer::get_rune(std::size_t point) const {
        return gap_start <= point ? content[point + (gap_end - gap_start)]
                                  : content[point];
    }

    AttrRune *Buffer::get_rune_ptr(std::size_t point) const {
        return gap_start <= point ? content + point + (gap_end - gap_start)
                                  : content + point;
    }

    void
    Buffer::add_cursor_move_listener(std::function<void(Buffer &)> listener) {
        on_cursor_move_listeners.listener.push_back(listener);
    }

    Buffer *buffer_from_stdin() {
        size_t len;
        enum LineEnding lend;
        AttrRune *content_buf =
            Ked::IO::create_content_buffer_stdin(1024, &len, &lend);

        Buffer *result = new Buffer;
        result->buf_name = "STDIN";
        result->content = content_buf;
        result->buf_size = len;
        result->gap_end = INIT_GAP_SIZE;
        result->lend = lend;

        return result;
    }
} // namespace Ked
