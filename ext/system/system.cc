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

#include <ked/Face.hh>
#include <ked/Rune.hh>
#include <ked/ked.hh>

namespace SystemExtension {
    static Ked::String *lf;

    static std::size_t current_col = 0;
    static bool moving;

    DEFINE_EDITOR_COMMAND(cursor_forward) { buf.cursor_move(1, true); }

    DEFINE_EDITOR_COMMAND(cursor_back) { buf.cursor_move(1, false); }

    DEFINE_EDITOR_COMMAND(cursor_forward_line) {
        Ked::SearchResult *result = buf.search(buf.point, *lf, true);
        std::size_t rest;
        if (result == nullptr) {
            moving = true;
            buf.cursor_move(
                buf.buf_size - (buf.gap_end - buf.gap_start) - buf.point, true);
            moving = false;

            return;
        } else {
            rest = result->start - buf.point;
        }

        size_t n_start = result->end;

        delete result;

        result = buf.search(n_start, *lf, true);
        if (result == nullptr) {
            moving = true;
            buf.cursor_move(rest + current_col, true);
            moving = false;
        } else {
            /* If I use result->end here, it includes '\n' so exclusive line end
             * point is result->start. */
            std::size_t len = result->start - n_start;
            if (len > current_col) {
                moving = true;
                buf.cursor_move(rest + current_col, 1);
                moving = false;
            } else {
                moving = true;
                buf.cursor_move(rest + len + 1, true);
                moving = false;
            }
        }

        delete result;
    }

    DEFINE_EDITOR_COMMAND(cursor_back_line) {
        Ked::SearchResult *result = buf.search(buf.point, *lf, false);
        if (result == nullptr) {
            moving = true;
            buf.cursor_move(buf.point, false);
            moving = false;

            return;
        }

        size_t rest = buf.point - result->end;
        size_t n_end = result->start;

        delete result;

        result = buf.search(n_end, *lf, false);
        size_t len;
        if (result == nullptr)
            len = n_end + 1;
        else
            len = n_end - result->start;

        if (len > current_col) {
            moving = true;
            buf.cursor_move(rest + len - current_col + 1, false);
            moving = false;
        } else {
            moving = true;
            buf.cursor_move(rest + 1, false);
            moving = false;
        }

        delete result;
    }

    DEFINE_EDITOR_COMMAND(cursor_beginning_of_line) {
        Ked::SearchResult *result = buf.search(buf.point, *lf, false);
        if (result == nullptr) {
            buf.cursor_move(buf.point, false);
        } else {
            buf.cursor_move(buf.point - result->end, false);
        }
        delete result;
    }

    DEFINE_EDITOR_COMMAND(cursor_end_of_line) {
        Ked::SearchResult *result = buf.search(buf.point, *lf, true);
        if (result == nullptr) {
            buf.cursor_move(
                buf.buf_size - (buf.gap_end - buf.gap_start) - buf.point, true);
        } else {
            buf.cursor_move(result->start - buf.point, true);
        }
    }

    DEFINE_EDITOR_COMMAND(delete_backward) { buf.delete_backward(); }

    DEFINE_EDITOR_COMMAND(delete_forward) { buf.delete_forward(); }

    DEFINE_EDITOR_COMMAND(buffer_save) { buf.save(); }

    DEFINE_EDITOR_COMMAND(editor_quit) { ui.exit_editor(); }

    DEFINE_EDITOR_COMMAND(display_way_of_quit) {
        ui.write_message("Ctrl+Q to quit.");
    }

    static void on_cursor_move(Ked::Buffer &buf) {
        if (moving) return;

        Ked::SearchResult *result = buf.search(buf.point, *lf, false);
        if (result == nullptr)
            current_col = buf.point;
        else
            current_col = buf.point - result->start;
    }

    static void on_buffer_entry_change(std::vector<Ked::Buffer *> &bufs) {
        for (auto itr = std::begin(bufs); itr != std::end(bufs); ++itr) {
            if ((*itr)->buf_name == "__system_header__")
                (*itr)->default_face_name = "SystemHeader";
            else if ((*itr)->buf_name == "__system_footer__")
                (*itr)->default_face_name = "SystemFooter";
            else
                (*itr)->add_cursor_move_listener(&on_cursor_move);
        }
    }

    extern "C" {
    void extension_on_load() {
        lf = new Ked::String("\n");

        moving = 0;
        current_col = 1;
    }

    void extension_on_attach_ui(Ked::Ui &ui) {
        ui.add_buffer_entry_change_listener(&on_buffer_entry_change);

        ui.add_global_keybind("^[[A", EDITOR_COMMAND_PTR(cursor_back_line));
        ui.add_global_keybind("^[[B", EDITOR_COMMAND_PTR(cursor_forward_line));
        ui.add_global_keybind("^[[C", EDITOR_COMMAND_PTR(cursor_forward));
        ui.add_global_keybind("^[[D", EDITOR_COMMAND_PTR(cursor_back));
        ui.add_global_keybind("^A",
                              EDITOR_COMMAND_PTR(cursor_beginning_of_line));
        ui.add_global_keybind("^B", EDITOR_COMMAND_PTR(cursor_back));
        ui.add_global_keybind("^C", EDITOR_COMMAND_PTR(display_way_of_quit));
        ui.add_global_keybind("^D", EDITOR_COMMAND_PTR(delete_forward));
        ui.add_global_keybind("^E", EDITOR_COMMAND_PTR(cursor_end_of_line));
        ui.add_global_keybind("^H", EDITOR_COMMAND_PTR(delete_backward));
        ui.add_global_keybind("^N", EDITOR_COMMAND_PTR(cursor_forward_line));
        ui.add_global_keybind("^P", EDITOR_COMMAND_PTR(cursor_back_line));
        ui.add_global_keybind("^Q", EDITOR_COMMAND_PTR(editor_quit));
        ui.add_global_keybind("^X^C", EDITOR_COMMAND_PTR(editor_quit));
        ui.add_global_keybind("^X^S", EDITOR_COMMAND_PTR(buffer_save));
        /* ui.add_global_keybind("^Z", EDITOR_COMMAND_PTR(process_stop)); */
        ui.add_global_keybind("^F", EDITOR_COMMAND_PTR(cursor_forward));
        ui.add_global_keybind("\x7f", EDITOR_COMMAND_PTR(delete_backward));

        Ked::Face::add("", FACE_NONE);
        Ked::Face::add("SystemHeader", FACE_ATTR_COLOR_256(1, 16, 231));
        Ked::Face::add("SystemFooter", FACE_COLOR_256(16, 231));
    }

    void extension_on_unload() {
        delete lf;
        lf = nullptr;
    }
    }

} // namespace SystemExtension
