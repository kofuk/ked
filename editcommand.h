#ifndef EDITCOMMAND_H
#define EDITCOMMAND_H

#include "buffer.h"

#define EDIT_COMMAND_ARG_LIST Buffer *buf

typedef void (*EditCommand)(EDIT_COMMAND_ARG_LIST);

void ec_cursor_forward(EDIT_COMMAND_ARG_LIST);
void ec_cursor_back(EDIT_COMMAND_ARG_LIST);
void ec_delete_backward(EDIT_COMMAND_ARG_LIST);
void ec_buffer_save(EDIT_COMMAND_ARG_LIST);
void ec_editor_quit(EDIT_COMMAND_ARG_LIST);

#endif
