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

#ifndef LIBKED_HH
#define LIBKED_HH

#include <fstream>

#include <ked/Buffer.hh>
#include <ked/Rune.hh>

namespace Ked {
    namespace IO {
        /* Reads up to given length of file and create an array of AttrRune.
         * Length pointer will be updated to represent length of array of
         * AttrRune. */
        AttrRune *create_content_buffer(std::ifstream &f, size_t *len, size_t gap_size,
                                        enum LineEnding *lend);

        /* Reads from stdin, and returns array of AttrRune with specfied first
         * gap size. Buffer size and line ending is stored to pointer len and
         * lend. */
        AttrRune *create_content_buffer_stdin(size_t gap_size, size_t *len,
                                              enum LineEnding *lend);

        /* Saves buffer as UTF-8 text file. */
        bool save_buffer_utf8(Buffer const &buf);

    } // namespace IO
} // namespace Ked

#endif
