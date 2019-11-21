#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "editcommand.h"
#include "keybind.h"
#include "ui.h"
#include "utilities.h"

static char key_buf[8];

typedef struct BindingElement {
    char *key;
    EditCommand func;
    struct BindingElement *next;
} BindingElement;

typedef struct Keybind {
    BindingElement *bind;
} Keybind;

static Keybind *keybind_create(void)
{
    Keybind *result = malloc(sizeof(Keybind));
    memset(result, 0, sizeof(Keybind));

    return result;
}

static void keybind_destruct(Keybind *this)
{
    BindingElement *current;
    BindingElement *next = this->bind;
    while (next != NULL)
    {
        current = next;
        next = current->next;

        memset(current, 0, sizeof(BindingElement));
        free(current);
    }

    memset(this, 0, sizeof(Keybind));
    free(this);
}

static char *key_compile(const char *key)
{
    size_t pattern_len = strlen(key);
    size_t len = pattern_len;

    for (size_t i = 0; i < pattern_len; i++)
        if (key[i] == '^' && i != pattern_len - 1) --len;

    char *result = malloc(sizeof(char) * len + 1);
    size_t r_i = 0;
    result[len] = 0;
    for (size_t i = 0; i < pattern_len; i++)
    {
        if (key[i] == '^' && i != pattern_len - 1)
        {
            result[r_i] = key[i + 1] - '@';
            ++i;
        }
        else
        {
            result[r_i] = key[i];
        }
        ++r_i;
    }

    return result;
}

static void keybind_add(Keybind *this, const char *key, EditCommand func)
{
    BindingElement *e = malloc(sizeof(BindingElement));
    memset(e, 0, sizeof(BindingElement));
    e->key = key_compile(key);
    e->func = func;

    if (this->bind == NULL)
    {
        this->bind = e;

        return;
    }

    int cmp = strcmp(this->bind->key, key);
    if (cmp == 0)
    {
        e->next = this->bind->next;
        memset(this->bind, 0, sizeof(BindingElement));
        free(this->bind);
        this->bind = e;

        return;
    }
    else if (cmp > 0)
    {
        e->next = this->bind;
        this->bind = e;

        return;
    }

    BindingElement *prev = this->bind;
    BindingElement *next = this->bind;
    while (next != NULL && strcmp(next->key, key) < 0)
    {
        prev = next;
        next = prev->next;
    }

    if (next == NULL)
    {
        prev->next = e;
    }
    else if (strcmp(next->key, key) == 0)
    {
        prev->next = e;
        e->next = next->next;
        memset(next, 0, sizeof(BindingElement));
        free(next);
    }
    else
    {
        prev->next = e;
        e->next = next;
    }
}

#define KEYBIND_NOT_HANDLED 0
#define KEYBIND_HANDLED 1
#define KEYBIND_WAIT 2

static int keybind_handle(Keybind *this, char *key, Buffer *buf)
{
    BindingElement *elem = this->bind;
    while (elem != NULL)
    {
        if (strncmp(key, elem->key, strlen(key)) == 0)
        {
            if (strcmp(key, elem->key) == 0)
            {
                write_message("");
                (*(elem->func))(buf);

                return KEYBIND_HANDLED;
            }

            return KEYBIND_WAIT;
        }

        elem = elem->next;
    }

    if (strlen(key_buf) != 1)
        return KEYBIND_HANDLED;

    return KEYBIND_NOT_HANDLED;
}

static Keybind *global_keybind;

static void init_global_keybind(void)
{
    global_keybind = keybind_create();

    keybind_add(global_keybind, "^[[C", ec_cursor_forward);
    keybind_add(global_keybind, "^[[D", ec_cursor_back);
    keybind_add(global_keybind, "^B", ec_cursor_back);
    keybind_add(global_keybind, "^F", ec_cursor_forward);
    keybind_add(global_keybind, "^H", ec_delete_backward);
    keybind_add(global_keybind, "^X^C", ec_editor_quit);
    keybind_add(global_keybind, "^X^S", ec_buffer_save);
    keybind_add(global_keybind, "\x7f", ec_delete_backward);
}

void keybind_set_up(void)
{
    memset(key_buf, 0, sizeof(key_buf));

    init_global_keybind();
}

void keybind_tear_down(void)
{
    keybind_destruct(global_keybind);
}

void handle_key(int c)
{
    unsigned int i = 0;
    while (key_buf[i]) ++i;

    key_buf[i] = (char)c;

    switch (keybind_handle(global_keybind, key_buf, current_buffer))
    {
    case KEYBIND_NOT_HANDLED:
        buffer_insert(current_buffer, (char)c);
        // fall through
    case KEYBIND_HANDLED:
        memset(key_buf, 0, sizeof(key_buf));
        break;
    case KEYBIND_WAIT:
        write_message("Waiting for next key...");
        break;
    }
}
