#ifndef STRING_H
#define STRING_H

#include <stddef.h>

typedef struct {
    char *buf;
    size_t length;
} String;

String *string_create(char*, size_t);
void string_destruct(String*);

#endif
