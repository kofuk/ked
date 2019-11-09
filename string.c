#include <stdlib.h>
#include <string.h>

#include "string.h"
#include "utilities.h"

String *string_create(char *str, size_t length)
{
    String *result = malloc(sizeof(String));
    result->length = length;
    result->buf = malloc(sizeof(char) * length);
    if (str != NULL) memcpy(result->buf, str, length);

    return result;
}

void string_destruct(String *this)
{
    clear_buffer(this->buf, this->length);
    free(this->buf);
    clear_buffer((char*)this, sizeof(String));
    free(this);

}
