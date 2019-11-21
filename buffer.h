#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>

#include "string.h"

#define INIT_GAP_SIZE 1024
#define MIN_GAP_SIZE 32

typedef struct {
    char *buf_name;
    char *path;
    char *content;
    size_t point;
    size_t buf_size;
    size_t gap_start;
    size_t gap_end;
    size_t n_lines;
    int modified;
    unsigned int cursor_x;
    unsigned int cursor_y;
} Buffer;

typedef struct
{
    size_t start;               /* inclusive */
    size_t end;                 /* exclusive */
} Range;

Buffer *current_buffer;

Buffer *buffer_create(const char*, const char*);

void buffer_cursor_forward(Buffer*);
void buffer_cursor_back(Buffer*);

void buffer_insert(Buffer*, char);
void buffer_delete_backward(Buffer*);
void buffer_delete_forward(Buffer*);

int buffer_save(Buffer*);

void set_buffer(Buffer*);

#endif
