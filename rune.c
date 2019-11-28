#include <stdlib.h>
#include <string.h>

#include "rune.h"

String *string_create(const char *str) {
    size_t n_rune = 0;
    size_t len = strlen(str);
    for (size_t i = 0; i < len; ++i) {
        if (str[i] >> 7 == 0 || (str[i] >> 6 & 0b11) == 0b11) ++n_rune;
    }

    Rune *ra = malloc(sizeof(Rune) * n_rune);
    size_t ra_i = 0;
    char rune_buf[4];
    size_t buf_i = 0;
    memset(rune_buf, 0, sizeof(rune_buf));
    for (size_t i = 0; i < len; ++i) {
        if (str[i] >> 7 == 0 || (str[i] >> 6 & 0b11) == 0b11) {
            if (buf_i != 0) {
                memcpy(ra + ra_i, rune_buf, sizeof(rune_buf));
                memset(rune_buf, 0, sizeof(rune_buf));
                buf_i = 0;
                ++ra_i;
            }
            rune_buf[buf_i] = str[i];
            ++buf_i;
        } else if (buf_i != 0 && (str[i] >> 6 & 0b11) == 0b10) {
            if (buf_i >= 4) {
                free(ra);

                return NULL;
            }

            rune_buf[buf_i] = str[i];
            ++buf_i;
        } else {
            free(ra);

            return NULL;
        }
    }
    if (buf_i != 0) {
        memcpy(ra + ra_i, rune_buf, sizeof(rune_buf));
    }

    String *result = malloc(sizeof(String));
    result->str = ra;
    result->len = n_rune;

    return result;
}

void string_destruct(String *this) {
    free(this->str);
    free(this);
}

void attr_runes_set_width(AttrRune *r, size_t len) {
    //FIXME: temporary implementation
    for (size_t i = 0; i < len; ++i) attr_rune_set_width(r + i);
}
