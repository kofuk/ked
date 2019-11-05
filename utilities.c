#include <math.h>
#include <stddef.h>
#include <stdlib.h>

#include "utilities.h"

char *itoa(unsigned int num)
{
    int n = num != 0 ? (unsigned int)log10((double)num) + 2 : 2;
    char *result = malloc(sizeof(char) * (unsigned int)n);

    for (int i = n - 2; i >= 0; i--)
    {
        result[i] = '0' + num % 10;
        num /= 10;
    }

    result[n - 1] = 0;

    return result;
}

void clear_buffer(char *buf, size_t n)
{
    for (size_t i =  0; i < n; i++) buf[i] = 0;
}
