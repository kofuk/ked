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

#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <ked/Buffer.hh>
#include <ked/Terminal.hh>
#include <ked/Ui.hh>
#include <ked/ked.hh>

#include "ked/Extension.hh"
#include "libked/libked.hh"

#include "ked.hh"

/* static void handle_signal_thread(sigset_t *sigs) { */
/*     int res, sig; */
/*     for (;;) { */
/*         res = sigwait(sigs, &sig); */

/*         if (res != 0) continue; */

/*         switch (sig) { */
/*         case SIGCONT: */
/*             term_set_up(); */
/*             ui_invalidate(); */

/*             display_buffer_unlock(); */
/*             break; */
/*         } */
/*     } */
/* } */

static void check_term(int use_stdin) {
    if ((use_stdin && !(isatty(1) && isatty(2))) ||
        (!use_stdin && !(isatty(0) && isatty(1) && isatty(2)))) {
        fputs("Not a tty.\n", stderr);

        exit(1);
    }
}

static void print_help(void) {
    std::cout << "usage: ked [options]... file" << std::endl
              << "Simple console text editor with minimal dependency."
              << std::endl
              << std::endl
              << "  --debug -d    Load config file for debug. (not ~/.kedrc)"
              << std::endl
              << "  --help        Print this help and exit." << std::endl
              << "  --version     Print version and brief license information "
                 "and exit."
              << std::endl;
}

static void print_version(void) {
    std::cout
        << "KED 0.0.1" << std::endl
        << "This program is distributed in the hope that it will be useful,"
        << std::endl
        << "but WITHOUT ANY WARRANTY; without even the implied warranty of"
        << std::endl
        << "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the"
        << std::endl
        << "GNU General Public License for more details." << std::endl;
}

static std::string opt_file_name;
static bool opt_debug;
static bool opt_print_version;
static bool opt_print_help;
static std::string opt_unrecognized;
static char opt_unrecognized_char;

static void handle_option(int argc, char **argv) {
    char *opt;
    for (int i = 1; i < argc; ++i) {
        opt = argv[i];

        if (std::strncmp(opt, "--", 2) == 0) {
            if (std::strcmp(opt, "--help") == 0)
                opt_print_help = 1;
            else if (std::strcmp(opt, "--version") == 0)
                opt_print_version = 1;
            else if (std::strcmp(opt, "--debug") == 0)
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
            if (opt_file_name == "") {
                opt_file_name = opt;
            }
        }
    }
}

int main(int argc, char **argv) {
    handle_option(argc, argv);

    if (opt_unrecognized.length() != 0) {
        std::cerr << PROGRAM_NAME << ": " << opt_unrecognized
                  << ": Unrecognized option" << std::endl;
        print_help();

        return 1;
    } else if (opt_unrecognized_char != 0) {
        std::cerr << PROGRAM_NAME << ": -" << opt_unrecognized_char
                  << ": Unrecognized option" << std::endl;
        print_help();

        return 1;
    }

    if (opt_print_help) print_help();
    if (opt_print_version) print_version();
    if (opt_print_help || opt_print_version) return 0;

    if (opt_file_name.length() == 0) {
        print_help();

        return 1;
    }

    check_term(opt_file_name == "-");

    /* sigset_t sigs; */
    /* sigemptyset(&sigs); */
    /* sigaddset(&sigs, SIGCONT); */
    /* pthread_sigmask(SIG_BLOCK, &sigs, NULL); */

    /* pthread_t thread; */
    /* pthread_create(&thread, NULL, handle_signal_thread, &sigs); */

    Ked::Buffer *buf;
    if (opt_file_name == "-") {
        std::cerr << "Reading from stdin..." << std::endl;
        buf = Ked::buffer_from_stdin();

        int fd = open("/dev/tty", O_RDONLY);
        if (fd < 0) {
            delete buf;

            return 1;
        }
        dup2(fd, 0);
    }

    if (userpref_load(opt_debug)) {
        Ked::Terminal *term = new Ked::Terminal;
        Ked::Ui *ui = new Ked::Ui(term);
        Ked::Extension::attach_ui(ui);

        if (opt_file_name != "-")
            buf = new Ked::Buffer(opt_file_name, opt_file_name);

        if (buf != nullptr) {
            buf->display_range_x_start = 1;
            buf->display_range_x_end = term->width;
            buf->display_range_y_start = 2;
            buf->display_range_y_end = term->height;
            ui->buffer_add(buf);
            ui->buffer_show(buf->buf_name);
            ui->buffer_switch(buf->buf_name);

            ui->main_loop();
        }

        Ked::Extension::detach_ui(ui);
        delete ui;
        delete term;
    }

    /* delete buf; */

    /* pthread_cancel(thread); */
    /* pthread_join(thread, NULL); */

    return 0;
}
