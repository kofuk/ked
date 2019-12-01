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

#include <math.h>
#include <stddef.h>
#include <stdlib.h>

#include "libked.h"

char *itoa(unsigned int num) {
    int n = num != 0 ? (unsigned int)log10((double)num) + 2 : 2;
    char *result = malloc(sizeof(char) * (unsigned int)n);

    for (int i = n - 2; i >= 0; i--) {
        result[i] = '0' + num % 10;
        num /= 10;
    }

    result[n - 1] = 0;

    return result;
}
