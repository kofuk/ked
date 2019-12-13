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

#ifndef KED_BUFFER_HH
#define KED_BUFFER_HH

#include <functional>
#include <string>

#include "Rune.hh"

/* Amont of buffer allocate once.  */
#define INIT_GAP_SIZE 1024
/* Allocates additional buffer when gap is smaller than MIN_GAP_SIZE. */
#define MIN_GAP_SIZE 32

namespace Ked {
    enum LineEnding { LEND_LF, LEND_CR, LEND_CRLF };

    struct SearchResult {
        std::size_t start;
        std::size_t end;
    };

    /* Buffer struct defines editing buffer for each file. every edit event take
     * place on buffer, rather than UI. */
    class Buffer {
        struct BufferListener {
            std::vector<std::function<void(Buffer &)>> listener;
            bool calling;

            void call(Buffer &buf);
        };

        BufferListener on_cursor_move_listeners;

        void update_cursor_position();
        void expand(std::size_t amount);
        void scroll_in_need();

    public:
        /* Buffer name to be displayed. */
        std::string buf_name;
        /* Buffer file path to be saved. */
        std::string path;
        /* Buffer file path to be saved. */
        AttrRune *content;
        /* Cursor position in this buffer excluding gap area. */
        std::size_t point;
        /* Buffer size including gap. */
        std::size_t buf_size;
        /* Start index of gap in this buffer. Inclusive. */
        std::size_t gap_start;
        /* End index of gap in this buffer. Exclusive */
        std::size_t gap_end;
        /* Line ending character for this file. */
        LineEnding lend;
        /* Point that should be placed on top-left. */
        std::size_t visible_start_point;

        /* Range in the display this buffer to be displayed. Must be specified
         * in [start..end). */
        std::size_t display_range_x_start;
        std::size_t display_range_x_end;
        std::size_t display_range_y_start;
        std::size_t display_range_y_end;
        /* Whether this buffer is modified of not. */
        bool modified;
        /* Default face name. */
        std::string default_face;
        /* Cursor X position in display area. */
        std::size_t cursor_x;
        /* Cursor Y position in display area. */
        std::size_t cursor_y;

        /* Constructor that initializes fundamental members. */
        Buffer();

        /* Constructs Buffer with size of INITIAL_BUFFER_SIZE. */
        Buffer(std::string const &name);
        /* Constructs Buffer with buffer of content of path. path must be
         * regular file. */
        Buffer(std::string const &name, std::string const &path);
        ~Buffer();
        /* Creates buffer for the path, specified in 1st argument, with name of
         * 2nd argument. If file is not exisiting, ked creates the file when
         * saved. */
        void cursor_move(std::size_t n, bool forward);
        /* Insertes Rune to buffer point position. */
        void insert(Rune const &r);
        /* Insertes char to buffer point position. */
        void insert(char c);
        /* Deletes 1 character backward. */
        void delete_backward();
        /* Deletes 1 character forward. */
        void delete_forward();
        /* Scroll for n lines vertically to forward or backward. */
        void scroll(std::size_t n_lines, bool forward);
        /* Searches specified string from Buffer and returns the range it
         * occrrs. If
         * the string not found after or before start_point, returns null. */
        SearchResult *search(std::size_t start_point, String const &search,
                             bool forward) const;
        /* Saves buffer content. */
        bool save();
        /* Gets point's rune. */
        AttrRune &get_rune(std::size_t point) const;
        AttrRune *get_rune_ptr(std::size_t point) const;

        /* Adds listener to be called just after change buffer's point in any
         * way. */
        void add_cursor_move_listener(std::function<void(Buffer &)> listener);
    };

    Buffer *buffer_from_stdin();
} // namespace Ked
#endif
