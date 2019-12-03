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

#include <stdlib.h>

#include <ked/ked.h>
#include <string.h>

#include "libked.h"

/* Definition of tree to save face. */
typedef struct Face {
    /* The name this face associated to. */
    char *name;
    /* Control sequence to realize this face. */
    char *face;
    /* Left branch of entry tree. */
    struct Face *left;
    /* Right branch of entry tree. */
    struct Face *right;
} Face;

static Face *face_tree_top;

static void face_add_internal(Face *top, const char *name, const char *face) {
    int cmp = strcmp(name, top->name);
    if (cmp < 0) {
        if (top->left == NULL) {
            top->left = malloc(sizeof(Face));
            memset(top->left, 0, sizeof(Face));
            top->left->name = cstr_dup(name);
            top->left->face = cstr_dup(face);
        } else {
            face_add_internal(top->left, name, face);
        }
    } else if (cmp == 0) {
        top->face = cstr_dup(face);
    } else {
        if (top->right == NULL) {
            top->right = malloc(sizeof(Face));
            memset(top->right, 0, sizeof(Face));
            top->right->name = cstr_dup(name);
            top->right->face = cstr_dup(face);
        } else {
            face_add_internal(top->right, name, face);
        }
    }
}

void face_add(const char *name, const char *face) {
    if (face_tree_top == NULL) {
        face_tree_top = malloc(sizeof(Face));
        memset(face_tree_top, 0, sizeof(Face));
        face_tree_top->name = cstr_dup(name);
        face_tree_top->face = cstr_dup(face);
    } else {
        face_add_internal(face_tree_top, name, face);
    }
}

static void face_tear_down_internal(Face *top) {
    if (top == NULL) return;

    face_tear_down_internal(top->left);
    face_tear_down_internal(top->right);

    free(top->name);
    free(top->face);
    free(top);
}

__attribute__((destructor)) static void face_tear_down(void) {
    face_tear_down_internal(face_tree_top);
    face_tree_top = NULL;
}

static const char *face_lookup_internal(Face *top, const char *name) {
    if (top == NULL) return NULL;

    int cmp = strcmp(name, top->name);
    if (cmp < 0)
        return face_lookup_internal(top->left, name);
    else if (cmp == 0)
        return top->face;
    else
        return face_lookup_internal(top->right, name);
}

static const char *default_face = "\e[0m";

const char *face_lookup(const char *name) {
    if (name == NULL) return default_face;

    const char *face = face_lookup_internal(face_tree_top, name);
    if (face == NULL) return default_face;
    else return face;
}

void face_set_default(const char *face) {
    default_face = face;
}
