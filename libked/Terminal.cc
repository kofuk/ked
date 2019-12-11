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

#include <string>

#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <ked/Terminal.hh>

namespace Ked {
    Terminal::Terminal() : io_buffer_off(0) {
        struct winsize w;
        ioctl(0, TIOCGWINSZ, &w);
        width = (std::size_t)w.ws_col;
        height = (std::size_t)w.ws_row;

        /* Save cursor, switch to alternate screen, and clear screen. */
        put_str("\e[?1049h");
        flush_buffer();

        orig_termios = new termios;
        tcgetattr(0, orig_termios);

        termios new_termios = *orig_termios;

        /* Disable XOFF and XON flow control. */
        new_termios.c_iflag &= ~(IXOFF | IXON | IGNCR);
        /* Set noncanonical mode, no echo, etc. */
        new_termios.c_lflag &= ~(ISIG | ICANON | ECHO);
        /* read byte-by-byte with blocking io. */
        new_termios.c_cc[VMIN] = 1;
        new_termios.c_cc[VTIME] = 0;

        tcsetattr(0, TCSADRAIN, &new_termios);
    }

    Terminal::~Terminal() {
        tcsetattr(0, TCSADRAIN, orig_termios);
        delete orig_termios;

        /* Clear screen and switch to normal screen, and restore cursor
         * position. */
        put_str("\e[?1049l");
        flush_buffer();
    }

    void Terminal::put_char(char c) {
        if (io_buffer_off >= 1024) flush_buffer();
        io_buffer[io_buffer_off++] = c;
    }

    void Terminal::put_str(char const *str) {
        for (; *str; ++str) {
            if (io_buffer_off >= 1024) flush_buffer();

            io_buffer[io_buffer_off++] = *str;
        }
    }

    void Terminal::put_str(std::string const &str) {
        for (auto itr = std::begin(str); itr != std::end(str); ++itr) {
            if (io_buffer_off >= 1024) flush_buffer();

            io_buffer[io_buffer_off++] = *itr;
        }
    }

    void Terminal::put_buf(char const *buf, std::size_t len) {
        for (size_t i = 0; i < len; ++i) {
            if (io_buffer_off >= 1024) flush_buffer();

            io_buffer[io_buffer_off++] = buf[i];
        }
    }

    char Terminal::get_char() {
        char buf[1];
        read(0, buf, 1);

        return *buf;
    }

    void Terminal::move_cursor(unsigned int x, unsigned int y) {
        put_str("\e[");
        put_str(std::to_string(y));
        put_str(";");
        put_str(std::to_string(x));
        put_str("f");
    }

    void Terminal::flush_buffer() {
        write(STDOUT_FILENO, io_buffer, io_buffer_off);
        io_buffer_off = 0;
    }

} // namespace Ked
