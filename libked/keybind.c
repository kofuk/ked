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
#include <string.h>

#include <ked/buffer.h>
#include <ked/keybind.h>
#include <ked/terminal.h>
#include <ked/ui.h>

#include <ked/ked.h>

#include "utilities.h"

static char key_buf[8];

typedef struct BindingElement {
    char *key;
    EditCommand func;
    struct BindingElement *next;
} BindingElement;

typedef struct Keybind {
    BindingElement *bind;
} Keybind;

static Keybind *keybind_create(void) {
    Keybind *result = malloc(sizeof(Keybind));
    memset(result, 0, sizeof(Keybind));

    return result;
}

static void keybind_destruct(Keybind *this) {
    BindingElement *current;
    BindingElement *next = this->bind;
    while (next != NULL) {
        current = next;
        next = current->next;

        free(current);
    }

    free(this);
}

static char *key_compile(const char *key) {
    size_t pattern_len = strlen(key);
    size_t len = pattern_len;

    for (size_t i = 0; i < pattern_len; i++)
        if (key[i] == '^' && i != pattern_len - 1) --len;

    char *result = malloc(sizeof(char) * len + 1);
    size_t r_i = 0;
    result[len] = 0;
    for (size_t i = 0; i < pattern_len; i++) {
        if (key[i] == '^' && i != pattern_len - 1) {
            result[r_i] = key[i + 1] - '@';
            ++i;
        } else {
            result[r_i] = key[i];
        }
        ++r_i;
    }

    return result;
}

static void keybind_add(Keybind *this, const char *key, EditCommand func) {
    BindingElement *e = malloc(sizeof(BindingElement));
    memset(e, 0, sizeof(BindingElement));
    e->key = key_compile(key);
    e->func = func;

    if (this->bind == NULL) {
        this->bind = e;

        return;
    }

    int cmp = strcmp(this->bind->key, key);
    if (cmp == 0) {
        e->next = this->bind->next;
        free(this->bind);
        this->bind = e;

        return;
    } else if (cmp > 0) {
        e->next = this->bind;
        this->bind = e;

        return;
    }

    BindingElement *prev = this->bind;
    BindingElement *next = this->bind;
    while (next != NULL && strcmp(next->key, key) < 0) {
        prev = next;
        next = prev->next;
    }

    if (next == NULL) {
        prev->next = e;
    } else if (strcmp(next->key, key) == 0) {
        prev->next = e;
        e->next = next->next;
        free(next);
    } else {
        prev->next = e;
        e->next = next;
    }
}

enum KeyBindState { KEYBIND_NOT_HANDLED, KEYBIND_HANDLED, KEYBIND_WAIT };

static enum KeyBindState keybind_handle(Keybind *this, char *key, Buffer *buf) {
    BindingElement *elem = this->bind;
    while (elem != NULL) {
        if (strncmp(key, elem->key, strlen(key)) == 0) {
            if (strcmp(key, elem->key) == 0) {
                write_message("");
                (*(elem->func))(buf);

                return KEYBIND_HANDLED;
            }

            return KEYBIND_WAIT;
        }

        elem = elem->next;
    }

    if (strlen(key_buf) != 1) return KEYBIND_HANDLED;

    return KEYBIND_NOT_HANDLED;
}

static Keybind *global_keybind;

static void init_global_keybind(void) { global_keybind = keybind_create(); }

void add_global_keybind(const char *key, EditCommand func) {
    keybind_add(global_keybind, key, func);
}

void keybind_set_up(void) {
    memset(key_buf, 0, sizeof(key_buf));

    init_global_keybind();
}

void keybind_tear_down(void) { keybind_destruct(global_keybind); }

void handle_key(int c) {
    unsigned int i = 0;
    while (key_buf[i])
        ++i;

    key_buf[i] = (char)c;

    switch (keybind_handle(global_keybind, key_buf, current_buffer)) {
    case KEYBIND_NOT_HANDLED:
        buffer_insert_char(current_buffer, (char)c);
        // fall through
    case KEYBIND_HANDLED:
        memset(key_buf, 0, sizeof(key_buf));
        break;
    case KEYBIND_WAIT:
        write_message("Waiting for next key...");
        break;
    }
}

void handle_rune(Rune r) {
    memset(key_buf, 0, sizeof(key_buf));
    buffer_insert(current_buffer, r);
}