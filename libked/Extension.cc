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
#include <iterator>
#include <vector>

#include <dlfcn.h>

#include <ked/Extension.hh>
#include <ked/Ui.hh>

namespace Ked {
    namespace Extension {
        static std::vector<void *> handles;
        static Ked::Ui *attached_ui;

        static void call_on_attach(void *handle) {
            auto on_attach_func = (void (*)(Ked::Ui &))dlsym(handle, "extension_on_attach_ui");
            if (on_attach_func != nullptr) (*on_attach_func)(*attached_ui);
        }

        static void call_on_detach(void *handle) {
            auto on_detach_func = (void (*)(Ked::Ui &))dlsym(handle, "extension_on_detach_ui");
            if (on_detach_func != nullptr) (*on_detach_func)(*attached_ui);
        }

        bool load(std::string const &path) {
            void *handle = dlopen(path.c_str(), RTLD_LAZY);
            if (handle == nullptr) return false;
            handles.push_back(handle);
            auto on_load_func =
                (void (*)())dlsym(handle, "extension_on_load");
            if (on_load_func != nullptr) (*on_load_func)();

            if (attached_ui != nullptr) call_on_attach(handle);

            return true;
        }

        void unload_all() {
            for (auto itr = handles.begin(); itr != handles.end(); ++itr) {
                call_on_detach(*itr);

                auto on_unload_func =
                    (void (*)())dlsym(*itr, "extension_on_unload");
                if (on_unload_func != nullptr) (*on_unload_func)();

                dlclose(*itr);
            }
        }

        void attach_ui(Ked::Ui *ui) {
            if (attached_ui != nullptr) throw 1;
            attached_ui = ui;

            for (auto itr = std::begin(handles); itr != std::end(handles); ++itr) {
                call_on_attach(*itr);
            }
        }

        void detach_ui(Ked::Ui *ui) {
            if (ui == nullptr || attached_ui != ui) throw 1;

            for (auto itr = std::begin(handles); itr != std::end(handles); ++itr) {
                call_on_detach(*itr);
            }

            attached_ui = nullptr;
        }

    } // namespace Extension
} // namespace Ked
