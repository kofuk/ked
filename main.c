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

#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <ked/buffer.h>
#include <ked/internal.h>
#include <ked/ked.h>

#include "libked/libked.h"

static void handle_signal_thread(sigset_t *sigs) {
    int res, sig;
    for (;;) {
        res = sigwait(sigs, &sig);

        if (res != 0) continue;

        switch (sig) {
        case SIGCONT:
            term_set_up();
            ui_invalidate();

            display_buffer_unlock();
            break;
        }
    }
}

static void check_term() {
    if (!(isatty(0) && isatty(1) && isatty(2))) {
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

    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGCONT);
    pthread_sigmask(SIG_BLOCK, &sigs, NULL);

    pthread_t thread;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
    pthread_create(&thread, NULL, handle_signal_thread, &sigs);
#pragma GCC diagnostic pop

    keybind_set_up();
    extension_set_up();

    char system_ext_load_fail = 0;

    if (!load_extension("ext/system/system.so")) {
        system_ext_load_fail = 1;
    } else {
        term_set_up();

        ui_set_up();
        init_system_buffers();

        Buffer *buf = buffer_create(argv[1], argv[1]);

        if (buf != NULL) {
            set_buffer(buf, BUF_MAIN);
            select_buffer(buf);

            editor_main_loop();
        }

        ui_tear_down();
        term_tear_down();
    }

    extension_tear_down();
    keybind_tear_down();

    pthread_cancel(thread);
    pthread_join(thread, NULL);

    if (system_ext_load_fail) {
        fputs("Failed to load system extension; Abort.\n", stderr);
    }

    return 0;
}
