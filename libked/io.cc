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

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <vector>

#include <ked/Buffer.hh>
#include <ked/Rune.hh>

#include "libked.hh"

namespace Ked {
    namespace IO {

        /* Converts given buffer's line ending character to lf, and returnes
         * converted buffer. This function may allocate new buffer to store
         * converted data, but this function never deletes old buffer so caller
         * must delete old buffer if returned buffer is not equal to gave one.
         */
        static char *convert_to_lf(char *buf, size_t *len, size_t n_crlf) {
            /* If there's no CRLF, it's not needed to allocate new buffer. */
            if (n_crlf == 0) {
                /* Simply replace \r with \n. */
                for (size_t i = 0; i < *len; ++i) {
                    if (buf[i] == '\r') buf[i] = '\n';
                }

                return buf;
            }

            size_t orig_len = *len;
            *len -= n_crlf;
            char *result = new char[*len];

            size_t buf_pos = 0;
            char prev_cr = 0;
            for (size_t i = 0; i < orig_len; ++i) {
                switch (buf[i]) {
                case '\n':
                    if (!prev_cr) {
                        result[buf_pos] = buf[i];
                        prev_cr = 0;
                        ++buf_pos;
                    }
                    break;
                case '\r':
                    result[buf_pos] = '\n';
                    prev_cr = 1;
                    ++buf_pos;
                    break;
                default:
                    result[buf_pos] = buf[i];
                    prev_cr = 0;
                    ++buf_pos;
                    break;
                }
            }

            return result;
        }

        /* Converts char array to an array of AttrRune. */
        static AttrRune *convert_to_rune_array(char *buf, size_t *len,
                                               size_t gap_size) {
            size_t n_rune = 0;
            for (size_t i = 0; i < *len; ++i) {
                if (buf[i] >> 7 == 0 || (buf[i] >> 6 & 0b11) == 0b11) ++n_rune;
            }

            AttrRune *result = new AttrRune[n_rune + gap_size];

            size_t res_i = gap_size;
            std::array<char, 4> rune_buf;
            rune_buf.fill(0);
            int rune_i = 0;
            for (size_t i = 0; i < *len; ++i) {
                if (buf[i] >> 7 == 0 || (buf[i] >> 6 & 0b11) == 0b11) {
                    if (rune_i != 0) {
                        // FIXME: check if the rune is valid or not.
                        std::copy(std::begin(rune_buf), std::end(rune_buf),
                                  std::begin(result[res_i].c));
                        rune_buf.fill(0);
                        rune_i = 0;
                        ++res_i;
                    }

                    rune_buf[rune_i] = buf[i];
                    ++rune_i;
                } else if (1 <= rune_i && rune_i < (int)sizeof(rune_buf) &&
                           (buf[i] >> 6 & 0b11) == 0b10) {
                    rune_buf[rune_i] = buf[i];
                    ++rune_i;
                } else {
                    // FIXME: broken utf-8 buffer.
                }
            }
            if (rune_i != 0)
                std::copy(std::begin(rune_buf), std::end(rune_buf), std::begin(result[res_i].c));

            for (size_t i = 0; i < n_rune; ++i)
                result[gap_size + i].calculate_width();

            *len = n_rune + gap_size;

            return result;
        }

        static inline void count_line_endings(const char *buf, size_t len,
                                              size_t *lf, size_t *cr,
                                              size_t *crlf) {
            *lf = 0;
            *cr = 0;
            *crlf = 0;
            int prev_cr = 0;
            for (size_t i = 0; i < len; i++) {
                switch (buf[i]) {
                case '\n':
                    if (prev_cr) {
                        ++(*crlf);
                        prev_cr = 0;
                    } else {
                        ++(*lf);
                    }
                    break;
                case '\r':
                    if (prev_cr) ++(*cr);
                    prev_cr = 1;
                    break;
                default:
                    if (prev_cr) {
                        ++(*cr);
                        prev_cr = 0;
                    }
                    break;
                }
            }
            if (prev_cr) ++(*cr);
        }

        /* Reads file f, and returns array of AttrRune. Original line ending is
         * saved to lend. */
        AttrRune *create_content_buffer(std::ifstream &f, size_t *len,
                                        size_t gap_size,
                                        enum LineEnding *lend) {
            char *tmp_buf = new char[*len];
            size_t buf_off = 0;
            size_t read_size = (*len) > 0xffff000 ? 0xffff000 : (*len);
            do {
                f.read(tmp_buf + buf_off, read_size);
                buf_off += f.gcount();
            } while (!f.eof() && buf_off < *len);

            size_t nlf, ncr, ncrlf;
            count_line_endings(tmp_buf, *len, &nlf, &ncr, &ncrlf);

            char *new_tmp_buf = convert_to_lf(tmp_buf, len, ncrlf);
            if (new_tmp_buf != tmp_buf) delete[] tmp_buf;
            AttrRune *result =
                convert_to_rune_array(new_tmp_buf, len, gap_size);
            delete[] new_tmp_buf;

            if (ncr > nlf && ncr > ncrlf)
                *lend = LEND_CR;
            else if (ncrlf > nlf && ncrlf > ncr)
                *lend = LEND_CRLF;
            else
                *lend = LEND_LF;

            return result;
        }

        AttrRune *create_content_buffer_stdin(size_t gap_size, size_t *len,
                                              enum LineEnding *lend) {
            std::array<char, 1024> io_buf;
            std::vector<char> content;
            *len = 0;
            do {
                std::cin.read(io_buf.data(), 1024);
                *len += std::cin.gcount();
                content.insert(content.end(), io_buf.begin(), io_buf.end());
            } while (!std::cin.eof());

            char *buf = content.data();

            size_t nlf, ncr, ncrlf;
            count_line_endings(buf, *len, &nlf, &ncr, &ncrlf);

            buf = convert_to_lf(buf, len, ncrlf);
            AttrRune *result = convert_to_rune_array(buf, len, gap_size);

            if (ncr > nlf && ncr > ncrlf)
                *lend = LEND_CR;
            else if (ncrlf > nlf && ncrlf > ncr)
                *lend = LEND_CRLF;
            else
                *lend = LEND_LF;

            return result;
        }

        bool save_buffer_utf8(Buffer const &buf) {
            if (buf.path == "") return 0;

            std::ofstream out(buf.path);
            if (!out) return false;

            for (size_t p = 0; p < buf.buf_size - (buf.gap_end - buf.gap_start);
                 ++p) {
                AttrRune r = buf.get_rune(p);
                if (r.is_lf() && buf.lend != LEND_LF) {
                    switch (buf.lend) {
                    case LEND_CR:
                        out << '\r';
                        break;
                    case LEND_CRLF:
                        out << "\r\n";
                        break;
                    default:
                        break;
                    }
                    if (out.bad()) return false;
                } else {
                    out << r.c[0];
                    for (int j = 1; j < 4; ++j) {
                        if ((r.c[j] >> 6 & 0b11) != 0b10) break;
                        out << r.c[j];
                    }
                    if (out.bad()) return false;
                }
            }

            return true;
        }

    } // namespace IO
} // namespace Ked
