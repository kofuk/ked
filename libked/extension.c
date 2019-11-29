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

#include <dlfcn.h>
#include <stddef.h>
#include <stdlib.h>

#include "extension.h"

static void **extension_table;
static size_t extension_table_size;
static size_t extension_table_i;

void extension_set_up(void) {
    extension_table_size = 8;
    extension_table_i = 0;
    extension_table = malloc(sizeof(void *) * extension_table_size);
}

void extension_tear_down(void) {
    for (; extension_table_i != 0; --extension_table_i) {
        void (*func)(void) = (void (*)(void))dlsym(
            extension_table[extension_table_i - 1], "extension_on_unload");
        if (func == NULL) continue;
        func();

        dlclose(extension_table[extension_table_i - 1]);
    }

    free(extension_table);
    extension_table = NULL;
}

int load_extension(const char *name) {
    if (extension_table_i >= extension_table_size) {
        if (extension_table_size <= 64) {
            extension_table_size = extension_table_size << 2;
        } else {
            extension_table_size += 64;
        }

        extension_table =
            realloc(extension_table, sizeof(void *) * extension_table_size);
    }

    void *handle = dlopen(name, RTLD_LAZY);
    if (handle == NULL) {
        return 0;
    }
    extension_table[extension_table_i] = handle;

    void (*func)(void) = (void (*)(void))dlsym(handle, "extension_on_load");
    if (func != NULL) (func)();

    ++extension_table_i;

    return 1;
}
