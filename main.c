// This file is part of `ked', licensed under the GPLv3 or later.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "keybind.h"
#include "terminal.h"
#include "ui.h"

static void check_term()
{
    if (!(isatty(0) || isatty(1) || isatty(2)))
    {
        fputs("stdin, stdout or stderr is not a TTY.", stderr);

        exit(1);
    }

    char *term = getenv("TERM");

    if (term == NULL || strncmp(term, "xterm", 5))
    {
        fprintf(stderr, "ked: Unknown terminal (%s != xterm*)\n",
                term == NULL ? "<NULL>" : term);

        exit(1);
    }
}

int main(int argc, char **argv)
{
    check_term();

    if (argc < 2)
    {
        fputs("Filename required.\n", stderr);

        return 1;
    }

    term_set_up();
    ui_set_up();
    keybind_set_up();

    Buffer *buf = buffer_create(argv[1], argv[1]);

    if (buf != NULL)
    {
        set_buffer(buf, BUF_MAIN);
        init_system_buffers();
        select_buffer(buf);

        editor_main_loop();
    }

    keybind_tear_down();
    term_tear_down();

    return 0;
}
