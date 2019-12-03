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
#include <fcntl.h>
#include <unistd.h>

#include <ked/buffer.h>
#include <ked/internal.h>
#include <ked/ked.h>

#include "libked/libked.h"

#include "ked.h"

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

static void check_term(int use_stdin) {
    if ((use_stdin && !(isatty(1) && isatty(2))) ||
        (!use_stdin && !(isatty(0) && isatty(1) && isatty(2)))) {
        fputs("Not a tty.\n", stderr);

        exit(1);
    }
}

static void print_help(void) {
    puts("usage: ked [options]... file\n"
         "Simple console text editor with minimal dependency.\n\n"
         "  --debug -d    Load config file for debug. (not ~/.kedrc)\n"
         "  --help        Print this help and exit.\n"
         "  --version     Print version and brief license information and "
         "exit.");
}

static void print_version(void) {
    puts("KED 0.0.1\n\n"
         "This program is distributed in the hope that it will be useful,\n"
         "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
         "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
         "GNU General Public License for more details.");
}

static char *opt_file_name;
static int opt_debug;
static int opt_print_version;
static int opt_print_help;
static char *opt_unrecognized;
static char opt_unrecognized_char;

static void handle_option(int argc, char **argv) {
    char *opt;
    for (int i = 1; i < argc; ++i) {
        opt = argv[i];

        if (strncmp(opt, "--", 2) == 0) {
            if (strcmp(opt, "--help") == 0)
                opt_print_help = 1;
            else if (strcmp(opt, "--version") == 0)
                opt_print_version = 1;
            else if (strcmp(opt, "--debug") == 0)
                opt_debug = 1;
            else {
                opt_unrecognized = opt;

                return;
            }
        } else if (strncmp(opt, "-", 1) == 0 && strcmp(opt, "-") != 0) {
            size_t len = strlen(opt);
            for (size_t j = 1; j < len; ++j) {
                if (opt[j] == 'd')
                    opt_debug = 1;
                else {
                    opt_unrecognized_char = opt[j];

                    return;
                }
            }
        } else {
            if (opt_file_name == NULL) {
                opt_file_name = opt;
            }
        }
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

int main(int argc, char **argv) {
    handle_option(argc, argv);

    if (opt_unrecognized != NULL) {
        fprintf(stderr, "%s: %s: Unrecognized option\n", PROGRAM_NAME,
                opt_unrecognized);
        print_help();

        return 1;
    } else if (opt_unrecognized_char != 0) {
        fprintf(stderr, "%s: -%c: Unrecognized option\n", PROGRAM_NAME,
                opt_unrecognized_char);
        print_help();

        return 1;
    }

    if (opt_print_help) {
        print_help();
    }
    if (opt_print_version) {
        print_version();
    }
    if (opt_print_help || opt_print_version) {
        exit(0);
    }

    if (opt_file_name == NULL) {
        print_help();

        return 1;
    }

    check_term(strcmp(opt_file_name, "-") == 0);

    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGCONT);
    pthread_sigmask(SIG_BLOCK, &sigs, NULL);

    pthread_t thread;
    pthread_create(&thread, NULL, handle_signal_thread, &sigs);

    Buffer *buf;
    if (strcmp(opt_file_name, "-") == 0) {
        buf = buffer_create_stdin();

        int fd = open("/dev/tty", O_RDONLY);
        if (fd < 0) {
            buffer_destruct(buf);

            return 1;
        }
        dup2(fd, 0);
    }

    keybind_set_up();
    extension_set_up();

    if (userpref_load(opt_debug)) {
        term_set_up();

        ui_set_up();
        init_system_buffers();

        if (strcmp(opt_file_name, "-") != 0)
            buf = buffer_create(opt_file_name, opt_file_name);

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

    return 0;
}
#pragma GCC diagnostic pop
