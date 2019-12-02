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

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "libked/libked.h"

#include "ked.h"

static char *home_dir;

static inline int print_error_2(const char *msg1, const char *msg2) {
    return fprintf(stderr, "%s: %s: %s\n", PROGRAM_NAME, msg1, msg2);
}

static int detect_home_dir(void) {
    // FIXME: multithread handling
    struct passwd *p = getpwuid(getuid());
    if (p == NULL) {
        print_error_2(__FUNCTION__, strerror(errno));

        return 0;
    }
    if (p->pw_dir == NULL) {
        fputs("ked: Home directory not registered.\n", stderr);

        return 0;
    }

    size_t dir_len = strlen(p->pw_dir);
    home_dir = malloc(sizeof(char) * (dir_len + 1));
    memcpy(home_dir, p->pw_dir, dir_len + 1);

    return 1;
}

static char *path_expand(const char *path) {
    size_t path_len = strlen(path);
    size_t result_len = path_len;

    int tilde = strncmp(path, "~/", 2) == 0;
    if (tilde) {
        --result_len;
        result_len += strlen(home_dir);

        ++path;
    }

    char *result = malloc(sizeof(char) * (result_len + 1));
    size_t off = 0;
    if (tilde) {
        off = strlen(home_dir);
        memcpy(result, home_dir, off);
    }
    memcpy(result + off, path, strlen(path) + 1);

    return result;
}

static inline void skip_spaces(const char *content, size_t *off, size_t len) {
    while (*off < len) {
        if (content[*off] == '\n' || content[*off] == '\r' ||
            content[*off] == ' ' || content[*off] == '\t')
            ++(*off);
        else
            break;
    }
}

static int parse_and_eval(const char *content, size_t len) {
    // temporary implementation
    size_t off = 0;
    skip_spaces(content, &off, len);
    if (strncmp(content + (off), "loadSystemExtension", 19) != 0) {
        return 0;
    }
    off += 19;
    skip_spaces(content, &off, len);
    if (content[off] != '(') {
        return 0;
    }
    ++off;
    skip_spaces(content, &off, len);
    size_t start, end = 0;
    if (content[off] == '"') {
        ++off;
        start = off;
        while (off < len) {
            if (content[off] == '"') {
                end = off;
                break;
            } else
                ++off;
        }
    } else if (content[off] == '\'') {
        ++off;
        start = off;
        while (off < len) {
            if (content[off] == '\'') {
                end = off;
                break;
            } else
                ++off;
        }
    } else {
        return 0;
    }

    if (end == 0) return 0;

    char *path = malloc(sizeof(char) * (end - start + 1));
    memcpy(path, content + start, end - start);
    path[end - start] = 0;

    char *expanded = path_expand(path);
    free(path);

    int res = load_extension(expanded);

    free(expanded);

    return res;
}

int userpref_load(int debug) {
    if (!detect_home_dir()) return 0;

    char *file_name;
    if (debug)
        file_name = path_expand("etc/kedrc");
    else
        file_name = path_expand("~/.kedrc");

    struct stat stat_buf;
    if (stat(file_name, &stat_buf) == -1) {
        print_error_2(file_name, strerror(errno));
        free(file_name);

        return 0;
    }

    int fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        print_error_2(file_name, strerror(errno));
        free(file_name);

        return 0;
    }

    char *config = mmap(NULL, stat_buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (config == MAP_FAILED) {
        print_error_2(file_name, strerror(errno));
        close(fd);
        free(file_name);

        return 0;
    }

    int s = parse_and_eval(config, stat_buf.st_size);

    close(fd);
    free(file_name);

    return s;
}
