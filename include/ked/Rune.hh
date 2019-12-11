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

#ifndef KED_RUNE_HH
#define KED_RUNE_HH

#include <array>
#include <vector>
#include <string>

#include "Terminal.hh"

namespace Ked {
    /* Define array with 4 chars to represent UTF-8 character.
     * Actually, Unicode standard allows up to 6 byte of character
     * however, current longest encoding of UTF-8 character is 4 bytes so I
     * define 4 bytes array as UTF-8 character. */
    typedef std::array<unsigned char, 4> Rune;

    /* Hold a unicode character with its display attributes. */
    struct AttrRune {
        /* Holds unicode one character. */
        Rune c;
        /* Width of the character when drawn on terminals */
        unsigned char display_width;
        /* Rune attributes.
         * | MSB          LSB |
         * | ... | protected  |
         * |-----+------------|
         * | ... | 1 bit      | */
        unsigned int attrs;
        /* Face name this Rune should use. */
        std::string face;

        bool is_protected();
        bool is_lf();
        bool operator==(AttrRune const &r);
        bool operator!=(AttrRune const &r);
        void calculate_width();
        void print(Terminal &term) const;

    private:
        void print_char(unsigned char c, Terminal &term) const;
    };

    class String {
    public:
        std::vector<Rune> str;

        /* Converts given char array to array of Rune. */
        String(std::string const &src);
    };
} // namespace Ked

#endif
