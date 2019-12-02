#!/usr/bin/env bash

# Avoid fail because of locale
export LANG=C

if [ -e additional_cflags.txt ]; then
    rm -f additional_cflags.txt
fi

touch additional_cflags.txt

# Check if `for (int i = ...` is considered as valid.
gcc -xc -Wall -Wextra -c -o /dev/null - <<'EOF' 2>&1 | grep 99 &>/dev/null
#include <stdio.h>

int main(void) {
    for (int i = 0; i < 5; ++i) puts("a");
    return 0;
}
EOF

if [ $? -eq 0 ]; then
    echo -n '-std=c99 ' > additional_cflags.txt
fi

# Check if we can use kill(2) withour feature test macro.
gcc -xc -Wall -Wextra -c -o /dev/null - <<'EOF' 2>&1 | grep -i implicit &>/dev/null
#include <sys/types.h>
#include <signal.h>

int main(void) {
    kill(0, SIGCONT);
    return 0;
}
EOF

if [ $? -eq 0 ]; then
    echo -n '-D_POSIX_SOURCE -D_POSIX_C_SOURCE ' >> additional_cflags.txt
fi
