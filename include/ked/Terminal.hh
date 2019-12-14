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

#ifndef KED_TERMINAL_HH
#define KED_TERMINAL_HH

#include <string>

#include <termios.h>

namespace Ked {
    class Terminal {
        char io_buffer[4096];
        std::size_t io_buffer_off;

        struct termios *orig_termios;

      public:
        std::size_t width;
        std::size_t height;

        /* Initializes terminal to use alternate screen and noncanonical mode.
         */
        Terminal();
        /* Restores original terminal settings. */
        ~Terminal();

        /* Write 1 byte to the terminal. */
        void put_char(char c);
        void put_str(char const *str);
        void put_str(std::string const &str);
        void put_buf(char const *buf, std::size_t len);
        /* Reads 1 byte from stdin and return the value. */
        char get_char();

        void move_cursor(unsigned int, unsigned int);

        /* Flushes terminal IO buffer. */
        void flush_buffer();
    };

} // namespace Ked

#endif
