#ifndef KED_H
#define KED_H

#include <stddef.h>

#ifndef BUFFER_H
typedef struct _buffer Buffer;
#endif

#ifndef RUNE_H
typedef char Rune[4];
#endif

struct EditorEnv {
    // buffer.h
    void (*buffer_cursor_move)(Buffer *, size_t, int);
    void (*buffer_cursor_forward_line)(Buffer *);
    void (*buffer_insert)(Buffer *, Rune);
    void (*buffer_delete_backward)(Buffer *);
    void (*buffer_delete_forward)(Buffer *);
    int (*buffer_save)(Buffer *);

    // ui.h
    void (*exit_editor)(void);
    void (*write_message)(char *);

    // terminal.h
    void (*stop_editor)(void);
};

#define EDIT_COMMAND_ARG_LIST struct EditorEnv *env, Buffer *buf

#define DEFINE_EDIT_COMMAND(name) void ec_##name(EDIT_COMMAND_ARG_LIST)
#define EDIT_COMMAND_PTR(name) ec_##name

#define KED_FUNCALL(name) env->name

typedef void (*EditCommand)(EDIT_COMMAND_ARG_LIST);

struct KeybindEntry {
    EditCommand func;
    const char *name;
};

#define EXPORT_KEYBINDS struct KeybindEntry ked_keybind_exports[]
#define KEYBINDS_END {0, 0}

#endif
