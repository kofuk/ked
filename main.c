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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "keybind.h"
#include "terminal.h"
#include "ui.h"

static void check_term() {
    if (!(isatty(0) || isatty(1) || isatty(2))) {
        fputs("stdin, stdout or stderr is not a TTY.", stderr);

        exit(1);
    }

    char *term = getenv("TERM");

    if (term == NULL || strncmp(term, "xterm", 5)) {
        fprintf(stderr, "ked: Unknown terminal (%s != xterm*)\n",
                term == NULL ? "<NULL>" : term);

        exit(1);
    }
}

int main(int argc, char **argv) {
    check_term();

    if (argc < 2) {
        fputs("Filename required.\n", stderr);

        return 1;
    }

    term_set_up();
    ui_set_up();
    keybind_set_up();

    Buffer *buf = buffer_create(argv[1], argv[1]);

    if (buf != NULL) {
        set_buffer(buf, BUF_MAIN);
        init_system_buffers();
        select_buffer(buf);

        editor_main_loop();
    }

    keybind_tear_down();
    ui_tear_down();
    term_tear_down();

    return 0;
}
