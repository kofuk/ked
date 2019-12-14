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

#ifndef KED_UI_HH
#define KED_UI_HH

#include <list>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "Buffer.hh"
#include "Face.hh"
#include "Terminal.hh"

namespace Ked {
    class Ui;

    namespace KeyHandling {
        using EditorCommand =
            std::function<void(Ked::Ui &ui, Ked::Buffer &buf)>;

        enum KeyBindState {
            KEYBIND_NOT_HANDLED,
            KEYBIND_HANDLED,
            KEYBIND_WAIT
        };

        class Keybind {
            struct BindingElement {
                std::string key;
                EditorCommand func;
            };

            std::list<BindingElement *> binding;

            char *compile_key(std::string const &seq);
            int compare_key(std::string const &key,
                            std::vector<char> const &seq);
            int compare_key(std::string const &key,
                            std::vector<char> const &seq, std::size_t n);

        public:
            ~Keybind();

            void add(std::string const &key, EditorCommand func);
            KeyBindState handle(std::vector<char> const &seq, Ui &ui,
                                Buffer &buf);
        };
    } // namespace KeyHandling

    class Ui {
        bool editor_exited;
        std::vector<Buffer *> displayed_buffers;
        std::vector<Buffer *> buffers;
        std::vector<AttrRune> display_buffer;
        unsigned int maybe_next_x;
        unsigned int maybe_next_y;

        std::string current_face_name;
        std::mutex display_buffer_mutex;

        std::vector<std::function<void(std::vector<Buffer *> &)>>
            on_buffer_entry_changed_listener;

    public:
        Terminal *term;
        Ked::KeyHandling::Keybind global_keybind;
        Buffer *current_buffer;

        Ui(Terminal *term);
        ~Ui();

        /* Draws char to the terminal if needed. */
        void draw_char(unsigned char c, std::string const &face, unsigned int x,
                       unsigned int y);
        /* Draws AttrRune with its attrubutes to the termianl if needed. */
        void draw_rune(AttrRune const &r, std::string const &default_face,
                       unsigned int x, unsigned int y);
        /* Make next drawing to take place in the position. */
        void invalidate_point(unsigned int x, unsigned int y);
        /* Display message on the message area. */
        void write_message(std::string const &msg);
        /* Initializes buffers that are needed for system to work. */
        void init_system_buffers();
        /* Reads displayed_buffers and rewrites areas that are changed. */
        void redraw_editor();
        /* Reserve editor exit on next exit point. */
        void exit_editor();

        /* Assigns given function to given key sequence. */
        void add_global_keybind(std::string const &key,
                                KeyHandling::EditorCommand func);

        /* Adds listener to be called just after buffer changed. */
        void add_buffer_entry_change_listener(
            std::function<void(std::vector<Buffer *> &)>);

        void main_loop();
        /* Sets buffer to drawing target. */
        void buffer_show(std::string const &name);
        /* Select the buffer as current_buffer. */
        void buffer_switch(std::string const &name);
        /* Add buffer to internal buffer list. */
        void buffer_add(Buffer *buf);
    };

} // namespace Ked

#endif
