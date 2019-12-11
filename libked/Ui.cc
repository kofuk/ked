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
        void Keybind::add(std::string const &key, EditorCommand func) {
            bind_map[key] = func;
        }

        KeyBindState Keybind::handle(const std::vector<char> &seq, Ui &ui,
                                     Buffer &buf) {
            for (auto itr = std::begin(bind_map); itr != std::end(bind_map);
                 ++itr) {
                for (size_t i = 0; i < itr->first.size(); ++i) {
                    if (i >= seq.size()) return KEYBIND_NOT_HANDLED;

                    if (i == itr->first.size() - 1 && i == seq.size() - 1 &&
                        itr->first[i] == seq[i]) {
                        (itr->second)(ui, buf);

                        return KEYBIND_HANDLED;
                    } else if (itr->first[i] != seq[i])
                        break;
                }
            }

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
    void Ui::draw_char(unsigned char c, std::string const &face, unsigned int x,
                       unsigned int y) {
        if (x > term->width || y > term->height ||
            (display_buffer[(y - 1) * term->width + x - 1].c[0] == c &&
             face == display_buffer[(y - 1) * term->width + x - 1].face) ||
            c == '\n')
            return;

        if (face != current_face) {
            term->put_str(Face::lookup(face));

            current_face = face;
        }

        term->move_cursor(x, y);
        term->put_char(c);
        display_buffer[(y - 1) * term->width + x - 1].c[0] = c;
        display_buffer[(y - 1) * term->width + x - 1].face = face;
        for (int i = 1; i < 4; ++i)
            display_buffer[(y - 1) * term->width + x - 1].c[i] = 0;
    }

    /* Draws AttrRune with its attrubutes to the termianl if needed. */
    void Ui::draw_rune(AttrRune const &r, std::string const &default_face,
                       unsigned int x, unsigned int y) {
        std::string face = r.face;
        if (face.length() == 0) {
            face = default_face;
        }

        if (x > term->width || y > term->height ||
            (display_buffer[(y - 1) * term->width + x - 1].c == r.c &&
             face == display_buffer[(y - 1) * term->width + x - 1].face) ||
            r.c[0] == '\n')
            return;

        if (r.face != current_face) {
            term->put_str(Face::lookup(r.face));

            current_face = r.face;
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
        AttrRune c;
        for (size_t b = 0; b < displayed_buffers.size(); ++b) {
            Buffer *buf = displayed_buffers[b];

            x = (unsigned int)buf->display_range_x_start;
            y = (unsigned int)buf->display_range_y_start;
            i = buf->visible_start_point;
            while (i < buf->buf_size - (buf->gap_end - buf->gap_start)) {
                if (y >= buf->display_range_y_end) break;

                c = buf->get_rune(i);

                if (c.is_lf()) {
                    for (unsigned int j = x; j <= term->width; ++j)
                        draw_char(' ', buf->default_face, j, y);

                    ++y;

                    x = 1;
                } else {
                    draw_rune(c, buf->default_face, x, y);

                    for (unsigned int j = x + 1; j < x + c.display_width; ++j)
                        invalidate_point(j, y);

                    x += c.display_width;

                    if (i + 1 <
                        buf->buf_size - (buf->gap_end - buf->gap_start)) {
                        AttrRune next_rune = buf->get_rune(i + 1);
                        if (!next_rune.is_lf() &&
                            x + next_rune.display_width >= term->width) {
                            if (x + next_rune.display_width == term->width) {
                                draw_char('\\', buf->default_face, x, y);
                            } else {
                                draw_char(' ', buf->default_face, x, y);
                                ++x;
                                draw_char('\\', buf->default_face, x, y);
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
                    draw_char(' ', buf->default_face, k, j);
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
