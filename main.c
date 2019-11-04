// This file is part of `ked', licensed under the GPLv3 or later.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

int main(void)
{
    check_term();

    term_set_up();
    ui_set_up();

    editor_main_loop();

    term_tear_down();

    return 0;
}
