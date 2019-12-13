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

#include <array>
#include <cstring>
#include <functional>
#include <iterator>
#include <mutex>
#include <string>
#include <vector>

#include <ked/Buffer.hh>
#include <ked/Rune.hh>
#include <ked/Ui.hh>

namespace Ked {
    namespace KeyHandling {
        Keybind::~Keybind() {
            for (auto itr = std::begin(binding); itr != std::end(binding);
                 ++itr) {
                delete *itr;
            }
        }

        char *Keybind::compile_key(std::string const &key) {
            std::size_t len = key.size();

            for (auto itr = std::begin(key); itr != std::end(key); ++itr) {
                if (*itr == '^' && itr != std::end(key) - 1) --len;
            }

            char *result = new char[len + 1];
            std::size_t r_i = 0;
            result[len] = 0;
            for (auto itr = std::begin(key); itr != std::end(key); ++itr) {
                if (*itr == '^' && itr != std::end(key) - 1) {
                    result[r_i] = *(++itr) - '@';
                } else {
                    result[r_i] = *itr;
                }
                ++r_i;
            }

            return result;
        }

        void Keybind::add(std::string const &key, EditorCommand func) {
            char *seq = compile_key(key);

            for (auto itr = std::begin(binding); itr != std::end(binding);
                 ++itr) {
                int cmp = (*itr)->key.compare(seq);
                if (cmp == 0) {
                    (*itr)->func = func;
                    delete[] seq;

                    return;
                } else if (cmp > 0) {
                    BindingElement *e = new BindingElement;
                    e->key = std::string(seq);
                    e->func = func;
                    binding.insert(itr, e);

                    delete[] seq;

                    return;
                }
            }
            BindingElement *e = new BindingElement;
            e->key = std::string(seq);
            e->func = func;
            binding.insert(std::end(binding), e);
            delete[] seq;
        }

        int Keybind::compare_key(std::string const &key,
                                 std::vector<char> const &seq) {
            std::size_t len = seq.size() > key.size() ? key.size() : seq.size();
            for (std::size_t i = 0; i < len; ++i) {
                int cmp = seq[i] - key[i];
                if (cmp != 0) return cmp;
            }
            if (seq.size() == key.size())
                return 0;
            else if (seq.size() < key.size())
                return -key[len];
            else
                return seq[len];
        }

        int Keybind::compare_key(std::string const &key,
                                 std::vector<char> const &seq, std::size_t n) {
            char *str = new char[seq.size() + 1];
            str[seq.size()] = 0;
            for (std::size_t i = 0; i < seq.size(); ++i)
                str[i] = seq[i];

            int cmp = std::strncmp(key.c_str(), str, n);
            delete[] str;

            return cmp;
        }

        KeyBindState Keybind::handle(std::vector<char> const &seq, Ui &ui,
                                     Buffer &buf) {
            for (auto itr = std::begin(binding); itr != std::end(binding);
                 ++itr) {
                if (compare_key((*itr)->key, seq, seq.size()) == 0) {
                    int cmp = compare_key((*itr)->key, seq);
                    if (cmp == 0) {
                        ((*itr)->func)(ui, buf);

                        return KEYBIND_HANDLED;
                    }

                    return KEYBIND_WAIT;
                }
            }

            if (seq.size() != 1) return KEYBIND_HANDLED;

            return KEYBIND_NOT_HANDLED;
        }

        static std::vector<char> key_buf;

        void handle_key(Ui &ui, unsigned char c) {
            key_buf.push_back(c);

            switch (ui.global_keybind.handle(key_buf, ui, *ui.current_buffer)) {
            case KEYBIND_NOT_HANDLED:
                ui.current_buffer->insert(c);
                // fall through
            case KEYBIND_HANDLED:
                key_buf.clear();
                break;
            case KEYBIND_WAIT:
                break;
            }
        }

        void handle_rune(Ui &ui, Rune &r) {
            key_buf.clear();
            ui.current_buffer->insert(r);
        }

    } // namespace KeyHandling

    Ui::Ui(Terminal *term) : editor_exited(false), term(term) {
        init_system_buffers();
        display_buffer.resize(term->width * term->height);
    }

    Ui::~Ui() {
        for (auto itr = std::begin(buffers); itr != std::end(buffers); ++itr)
            delete *itr;
    }

    /* Draws char to the terminal if needed. */
    void Ui::draw_char(unsigned char c, std::string const &face_name, unsigned int x,
                       unsigned int y) {
        if (x > term->width || y > term->height ||
            (display_buffer[(y - 1) * term->width + x - 1].c[0] == c &&
             face_name == display_buffer[(y - 1) * term->width + x - 1].face_name) ||
            c == '\n')
            return;

        if (face_name != current_face_name) {
            term->put_str(Face::lookup(face_name));

            current_face_name = face_name;
        }

        term->move_cursor(x, y);
        term->put_char(c);
        display_buffer[(y - 1) * term->width + x - 1].c[0] = c;
        display_buffer[(y - 1) * term->width + x - 1].face_name = face_name;
        for (int i = 1; i < 4; ++i)
            display_buffer[(y - 1) * term->width + x - 1].c[i] = 0;
    }

    /* Draws AttrRune with its attrubutes to the termianl if needed. */
    void Ui::draw_rune(AttrRune const &r, std::string const &default_face,
                       unsigned int x, unsigned int y) {
        std::string face_name = r.face_name;
        if (face_name.empty()) {
            face_name = default_face;
        }

        if (x > term->width || y > term->height ||
            (display_buffer[(y - 1) * term->width + x - 1].c == r.c &&
             face_name == display_buffer[(y - 1) * term->width + x - 1].face_name) ||
            r.c[0] == '\n')
            return;

        if (face_name != current_face_name) {
            term->put_str(Face::lookup(face_name));

            current_face_name = face_name;
        }

        term->move_cursor(x, y);
        r.print(*term);
        display_buffer[(y - 1) * term->width + x - 1] = r;
    }

    /* Make next drawing in the position to be redrawn. */
    void Ui::invalidate_point(unsigned int x, unsigned int y) {
        if (x > term->width || y >= term->height) return;

        /* \n will never drawn. */
        display_buffer[(y - 1) * term->width + x - 1].c[0] = '\n';
    }

    void Ui::buffer_show(Buffer *buf) {
        displayed_buffers.push_back(buf);
        current_buffer = buf;
    }

    void Ui::buffer_add(Buffer *buf) {
        buffers.push_back(buf);

        for (auto itr = std::begin(on_buffer_entry_changed_listener);
             itr != std::end(on_buffer_entry_changed_listener); ++itr) {
            (*itr)(buffers);
        }
    }

    void Ui::init_system_buffers() {
        Buffer *header = new Buffer("__system_header__");
        header->display_range_x_start = 1;
        header->display_range_x_end = term->width;
        header->display_range_y_start = 1;
        header->display_range_y_end = 2;
        header->insert('K');
        header->insert('e');
        header->insert('d');
        buffer_add(header);
        buffer_show(header);

        Buffer *footer = new Buffer("__system_footer__");
        footer->display_range_x_start = 1;
        footer->display_range_x_end = term->width + 1;
        footer->display_range_y_start = term->height;
        footer->display_range_y_end = term->height + 1;
        buffer_add(footer);
        buffer_show(footer);
    }

    void Ui::exit_editor() { editor_exited = true; }

    void Ui::redraw_editor() {
        std::lock_guard<std::mutex> lock(display_buffer_mutex);

        unsigned int x = 1;
        unsigned int y = 1;

        size_t i;
        AttrRune *c;
        for (size_t b = 0; b < displayed_buffers.size(); ++b) {
            Buffer *buf = displayed_buffers[b];

            x = (unsigned int)buf->display_range_x_start;
            y = (unsigned int)buf->display_range_y_start;
            i = buf->visible_start_point;
            while (i < buf->buf_size - (buf->gap_end - buf->gap_start)) {
                if (y >= buf->display_range_y_end) break;

                c = buf->get_rune_ptr(i);

                if (c->is_lf()) {
                    for (unsigned int j = x; j <= term->width; ++j)
                        draw_char(' ', buf->default_face_name, j, y);

                    ++y;

                    x = 1;
                } else {
                    draw_rune(*c, buf->default_face_name, x, y);

                    for (unsigned int j = x + 1; j < x + c->display_width; ++j)
                        invalidate_point(j, y);

                    x += c->display_width;

                    if (i + 1 <
                        buf->buf_size - (buf->gap_end - buf->gap_start)) {
                        AttrRune *next_rune = buf->get_rune_ptr(i + 1);
                        if (!next_rune->is_lf() &&
                            x + next_rune->display_width >= term->width) {
                            if (x + next_rune->display_width == term->width) {
                                draw_char('\\', buf->default_face_name, x, y);
                            } else {
                                draw_char(' ', buf->default_face_name, x, y);
                                ++x;
                                draw_char('\\', buf->default_face_name, x, y);
                            }

                            x = buf->display_range_x_start;
                            ++y;
                        }
                    }
                }

                ++i;
            }

            for (unsigned int j = y; j < buf->display_range_y_end; j++) {
                for (unsigned int k = x; k <= term->width; k++)
                    draw_char(' ', buf->default_face_name, k, j);
                x = buf->display_range_x_start;
            }
        }

        term->move_cursor(current_buffer->display_range_x_start +
                              current_buffer->cursor_x - 1,
                          current_buffer->display_range_y_start +
                              current_buffer->cursor_y - 1);
        term->flush_buffer();
    }

    void Ui::write_message(std::string const &msg) {
        Buffer *footer = nullptr;
        for (auto itr = displayed_buffers.begin();
             itr != displayed_buffers.end(); ++itr) {
            if ((**itr).buf_name == "__system_footer__") {
                footer = *itr;
                break;
            }
        }
        if (footer == nullptr) return;

        Ked::String str(msg);
        for (auto itr = std::begin(str.str); itr != std::end(str.str); ++itr)
            footer->insert(*itr);
    }

    void Ui::add_global_keybind(std::string const &key,
                                KeyHandling::EditorCommand func) {
        global_keybind.add(key, func);
    }

    void Ui::add_buffer_entry_change_listener(
        std::function<void(std::vector<Buffer *> &)> listener) {
        on_buffer_entry_changed_listener.push_back(listener);
    }

    void Ui::main_loop() {
        Rune buf;
        unsigned int n_byte;
        bool broken;
        for (;;) {
            if (editor_exited) break;

            redraw_editor();

            unsigned char c = (unsigned char)term->get_char();
            if ((c >> 7 & 1) == 0)
                n_byte = 1;
            else if ((c >> 5 & 0b111) == 0b110)
                n_byte = 2;
            else if ((c >> 4 & 0b1111) == 0b1110)
                n_byte = 3;
            else if ((c >> 3 & 0b11111) == 0b11110)
                n_byte = 4;
            else {
                n_byte = 0;
            }

            if (n_byte == 1)
                KeyHandling::handle_key(*this, c);
            else if (1 < n_byte && n_byte <= 4) {
                buf[0] = c;

                broken = 0;
                for (--n_byte; n_byte > 0; --n_byte) {
                    c = (unsigned char)term->get_char();
                    if ((c >> 6 & 0b11) != 0b10) {
                        broken = 1;

                        break;
                    }

                    buf[sizeof(Rune) - n_byte - 1] = c;
                }

                if (!broken) KeyHandling::handle_rune(*this, buf);
                buf.fill(0);
            }
        }
    }

} // namespace Ked
