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
#include <array>
#include <vector>

#include <ked/Rune.hh>
#include <ked/Terminal.hh>

namespace Ked {

    // AttrRune
    bool AttrRune::is_protected() const { return (attrs & 1); }

    bool AttrRune::is_lf() const {
        if (c[0] != '\n') return false;

        for (auto itr = c.begin() + 1; itr != c.end(); ++itr)
            if (*itr != 0) return false;

        return true;
    }

    const AttrRune &AttrRune::operator=(const AttrRune &r) {
        std::copy(std::begin(r.c), std::end(r.c), std::begin(c));
        display_width = r.display_width;
        attrs = r.attrs;
        face = r.face;

        return r;
    }

    bool AttrRune::operator==(AttrRune const &r) const {
        return r.c == c && r.face == face;
    }

    bool AttrRune::operator!=(AttrRune const &r) const {
        return !operator==(r);
    }

    void AttrRune::calculate_width() {
        if ((c[0] >> 7 & 1) == 0) {
            unsigned char ch = c[0];

            if (ch == '\t')
                display_width = 8;
            else if (ch <= 0x1f)
                display_width = 2;
            else
                display_width = 1;
        } else
            display_width = 2;
    }

    void AttrRune::print(Terminal &term) const {
        if ((c[0] >> 7 & 1) == 0) {
            print_char(c[0], term);
        } else {
            size_t len = 1;
            for (; len < 4; ++len)
                if ((c[len] >> 6 & 0b11) != 0b10) break;

            term.put_buf((char const *)c.data(), len);
        }
    }

    void AttrRune::print_char(unsigned char c, Terminal &term) const {
        if (c == '\t')
            term.put_str("        ");
        else if (c <= 0x1f) {
            term.put_char('^');
            term.put_char(c + '@');
        } else
            term.put_char(c);
    }

    // String
    String::String(std::string const &src) {
        size_t n_rune = 0;
        for (auto itr = src.begin(); itr != src.end(); ++itr) {
            if (*itr >> 7 == 0 || (*itr >> 6 & 0b11) == 0b11) ++n_rune;
        }

        str.reserve(n_rune);
        Rune buf;
        buf.fill(0);
        int buf_i = 0;
        for (auto itr = src.begin(); itr != src.end(); ++itr) {
            if (*itr >> 7 == 0 || (*itr >> 6 & 0b11) == 0b11) {
                if (buf_i != 0) {
                    str.push_back(buf);
                    buf.fill(0);
                    buf_i = 0;
                }
                buf[buf_i] = *itr;
                ++buf_i;
            } else if (buf_i != 0 && (*itr >> 6 & 0b11) == 0b10) {
                if (buf_i >= 4) {
                    // FIXME
                    throw 1;
                }

                buf[buf_i] = *itr;
                ++buf_i;
            } else {

                // FIXME
                throw 1;
            }
        }
        if (buf_i != 0) str.push_back(buf);
    }

} // namespace Ked
